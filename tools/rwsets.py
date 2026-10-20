"""List the globals, struct fields and functions each function reads, writes and calls.

    uv run tools/rwsets.py                 # writes data/rwsets.csv
    uv run tools/rwsets.py --check         # fail if data/rwsets.csv is out of date
    uv run tools/rwsets.py --build-dir D   # use D/compile_commands.json (default build/rwsets)
    uv run tools/rwsets.py --only src/game/economy.cpp   # print the rows of some files, write nothing
    uv run tools/test_rwsets.py            # tests, on hand-written functions

The pass runs the clang-cl of the modern build over every file under src/ with
the flags CMake uses (from compile_commands.json), as `-fsyntax-only -Xclang
-ast-dump=json`, and walks the JSON syntax tree in Python, so no C++ tool has to
be built. libtooling was not needed: the dump has the referenced declaration of
every name, the type of every expression and the cast kinds, which is all the
reads, writes and calls need. Nothing is compiled or linked. The default build
directory is configured on first use with `-DBT_COMPILE_ONLY=ON
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`; any directory configured that way works.
Needs LLVM 22.1.8 (cmake/clang-cl-i686.cmake). --check regenerates the file in
memory, writes nothing and exits 1 with the first differing lines when it
differs from the committed file.

data/rwsets.csv has one row per function, kind and target, sorted by address
(functions without a `// FUNCTION: 0x...` marker last), then function, file,
kind, target. Columns:

  address   from the `// FUNCTION: 0x...` marker above the definition, or empty
  function  qualified name (Class::Method); overloads share a name
  file      source file, relative to the repository root
  kind      one of the kinds below
  target    what is accessed or called

Kinds:

  global_read, global_write        a global variable (`name@file.cpp` for a file-local
                                   static, `name@Function` for a function-local static),
                                   as written in the source, not canonicalised through
                                   data/aliases.csv. `g_game->x` reads g_game and
                                   accesses Game::x.
  field_read, field_write          `Type::field`, through any pointer, reference or array
                                   element; Type is the type of the object the field is
                                   in, typedefs removed. Only the last field of a path is
                                   recorded (`g_game->players[i].hp` is Player::hp).
  deref_read, deref_write          `*type`, an access through a pointer that is not a
                                   field (`*p` or `p[i]` with int *p is `*int`)
  *_maybe_written                  an lvalue passed to a call as a non-const pointer or
                                   reference, or as the object of a non-const method: the
                                   callee may write it
  *_escape                         an lvalue whose address is taken (`&x`, an array
                                   decaying to a pointer) outside a call argument:
                                   anything may then read or write it
  function                         the definition itself, so a function that touches nothing
                                   has a row (empty target)
  call                             a function called directly, by name; also
                                   `Class::~Class` for a destructor that runs (see below)
  indirect_call                    `indirect via <type> [<field or variable>]` for a call
                                   through a function pointer; `virtual Class::Method`
                                   for a virtual method, `virtual Class::~Class` for a
                                   `delete` of a class with a virtual destructor
  fn_ref                           a function whose address is taken without a call
  asm                              the function has inline assembly (target `inline`)

Destructors. Only a class with a non-trivial destructor (user-provided, virtual, or
implied by a member or base that has one) gets rows; `Class` is the type with
typedefs removed. These cases are covered:

  delete p, delete[] p      call `Class::~Class` of the pointed-to type, besides
                            `operator delete`; for `delete p` (not `delete[]`) of a
                            class whose destructor is virtual, itself or by overriding
                            a base's, also indirect_call `virtual Class::~Class`
  temporaries               a CXXBindTemporaryExpr (clang writes one only for a
                            non-trivial destructor) is a call `Class::~Class`
  locals                    a local variable of class type (or an array of it) that is
                            not static, extern, a pointer or a reference is a call
                            `Class::~Class`. clang's dump has no node for the call at
                            scope exit, so it is read off the declaration

Not covered: destructors that run for by-value parameters, for the members and bases
inside a destructor, for objects thrown or caught, and `delete` of a pointer to an
incomplete or dependent type. A destructor called by name (`p->~Class()`) is a
normal call.

Globals that live in the shared state block (issue #284) are reached as members of
`SimGlobals` through `Sim()`, mostly via `#define g_x (Sim().g_x)`. Such an access is
recorded as the global it names: the same rows and target as a plain `g_x`. The
mapping table is src/state/sim_globals.csv (see load_sim_map). A cast view of a field
(`*reinterpret_cast<T*>(&E)`) takes the access mode of E, and an interior global
(`*reinterpret_cast<T*>(reinterpret_cast<char*>(&E) + N)`) is recorded under its own
name. Calls of `Sim()` and the static `g_simGlobals` pointer add no rows.

A compound assignment (`x += 1`) and `++` and `--` are a read and a write.
Locals and parameters are not recorded.

Known limits. The pass reads the syntax tree after preprocessing, so a macro is
analysed as what it expands to. Inline assembly (`__asm`) is not looked into: the
function is only marked `asm`. A write through a cast pointer (`*(int*)&x = 1`,
`((Foo*)p)->f = 1`) is recorded against the type it is cast to, not the type of
the original object, so two views of one memory block have different names.
Aliasing is not tracked: `*p = 1` is `deref_write *int`, not a write of whatever
p points to. A call through a function pointer is not resolved. Whether a callee
writes a pointer argument is not looked into: the constness of the parameter
decides `maybe_written`. Dynamic initialisers of globals and compiler-generated
functions are not rows.
"""

import argparse
import bisect
import csv
import difflib
import io
import json
import os
import re
import shlex
import subprocess
import sys
import time
from collections.abc import Callable
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

from sources import ANNOTATION, ROOT

DEFAULT_BUILD_DIR = ROOT / "build/rwsets"
RWSETS = ROOT / "data/rwsets.csv"
MARKER_DISTANCE = 14
COLUMNS = ["address", "function", "file", "kind", "target"]
MODE_KINDS = {"R": ["read"], "W": ["write"], "RW": ["read", "write"], "MW": ["maybe_written"],
              "ESC": ["escape"]}
FUNCTION_KINDS = {"FunctionDecl", "CXXMethodDecl", "CXXConstructorDecl", "CXXDestructorDecl",
                  "CXXConversionDecl"}
LVALUE_KINDS = {"DeclRefExpr", "MemberExpr", "ArraySubscriptExpr", "ParenExpr"}
UNEVALUATED = {"UnaryExprOrTypeTraitExpr", "CXXTypeidExpr", "CXXNoexceptExpr"}
JSON_START = re.compile(r'^\{\n  "id"', re.M)
CHUNK = re.compile(r"^    \{\n.*?^    \},?\n", re.S | re.M)
INCLUDED_FROM = re.compile(r'"includedFrom": \{\s*"file": "[^"]*"\s*\}')
SIM_MAP_FILE = ROOT / "src/state/sim_globals.csv"
SIM_POINTER = "g_simGlobals"
CAST_KINDS = ("CXXReinterpretCastExpr", "CStyleCastExpr")
QUALIFIERS = re.compile(r"\b(const|volatile|struct|class|union|enum|__restrict)\b")


class SimMap:
    """Which SimGlobals fields stand for which rows, and which globals sit inside others."""

    def __init__(self):
        self.fields: dict[str, str] = {}
        self.statics: dict[str, dict[str, str]] = {}
        self.names: dict[str, str] = {}
        self.interiors: dict[tuple[str, int], str] = {}
        self.interior_of: dict[str, tuple[str, int]] = {}
        self.current_file = ""

    def name(self, field: str) -> str:
        """The row an access of the field in the file being analysed is attributed to."""
        statics = self.statics.get(field)
        if statics and self.current_file in statics:
            return statics[self.current_file]
        if field in self.fields:
            return self.fields[field]
        if statics:
            return next(iter(statics.values()))
        return field


def load_sim_map(path: Path | None = None) -> SimMap:
    """Read the mapping table of the globals moved into SimGlobals.

    src/state/sim_globals.csv, beside sim_globals.h, header `field,name,container,offset,file`,
    one row per moved global or static:

      field      the SimGlobals member (for an interior global, the name of its view macro)
      name       the row name in rwsets.py: `g_x`, or `g_x@src/file.cpp` for a static
      container  empty for a direct field; for an interior global, the field it sits in
      offset     for an interior global, its byte offset in the container (0x.. or decimal)
      file       optional, for a file-static: the file it belongs to (default: the part of
                 `name` after the `@`)

    A field with a global row and a file-static row is the global in every file but the
    static's own, where it is the static.

    Example: `g_packetChannels,g_packetChannels,g_packetManager,0x8`. A field that is not
    listed is its own name, so only statics need rows for the analysis; traceschema.py
    also reads the table to know which globals moved. A missing file is an empty table.
    """
    sim = SimMap()
    path = SIM_MAP_FILE if path is None else path
    if not path.exists():
        return sim
    with path.open(newline="") as f:
        rows = list(csv.DictReader(f))
    for row in rows:
        if not row["container"]:
            file = row.get("file") or (row["name"].split("@", 1)[1] if "@" in row["name"] else "")
            if file:
                sim.statics.setdefault(row["field"], {})[file] = row["name"]
            else:
                sim.fields[row["field"]] = row["name"]
            sim.names[row["name"]] = row["field"]
    for row in rows:
        if row["container"]:
            offset = int(row["offset"], 0)
            sim.interiors[(row["container"], offset)] = row["name"]
            sim.interior_of[row["name"]] = (row["container"], offset)
    return sim


SIM = load_sim_map()


def is_sim_call(node: dict) -> bool:
    """Whether a node is a call of Sim(), which returns the SimGlobals."""
    node = strip(node)
    if node["kind"] != "CallExpr" or not node.get("inner"):
        return False
    callee = strip(node["inner"][0])
    return callee["kind"] == "DeclRefExpr" and callee["referencedDecl"].get("name") == "Sim" and \
        record_name(node["type"]) == "SimGlobals"


def sim_global(node: dict) -> str | None:
    """The global an lvalue stands for when it is a SimGlobals member or a view of one, else None."""
    field = sim_field(node)
    return None if field is None else SIM.name(field)


def sim_field(node: dict) -> str | None:
    """The SimGlobals field (or interior global) an lvalue stands for, else None."""
    node = strip(node)
    if node["kind"] == "MemberExpr" and not node.get("isArrow") and node.get("inner") and \
            is_sim_call(node["inner"][0]):
        return node.get("name") or ""
    if node["kind"] == "UnaryOperator" and node.get("opcode") == "*":
        return view_target(node["inner"][0])
    return None


def address_operand(node: dict) -> dict | None:
    """E in `reinterpret_cast<T*>(&E)`, else None."""
    node = strip(node)
    if node["kind"] in CAST_KINDS and node.get("inner"):
        node = strip(node["inner"][0])
        if node["kind"] == "UnaryOperator" and node.get("opcode") == "&":
            return node["inner"][0]
    return None


def view_target(pointer: dict) -> str | None:
    """The global the pointer of a `*reinterpret_cast<T*>(...)` points at, else None.

    The pointer is `&E` for a view of a whole global and `reinterpret_cast<char*>(&E) + N`
    for an interior global, N being a byte offset in the global E.
    """
    pointer = strip(pointer)
    if pointer["kind"] not in CAST_KINDS or not pointer.get("inner"):
        return None
    operand = strip(pointer["inner"][0])
    if operand["kind"] == "UnaryOperator" and operand.get("opcode") == "&":
        return sim_field(operand["inner"][0])
    if operand["kind"] == "BinaryOperator" and operand.get("opcode") == "+":
        base, offset = strip(operand["inner"][0]), strip(operand["inner"][1])
        target = address_operand(base)
        if target is not None and offset["kind"] == "IntegerLiteral":
            container = sim_field(target)
            if container is not None:
                return SIM.interiors.get((container, int(offset["value"], 0)))
    return None


def configure(build_dir: Path) -> None:
    """Configure build_dir for the modern build with a compilation database."""
    env = dict(os.environ, BT_NO_ORIG="1", BT_NO_GHIDRA="1", BT_NO_ZLIB="1")
    subprocess.run(["cmake", "-S", str(ROOT), "-B", str(build_dir), "-G", "Ninja",
                    "-DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-i686.cmake", "-DBT_COMPILE_ONLY=ON",
                    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"], check=True, cwd=ROOT, env=env,
                   stdout=subprocess.DEVNULL)


def compile_commands(build_dir: Path, only: list[str]) -> list[tuple[str, list[str], str]]:
    """(relative source, clang-cl argv for the AST dump, directory) for every file under src/."""
    database = build_dir / "compile_commands.json"
    if not database.exists():
        configure(build_dir)
    out = []
    src_root = os.path.realpath(ROOT / "src") + os.sep
    for entry in json.loads(database.read_text()):
        path = os.path.realpath(entry["file"])
        if not path.startswith(src_root):
            continue
        rel = os.path.relpath(path, ROOT)
        if only and rel not in only:
            continue
        args = shlex.split(entry["command"])
        keep = []
        skip = False
        for arg in args:
            if skip:
                skip = False
            elif arg == "--":
                skip = True
            elif arg in ("-c", "/c") or arg.startswith(("/Fo", "/Fd")):
                continue
            else:
                keep.append(arg)
        keep += ["-fsyntax-only", "-Xclang", "-ast-dump=json", "--", path]
        out.append((rel, keep, entry["directory"]))
    return sorted(out)


def strip(node: dict) -> dict:
    """The node under implicit casts and parentheses."""
    while node["kind"] in ("ImplicitCastExpr", "ParenExpr") and node.get("inner"):
        node = node["inner"][0]
    return node


def type_text(t: dict) -> str:
    """A type with typedefs removed, unless that leaves an anonymous type."""
    desugared = t.get("desugaredQualType")
    if desugared and "unnamed" not in desugared and "anonymous" not in desugared:
        return desugared
    return t.get("qualType", "")


def record_name(t: dict) -> str:
    """The struct or class a type names, without qualifiers, pointers or array extents."""
    text = re.sub(r"\[[^\]]*\]", "", type_text(t))
    text = QUALIFIERS.sub("", text).replace("*", "").replace("&", "")
    text = re.sub(r"\(unnamed[^)]*\)|\(anonymous[^)]*\)", "<anon>", text)
    return " ".join(text.split()).replace("::<anon>", "") or "?"


def pointee(t: dict) -> dict:
    """The type a pointer type points to, as a type dictionary for record_name."""
    text = type_text(t).strip()
    return {"qualType": text[:text.rfind("*")].strip()}


def is_pointer(t: dict) -> bool:
    """Whether a type is a pointer (not a reference, array or function)."""
    return bool(re.search(r"\*\s*(const|volatile|\s)*$", type_text(t)))


def pointee_const(t: dict) -> bool:
    """Whether the pointee of a pointer or reference type is const."""
    text = type_text(t).strip()
    if text.endswith("&"):
        pointee = text[:-1].strip()
    else:
        text = re.sub(r"\s*(const|volatile)\s*$", "", text) if text.endswith(("const", "volatile")) else text
        pointee = text[:text.rfind("*")].strip() if "*" in text else text
    return pointee.endswith("const") or ("*" not in pointee and re.search(r"\bconst\b", pointee) is not None)


class Position:
    """Follows clang's delta-encoded source locations through a dump in order."""

    def __init__(self, file: str, line: int):
        self.file = file
        self.line = line

    def feed(self, loc: dict | None) -> tuple[str, int] | None:
        """Apply one location (its macro expansion if it has one) and return (file, line)."""
        if not loc:
            return None
        if "spellingLoc" in loc:
            self.feed(loc["spellingLoc"])
            loc = loc["expansionLoc"]
        if "file" in loc:
            self.file = loc["file"]
        if "line" in loc:
            self.line = loc["line"]
        return self.file, self.line

    def annotate(self, node: dict) -> None:
        """Set _file, _line and _end on a node and every node under it."""
        here = self.feed(node.get("loc"))
        begin = end = None
        rng = node.get("range")
        if rng:
            begin = self.feed(rng.get("begin"))
            end = self.feed(rng.get("end"))
        first = here or begin or (self.file, self.line)
        node["_file"], node["_line"] = first
        node["_end"] = (end or first)[1]
        for child in node.get("inner", ()):
            self.annotate(child)


def last_position(chunk: str, file: str, line: int) -> tuple[str, int]:
    """The (file, line) state after a chunk of dump text."""
    text = INCLUDED_FROM.sub("", chunk)
    i = text.rfind('"file": "')
    if i >= 0:
        j = text.index('"', i + 9)
        file = text[i + 9:j]
    i = text.rfind('"line": ')
    if i >= 0:
        m = re.match(r"\d+", text[i + 8:])
        line = int(m.group())
    return file, line


def first_file(chunk: str) -> str | None:
    """The file the first location of a chunk names, or None when it repeats the previous one."""
    text = INCLUDED_FROM.sub("", chunk[:2000])
    i = text.find('"file": "')
    c = text.find('"col"')
    if i < 0 or (0 <= c < i):
        return None
    return text[i + 9:text.index('"', i + 9)]


class Analyzer:
    """Collects the rows of one translation unit."""

    walker_class: type = None

    def __init__(self, root: Path):
        self.root = root
        self.records: dict[str, str] = {}
        self.methods: dict[str, tuple[str, bool, bool]] = {}
        self.statics: dict[str, str] = {}
        self.destructors: dict[str, dict] = {}
        self.functions: list[dict] = []
        self.root_cache: dict[str, str | None] = {}

    def relative(self, file: str) -> str | None:
        """The repository-relative path of a project file, None for system headers."""
        if file not in self.root_cache:
            real = os.path.realpath(file)
            rel = os.path.relpath(real, self.root)
            self.root_cache[file] = rel if rel.startswith(("src" + os.sep, "include" + os.sep)) else None
        return self.root_cache[file]

    def assign_markers(self) -> None:
        """Give each function the address of the // FUNCTION: marker just above its definition."""
        by_file: dict[str, list[dict]] = {}
        for fn in self.functions:
            by_file.setdefault(fn["file"], []).append(fn)
        for rel, fns in by_file.items():
            fns.sort(key=lambda f: f["start"])
            starts = [f["start"] for f in fns]
            for n, text in enumerate((self.root / rel).read_text(errors="replace").splitlines(), 1):
                m = ANNOTATION.match(text)
                i = bisect.bisect_left(starts, n)
                if m and i < len(fns) and starts[i] - n <= MARKER_DISTANCE:
                    fns[i]["address"] = "0x%x" % int(m.group(1), 16)

    def declare(self, node: dict, owner: str | None) -> None:
        """Register records, methods and statics of a declaration, and analyse function bodies."""
        kind = node["kind"]
        if kind in ("LinkageSpecDecl", "NamespaceDecl"):
            for child in node.get("inner", ()):
                self.declare(child, owner)
        elif kind == "CXXRecordDecl":
            name = node.get("name") or "<anon>"
            if owner and node.get("name"):
                name = owner + "::" + name
            self.records[node["id"]] = name
            data = node.get("definitionData")
            if node.get("completeDefinition") and data is not None and node.get("name"):
                self.destructors[name] = {
                    "nontrivial": "nonTrivial" in data.get("dtor", {}),
                    "virtual": any(c["kind"] == "CXXDestructorDecl" and c.get("virtual")
                                   for c in node.get("inner", ())),
                    "bases": [record_name(b["type"]) for b in node.get("bases", ())],
                }
            for child in node.get("inner", ()):
                self.declare(child, name)
        elif kind == "VarDecl":
            if node.get("storageClass") == "static":
                name = node.get("name", "?")
                if owner:
                    self.statics[node["id"]] = owner + "::" + name
                else:
                    self.statics[node["id"]] = "%s@%s" % (name, self.relative(node["_file"]) or node["_file"])
        elif kind in FUNCTION_KINDS and not node.get("isImplicit"):
            parent = node.get("parentDeclContextId")
            cls = self.records.get(parent) if parent else owner
            name = node.get("name", "?")
            if kind == "CXXConstructorDecl" and cls:
                name = cls.split("::")[-1]
            qualified = cls + "::" + name if cls and not name.startswith(cls + "::") else name
            const = node.get("type", {}).get("qualType", "").endswith(" const")
            self.methods[node["id"]] = (qualified, bool(node.get("virtual")), const)
            if any(c["kind"].endswith("Stmt") for c in node.get("inner", ())):
                self.function(node, qualified)

    def virtual_destructor(self, name: str) -> bool:
        """Whether the destructor of a class is virtual, itself or by overriding a base's."""
        info = self.destructors.get(name)
        return bool(info) and (info["virtual"] or any(self.virtual_destructor(b) for b in info["bases"]))

    def function(self, node: dict, name: str) -> None:
        """Analyse one function definition."""
        rel = self.relative(node["_file"])
        if rel is None:
            return
        SIM.current_file = rel
        walker = (self.walker_class or Walker)(self, name)
        walker.prepare(node)
        for child in node.get("inner", ()):
            if child["kind"].endswith("Stmt") or child["kind"] == "CXXCtorInitializer":
                walker.visit(child, "R")
        start = node["_line"]
        self.functions.append({
            "address": "", "function": name, "file": rel,
            "start": start, "items": sorted(walker.items),
        })


class Walker:
    """Walks one function body, filling its read and write sets."""

    def __init__(self, unit: Analyzer, name: str):
        self.unit = unit
        self.name = name
        self.locals: dict[str, str | None] = {}
        self.items: set[tuple[str, str]] = set()

    def prepare(self, node: dict) -> None:
        """Find the variables declared in the function, so the rest are globals."""
        stack = [node]
        while stack:
            n = stack.pop()
            if n.get("kind") in ("VarDecl", "ParmVarDecl"):
                static = n["kind"] == "VarDecl" and n.get("storageClass") == "static"
                self.locals[n["id"]] = "%s@%s" % (n["name"], self.name) if static else None
                if n["kind"] == "VarDecl" and n.get("storageClass") not in ("static", "extern"):
                    self.destructor(n["type"], False)
            stack.extend(n.get("inner", ()))

    def record(self, kind: str, target: str) -> None:
        self.items.add((kind, target))

    def destructor(self, t: dict, dispatch: bool) -> None:
        """A call of the destructor of a class type, a virtual one if the object may be a subclass."""
        text = type_text(t)
        name = record_name(t)
        info = self.unit.destructors.get(name)
        if not info or not info["nontrivial"] or (text.endswith("&") or is_pointer(t)):
            return
        target = "%s::~%s" % (name, name.split("::")[-1])
        self.record("call", target)
        if dispatch and self.unit.virtual_destructor(name):
            self.record("indirect_call", "virtual " + target)

    def access(self, category: str, target: str, mode: str) -> None:
        """Record an access of a global, field or dereference: R, W, RW, MW (maybe written) or ESC."""
        if category == "global" and target.split("@")[0] == SIM_POINTER:
            return
        for suffix in MODE_KINDS[mode]:
            self.record("%s_%s" % (category, suffix), target)

    def visit(self, node: dict, mode: str = "R") -> None:
        """Walk an expression or statement, mode being how the context uses its value."""
        kind = node.get("kind")
        inner = node.get("inner", [])
        if kind is None or kind in UNEVALUATED:
            return
        if kind in LVALUE_KINDS:
            self.lvalue(node, mode)
        elif kind in ("ImplicitCastExpr", "CStyleCastExpr", "CXXFunctionalCastExpr", "CXXStaticCastExpr",
                      "CXXReinterpretCastExpr", "CXXConstCastExpr"):
            self.cast(node, mode)
        elif kind == "BinaryOperator":
            op = node["opcode"]
            if op == "=":
                self.visit(inner[0], "W")
                self.visit(inner[1], "R")
            else:
                for child in inner:
                    self.visit(child, "R")
        elif kind == "CompoundAssignOperator":
            self.visit(inner[0], "RW")
            self.visit(inner[1], "R")
        elif kind == "UnaryOperator":
            op = node["opcode"]
            if op in ("++", "--"):
                self.visit(inner[0], "RW")
            elif op == "&":
                self.address_of(inner[0], "ESC")
            elif op == "*":
                name = sim_global(node)
                if name:
                    self.access("global", name, mode)
                else:
                    self.deref(node, mode)
            else:
                self.visit(inner[0], "R")
        elif kind in ("CallExpr", "CXXMemberCallExpr", "CXXOperatorCallExpr"):
            self.call(node)
        elif kind in ("MSAsmStmt", "GCCAsmStmt"):
            self.record("asm", "inline")
        elif kind == "DeclStmt":
            for child in inner:
                for init in child.get("inner", ()):
                    self.visit(init, "R")
        elif kind == "CXXNewExpr":
            self.record("call", "operator new")
            for child in inner:
                self.visit(child, "R")
        elif kind == "CXXDeleteExpr":
            self.record("call", "operator delete")
            if inner and is_pointer(inner[0]["type"]):
                self.destructor(pointee(inner[0]["type"]), not node.get("isArray"))
            for child in inner:
                self.visit(child, "R")
        elif kind == "ConditionalOperator" and self.array_branches(inner):
            self.visit(inner[0], "R")
            for branch in inner[1:]:
                self.visit(branch["inner"][0], mode)
        elif kind == "CXXBindTemporaryExpr":
            self.destructor(node["type"], False)
            for child in inner:
                self.visit(child, "R")
        else:
            for child in inner:
                self.visit(child, "R")

    @staticmethod
    def array_branches(inner: list) -> bool:
        """True when both branches of a conditional decay arrays of one element type.

        A sim global used to be an `extern` of the same unbounded type as its partner, so the pair was
        one lvalue and its reads were reads; a field has a bound, which would make them escapes.
        """
        if len(inner) != 3:
            return False
        elements = set()
        for branch in inner[1:]:
            if branch["kind"] != "ImplicitCastExpr" or branch.get("castKind") != "ArrayToPointerDecay":
                return False
            elements.add(re.sub(r"\s*\[\d*\]$", "", branch["inner"][0].get("type", {}).get("qualType", "")))
        return len(elements) == 1 and any(sim_global(branch["inner"][0]) for branch in inner[1:])

    def cast(self, node: dict, mode: str) -> None:
        kind = node.get("castKind")
        child = node["inner"][0]
        if kind == "LValueToRValue":
            self.visit(child, "R")
        elif kind == "ArrayToPointerDecay":
            self.visit(child, "ESC")
        elif kind == "FunctionToPointerDecay":
            target = strip(child)
            if target["kind"] == "DeclRefExpr" and target["referencedDecl"]["kind"] == "FunctionDecl":
                self.record("fn_ref", target["referencedDecl"]["name"])
        else:
            self.visit(child, mode)

    def address_of(self, node: dict, mode: str) -> None:
        node = strip_parens(node)
        if node["kind"] == "DeclRefExpr" and node["referencedDecl"]["kind"] == "FunctionDecl":
            self.record("fn_ref", node["referencedDecl"]["name"])
        elif node["kind"] == "MemberExpr" and node.get("type", {}).get("qualType", "").endswith(")"):
            return
        else:
            self.visit(node, mode)

    def deref(self, node: dict, mode: str, path: bool = False) -> None:
        self.visit(node["inner"][0], "R")
        if not path:
            self.access("deref", "*" + record_name(node["type"]), mode)

    def lvalue(self, node: dict, mode: str, path: bool = False) -> None:
        """An lvalue, accessed in a mode; in a path only the root and the indexes count."""
        kind = node["kind"]
        inner = node.get("inner", [])
        if kind == "ParenExpr":
            self.visit(inner[0], mode) if not path else self.path(inner[0], mode)
        elif kind == "DeclRefExpr":
            ref = node["referencedDecl"]
            if ref["kind"] == "VarDecl":
                if ref["id"] in self.locals:
                    name = self.locals[ref["id"]]
                    if name:
                        self.access("global", name, mode)
                else:
                    self.access("global", self.unit.statics.get(ref["id"], ref["name"]), mode)
            elif ref["kind"] == "FunctionDecl" and mode == "ESC":
                self.record("fn_ref", ref["name"])
        elif kind == "MemberExpr" and sim_global(node):
            self.access("global", sim_global(node), mode)
        elif kind == "MemberExpr":
            base = inner[0] if inner else None
            if base is not None:
                if node.get("isArrow"):
                    self.visit(base, "R")
                else:
                    self.path(base, mode)
                if not path:
                    self.access("field", "%s::%s" % (record_name(base["type"]), node.get("name") or "<anon>"), mode)
        elif kind == "ArraySubscriptExpr":
            base, index = inner[0], inner[1]
            self.visit(index, "R")
            decayed = strip(base)
            if base["kind"] == "ImplicitCastExpr" and base.get("castKind") == "ArrayToPointerDecay":
                if decayed["kind"] == "MemberExpr" and not path:
                    self.lvalue(decayed, mode)
                else:
                    self.path(decayed, mode)
            else:
                self.visit(base, "R")
                if not path:
                    self.access("deref", "*" + record_name(node["type"]), mode)

    def path(self, node: dict, mode: str) -> None:
        """The object an element or field of which is accessed."""
        inner = strip_parens(node)
        name = sim_global(inner)
        if name:
            self.access("global", name, mode)
        elif inner["kind"] in LVALUE_KINDS:
            self.lvalue(inner, mode, path=True)
        else:
            self.visit(inner, "R")

    def call(self, node: dict) -> None:
        kind = node["kind"]
        inner = node["inner"]
        callee = strip(inner[0])
        args = inner[1:]
        if kind == "CXXOperatorCallExpr":
            op = callee
            if op["kind"] == "DeclRefExpr":
                self.record("call", op["referencedDecl"]["name"])
            if op["kind"] == "DeclRefExpr" and op["referencedDecl"]["name"] == "operator=" and args:
                self.visit(args[0], "W")
                args = args[1:]
        elif is_sim_call(node):
            pass
        elif callee["kind"] == "DeclRefExpr" and callee["referencedDecl"]["kind"] == "FunctionDecl":
            self.record("call", callee["referencedDecl"]["name"])
        elif callee["kind"] == "MemberExpr" and callee.get("referencedMemberDecl") and \
                callee["type"].get("qualType") == "<bound member function type>":
            name, virtual, const_method = self.unit.methods.get(
                callee["referencedMemberDecl"], (callee.get("name", "?"), False, False))
            self.record("indirect_call" if virtual else "call", ("virtual " + name) if virtual else name)
            base = callee["inner"][0] if callee.get("inner") else None
            if base is not None:
                if callee.get("isArrow"):
                    self.visit(base, "R")
                elif const_method or base_is_const(base):
                    self.visit(base, "R")
                else:
                    self.lvalue_arg(base, False)
        else:
            self.record("indirect_call", "indirect via %s%s" % (callee_type(callee), callee_hint(callee)))
            self.visit(inner[0], "R")
        for arg in args:
            self.argument(arg)

    def argument(self, arg: dict) -> None:
        """A call argument: an lvalue passed by non-const reference or pointer may be written."""
        inner = strip(arg)
        arg_type = arg.get("type", {})
        if inner["kind"] == "UnaryOperator" and inner.get("opcode") == "&" and is_pointer(arg_type):
            self.lvalue_arg(inner["inner"][0], pointee_const(arg_type))
        elif arg["kind"] == "ImplicitCastExpr" and arg.get("castKind") == "ArrayToPointerDecay":
            self.lvalue_arg(arg["inner"][0], pointee_const(arg_type))
        elif arg.get("valueCategory") == "lvalue" and inner["kind"] in LVALUE_KINDS and \
                type_text(arg_type).endswith("&"):
            self.lvalue_arg(inner, pointee_const(arg_type))
        elif inner["kind"] == "ConditionalOperator" and is_pointer(arg_type) and self.array_branches(inner["inner"]):
            self.visit(arg, "R" if pointee_const(arg_type) else "MW")
        else:
            self.visit(arg, "R")

    def lvalue_arg(self, node: dict, const: bool) -> None:
        node = strip_parens(node)
        if node["kind"] == "DeclRefExpr" and node["referencedDecl"]["kind"] == "FunctionDecl":
            self.visit(node, "ESC")
        elif node["kind"] == "UnaryOperator" and sim_global(node):
            self.access("global", sim_global(node), "R" if const else "MW")
        elif node["kind"] in LVALUE_KINDS:
            self.lvalue(node, "R" if const else "MW")
        else:
            self.visit(node, "R")


def base_is_const(node: dict) -> bool:
    qual = node.get("type", {}).get("qualType", "")
    return qual.startswith("const ") and not qual.endswith("*")


def strip_parens(node: dict) -> dict:
    while node["kind"] == "ParenExpr" and node.get("inner"):
        node = node["inner"][0]
    return node


def callee_type(callee: dict) -> str:
    return type_text(callee.get("type", {})) or "?"


def callee_hint(callee: dict) -> str:
    """The variable or field a function pointer comes from, for an indirect call."""
    node = callee
    while node["kind"] in ("ParenExpr", "ImplicitCastExpr", "ArraySubscriptExpr") or \
            (node["kind"] == "UnaryOperator" and node.get("opcode") == "*"):
        name = sim_global(node)
        if name:
            return " [%s]" % name.split("@")[0]
        node = node["inner"][0]
    name = sim_global(node)
    if name:
        return " [%s]" % name.split("@")[0]
    if node["kind"] == "MemberExpr" and node.get("inner"):
        return " [%s::%s]" % (record_name(node["inner"][0]["type"]), node.get("name", "?"))
    if node["kind"] == "DeclRefExpr":
        return " [%s]" % node["referencedDecl"]["name"]
    return ""


def analyse(job: tuple[Path, list[str], str]) -> list[list]:
    """Run clang-cl on one file and return its rows: [address, function, file, kind, target]."""
    unit, _ = analyse_unit(job, Analyzer)
    return [[fn["address"], fn["function"], fn["file"], kind, target]
            for fn in unit.functions for kind, target in [("function", "")] + fn["items"]]


def analyse_unit(job: tuple[Path, list[str], str], analyzer: type,
                 prepare: Callable[[Analyzer, str], None] | None = None) -> tuple[Analyzer, str]:
    """Run clang-cl on one file and walk its dump with an analyzer; returns it and clang's stderr.

    Whatever clang prints before the JSON (record layouts) goes to prepare(unit, text) first."""
    root, argv, directory = job
    run = subprocess.run(argv, cwd=directory, capture_output=True, text=True, errors="replace")
    if run.returncode != 0 and "error:" in run.stderr:
        raise SystemExit("%s: clang-cl failed\n%s" % (argv[-1], run.stderr[-2000:]))
    found = JSON_START.search(run.stdout)
    if found is None:
        raise SystemExit("%s: clang-cl printed no syntax tree\n%s" % (argv[-1], run.stderr[-2000:]))
    text = run.stdout[found.start():]
    unit = analyzer(root)
    if prepare:
        prepare(unit, run.stdout[:found.start()])
    position = ("", 0)
    start = text.index('"inner": [')
    for m in CHUNK.finditer(text, start):
        chunk = m.group()
        file = first_file(chunk) or position[0]
        if not file or unit.relative(file) is None:
            position = last_position(chunk, *position)
            continue
        node = json.loads(chunk.rstrip().rstrip(","))
        tracker = Position(*position)
        tracker.annotate(node)
        position = (tracker.file, tracker.line)
        unit.declare(node, None)
    unit.assign_markers()
    return unit, run.stderr


def merge(results: list[list[list]]) -> list[list[str]]:
    """Combine the per-file rows into the sorted, de-duplicated rows of the data file."""
    rows = {tuple(row) for result in results for row in result}

    def order(row: tuple) -> tuple:
        address, name, file, kind, target = row
        return (0, int(address, 16)) if address else (1, 0), name, file, kind, target
    return [list(row) for row in sorted(rows, key=order)]


def render(rows: list[list[str]]) -> str:
    out = io.StringIO()
    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(COLUMNS)
    writer.writerows(rows)
    return out.getvalue()


def analyse_source(path: Path, root: Path) -> list[list[str]]:
    """The rows for one source file outside the build, compiled with no headers (for tests)."""
    argv = ["clang-cl", "--target=i686-pc-windows-msvc", "/nologo", "-TP", "-fms-compatibility",
            "-fms-extensions", "-fsyntax-only", "-Xclang", "-ast-dump=json", "--", str(path)]
    return merge([analyse((root, argv, str(root)))])


def generate(build_dir: Path, jobs: int, only: list[str]) -> tuple[str, int]:
    commands = compile_commands(build_dir, only)
    with ProcessPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(analyse, [(ROOT, argv, directory) for _, argv, directory in commands],
                                chunksize=4))
    return render(merge(results)), len(commands)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if data/rwsets.csv is out of date")
    parser.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD_DIR)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--only", nargs="+", default=[], help="print the rows of these files, write nothing")
    args = parser.parse_args()
    sys.setrecursionlimit(100000)
    started = time.time()
    text, files = generate(args.build_dir, args.jobs, args.only)
    elapsed = time.time() - started
    if args.only:
        sys.stdout.write(text)
        return
    functions = len({tuple(line.split(",", 3)[:3]) for line in text.splitlines()[1:]})
    if args.check:
        old = RWSETS.read_text() if RWSETS.exists() else ""
        if old != text:
            diff = list(difflib.unified_diff(old.splitlines(), text.splitlines(), "data/rwsets.csv",
                                             "regenerated", lineterm="", n=0))
            print("\n".join(diff[:40]))
            if len(diff) > 40:
                print("... %d more lines" % (len(diff) - 40))
            print("rwsets: data/rwsets.csv is out of date; run `uv run tools/rwsets.py` and commit the result")
            sys.exit(1)
        print("rwsets: up to date (%d files, %.0fs)" % (files, elapsed))
        return
    RWSETS.write_text(text)
    print("%d rows, ~%d functions, %d files, %.0fs" % (text.count("\n") - 1, functions, files, elapsed))


if __name__ == "__main__":
    main()

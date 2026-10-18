"""List the types that have more than one definition, and keep that list from growing.

    uv run tools/typedupes.py                 # writes data/type_dupes.csv
    uv run tools/typedupes.py --out F         # write F instead (a dry run)
    uv run tools/typedupes.py --check         # fail if a type gained a definition or the list is out of date
    uv run tools/typedupes.py --build-dir D   # use D/compile_commands.json (default build/rwsets)
    uv run tools/typedupes.py --jobs N        # clang-cl processes at once (default: the CPU count)
    uv run tools/test_typedupes.py            # tests, on fixture sources and mocked layouts

A text scan of include/, src/, link/ and interfaces/ finds every `struct`, `class` and `union`
with a body, and every `typedef struct {} X`, outside `#if 0` blocks. Forward declarations have no
body and are not definitions. A name defined in more than one place is a row of data/type_dupes.csv:

  name       the qualified type name
  in_scope   `yes` if the shared state block reaches the type, else `no`. The scope starts from the
             `sim` and `mixed` rows of data/state_classes.csv, each resolved to a type (a global
             through data/globals.csv, a region of `struct Game` through its field in
             include/ta_types.h, a heap row through the allocated type, a static through its
             declaration) and closes over base classes and pointer, embedded and array members.
             `Game` is entered only through those regions, never expanded as a whole.
  defs       the number of definitions
  files      the definitions as `path:line`, sorted, `|` between
  layout     `identical` if the definitions are the same text, `same-layout` if clang gives them the
             same size and the same members (offset, type, name), else `differs`
  canonical  the file whose definition the rules below pick

Layouts come from clang-cl `-Xclang -fdump-record-layouts-complete`, run once per translation unit
that sees a definition and no other definition of the name (the #include graph of the files in
compile_commands.json decides which, with a .cpp's `#define A B` renames applied).

Canonical definition, in order:

  1. The size matches 3.1c. The sizes known from the exe are those of data/globals.csv rows whose
     size comes from the gap between addresses (`size_from` gap) and whose type is the name, divided
     by the array length. When no definition matches, or none is known, this rule does not apply.
  2. Among those, one in a real header (a .h under src/) wins over another file, then the one with
     the most named members.
  3. A header class with no data members (only methods) takes the member count of the fullest
     layout and keeps its place, so the header wins even though its own size is that of an empty class.

--check regenerates the list in memory and compares it with data/type_dupes.csv, which is the
allowlist. It fails when a name is new or gains a definition (a file is added, or `defs` grows),
and when the committed list differs in any other way, say a name is gone or `defs` shrunk: rerun the
tool and commit the result, so the list only shrinks. Line numbers in `files` are not compared.
"""

import argparse
import csv
import io
import json
import os
import re
import shlex
import subprocess
import sys
import time
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import rwsets
import stateclasses
from rwsets import ROOT

OUT = ROOT / "data/type_dupes.csv"
COLUMNS = ["name", "in_scope", "defs", "files", "layout", "canonical"]
SCAN_DIRS = ["include", "src", "link", "interfaces"]
SUFFIXES = {".h", ".cpp", ".hpp", ".inl", ".c"}
KW = {"struct", "class", "union"}
ACCESS = {"public", "private", "protected"}
TOKEN = re.compile(r"[A-Za-z_]\w*|\d[\w.]*|::|->|\S")
IDENT = re.compile(r"[A-Za-z_]\w*$")
LAYOUT_LINE = re.compile(r"^\s*(\d+)(?::(\d+)-(\d+))?\s*\|(\s*)(.*?)\s*$")
HEAP_ALLOCATOR = re.compile(r"^(operator new|new\[\]|new|GameAllocIgnoreTag|GameAllocShared|GameAllocThunk|GameAlloc|"
                            r"GameRealloc|PoolAlloc|malloc|calloc|realloc|GlobalAlloc\w*|LocalAlloc|HeapAlloc|"
                            r"VirtualAlloc)\s*")
NOT_MEMBER = ("typedef", "using", "friend", "static", "template", "enum")


def clean(text: str) -> str:
    """Source without comments and literals, with newlines kept; include paths survive."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif c == "/" and i + 1 < n and text[i + 1] == "*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("\n" * text.count("\n", i, j))
            i = j
        elif c == '"' and re.match(r"\s*#\s*include\s*$", text[text.rfind("\n", 0, i) + 1:i]):
            j = text.find('"', i + 1)
            out.append(text[i:j + 1])
            i = j + 1
        elif c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            out.append('""' if c == '"' else "0")
            i = j + 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


class FileScan:
    """The record definitions, typedef aliases, includes and `#define A B` renames of one file."""

    def __init__(self, path: Path, root: Path = ROOT):
        self.path = path
        self.rel = path.relative_to(root).as_posix()
        self.includes: list[tuple[str, str, dict]] = []
        self.renames: dict[str, str] = {}
        self.defs: list[dict] = []
        self.aliases: dict[str, set] = defaultdict(set)
        self.text = clean(path.read_text(errors="replace"))
        self.scan()

    def scan(self) -> None:
        toks = []
        lines = self.text.split("\n")
        skip_stack: list[bool] = []
        li = 0
        while li < len(lines):
            line = lines[li]
            if line.strip().startswith("#"):
                full = line.strip()
                while full.endswith("\\") and li + 1 < len(lines):
                    li += 1
                    full = full[:-1] + " " + lines[li].strip()
                self.directive(full[1:].strip(), skip_stack)
            elif not any(skip_stack):
                toks.extend((m.group(0), li + 1) for m in TOKEN.finditer(line))
            li += 1
        self.parse(toks)

    def directive(self, text: str, skip_stack: list[bool]) -> None:
        m = re.match(r"(\w+)\s*(.*)", text)
        word, rest = (m.group(1), m.group(2)) if m else ("", "")
        if word in ("if", "ifdef", "ifndef"):
            skip_stack.append(word == "if" and rest.strip() == "0")
        elif word == "else" and skip_stack:
            skip_stack[-1] = False
        elif word == "elif" and skip_stack:
            skip_stack[-1] = False
        elif word == "endif" and skip_stack:
            skip_stack.pop()
        elif any(skip_stack):
            return
        elif word == "include":
            mm = re.match(r'"([^"]+)"|<([^>]+)>', rest)
            if mm:
                self.includes.append(("q" if mm.group(1) else "a", mm.group(1) or mm.group(2), dict(self.renames)))
        elif word == "define":
            mm = re.match(r"(\w+)\s+([A-Za-z_]\w*)\s*$", rest)
            if mm:
                self.renames[mm.group(1)] = mm.group(2)
        elif word == "undef":
            self.renames.pop(rest.strip(), None)

    def parse(self, toks: list[tuple[str, int]]) -> None:
        stack: list[dict] = []
        stmt: list[int] = []
        i, n = 0, len(toks)

        def tok(k):
            return toks[k][0]

        def qualname():
            return "::".join(s["name"] for s in stack if s["kind"] in ("ns", "rec") and s["name"])

        def in_func():
            return any(s["kind"] == "other" for s in stack)

        while i < n:
            t = tok(i)
            if t in ACCESS and i + 1 < n and tok(i + 1) == ":":
                i += 2
                stmt = []
                continue
            if t == "{":
                stack.append(self.open_scope([tok(k) for k in stmt], stmt, toks, i, qualname, in_func))
                stmt = []
            elif t == "}":
                if stack:
                    self.close_scope(stack.pop(), toks, i, qualname() if stack else "")
                stmt = []
            elif t == ";":
                if not in_func() and stmt and tok(stmt[0]) in ("typedef", "using") and not (
                        stack and stack[-1]["kind"] == "rec"):
                    self.add_typedef([tok(k) for k in stmt])
                stmt = []
            else:
                stmt.append(i)
            i += 1

    def close_scope(self, scope: dict, toks: list, i: int, outer: str) -> None:
        d = scope.get("d")
        if scope["kind"] != "rec" or d is None:
            return
        j = i + 1
        declarators = []
        while j < len(toks) and toks[j][0] not in (";", "{", "}"):
            declarators.append(toks[j][0])
            j += 1
        ids = [x for x in declarators if IDENT.match(x)]
        if scope["typedef"]:
            if not d["name"] and ids:
                d["name"] = ids[0] if not outer else outer + "::" + ids[0]
            elif d["name"]:
                for a in ids:
                    self.aliases[a].add(d["name"].split("::")[-1])
        d["toks"] = [toks[k][0] for k in range(d["tok_start"], i + 1)]
        if d["name"]:
            self.defs.append(d)

    def add_typedef(self, st: list[str]) -> None:
        if st[0] == "using" and "=" in st:
            self.aliases[st[1]].update(x for x in st[st.index("=") + 1:] if re.match(r"[A-Za-z_]", x))
            return
        if "(" in st:
            return
        ids = [x for x in st[1:] if IDENT.match(x)]
        if len(ids) >= 2:
            self.aliases[ids[-1]].update(ids[:-1])

    def open_scope(self, st, stmt, toks, i, qualname, in_func) -> dict:
        if st and st[0] == "namespace":
            return {"kind": "ns", "name": st[1] if len(st) > 1 else ""}
        if st[:2] == ["extern", '""']:
            return {"kind": "ns", "name": ""}
        if "enum" in st:
            return {"kind": "other", "name": ""}
        for k in range(len(st) - 1, -1, -1):
            if st[k] not in KW or (k > 0 and st[k - 1] in ("enum", "<", ",")):
                continue
            rest = skip_attributes(st[k + 1:])
            if rest is None:
                continue
            name, tail = split_name(rest)
            if tail and tail[0] != ":":
                continue
            if tail and not all(re.match(r"[A-Za-z_]\w*$|::|,|:|<|>|\d+", x) for x in tail):
                continue
            outer = qualname()
            d = {"name": (outer + "::" + name).lstrip(":") if name else "", "file": self.rel,
                 "line": toks[stmt[0] + k][1] if stmt else toks[i][1], "tok_start": stmt[k],
                 "bases": [x for x in tail if IDENT.match(x) and x not in ACCESS and x != "virtual"],
                 "local": in_func()}
            return {"kind": "rec", "name": name, "d": d, "typedef": st[0] == "typedef"}
        return {"kind": "other", "name": ""}


def skip_attributes(rest: list[str]) -> list[str] | None:
    """The tokens after any `__declspec(...)` or `alignas(...)`, or None if parentheses remain."""
    while rest and rest[0] in ("__declspec", "alignas", "__attribute__"):
        depth = 0
        j = 1
        while j < len(rest):
            if rest[j] == "(":
                depth += 1
            elif rest[j] == ")":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        rest = rest[j + 1:]
    return None if "(" in rest else rest


def split_name(rest: list[str]) -> tuple[str, list[str]]:
    """The record name (with template arguments) and the tokens after it."""
    if not rest or not IDENT.match(rest[0]) or rest[0] in ACCESS:
        return "", rest
    name = rest[0]
    ri = 1
    if ri < len(rest) and rest[ri] == "<":
        depth = 0
        while ri < len(rest):
            name += rest[ri]
            depth += (rest[ri] == "<") - (rest[ri] == ">")
            ri += 1
            if depth == 0:
                break
    return name, rest[ri:]


def collect(root: Path = ROOT) -> dict[str, FileScan]:
    """The scan of every source and header under the scanned folders, by relative path."""
    files = {}
    for d in SCAN_DIRS:
        for p in sorted((root / d).rglob("*")):
            if p.suffix in SUFFIXES and p.is_file():
                fs = FileScan(p, root)
                files[fs.rel] = fs
    return files


def definitions(files: dict[str, FileScan]) -> dict[str, list[dict]]:
    """Every record name defined more than once (at distinct places), with its definitions."""
    by_name: dict[str, list[dict]] = defaultdict(list)
    for fs in files.values():
        for d in fs.defs:
            d["id"] = "%s:%d" % (d["file"], d["line"])
            d["norm"] = " ".join(d["toks"])
            by_name[d["name"]].append(d)
    for name, ds in by_name.items():
        seen = {}
        for d in ds:
            seen.setdefault(d["id"], d)
        by_name[name] = list(seen.values())
    return by_name


def resolve_include(files: dict, cur: str, kind: str, name: str) -> str | None:
    cands = []
    if kind == "q":
        cands.append(os.path.normpath(os.path.join(os.path.dirname(cur), name)))
    cands.append(os.path.normpath(os.path.join("include", name)))
    for c in cands:
        if c in files:
            return c
    low = {k.lower(): k for k in files}
    for c in cands:
        if c.lower() in low:
            return low[c.lower()]
    return None


def visible(files: dict, tu: str) -> dict[str, set]:
    """file -> the sets of renames in force on each visit to it from the translation unit `tu`."""
    seen: dict[str, set] = {}
    todo = [(tu, frozenset())]
    done = set()
    while todo:
        f, ren = todo.pop()
        if (f, ren) in done:
            continue
        done.add((f, ren))
        seen.setdefault(f, set()).add(ren)
        for kind, name, active in files[f].includes:
            r = resolve_include(files, f, kind, name)
            if r is not None:
                extra = frozenset(active.items()) if f == tu else frozenset()
                todo.append((r, ren | extra))
    return seen


def translation_units(build_dir: Path) -> dict[str, dict]:
    """The compile_commands.json entries of the files under src/ and link/, by relative path."""
    database = build_dir / "compile_commands.json"
    if not database.exists():
        rwsets.configure(build_dir)
    out = {}
    for entry in json.loads(database.read_text()):
        rel = os.path.relpath(os.path.realpath(entry["file"]), ROOT)
        if rel.startswith(("src/", "link/")):
            out[rel] = entry
    return out


def layout_argv(entry: dict) -> list[str]:
    argv = shlex.split(entry["command"])
    i = argv.index("-c") if "-c" in argv else argv.index("/c")
    argv = argv[:i] + ["-fsyntax-only"] + stateclasses.LAYOUT_FLAG + argv[i + 1:]
    return [a for a in argv if not a.startswith(("/Fo", "/Fd"))]


def parse_layouts(stdout: str) -> dict[str, dict]:
    """Every record of a clang layout dump: name -> {rows: [(offset, text)], size}."""
    recs: dict[str, dict] = {}
    cur = None
    for line in stdout.split("\n"):
        if line.startswith("*** Dumping AST Record Layout"):
            cur = None
            continue
        m = re.match(r"^\s*\|\s*\[sizeof=(\d+)", line)
        if m and cur is not None:
            cur["size"] = int(m.group(1))
            cur = None
            continue
        m = LAYOUT_LINE.match(line)
        if not m:
            continue
        sp = len(m.group(4))
        text = m.group(5)
        if cur is None and sp == 1 and re.match(r"(struct|class|union) ", text):
            name = re.sub(r"\s*\(empty\)$", "", re.sub(r"^(struct|class|union) ", "", text))
            cur = {"rows": [], "size": None, "sp": sp}
            recs[name] = cur
        elif cur is not None and sp == cur["sp"] + 2 and text != "(empty)":
            cur["rows"].append((int(m.group(1)), text))
    return recs


def run_layout(entry: dict) -> dict[str, dict]:
    p = subprocess.run(layout_argv(entry), capture_output=True, text=True, errors="replace", cwd=entry["directory"])
    return parse_layouts(p.stdout)


def norm_type(t: str) -> str:
    return re.sub(r"\s+", "", re.sub(r"\b(struct|class|union|enum)\s+", "", t))


def split_row(text: str) -> tuple[str, str]:
    """(type, member name) of a layout row; a base class or vtable pointer has no name."""
    text = re.sub(r"\((anonymous|unnamed) at [^)]*\)", "(anonymous)", text)
    if re.search(r"\((primary |virtual )*base\)$", text) or "vtable pointer" in text or "vbptr" in text \
            or text.endswith("(anonymous)"):
        return norm_type(text), ""
    m = re.match(r"^(.*?)(\S+)$", text)
    return norm_type(m.group(1)), m.group(2)


def signature(layout: dict | None) -> tuple[int, tuple] | None:
    """(size, ((offset, type, name), ...)) of a record layout, or None if clang gave none."""
    if not layout or layout["size"] is None:
        return None
    return layout["size"], tuple((o,) + split_row(t) for o, t in layout["rows"])


def named_members(sig: tuple | None) -> int:
    return sum(1 for _, _, name in sig[1] if name) if sig else 0


def layout_class(ds: list[dict], sigs: list) -> str:
    """`identical`, `same-layout` or `differs` for the definitions of one name."""
    if len({d["norm"] for d in ds}) == 1:
        return "identical"
    if all(s is not None for s in sigs) and len(set(sigs)) == 1:
        return "same-layout"
    return "differs"


def rank(d: dict) -> int:
    return 0 if d["file"].startswith("src/") and d["file"].endswith(".h") else 1


def pick_canonical(ds: list[dict], sigs: list, known_sizes: set[int]) -> str:
    """The file of the definition the canonical rules pick (see the module docstring)."""
    pairs = list(zip(ds, sigs))
    matching = [p for p in pairs if p[1] is not None and p[1][0] in known_sizes]
    fullest = max((named_members(s) for _, s in pairs), default=0)

    def method_only_header(p):
        return rank(p[0]) == 0 and p[1] is not None and named_members(p[1]) == 0

    if matching:
        pool = matching + [p for p in pairs if method_only_header(p) and p not in matching]
    else:
        pool = pairs

    def key(p):
        members = fullest if method_only_header(p) else named_members(p[1])
        return rank(p[0]), -members, p[0]["file"], p[0]["line"]

    return min(pool, key=key)[0]["file"]


def type_idents(text: str) -> list[str]:
    return re.findall(r"[A-Za-z_]\w*", text)


def member_idents(d: dict) -> tuple[list[str], list[str]]:
    """(member names, identifiers of the member types) from the token list of a definition."""
    toks = d["toks"]
    j = toks.index("{") if "{" in toks else len(toks)
    depth, cur, stmts = 0, [], []
    for t in toks[j:]:
        if t == "{":
            depth += 1
            if depth > 1:
                cur = []
            continue
        if t == "}":
            depth -= 1
            if depth == 0:
                break
            if depth == 1:
                cur = []
            continue
        if depth != 1:
            continue
        if t == ";":
            stmts.append(cur)
            cur = []
        elif t in ACCESS:
            continue
        elif t == ":" and cur and cur[-1] in ACCESS:
            cur = []
        else:
            cur.append(t)
    names, types = [], []
    for s in stmts:
        if not s or "(" in s or s[0] in NOT_MEMBER:
            continue
        cut = next((k for k, x in enumerate(s) if x in ("[", ":", "=")), len(s))
        head = [x for x in s[:cut] if IDENT.match(x)]
        if head:
            names.append(head[-1])
            types.extend(head[:-1])
    return names, types


class Scope:
    """Which type names the shared state block reaches."""

    def __init__(self, files: dict[str, FileScan], by_name: dict[str, list[dict]]):
        self.files = files
        self.by_name = by_name
        self.aliases: dict[str, set] = defaultdict(set)
        for fs in files.values():
            for a, v in fs.aliases.items():
                self.aliases[a] |= v
        self.last: dict[str, set] = defaultdict(set)
        for n in by_name:
            self.last[n.split("::")[-1]].add(n)
        for a in self.aliases:
            self.last[a].add(a)
        self.members: dict[str, list[str]] = {}
        for ds in by_name.values():
            for d in ds:
                self.members[d["id"]] = member_idents(d)[1]
        self.game_fields = self.game_field_types()

    def game_field_types(self) -> dict[str, list[str]]:
        out = {}
        for d in self.by_name.get("Game", []):
            if d["file"] != "include/ta_types.h":
                continue
            toks = d["toks"]
            j = toks.index("{") if "{" in toks else len(toks)
            depth, cur = 0, []
            for t in toks[j:]:
                if t == "{":
                    depth += 1
                    cur = [] if depth > 1 else cur
                elif t == "}":
                    depth -= 1
                    cur = []
                elif depth == 1 and t == ";":
                    cut = next((k for k, x in enumerate(cur) if x in ("[", ":", "=")), len(cur))
                    head = [x for x in cur[:cut] if IDENT.match(x)]
                    if head and "(" not in cur:
                        out[head[-1]] = head[:-1]
                    cur = []
                elif depth == 1:
                    cur.append(t)
        return out

    def expand(self, x: str, seen: set) -> set[str]:
        out = set(self.last.get(x, ()))
        if x in self.aliases and x not in seen:
            seen.add(x)
            for y in self.aliases[x]:
                out |= self.expand(y, seen)
        return out

    def names_of(self, idents) -> set[str]:
        out: set[str] = set()
        for x in idents:
            out |= self.expand(x, set())
        return out

    def find_declaration(self, name: str, path: str | None = None) -> str | None:
        pattern = re.compile(r"([\w:<>,\*&\s]+?)\b%s\b\s*(?:\[[^\]]*\]\s*)*(?:=|;|\{|\(|\))" % re.escape(name))
        paths = [path] if path and path in self.files else list(self.files)
        for p in paths:
            for m in pattern.finditer(self.files[p].text):
                pre = m.group(1).strip().split("\n")[-1].strip()
                if pre and not re.search(r"\b(return|else|case|goto|delete|new)\s*$", pre) \
                        and re.search(r"[A-Za-z_]\w*[\s\*&>]*$", pre) and "(" not in pre:
                    return pre
        return None

    def row_type(self, row: dict, globals_by_name: dict) -> list[str]:
        """The identifiers of the type a state_classes row resolves to."""
        kind, name = row["kind"], row["name"]
        if kind == "global":
            g = globals_by_name.get(name) or globals_by_name.get(name.split("::")[-1])
            if g:
                return type_idents(g["type"])
            pre = self.find_declaration(name.split("::")[-1])
            return type_idents(pre) if pre else []
        if kind == "game-region":
            return [x for part in name.split("|") for x in self.game_fields.get(part, [])]
        if kind == "static":
            base, _, where = name.partition("@")
            pre = self.find_declaration(base.split("::")[-1], where)
            return type_idents(pre) if pre else []
        if kind == "heap":
            tag = HEAP_ALLOCATOR.sub("", name.split(": ", 1)[-1].rsplit(" #", 1)[0])
            return type_idents(tag)
        return []

    def closure(self, rows: list[dict], globals_by_name: dict) -> set[str]:
        """The type names reached from the sim and mixed rows, through bases and members."""
        seeds: set[str] = set()
        for row in rows:
            seeds |= self.names_of(self.row_type(row, globals_by_name))
        reached = set(seeds)
        todo = list(seeds)
        while todo:
            n = todo.pop()
            if n == "Game":
                continue
            nxt: set[str] = set()
            for d in self.by_name.get(n, []):
                nxt |= self.names_of(self.members[d["id"]]) | self.names_of(d["bases"])
            if n in self.aliases and n not in self.by_name:
                nxt |= self.names_of(self.aliases[n])
            for m in nxt - reached:
                reached.add(m)
                todo.append(m)
        return reached


def known_sizes(globals_rows: list[dict], scope: Scope) -> dict[str, set[int]]:
    """Name -> the sizes 3.1c gives it, from globals whose size is the gap between addresses."""
    out: dict[str, set[int]] = defaultdict(set)
    for g in globals_rows:
        if g["size_from"] != "gap" or not g["size"].isdigit():
            continue
        t = re.sub(r"\b(const|volatile|struct|class|union)\b", " ", g["type"])
        m = re.fullmatch(r"\s*([A-Za-z_]\w*)\s*((?:\[\d+\]\s*)*)", t)
        if not m:
            continue
        count = 1
        for dim in re.findall(r"\d+", m.group(2)):
            count *= int(dim)
        size = int(g["size"])
        if count and size % count == 0:
            for name in scope.expand(m.group(1), set()):
                out[name].add(size // count)
    return out


def layout_units(dupes: dict, files: dict, units: dict) -> dict[str, set]:
    """Translation unit -> the (name, definition id) pairs whose layout it should print."""
    vis = {tu: visible(files, tu) for tu in units if tu in files}
    seen_in: dict[str, set] = defaultdict(set)
    for name, ds in dupes.items():
        short = name.split("::")[-1]
        for d in ds:
            for tu, v in vis.items():
                visits = v.get(d["file"])
                if visits and any(not any(a in (name, short) for a, _ in ren) for ren in visits):
                    seen_in[d["id"]].add(tu)
    wanted: dict[str, set] = defaultdict(set)
    for name, ds in dupes.items():
        for d in ds:
            cands = [d["file"]] if d["file"] in units else sorted(seen_in[d["id"]])
            if not cands:
                continue
            others = [o for o in ds if o["id"] != d["id"]]
            clean_cands = [c for c in cands if not any(c in seen_in[o["id"]] for o in others)]
            wanted[(clean_cands or cands)[0]].add((name, d["id"]))
    return wanted


def read_layouts(dupes: dict, files: dict, units: dict, jobs: int, run=run_layout) -> dict[str, dict | None]:
    """Definition id -> its clang layout (size and rows), None where no translation unit printed one."""
    wanted = layout_units(dupes, files, units)
    order = sorted(wanted)
    layouts: dict[str, dict | None] = {}
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        for tu, recs in zip(order, pool.map(lambda tu: run(units[tu]), order)):
            for name, did in wanted[tu]:
                layouts[did] = recs.get(name)
    return layouts


def build_rows(files: dict, units: dict, state_rows: list[dict], globals_rows: list[dict], jobs: int,
               run=run_layout) -> list[dict]:
    """The rows of data/type_dupes.csv from the scanned sources, the data files and the layouts."""
    by_name = definitions(files)
    dupes = {n: ds for n, ds in by_name.items() if len(ds) > 1}
    scope = Scope(files, by_name)
    in_scope = scope.closure([r for r in state_rows if r["class"] in ("sim", "mixed")],
                             {g["name"]: g for g in globals_rows})
    sizes = known_sizes(globals_rows, scope)
    layouts = read_layouts(dupes, files, units, jobs, run)
    rows = []
    for name in sorted(dupes):
        ds = sorted(dupes[name], key=lambda d: (d["file"], d["line"]))
        sigs = [signature(layouts.get(d["id"])) for d in ds]
        rows.append({"name": name, "in_scope": "yes" if name in in_scope else "no", "defs": str(len(ds)),
                     "files": "|".join(d["id"] for d in ds), "layout": layout_class(ds, sigs),
                     "canonical": pick_canonical(ds, sigs, sizes.get(name, set()))})
    return rows


def paths(files_cell: str) -> list[str]:
    return sorted(re.sub(r":\d+$", "", f) for f in files_cell.split("|") if f)


def compare(committed: list[dict], fresh: list[dict]) -> tuple[list[str], list[str]]:
    """(growth, stale): the ways the fresh list grew past the committed one, and the other differences."""
    old = {r["name"]: r for r in committed}
    new = {r["name"]: r for r in fresh}
    growth, stale = [], []
    for name, r in new.items():
        if name not in old:
            growth.append("new name %s: %s defs (%s)" % (name, r["defs"], r["files"].replace("|", ", ")))
            continue
        o = old[name]
        added = sorted(set(paths(r["files"])) - set(paths(o["files"])))
        if int(r["defs"]) > int(o["defs"]) or added:
            growth.append("%s gained a definition: %s defs, was %s%s" % (
                name, r["defs"], o["defs"], " (added " + ", ".join(added) + ")" if added else ""))
        elif any(r[c] != o[c] for c in COLUMNS if c != "files") or paths(r["files"]) != paths(o["files"]):
            stale.append("%s changed: %s" % (name, ", ".join(
                c for c in COLUMNS if (paths(r[c]) != paths(o[c]) if c == "files" else r[c] != o[c]))))
    stale.extend("%s is gone" % name for name in old if name not in new)
    return growth, stale


def read_rows(path: Path) -> list[dict]:
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def render(rows: list[dict]) -> str:
    buf = io.StringIO()
    w = csv.writer(buf, lineterminator="\n")
    w.writerow(COLUMNS)
    for r in rows:
        w.writerow([r[c] for c in COLUMNS])
    return buf.getvalue()


def generate(build_dir: Path, jobs: int) -> list[dict]:
    sys.setrecursionlimit(100000)
    return build_rows(collect(), translation_units(build_dir), stateclasses.read_rows(), stateclasses.read_globals(),
                      jobs)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if a type gained a definition or the list is stale")
    parser.add_argument("--build-dir", type=Path, default=rwsets.DEFAULT_BUILD_DIR)
    parser.add_argument("--out", type=Path, default=OUT, help="write the rows here instead of the data file")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    args = parser.parse_args()
    started = time.time()
    rows = generate(args.build_dir, args.jobs)
    in_scope = sum(1 for r in rows if r["in_scope"] == "yes")
    if not args.check:
        args.out.write_text(render(rows))
        print("typedupes: %d names with more than one definition (%d in scope), %.0fs" % (
            len(rows), in_scope, time.time() - started))
        return
    growth, stale = compare(read_rows(OUT), rows)
    for line in growth:
        print("typedupes: " + line)
    if growth:
        print("typedupes: a type must not gain a definition; remove the new definition or reuse the existing one")
    for line in stale[:40]:
        print("typedupes: " + line)
    if len(stale) > 40:
        print("typedupes: ... %d more" % (len(stale) - 40))
    if stale:
        print("typedupes: data/type_dupes.csv is out of date; run `uv run tools/typedupes.py` and commit the result")
    if growth or stale:
        sys.exit(1)
    print("typedupes: %d names with more than one definition (%d in scope), list up to date" % (len(rows), in_scope))


if __name__ == "__main__":
    main()

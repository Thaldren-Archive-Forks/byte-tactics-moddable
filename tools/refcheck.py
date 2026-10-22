"""Fail when a struct the shared state block holds has a raw pointer field that no CSV lists.

    uv run tools/refcheck.py                  # print the summary: block structs, rows by kind and issue, gaps
    uv run tools/refcheck.py --check          # fail on an unlisted raw pointer, a stale row, or an out-of-date outside list
    uv run tools/refcheck.py --write          # regenerate data/outside_pointers.csv, prune data/ref_exceptions.csv
    uv run tools/refcheck.py --write --init   # also (re)create data/ref_exceptions.csv from every raw field
    uv run tools/refcheck.py --check --open-issues   # also check that each listed issue is open
    uv run tools/refcheck.py --build-dir D    # use D/compile_commands.json (default build/modern)
    uv run tools/test_refcheck.py             # tests

The block-held structs are the type names tools/typedupes.py's Scope.closure reaches from the `sim` and
`mixed` rows of data/state_classes.csv, plus Game and SimGlobals and the structs Game holds by value
(block_structs). Every definition of each name is read, so a struct defined separately in several files
is checked once per file.

clang-cl reads each field, not the text: the AST JSON gives a FieldDecl's type with its typedefs
resolved (HANDLE is void *), and the record layout (-fdump-record-layouts-complete) gives offsets and
the vftable slots. A field is one of these kinds, anything else is not a pointer:

  pointer           T *, an array of them, a reference, in nested or anonymous structs and unions too
  function-pointer  a pointer to a function or to a member
  container         a std:: type, Vector_00438480, SimVector or SyncTaggedVector: pointers inside
  int-pointer       an integer field that code casts to a pointer (see CASTS), found in the syntax trees
  Ref<T>            allowed, not listed

A vtable pointer is not a field. It is allowed when the layout shows a vftable slot and the class is
polymorphic, and the check fails when the two disagree.

data/ref_exceptions.csv (struct,field,kind,file,issue) lists every raw field that remains, with the open
issue that removes it, sorted by struct then field. A field of a nested unnamed struct is `outer.inner`. A
row whose field is gone, is no longer raw or has another kind or file is stale, so the list only shrinks:
--write prunes it and never adds a row, only --init does.

data/outside_pointers.csv (struct,field,holder,offset,words,issue,kind,note) is generated from the
`outside` rows of data/block_pointers.csv: pointers whose targets are outside the block. A holder that is
a struct (Game, SimGlobals) is mapped through its record layout to the field that holds each offset; the
other holders have no struct, so their rows have a blank field and the first offset of the row. A field on
this list needs no exceptions row.

Prints one line per problem and exits 1 if there are any.
"""

import argparse
import csv
import io
import json
import os
import re
import subprocess
import sys
import time
from collections import Counter, defaultdict
from concurrent.futures import ProcessPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rwsets
import stateclasses
import typedupes
from rwsets import ROOT

EXCEPTIONS = "data/ref_exceptions.csv"
OUTSIDE = "data/outside_pointers.csv"
BLOCK_POINTERS = "data/block_pointers.csv"
SUBSYSTEMS = "data/subsystems.csv"
REPO = "ozgb/byte-tactics-moddable"
EXCEPTION_COLUMNS = ["struct", "field", "kind", "file", "issue"]
OUTSIDE_COLUMNS = ["struct", "field", "holder", "offset", "words", "issue", "kind", "note"]
KINDS = ("pointer", "function-pointer", "container", "int-pointer")
DEFAULT_BUILD_DIR = ROOT / "build/modern"
HOLDERS = {"Game": "AllocZeroedWithTickOffset"}
EXTRA_BLOCK = ("Game", "SimGlobals")
CONTAINERS = ("Vector_00438480", "SimVector", "SyncTaggedVector")
RECORD_KINDS = ("RecordDecl", "CXXRecordDecl")
PEELED = ("ImplicitCastExpr", "ParenExpr", "CStyleCastExpr", "CXXFunctionalCastExpr")
REPO_DIRS = ("src/", "include/", "link/", "interfaces/")
GAME = "Game"
VIEW_ISSUE = "#22"
GAME_SIM_ISSUE = "#350"
OTHER_ISSUE = "#353"
SUBSYSTEM_ISSUES = {"units": "#349", "orders": "#349", "weapons": "#349", "pathfinding": "#350", "map": "#350",
                    "los_radar": "#350", "game": "#350", "network": "#351", "ai": "#351", "players": "#351"}
STRUCT_ISSUES = {"Unit": "#349", "UnitDef": "#349", "WeaponDef": "#349", "Order": "#349", "OrderFx": "#349",
                 "Mission": "#349"}
OUTSIDE_ISSUES = [
    (re.compile(r"FeatureAnim\.state|view state|presentation|render cache"), VIEW_ISSUE),
    (re.compile(r"Mission order|damage table"), "#349"),
    (re.compile(r"MovementClass|LOS table"), GAME_SIM_ISSUE),
]
OUTSIDE_DEFAULT_ISSUE = "#352"
COUNTER_PATTERN = re.compile(r"^(0x[0-9a-f]+)(?:\+(0x[0-9a-f]+)\*k)?$")
ARRAY_SUFFIX = re.compile(r"\s*\[([^\]]*)\]")
LAYOUT_ROW = re.compile(r"^\s*(\d+)(?::\d+-\d+)? \|( *)(.*?)\s*$")
UNNAMED_AT = re.compile(r"\((?:unnamed|anonymous)(?: \w+)? at (.*?):(\d+):\d+\)")
TRANSPARENT = re.compile(r"\((?:unnamed|anonymous)[^)]*\)\s*$")
VFTABLE = re.compile(r"\((?:\S+ )?(?:vftable|vbtable) pointer\)|vtable pointer")
FUNCTION_POINTER = re.compile(r"\(\s*(?:__\w+\s+)?(?:[\w:]+::)?\*[^)]*\)\s*\(")
REF = re.compile(r"^(?:const\s+|volatile\s+)*(?:struct\s+|class\s+)?(?:[\w:]+::)?Ref$")
BUILTIN_WORDS = {"int", "char", "short", "long", "unsigned", "signed", "float", "double", "void", "bool", "const",
                 "struct", "BYTE", "WORD", "DWORD", "BOOL", "uint8_t", "uint16_t", "uint32_t", "size_t"}
csv.field_size_limit(1 << 28)


def strip_templates(t: str) -> str:
    """The type with every <...> argument list removed."""
    while True:
        u = re.sub(r"<[^<>]*>", "", t)
        if u == t:
            return t
        t = u


def field_kind(qual: str, desugared: str | None = None) -> str | None:
    """pointer, function-pointer, container, ref or None for a field of this clang type."""
    t = (desugared or qual).strip()
    flat = ARRAY_SUFFIX.sub("", strip_templates(t)).strip()
    if FUNCTION_POINTER.search(flat):
        return "function-pointer"
    if "*" in flat or flat.endswith("&"):
        return "pointer"
    name = re.sub(r"^(?:(?:const|volatile)\s+)*(?:(?:struct|class|union)\s+)?", "", flat)
    if REF.match(flat) or REF.match(strip_templates(qual.strip())):
        return "ref"
    if name.startswith("std::") or name in CONTAINERS:
        return "container"
    return None


@dataclass
class Layout:
    """A record layout: its size and its rows as (offset, depth, text)."""
    size: int = 0
    rows: list = field(default_factory=list)


def parse_layouts(text: str, directory: str = ".") -> tuple[dict[str, Layout], dict[tuple[str, int], Layout]]:
    """Every record of a clang layout dump: by name, and by (file, line) for the unnamed ones."""
    by_name: dict[str, Layout] = {}
    by_loc: dict[tuple[str, int], Layout] = {}
    cur = None
    for line in text.split("\n"):
        if line.startswith("*** Dumping AST Record Layout"):
            cur = None
            continue
        m = re.match(r"^\s*\|\s*\[sizeof=(\d+)", line)
        if m:
            if cur is not None:
                cur.size = int(m.group(1))
            cur = None
            continue
        m = LAYOUT_ROW.match(line)
        if not m:
            continue
        depth = (len(m.group(2)) - 1) // 2
        body = m.group(3)
        if depth == 0:
            head = re.match(r"(?:struct|class|union) (.*?)(?: \(empty\))?$", body)
            if not head:
                cur = None
                continue
            cur = Layout()
            name = head.group(1)
            loc = UNNAMED_AT.fullmatch(name)
            if loc:
                path = os.path.realpath(os.path.join(directory, loc.group(1)))
                by_loc[(path, int(loc.group(2)))] = cur
            else:
                by_name.setdefault(name, cur)
        elif cur is not None:
            cur.rows.append((int(m.group(1)), depth, body))
    return by_name, by_loc


@dataclass
class Leaf:
    """A layout member without members under it: its offset, the named struct that owns it, its path and type."""
    offset: int
    owner: str
    path: str
    type: str


def strip_keyword(t: str) -> str:
    return re.sub(r"^(?:struct|class|union|enum)\s+", "", t.strip())


def flatten(layout: Layout, record: str) -> tuple[list[Leaf], list[int]]:
    """(leaves, vftable offsets): the members of a record with nested structs expanded.

    A member of an anonymous struct or union belongs to the struct that holds it, a member of a named
    struct to that struct, and a member of an unnamed struct type to the holder as `member.inner`. The
    vftable offsets are the record's own slots, those of its bases, not those of members it holds by value."""
    leaves: list[Leaf] = []
    slots: list[int] = []
    stack: list[tuple[int, str, str, bool]] = [(0, record, "", False)]
    rows = layout.rows
    for i, (offset, depth, body) in enumerate(rows):
        while len(stack) > 1 and stack[-1][0] >= depth:
            stack.pop()
        owner, prefix, inside = stack[-1][1:]
        if VFTABLE.search(body):
            if not inside:
                slots.append(offset)
            continue
        if body == "(empty)":
            continue
        parent = i + 1 < len(rows) and rows[i + 1][1] > depth
        base = re.match(r"(?:struct |class )?(.*?) \((?:primary |virtual )*base\)$", body)
        if base:
            if parent:
                stack.append((depth, strip_keyword(base.group(1)), "", inside))
            continue
        if TRANSPARENT.search(body):
            if parent:
                stack.append((depth, owner, prefix, inside))
            continue
        name = body.rsplit(None, 1)[-1]
        type_ = body[:-len(name)].strip()
        if parent:
            inner = strip_keyword(type_)
            if UNNAMED_AT.search(inner):
                stack.append((depth, owner, prefix + name + ".", True))
            else:
                stack.append((depth, inner, "", True))
        else:
            leaves.append(Leaf(offset, owner, prefix + name, type_))
    return leaves, slots


@dataclass
class Member:
    """One field of a record in the syntax tree: its path, clang type and kind."""
    path: str
    qual: str
    kind: str | None


@dataclass
class Record:
    """A struct definition from the syntax tree, with the ids that member expressions refer to its fields by."""
    name: str
    file: str
    line: int
    polymorphic: bool
    members: list
    ids: dict


def members_of(node: dict, prefix: str, nested: list, ids: dict) -> list[Member]:
    """The fields of a record node, with anonymous members merged in and unnamed struct members as `a.b`."""
    out: list[Member] = []
    pending = None
    for child in node.get("inner", ()):
        kind = child.get("kind")
        if kind in RECORD_KINDS and child.get("completeDefinition"):
            if child.get("name"):
                nested.append(child)
                pending = None
            else:
                pending = child
        elif kind == "IndirectFieldDecl":
            ids[child.get("id", "")] = prefix + child.get("name", "")
        elif kind == "FieldDecl":
            type_ = child.get("type", {})
            qual = type_.get("qualType", "")
            name = child.get("name")
            unnamed = "(unnamed" in qual or "(anonymous" in qual
            if not name:
                if pending is not None:
                    out.extend(members_of(pending, prefix, nested, ids))
                pending = None
                continue
            ids[child.get("id", "")] = prefix + name
            out.append(Member(prefix + name, qual, field_kind(qual, type_.get("desugaredQualType"))))
            if pending is not None and unnamed and "*" not in qual:
                out.extend(members_of(pending, prefix + name + ".", nested, ids))
            pending = None
    return out


def record_of(node: dict, name: str, file: str, line: int, nested: list) -> Record:
    ids: dict[str, str] = {}
    members = members_of(node, "", nested, ids)
    return Record(name, file, line, bool((node.get("definitionData") or {}).get("isPolymorphic")), members, ids)


def repo_relative(file: str) -> str | None:
    rel = os.path.relpath(os.path.realpath(file), ROOT)
    return rel if rel.startswith(REPO_DIRS) else None


def cast_targets(node: dict) -> set[str]:
    """The ids of the fields a node reads as a pointer: an integer member expression under an IntegralToPointer cast."""
    out: set[str] = set()
    todo = [node]
    while todo:
        n = todo.pop()
        inner = n.get("inner", ())
        if n.get("castKind") == "IntegralToPointer" and inner:
            sub = inner[0]
            while sub.get("kind") in PEELED and sub.get("inner"):
                sub = sub["inner"][0]
            if sub.get("kind") == "MemberExpr" and sub.get("referencedMemberDecl"):
                out.add(sub["referencedMemberDecl"])
        todo.extend(inner)
    return out


def parse_tree(text: str) -> tuple[list[Record], dict[str, tuple[str, int]], set[tuple[str, str, str]]]:
    """(records, typedefs, casts) of a clang AST dump: the project struct definitions, each typedef'd
    unnamed struct's location, and the (struct, file, field) triples that code casts from integer to pointer."""
    start = text.index('"inner": [')
    position = ("", 0)
    records: list[Record] = []
    unnamed: dict[str, tuple[dict, str, int]] = {}
    typedefs: dict[str, tuple[str, int]] = {}
    targets: set[str] = set()
    for chunk in rwsets.CHUNK.finditer(text, start):
        body = chunk.group()
        file = rwsets.first_file(body) or position[0]
        kind = re.search(r'"kind": "(\w+)"', body[:400])
        wanted = kind and (kind.group(1) in RECORD_KINDS + ("TypedefDecl",) or "IntegralToPointer" in body)
        if not file or not wanted or repo_relative(file) is None:
            position = rwsets.last_position(body, *position)
            continue
        node = json.loads(body.rstrip().rstrip(","))
        tracker = rwsets.Position(*position)
        tracker.annotate(node)
        position = (tracker.file, tracker.line)
        rel = repo_relative(node["_file"])
        if "IntegralToPointer" in body:
            targets |= cast_targets(node)
        if rel is None:
            continue
        if node["kind"] == "TypedefDecl":
            for sub in node.get("inner", ()):
                found = unnamed.get(sub.get("decl", {}).get("id", ""))
                if found:
                    typedefs[node["name"]] = (found[1], found[2])
                    records.append(record_of(found[0], node["name"], found[1], found[2], []))
            continue
        if node["kind"] not in RECORD_KINDS or not node.get("completeDefinition"):
            continue
        if not node.get("name"):
            unnamed[node["id"]] = (node, rel, node["_line"])
            continue
        todo = [(node, node["name"])]
        while todo:
            current, qualified = todo.pop()
            nested: list = []
            records.append(record_of(current, qualified, rel, current.get("_line", node["_line"]), nested))
            todo.extend((n, qualified + "::" + n["name"]) for n in nested)
    casts = {(r.name, r.file, path) for r in records for id_, path in r.ids.items() if id_ in targets}
    return records, typedefs, casts


@dataclass
class UnitScan:
    """What one translation unit says: its records, typedefs and layouts."""
    records: list
    typedefs: dict
    by_name: dict
    by_loc: dict
    casts: set


def ast_argv(entry: dict) -> list[str]:
    argv = typedupes.layout_argv(entry)
    i = argv.index("--")
    return argv[:i] + ["-Xclang", "-ast-dump=json"] + argv[i:]


def scan_unit(job: tuple[dict, bool]) -> UnitScan:
    """Run clang-cl on one translation unit (layouts, then the syntax tree) and read both; keep=False drops all but the casts."""
    entry, keep = job
    run = subprocess.run(ast_argv(entry), cwd=entry["directory"], capture_output=True, text=True, errors="replace")
    found = rwsets.JSON_START.search(run.stdout)
    if found is None:
        raise SystemExit("refcheck: clang-cl printed no syntax tree for %s\n%s" % (entry["file"], run.stderr[-2000:]))
    records, typedefs, casts = parse_tree(run.stdout[found.start():])
    if not keep:
        return UnitScan([], {}, {}, {}, casts)
    by_name, by_loc = parse_layouts(run.stdout[:found.start()], entry["directory"])
    return UnitScan(records, typedefs, by_name, by_loc, casts)


def game_fields(by_name: dict) -> dict[str, tuple[list[str], bool]]:
    """field -> (type identifiers, held by value) for the fields of `struct Game`, anonymous unions and structs flattened.

    typedupes.Scope reads Game from include/ta_types.h only, where it no longer is, so its game-region
    rows resolve to nothing; this reads the definition wherever it is."""
    out = {}
    for d in by_name.get("Game", []):
        toks = d["toks"]
        depth, cur = 0, []
        for t in toks[toks.index("{") if "{" in toks else len(toks):]:
            if t == "{":
                depth += 1
                cur = []
            elif t == "}":
                depth -= 1
                cur = []
            elif depth >= 1 and t == ";":
                cut = next((k for k, x in enumerate(cur) if x in ("[", ":", "=")), len(cur))
                head = [x for x in cur[:cut] if x == "*" or typedupes.IDENT.match(x)]
                if len(head) > 1 and "(" not in cur and head[-1] != "*":
                    out[head[-1]] = ([x for x in head[:-1] if x != "*"], "*" not in head)
                cur = []
            elif depth >= 1:
                cur.append(t)
    return out


def block_structs(files: dict, by_name: dict, state_rows: list[dict], globals_rows: list[dict],
                  extra=EXTRA_BLOCK) -> set[str]:
    """The type names the state block holds: the closure from the sim and mixed rows, Game, SimGlobals, the
    types of Game's sim and mixed fields, and the structs Game holds by value."""
    scope = typedupes.Scope(files, by_name)
    fields = game_fields(by_name)
    rows = [r for r in state_rows if r["class"] in ("sim", "mixed") and r["kind"] != "game-region"]
    rows += [{"kind": "heap", "name": "new: " + name} for name in extra]
    wanted = {f for r in state_rows if r["kind"] == "game-region" and r["class"] in ("sim", "mixed")
              for f in r["name"].split("|")}
    wanted |= {f for f, (_, by_value) in fields.items() if by_value}
    rows += [{"kind": "heap", "name": "new: " + " ".join(fields[f][0])} for f in sorted(wanted) if f in fields]
    return scope.closure(rows, {g["name"]: g for g in globals_rows})


def unresolved_rows(files: dict, by_name: dict, state_rows: list[dict], globals_rows: list[dict]) -> list[dict]:
    """The sim and mixed rows whose type the text scan does not resolve to a struct with a definition."""
    scope = typedupes.Scope(files, by_name)
    gmap = {g["name"]: g for g in globals_rows}
    out = []
    for r in state_rows:
        if r["class"] in ("sim", "mixed") and r["kind"] in ("global", "heap", "static"):
            idents = [i for i in scope.row_type(r, gmap) if i not in BUILTIN_WORDS]
            if idents and not any(n in by_name for n in scope.names_of(idents)):
                out.append(r)
    return out


@dataclass
class Def:
    """One definition of a block struct and the layout and syntax tree of the unit that read it."""
    name: str
    file: str
    record: Record
    layout: Layout | None
    unit: str


def short(name: str) -> str:
    return name.split("::")[-1]


def read_definitions(build_dir: Path, names: set[str], by_name: dict, files: dict, jobs: int) \
        -> tuple[list[Def], dict[str, UnitScan], set]:
    """(definitions, scans, casts): every definition of the block structs with its record and layout, the
    scans of the units that read them, and the integer fields cast to pointers anywhere in the build."""
    units = typedupes.translation_units(build_dir)
    plan = typedupes.layout_units({n: by_name[n] for n in names if n in by_name}, files, units)
    order = sorted(units)
    scans: dict[str, UnitScan] = {}
    casts: set = set()
    with ProcessPoolExecutor(max_workers=jobs) as pool:
        for tu, scan in zip(order, pool.map(scan_unit, [(units[tu], tu in plan) for tu in order], chunksize=4)):
            casts |= scan.casts
            if tu in plan:
                scans[tu] = scan
    defs: list[Def] = []
    for tu in sorted(plan):
        scan = scans[tu]
        records = {(r.name, r.file): r for r in scan.records}
        for name, did in sorted(plan[tu]):
            file = did.rsplit(":", 1)[0]
            rec = records.get((name, file)) or records.get((short(name), file))
            if rec is None:
                continue
            layout = scan.by_name.get(name) or scan.by_name.get(short(name)) or scan.by_loc.get(
                (os.path.realpath(ROOT / file), rec.line))
            defs.append(Def(name, file, rec, layout, tu))
    return defs, scans, casts


@dataclass
class Found:
    """A raw pointer field of a block struct, with every file whose definition has it."""
    struct: str
    field: str
    kind: str
    files: set


def raw_fields(defs: list[Def], casts: set) -> dict[tuple[str, str, str], Found]:
    """(struct, field, kind) -> the Found, merging definitions that agree. An integer field that code
    casts to a pointer is an int-pointer."""
    out: dict[tuple[str, str, str], Found] = {}
    for d in defs:
        for m in d.record.members:
            kind = m.kind
            if kind is None and (d.record.name, d.file, m.path) in casts:
                kind = "int-pointer"
            if kind in KINDS:
                key = (short(d.name), m.path, kind)
                out.setdefault(key, Found(*key, files=set())).files.add(d.file)
    return out


def vptr_problems(defs: list[Def]) -> tuple[list[str], int]:
    """(problems, count): a class whose layout shows a vftable slot must be polymorphic, and the reverse."""
    problems, count = [], 0
    for d in defs:
        if d.layout is None:
            if d.record.polymorphic:
                problems.append("%s: %s is polymorphic but has no layout to verify its vftable slot" % (d.file, d.name))
            continue
        slots = flatten(d.layout, d.name)[1]
        count += bool(slots)
        if slots and not d.record.polymorphic:
            problems.append("%s: %s has a vftable slot at +0x%x but is not polymorphic" % (d.file, d.name, slots[0]))
        if d.record.polymorphic and not slots:
            problems.append("%s: %s is polymorphic but its layout has no vftable slot" % (d.file, d.name))
    return problems, count


def file_subsystems(root: Path) -> dict[str, Counter]:
    path = root / SUBSYSTEMS
    out: dict[str, Counter] = defaultdict(Counter)
    if path.exists():
        with path.open(newline="") as f:
            for r in csv.DictReader(f):
                out[r["file"]][r["subsystem"]] += 1
    return out


def subsystem_of(file: str, table: dict[str, Counter]) -> str:
    """The subsystem of a file: its own rows, else its same-stem .cpp, else the majority of its directory."""
    if table.get(file):
        return table[file].most_common(1)[0][0]
    stem = os.path.splitext(file)[0]
    for suffix in (".cpp", ".c"):
        if table.get(stem + suffix):
            return table[stem + suffix].most_common(1)[0][0]
    directory = os.path.dirname(file)
    votes: Counter = Counter()
    for f, c in table.items():
        if os.path.dirname(f) == directory:
            votes.update(c)
    return votes.most_common(1)[0][0] if votes else "other"


def game_field_classes(rows: list[dict]) -> dict[str, str]:
    """Game field name -> the state class of its region."""
    out = {}
    for r in rows:
        if r["kind"] == "game-region":
            for name in r["name"].split("|"):
                out[name] = r["class"]
    return out


def exception_issue(f: Found, table: dict[str, Counter], classes: dict[str, str]) -> str:
    if f.struct == GAME:
        cls = classes.get(f.field.split(".")[0], "")
        return GAME_SIM_ISSUE if cls in ("sim", "mixed") else OTHER_ISSUE
    if f.struct in STRUCT_ISSUES:
        return STRUCT_ISSUES[f.struct]
    return SUBSYSTEM_ISSUES.get(subsystem_of(sorted(f.files)[0], table), OTHER_ISSUE)


def kind_of_leaf(rec: Record | None, path: str) -> str | None:
    if rec is None:
        return None
    for m in rec.members:
        if m.path == path:
            return m.kind
    return None


class Locator:
    """Finds the struct field that holds an offset of a holder, through the layouts of one translation unit."""

    def __init__(self, scan: UnitScan, casts: set = frozenset()):
        self.scan = scan
        self.casts = {(r, p) for r, _, p in casts}
        self.records = {r.name: r for r in scan.records}
        self.cache: dict[str, list[Leaf]] = {}

    def layout(self, name: str) -> Layout | None:
        name = strip_keyword(name)
        if name in self.scan.by_name:
            return self.scan.by_name[name]
        if name in self.scan.typedefs:
            file, line = self.scan.typedefs[name]
            return self.scan.by_loc.get((os.path.realpath(ROOT / file), line))
        return None

    def leaves(self, name: str) -> list[Leaf]:
        if name not in self.cache:
            lay = self.layout(name)
            self.cache[name] = flatten(lay, name)[0] if lay else []
        return self.cache[name]

    def extent(self, leaf: Leaf) -> tuple[int, str | None, int]:
        """(count, element record or None, element size) of a leaf, for arrays of structs and of pointers."""
        count = 1
        for dim in re.findall(r"\[(\d+)\]", leaf.type):
            count *= int(dim)
        elem = re.sub(r"(?:\[\d+\])+$", "", leaf.type).strip()
        if elem.endswith("*") or "(*" in elem:
            return count, None, 4
        lay = self.layout(elem)
        return (count, strip_keyword(elem), lay.size) if lay and lay.size else (count, None, 0)

    def start(self, record: str, path: str) -> int | None:
        for leaf in self.leaves(record):
            if leaf.owner == record and leaf.path == path:
                return leaf.offset
        return None

    def find(self, record: str, offset: int, depth: int = 0) -> tuple[str, str, str, str] | None:
        """(owner struct, field path, kind, top-level field) of the raw field of `record` that covers `offset`."""
        if depth > 8:
            return None
        for leaf in self.leaves(record):
            count, elem, size = self.extent(leaf)
            if not size or not leaf.offset <= offset < leaf.offset + count * size:
                continue
            if elem is not None:
                found = self.find(elem, (offset - leaf.offset) % size, depth + 1)
                if found:
                    return found[:3] + (found[3] if depth else leaf.path.split(".")[0],)
                continue
            kind = kind_of_leaf(self.records.get(short(leaf.owner)), leaf.path)
            if kind is None and (short(leaf.owner), leaf.path) in self.casts:
                kind = "int-pointer"
            if kind in KINDS:
                return short(leaf.owner), leaf.path, kind, leaf.path.split(".")[0]
        return None


def expand_offsets(cell: str) -> list[tuple[str, int]]:
    """(token, base offset) of each offset in a block_pointers.csv cell; `a+s*k` stands for a, a+s, ..."""
    out = []
    for token in cell.split():
        m = COUNTER_PATTERN.match(token)
        if m:
            out.append((token, int(m.group(1), 16)))
    return out


def outside_issue(note: str, struct: str, top: str, classes: dict[str, str]) -> str:
    if struct == GAME:
        return VIEW_ISSUE if classes.get(top, "") in ("view", "platform") else GAME_SIM_ISSUE
    for pattern, issue in OUTSIDE_ISSUES:
        if pattern.search(note):
            return issue
    return OUTSIDE_DEFAULT_ISSUE


def outside_rows(root: Path, locators: dict[str, Locator], classes: dict[str, str]) -> list[dict]:
    """The rows of data/outside_pointers.csv from the `outside` rows of data/block_pointers.csv."""
    with (root / BLOCK_POINTERS).open(newline="") as f:
        source = [r for r in csv.DictReader(f) if r["class"] == "outside" and r["offsets"] != "*"]
    rows: dict[tuple, dict] = {}

    def add(struct, name, holder, token, words, issue, kind, note):
        rows.setdefault((struct, name, holder if not struct else "", token if not struct else ""), {
            "struct": struct, "field": name, "holder": holder, "offset": token, "words": words, "issue": issue,
            "kind": kind, "note": note})

    for r in source:
        holder = r["holder"].split(": ")[-1]
        tokens = expand_offsets(r["offsets"])
        note = r["note"]
        member = re.fullmatch(r"SimGlobals\.(\w+)", holder)
        if holder == HOLDERS[GAME] and GAME in locators:
            for token, base in tokens:
                hit = locators[GAME].find(GAME, base)
                if hit:
                    add(hit[0], hit[1], holder, token, 1, outside_issue(note, GAME, hit[3], classes), hit[2], note)
        elif member and "SimGlobals" in locators and tokens:
            loc = locators["SimGlobals"]
            begin = loc.start("SimGlobals", member.group(1))
            hit = loc.find("SimGlobals", begin + tokens[0][1]) if begin is not None else None
            if hit is None:
                kind = kind_of_leaf(loc.records.get("SimGlobals"), member.group(1))
                hit = ("SimGlobals", member.group(1), kind, member.group(1)) if kind in KINDS else None
            if hit:
                add(hit[0], hit[1], holder, tokens[0][0], len(tokens), outside_issue(note, "SimGlobals", "", classes),
                    hit[2], note)
        elif tokens:
            add("", "", holder, tokens[0][0], len(tokens), outside_issue(note, "", "", classes), "", note)
    return sorted(rows.values(), key=lambda r: (r["struct"], r["field"], r["holder"], r["offset"]))


def render(columns: list[str], rows: list[dict]) -> str:
    buf = io.StringIO()
    w = csv.writer(buf, lineterminator="\n")
    w.writerow(columns)
    for r in rows:
        w.writerow([r[c] for c in columns])
    return buf.getvalue()


def read_csv(path: Path) -> list[dict]:
    if not path.exists():
        return []
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def fresh_exceptions(found: dict, outside_fields: set, table, classes) -> list[dict]:
    rows = []
    for (struct, name, kind), f in found.items():
        if (struct, name) in outside_fields:
            continue
        rows.append({"struct": struct, "field": name, "kind": kind, "file": "|".join(sorted(f.files)),
                     "issue": exception_issue(f, table, classes)})
    return sorted(rows, key=lambda r: (r["struct"], r["field"], r["kind"]))


@dataclass
class Result:
    """Everything one run reads: the structs, definitions, raw fields and the generated outside list."""
    names: set
    defs: list
    found: dict
    outside: list
    vptr_problems: list
    vptrs: int
    unresolved: list
    table: dict
    classes: dict


def analyse(root: Path, build_dir: Path, jobs: int) -> Result:
    sys.setrecursionlimit(100000)
    files = typedupes.collect(root)
    by_name = typedupes.definitions(files)
    state_rows = stateclasses.read_rows()
    globals_rows = stateclasses.read_globals()
    names = block_structs(files, by_name, state_rows, globals_rows)
    defs, scans, casts = read_definitions(build_dir, names, by_name, files, jobs)
    classes = game_field_classes(state_rows)
    table = file_subsystems(root)
    locators = {}
    for d in defs:
        if short(d.name) in (GAME, "SimGlobals") and short(d.name) not in locators:
            locators[short(d.name)] = Locator(scans[d.unit], casts)
    found = raw_fields(defs, casts)
    problems, vptrs = vptr_problems(defs)
    return Result(names, defs, found, outside_rows(root, locators, classes), problems, vptrs,
                  unresolved_rows(files, by_name, state_rows, globals_rows), table, classes)


def check(root: Path, result: Result, open_issues: bool = False, gh: str = "gh") -> list[str]:
    problems = list(result.vptr_problems)
    outside_fields = {(r["struct"], r["field"]) for r in result.outside if r["field"]}
    rows = read_csv(root / EXCEPTIONS)
    listed: dict[tuple, dict] = {}
    for r in rows:
        if not re.fullmatch(r"#[0-9]+", r["issue"]):
            problems.append("%s: %s.%s: issue %r is not #<number>" % (EXCEPTIONS, r["struct"], r["field"], r["issue"]))
        listed[(r["struct"], r["field"], r["kind"])] = r
    for key, f in sorted(result.found.items()):
        if (f.struct, f.field) in outside_fields or key in listed:
            continue
        problems.append("%s: %s.%s is a raw %s in a block-held struct and is in neither %s nor %s, use Ref<T>" % (
            sorted(f.files)[0], f.struct, f.field, f.kind, EXCEPTIONS, OUTSIDE))
    for key, r in sorted(listed.items()):
        f = result.found.get(key)
        if f is None:
            problems.append("%s: stale row %s.%s: the field is gone, is no longer a raw %s or is in the struct no more, "
                            "remove the row" % (EXCEPTIONS, key[0], key[1], key[2]))
        elif (key[0], key[1]) in outside_fields:
            problems.append("%s: stale row %s.%s: the field is on %s, remove the row" % (
                EXCEPTIONS, key[0], key[1], OUTSIDE))
        elif r["file"] != "|".join(sorted(f.files)):
            problems.append("%s: stale row %s.%s: file is %s, expected %s, run `uv run tools/refcheck.py --write`" % (
                EXCEPTIONS, key[0], key[1], r["file"], "|".join(sorted(f.files))))
    want = render(OUTSIDE_COLUMNS, result.outside)
    path = root / OUTSIDE
    have = path.read_text() if path.exists() else ""
    if have != want:
        problems.append("%s: out of date, run `uv run tools/refcheck.py --write` and commit the result" % OUTSIDE)
    if open_issues:
        states = {}
        for issue in sorted({r["issue"] for r in rows} | {r["issue"] for r in read_csv(path)}):
            if re.fullmatch(r"#[0-9]+", issue):
                states[issue] = gh_issue_state(issue[1:], gh)
        for issue, state in sorted(states.items()):
            if state != "OPEN":
                problems.append("%s: issue %s is %s, not open" % (EXCEPTIONS, issue, state or "not readable"))
    return problems


def gh_issue_state(number: str, gh: str) -> str | None:
    cmd = [*gh.split(), "issue", "view", str(number), "-R", REPO, "--json", "state"]
    try:
        return json.loads(subprocess.run(cmd, capture_output=True, text=True, check=True).stdout)["state"]
    except (OSError, subprocess.CalledProcessError, ValueError, KeyError):
        return None


def write(root: Path, result: Result, init: bool) -> None:
    outside_fields = {(r["struct"], r["field"]) for r in result.outside if r["field"]}
    path = root / EXCEPTIONS
    if init or not path.exists():
        rows = fresh_exceptions(result.found, outside_fields, result.table, result.classes)
    else:
        fresh = {k: f for k, f in result.found.items() if (f.struct, f.field) not in outside_fields}
        rows = []
        for r in read_csv(path):
            f = fresh.get((r["struct"], r["field"], r["kind"]))
            if f is not None:
                rows.append({**r, "file": "|".join(sorted(f.files))})
        rows.sort(key=lambda r: (r["struct"], r["field"], r["kind"]))
    path.write_text(render(EXCEPTION_COLUMNS, rows))
    (root / OUTSIDE).write_text(render(OUTSIDE_COLUMNS, result.outside))


def summary(result: Result) -> list[str]:
    outside_fields = {(r["struct"], r["field"]) for r in result.outside if r["field"]}
    exc = [f for f in result.found.values() if (f.struct, f.field) not in outside_fields]
    lines = ["refcheck: %d block-held names, %d definitions, %d with a vftable slot" % (
        len(result.names), len(result.defs), result.vptrs)]
    lines.append("refcheck: exceptions by kind: " + ", ".join(
        "%s %d" % (k, n) for k, n in sorted(Counter(f.kind for f in exc).items())))
    lines.append("refcheck: outside rows by kind: " + ", ".join(
        "%s %d" % (k or "unstructured", n) for k, n in sorted(Counter(r["kind"] for r in result.outside).items())))
    lines.append("refcheck: outside rows by issue: " + ", ".join(
        "%s %d" % (k, n) for k, n in sorted(Counter(r["issue"] for r in result.outside).items())))
    for r in result.unresolved:
        lines.append("refcheck: gap: %s %s resolves to no struct definition" % (r["kind"], r["name"]))
    return lines


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD_DIR)
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--init", action="store_true", help="with --write: rebuild the exceptions from every raw field")
    parser.add_argument("--open-issues", action="store_true", help="also check that each listed issue is open")
    parser.add_argument("--gh", default="gh", help="the gh command to run (default gh)")
    args = parser.parse_args(argv)
    started = time.time()
    result = analyse(args.root, args.build_dir, args.jobs)
    if args.write:
        write(args.root, result, args.init)
    if args.check:
        problems = check(args.root, result, args.open_issues, args.gh)
        for line in problems:
            print(line)
        if problems:
            return 1
    if args.check:
        print("refcheck: %d block-held structs, %d exceptions, %d outside rows, up to date, %.0fs" % (
            len(result.names), len(read_csv(args.root / EXCEPTIONS)), len(result.outside), time.time() - started))
        return 0
    for line in summary(result):
        print(line)
    print("refcheck: %.0fs" % (time.time() - started))
    return 0


if __name__ == "__main__":
    sys.exit(main())

"""Move the sim globals into the sim block (#284).

Every row of data/state_classes.csv with kind global or static and class sim
(bar the exclusions below) becomes a field of `SimGlobals`, a struct that
lives in the sim block. The script

  - writes src/state/sim_globals.h and src/state/sim_globals.csv,
  - deletes the extern declaration of each moved global from the sources and
    includes sim_globals.h instead, with a per-file typed view where the
    file declared another type than the field has,
  - deletes the definitions (link/data.cpp, link/globals.h and the sources),
  - sets `defined` to SimGlobals in data/globals.csv.

It is re-runnable: on a rewritten tree it changes nothing, and after a rebase
that brings new declarations or uses it rewrites them.

    uv run tools/simglobals.py [--check]
"""

from __future__ import annotations

import argparse
import csv
import io
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STATE_CLASSES = ROOT / "data" / "state_classes.csv"
GLOBALS = ROOT / "data" / "globals.csv"
LAYOUT_REFS = ROOT / "data" / "layout_refs.csv"
SIM_HEADER = ROOT / "src" / "state" / "sim_globals.h"
SIM_CSV = ROOT / "src" / "state" / "sim_globals.csv"
VECTOR_HEADER = ROOT / "src" / "map" / "vector_00438480.h"
LINK_DATA = ROOT / "link" / "data.cpp"
LINK_GLOBALS = ROOT / "link" / "globals.h"

EXCLUDED = {"s_cobScriptCache@src/util/int_map.cpp", "g_game"}
SKIP_FILES = {"src/state/sim_globals.h", "src/state/sim_globals.cpp", "src/harness/trace_schema.h"}
SKIP_FILES |= {"src/map/vector_00438480.h"}
SOURCE_SUFFIXES = (".cpp", ".h", ".inl")

TYPE_OVERRIDES = {
    "g_explodedPieces": "ExplodedPiece*[100]",
    "g_missionOrderTableVec": "Vector_00438480<int>",
    "g_missionOrderTableBegin": "char*",
    "g_playerAI": "PlayerAI*[10]",
    "g_quirks": "Quirks",
}
STATIC_FILES = {"g_movementClasses": "src/units/movement_class.cpp"}
ELEMENT_SIZES = {"char": 1, "unsigned char": 1, "signed char": 1, "short": 2, "unsigned short": 2, "int": 4,
                 "unsigned int": 4, "float": 4}

HEADER_INCLUDES = [
    '"win_types.h"',
    "<stddef.h>",
    "<memory>",
    '"block.h"',
    '"../graphics/memory_cache.h"',
    '"../map/los_tables.h"',
    '"../map/vector_00438480.h"',
    '"../network/packet_manager.h"',
    '"../quirks/quirks.h"',
    '"../units/movement_class.h"',
    '"../util/vec3.h"',
]
FORWARD_DECLARATIONS = ["struct ExplodedPiece;", "struct PlayerAI;", "struct WeaponDef;"]

VECTOR_CLASS = """#ifndef MAP_VECTOR_00438480_H
#define MAP_VECTOR_00438480_H

#include <memory>

template <class T, class A = std::allocator<T> >
class Vector_00438480 {
public:
    explicit Vector_00438480(const A& al = A())
        : allocator(al), first(0), last(0), end(0) {}
    ~Vector_00438480()
    {
        allocator.deallocate(first, end - first);
        first = 0, last = 0, end = 0;
    }

    A allocator;
    T* first;
    T* last;
    T* end;
};

#endif
"""
VECTOR_CLASS_IN_SOURCE = re.compile(
    r"(?:^//[^\n]*\n)*^template <class T, class A = std::allocator<T> >\nclass Vector_00438480 \{.*?^\};\n\n?",
    re.S | re.M)


class Row:
    def __init__(self, name: str, address: int | None, kind: str):
        self.name = name
        self.address = address
        self.kind = kind
        self.leaf = name.split("@")[0].split("::")[-1]
        self.csv: dict[str, str] | None = None
        self.container: str | None = None
        self.offset: int | None = None
        self.alias_of: str | None = None
        self.type = ""
        self.size: int | None = None
        self.init = ""

    @property
    def interior(self) -> bool:
        return self.container is not None

    @property
    def field(self) -> str:
        return self.alias_of or self.leaf


def make_decl(type_: str, name: str) -> str:
    """The declaration of name with the type, in C++ declarator syntax."""
    if "(" in type_:
        m = re.match(r"^(.*?\()([^()]*)\)(.*)$", type_)
        head, inside, tail = m.groups()
        cut = inside.find("[")
        pre, post = (inside, "") if cut < 0 else (inside[:cut], inside[cut:])
        return f"{head}{pre} {name}{post}){tail}"
    if type_.endswith("]"):
        cut = type_.index("[")
        return f"{type_[:cut]} {name}{type_[cut:]}"
    return f"{type_} {name}"


def type_key(decl: str, name: str) -> str:
    """The declaration with the name removed and the spaces squeezed."""
    return re.sub(r"\s+", "", re.sub(rf"\b{re.escape(name)}\b", "", decl))


def same_type(a: str, b: str) -> bool:
    """Whether two type keys are one type, an unsized array matching any size."""
    if a == b:
        return True
    return re.sub(r"\[\d*\]", "[]", a) == re.sub(r"\[\d*\]", "[]", b) and (
        "[]" in a or "[]" in b)


def load_rows() -> list[Row]:
    with STATE_CLASSES.open(newline="") as fh:
        sim = [r for r in csv.DictReader(fh)
               if r["kind"] in ("global", "static") and r["class"] == "sim" and r["name"] not in EXCLUDED]
    with GLOBALS.open(newline="") as fh:
        globals_by_name = {r["name"]: r for r in csv.DictReader(fh)}
    rows = []
    for r in sim:
        row = Row(r["name"], int(r["address"], 16) if r["address"] else None, r["kind"])
        row.csv = globals_by_name.get(r["name"])
        rows.append(row)
    by_leaf = {r.leaf: r for r in rows if r.kind == "global"}
    for row in rows:
        if row.kind == "static":
            twin = by_leaf.get(row.leaf)
            if twin is not None:
                row.alias_of = twin.leaf
        if row.csv is None:
            continue
        m = re.match(r"^in (.+)\+0x([0-9a-f]+)$", row.csv["defined"])
        if m:
            row.container = m.group(1).split("::")[-1]
            row.offset = int(m.group(2), 16)
        row.size = int(row.csv["size"])
        row.type = row.csv["type"]
        row.init = row.csv["init"]
    return rows


def resolve_type(row: Row) -> str:
    """The canonical type of a field or interior view, with an unsized array sized."""
    if row.leaf in TYPE_OVERRIDES:
        return TYPE_OVERRIDES[row.leaf]
    if row.csv is None:
        sys.exit(f"simglobals: {row.name} has no row in data/globals.csv and no entry in TYPE_OVERRIDES")
    t = row.type
    if row.leaf == "g_missionOrderTableVec":
        t = "Vector_00438480<int>"
    elif "(" not in t and "[]" in t and not row.interior:
        elem = t[:t.index("[")]
        width = 4 if elem.endswith("*") else ELEMENT_SIZES.get(elem)
        if width is None or row.size % width:
            sys.exit(f"simglobals: cannot size {row.name}: {t} ({row.size} bytes)")
        t = f"{elem}[{row.size // width}]"
    return t


def field_rows(rows: list[Row]) -> list[Row]:
    """One row per field of the struct, in 3.1c address order, the unaddressed last."""
    seen: dict[str, Row] = {}
    for row in rows:
        if row.interior or row.field in seen:
            continue
        if row.alias_of:
            continue
        seen[row.field] = row
    ordered = sorted(seen.values(), key=lambda r: (r.address is None, r.address or 0, r.leaf))
    return ordered


def initialiser(row: Row) -> str:
    """The default member initialiser of a .data field: the first dword of its init."""
    if row.csv is None or row.csv["section"] != ".data":
        return ""
    value = int.from_bytes(bytes.fromhex(row.init)[:4], "little") if row.init else 0
    return f" = {value}" if value else ""


def header_text(rows: list[Row]) -> str:
    fields = field_rows(rows)
    out = ["#ifndef STATE_SIM_GLOBALS_H", "#define STATE_SIM_GLOBALS_H", "", "#pragma pack(push, 8)", ""]
    out += [f"#include {i}" for i in HEADER_INCLUDES]
    out += [""] + FORWARD_DECLARATIONS + ["", "struct SimGlobals {"]
    for row in fields:
        decl = make_decl(resolve_type(row), row.leaf)
        out.append(f"    {decl}{initialiser(row)};")
    out += ["};", "", "SimGlobals& SimCreate();", "", "extern SimGlobals* g_simGlobals;", "",
            "inline SimGlobals& Sim()", "{", "    return g_simGlobals != 0 ? *g_simGlobals : SimCreate();", "}", ""]
    out += ['#pragma clang diagnostic push', '#pragma clang diagnostic ignored "-Winvalid-offsetof"']
    for a, b in zip(fields, fields[1:]):
        if a.address is not None and b.address is not None and a.address + a.size == b.address:
            out.append(f"static_assert(__builtin_offsetof(SimGlobals, {b.leaf}) - __builtin_offsetof(SimGlobals, {a.leaf}) "
                       f"== {b.address - a.address}, \"{a.leaf} is followed by {b.leaf}\");")
    for row in fields:
        if row.csv is not None and row.csv["size_from"] == "definition" and row.size:
            out.append(f"static_assert(sizeof(((SimGlobals*)0)->{row.leaf}) == {row.size}, \"{row.leaf} size\");")
    out += ['#pragma clang diagnostic pop', "", "#pragma pack(pop)", ""]
    for row in fields:
        out.append(f"#define {row.leaf} (Sim().{row.leaf})")
    for row in rows:
        if not row.interior:
            continue
        t = resolve_type(row)
        tname = f"SimType_{row.leaf}"
        out.append(f"typedef {make_decl(t, tname)};")
        out.append(f"#define {row.leaf} (*reinterpret_cast<{tname}*>"
                   f"(reinterpret_cast<char*>(&{row.container}) + 0x{row.offset:x}))")
    out += ["", "#endif", ""]
    return "\n".join(out)


def static_file(row: Row) -> str:
    """The file a file-static row belongs to when its name does not say (only a static with a global twin needs it)."""
    if row.kind != "static" or not row.alias_of or "@" in row.name:
        return ""
    if row.leaf not in STATIC_FILES:
        sys.exit(f"simglobals: the static {row.name} shares its field with a global; add its file to STATIC_FILES")
    return STATIC_FILES[row.leaf]


def csv_text(rows: list[Row]) -> str:
    buf = io.StringIO()
    w = csv.writer(buf, lineterminator="\n")
    w.writerow(["field", "name", "container", "offset", "file"])
    for row in sorted(rows, key=lambda r: (r.address is None, r.address or 0, r.name)):
        if row.interior:
            w.writerow([row.leaf, row.name, row.container, f"0x{row.offset:x}", ""])
        else:
            w.writerow([row.field, row.name, "", "", static_file(row)])
    return buf.getvalue()


def strip_code(line: str) -> str:
    line = re.sub(r'"(?:\\.|[^"\\])*"', '""', line)
    return re.sub(r"//.*$", "", line)


class Tool:
    def __init__(self, rows: list[Row], check: bool):
        self.rows = rows
        self.check = check
        self.by_leaf: dict[str, Row] = {}
        for row in rows:
            self.by_leaf.setdefault(row.leaf, row)
        self.names = sorted(self.by_leaf, key=len, reverse=True)
        self.word = re.compile(r"\b(" + "|".join(re.escape(n) for n in self.names) + r")\b")
        self.canonical = {leaf: type_key(make_decl(resolve_type(r), leaf), leaf) for leaf, r in self.by_leaf.items()}
        self.changed: list[str] = []
        self.views: list[tuple[str, str]] = []
        self.warnings: list[str] = []

    def write(self, path: Path, text: str) -> None:
        old = path.read_text() if path.exists() else None
        if old != text:
            self.changed.append(str(path.relative_to(ROOT)))
            if not self.check:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text)

    def declaration_of(self, text: str, initialised: bool = False) -> tuple[str, str] | None:
        """(name, decl) of a one-declarator variable declaration naming a moved global."""
        code = strip_code(text).strip()
        m = re.match(r"^(?:static\s+)?(?:extern\s+)?(.+?);\s*$", code)
        if not m:
            return None
        decl = m.group(1)
        if initialised:
            decl = decl.split(" = ")[0]
        if "," in decl.split("(")[0] or "=" in decl:
            return None
        found = [n for n in self.names if re.search(rf"\b{re.escape(n)}\b", decl)]
        if len(found) != 1:
            return None
        name = found[0]
        decl = re.sub(rf"\b\w+::(?={re.escape(name)}\b)", "", decl)
        if len(re.findall(rf"\b{re.escape(name)}\b", decl)) != 1:
            return None
        return name, decl

    def rewrite_source(self, rel: str) -> None:
        path = ROOT / rel
        text = path.read_text()
        original = text
        lines = text.split("\n")
        include_rel = os.path.relpath(SIM_HEADER, path.parent).replace(os.sep, "/")
        out: list[str] = []
        for line in lines:
            extern = re.match(r"^(\s*)extern\s+(?!\")(.*?);\s*(?://.*)?$", line)
            hit = self.declaration_of(line) if extern else None
            is_definition = False
            if hit is None and line.strip() and not line.startswith((" ", "\t", "#", "/")):
                hit = self.declaration_of(line)
                is_definition = hit is not None
            if hit is None:
                out.append(line)
                continue
            name, decl = hit
            row = self.by_leaf[name]
            if is_definition:
                while out and out[-1].startswith("//"):
                    out.pop()
            bare = re.sub(r"^(static\s+)?(extern\s+)?", "", decl.strip())
            if same_type(type_key(bare, name), self.canonical[name]):
                continue
            indent = extern.group(1) if extern else ""
            view = f"SimView_{name}"
            typedef = re.sub(rf"\b{re.escape(name)}\b", view, bare)
            out += [f"{indent}typedef {typedef};", f"#undef {name}",
                    f"#define {name} (*reinterpret_cast<{view}*>({self.address_expr(row)}))"]
            self.views.append((rel, name))
        text = "\n".join(out)
        if text.endswith("\n\n") and not original.endswith("\n\n"):
            text = text.rstrip("\n") + "\n"
        text = re.sub(r"\bMovementClassTable::g_movementClasses\b", "g_movementClasses", text)
        text = self.add_include(text, include_rel)
        self.write(path, text)

    def address_expr(self, row: Row) -> str:
        if row.interior:
            return f"reinterpret_cast<char*>(&{row.container}) + 0x{row.offset:x}"
        return f"&Sim().{row.leaf}"

    def add_include(self, text: str, include_rel: str) -> str:
        lines = text.split("\n")
        if any(re.match(r'^\s*#\s*include\s+"[^"]*state/sim_globals\.h"', l) for l in lines):
            return text
        first = None
        in_comment = False
        for n, l in enumerate(lines):
            code = l
            if in_comment:
                if "*/" in code:
                    in_comment = False
                continue
            if code.strip().startswith("/*") and "*/" not in code:
                in_comment = True
                continue
            if code.startswith("#define") or code.startswith("#undef") or self.word.search(strip_code(code)):
                if not code.startswith("#include") and self.word.search(strip_code(code)):
                    first = n
                    break
        if first is None:
            return text
        last = -1
        for n in range(first):
            if re.match(r"^\s*#\s*include\b", lines[n]):
                last = n
        lines.insert(last + 1, f'#include "{include_rel}"')
        if "#include <new.h>" in lines and "#include <new>" not in lines:
            lines.insert(lines.index("#include <new.h>"), "#include <new>")
        return "\n".join(lines)

    def strip_link(self) -> None:
        for path in (LINK_DATA, LINK_GLOBALS):
            kept = []
            for line in path.read_text().split("\n"):
                code = strip_code(line)
                comment_row = re.match(r"^//\s+0x[0-9a-f]+\s+(\S+?):", line)
                if comment_row and comment_row.group(1).split("::")[-1] in self.by_leaf \
                        and comment_row.group(1) in {r.name for r in self.rows}:
                    continue
                if line.startswith("//") or line.startswith("#") or not code.strip():
                    kept.append(line)
                    continue
                if self.declaration_of(line, initialised=True) is not None:
                    continue
                kept.append(line)
            self.write(path, "\n".join(kept))

    def update_globals_csv(self) -> None:
        moved = {r.name for r in self.rows if r.csv is not None and not r.interior}
        with GLOBALS.open(newline="") as fh:
            rows = list(csv.reader(fh))
        header = rows[0]
        at = header.index("defined")
        for r in rows[1:]:
            if r[header.index("name")] in moved:
                r[at] = "SimGlobals"
        buf = io.StringIO()
        csv.writer(buf, lineterminator="\n").writerows(rows)
        self.write(GLOBALS, buf.getvalue())

    def strip_vector_class(self) -> None:
        self.write(VECTOR_HEADER, VECTOR_CLASS)
        path = ROOT / "src" / "map" / "meteors.cpp"
        text = path.read_text()
        new = VECTOR_CLASS_IN_SOURCE.sub("", text, count=1)
        if new != text:
            self.write(path, new)

    def strip_member_declarations(self) -> None:
        path = ROOT / "src" / "units" / "movement_class.h"
        text = path.read_text()
        new = re.sub(r"\n\n    static MovementClassTable g_movementClasses;", "", text)
        new = new.replace("\nextern Quirks g_quirks;\n", "")
        self.write(path, new)
        path = ROOT / "src" / "quirks" / "quirks.h"
        text = path.read_text()
        self.write(path, text.replace("extern Quirks g_quirks;\n\n", "").replace("extern Quirks g_quirks;\n", ""))

    def source_files(self) -> list[str]:
        out = []
        for base in ("src", "include"):
            for path in sorted((ROOT / base).rglob("*")):
                rel = path.relative_to(ROOT).as_posix()
                if path.suffix in SOURCE_SUFFIXES and rel not in SKIP_FILES:
                    out.append(rel)
        return out

    def run(self) -> None:
        self.write(SIM_HEADER, header_text(self.rows))
        self.write(SIM_CSV, csv_text(self.rows))
        self.strip_vector_class()
        self.strip_member_declarations()
        self.strip_link()
        self.update_globals_csv()
        for rel in self.source_files():
            text = (ROOT / rel).read_text()
            if self.word.search("\n".join(strip_code(l) for l in text.split("\n"))) or \
                    "MovementClassTable::g_movementClasses" in text:
                self.rewrite_source(rel)


def layout_report(rows: list[Row]) -> list[str]:
    """Every layout_refs row that names a moved symbol, and whether its neighbour holds."""
    by_name = {r.name: r for r in rows if r.address is not None}
    by_name.update({r.name: r for r in rows if r.name not in by_name})
    lines = []
    with LAYOUT_REFS.open(newline="") as fh:
        for ref in csv.DictReader(fh):
            names = [ref["symbol"], ref["array"], ref["neighbour"].split("+")[0]]
            if not any(n in by_name for n in names if n):
                continue
            verdict = ref["verdict"]
            note = "ok"
            nb = ref["neighbour"].split("+")[0]
            sym = by_name.get(ref["symbol"])
            if ref["kind"] == "past-end" and nb and sym is not None and verdict != "none":
                target = by_name.get(nb)
                if target is None:
                    note = "neighbour stays in the image: the end pointer is only compared"
                else:
                    a = by_name.get(ref["array"], sym)
                    end = (a.address or 0) + (a.size or 0)
                    note = "ok: neighbour follows in the struct" if target.address == end else "CHECK"
            lines.append(f"{ref['function']} {ref['kind']} {ref['symbol']}{int(ref['addend']):+d} "
                         f"array={ref['array']} neighbour={ref['neighbour']} verdict={verdict}: {note}")
    return lines


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--check", action="store_true", help="report what would change and exit 1 if anything would")
    ap.add_argument("--layout", action="store_true", help="print the layout_refs rows that name a moved symbol")
    args = ap.parse_args()
    rows = load_rows()
    if args.layout:
        print("\n".join(layout_report(rows)))
        return
    tool = Tool(rows, args.check)
    tool.run()
    print(f"simglobals: {len(rows)} rows, {len(field_rows(rows))} fields, "
          f"{len(tool.changed)} files {'would change' if args.check else 'changed'}, {len(tool.views)} views",
          file=sys.stderr)
    for rel, name in tool.views:
        print(f"view {rel} {name}", file=sys.stderr)
    if args.check and tool.changed:
        sys.exit(1)


if __name__ == "__main__":
    main()

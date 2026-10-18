"""List the places where game code depends on the layout of 3.1c's data.

    uv run tools/layoutrefs.py --objects @build/modern/objects.rsp --out data/layout_refs.csv
    uv run tools/layoutrefs.py --objects @build/modern/objects.rsp --check
    uv run tools/test_layoutrefs.py          # tests, on hand-written references

The decompiled source sometimes writes "the end of an array" as the address of
whatever global followed it in 3.1c's .data (BuildAllPassMaps ends its loop over
the movement classes at g_serialPortNumber). The modern build keeps the original
order, so these work; the shared state block (#7) will not. This reads the
objects of the modern build (the `game` library and data.cpp's), looks at every
relocation in a code section that names a global, and flags two kinds:

  past-end  the addend of the reference (implicit, in the four bytes at the
            relocation) is negative or at least the size of the global in
            data/globals.csv: the code indexes past the global into the one
            around original address + addend. `neighbour` is that place as
            `name+0xoff`.
  end-of    a global B whose address is used (not loaded or stored through)
            at addend 0, in a function that also references a global A that
            ends where B starts, or at most GAP bytes before it, with no
            global between that A could be the end of. B is a candidate "end
            of A"; `array` is A.

A use of B is an address when the instruction takes its address as an
immediate (`mov r, imm32`, `cmp r, imm32`, `push imm32`, a store of it) or with
`lea`, as capstone disassembles the function; any other instruction loads or
stores through B or calls it, and B is then not an end. The count is about 170
without a tighter rule, so no tighter rule is used. Both kinds over-report (a
function may use two adjacent globals for their own sake); the `verdict` column
is left empty for a later pass: `end`, `quirk` (a real read into the neighbour
that 3.1c relies on) or `none`.

Columns: function, function_address (the `// FUNCTION:` marker), file, kind,
symbol, addend, array, neighbour, verdict. Sorted by function_address, deduplicated.
"""

import argparse
import bisect
import csv
import io
import struct
import sys
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

from capstone import CS_ARCH_X86, CS_MODE_32, Cs

from objread import address_of, load_known, parse
from sources import ROOT, annotations, base_name

GLOBALS = ROOT / "data/globals.csv"
OUT = ROOT / "data/layout_refs.csv"
COLUMNS = ["function", "function_address", "file", "kind", "symbol", "addend", "array", "neighbour", "verdict"]
REL_DIR32, REL_REL32 = 6, 20
GLOBAL_LO, GLOBAL_HI = 0x4FC000, 0x52D000
GAP = 0x20


@dataclass(frozen=True)
class Row:
    address: int
    name: str
    size: int

    @property
    def end(self) -> int:
        return self.address + self.size


@dataclass(frozen=True)
class Ref:
    row: Row
    addend: int
    use: str


class Globals:
    """data/globals.csv by original address."""

    def __init__(self, rows: list[Row]):
        self.rows = sorted(rows, key=lambda r: (r.address, -r.size))
        self.starts = [r.address for r in self.rows]

    def before(self, address: int) -> Row | None:
        """The innermost row that starts at or before address and holds it, else the last row before it."""
        i = bisect.bisect_right(self.starts, address)
        for row in reversed(self.rows[:i]):
            if row.address <= address < row.end:
                return row
        return self.rows[i - 1] if i else None

    def exact(self, address: int) -> Row | None:
        """The row that starts at address (the first, when interior rows share it)."""
        i = bisect.bisect_left(self.starts, address)
        return self.rows[i] if i < len(self.rows) and self.starts[i] == address else None

    def after(self, row: Row) -> Row | None:
        """The first row that starts at or after the end of row."""
        i = bisect.bisect_left(self.starts, row.end)
        return self.rows[i] if i < len(self.rows) else None

    def neighbour(self, address: int) -> str:
        """`name+0xoff` of the global around an address."""
        row = self.before(address)
        return "%s+0x%x" % (row.name, address - row.address) if row else ""


def load_globals() -> Globals:
    """The globals of data/globals.csv that have a size."""
    with GLOBALS.open() as fh:
        rows = [Row(int(r["address"], 16), r["name"], int(r["size"])) for r in csv.DictReader(fh) if r["size"]]
    return Globals([r for r in rows if r.size > 0])


@lru_cache(maxsize=64)
def instructions(code: bytes) -> tuple[tuple[int, int, str, int, int], ...]:
    """(offset, size, mnemonic, immediate offset, displacement offset) of each instruction of a function, in order."""
    out = []
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    for insn in md.disasm(code, 0):
        enc = insn.encoding
        out.append((insn.address, insn.size, insn.mnemonic, enc.imm_offset, enc.disp_offset))
    return tuple(out)


def use_of(code: bytes, offset: int) -> str:
    """How the instruction holding the 4 bytes at offset uses what they name: `address` (mov r, imm32),
    `compare` (cmp with imm32), `argument` (any other immediate, and lea) or `memory` (a load, a store, a call)."""
    insns = instructions(code)
    i = bisect.bisect_right([n[0] for n in insns], offset) - 1
    if i < 0:
        return "memory"
    start, size, mnemonic, imm, disp = insns[i]
    if offset >= start + size:
        return "memory"
    if imm and start + imm == offset:
        if mnemonic == "mov" and 0xB8 <= code[start] <= 0xBF:
            return "address"
        return "compare" if mnemonic == "cmp" else "argument"
    return "argument" if mnemonic == "lea" else "memory"


def past_end(addend: int, size: int) -> bool:
    """Whether an access at addend lies outside a global of that size."""
    return addend < 0 or addend >= size


def end_candidates(refs: list[Ref], globals_: Globals) -> set[tuple[str, str]]:
    """(B, A) for every global B whose address one function uses at addend 0 and
    that is the end of another global A the function references."""
    found = set()
    for b in refs:
        if b.addend != 0 or b.use == "memory":
            continue
        for a in refs:
            if a.row == b.row or not a.row.end <= b.row.address <= a.row.end + GAP:
                continue
            between = globals_.after(a.row)
            if between is not None and between.address < b.row.address and between.size > 0 \
                    and any(r.row == between for r in refs):
                continue
            found.add((b.row.name, a.row.name))
    return found


def rows_of(function: str, address: str, file: str, refs: list[Ref], globals_: Globals) -> list[list]:
    """The CSV rows of one function's references."""
    out = []
    for ref in refs:
        if past_end(ref.addend, ref.row.size):
            out.append([function, address, file, "past-end", ref.row.name, ref.addend, ref.row.name,
                        globals_.neighbour(ref.row.address + ref.addend), ""])
    for b, a in end_candidates(refs, globals_):
        out.append([function, address, file, "end-of", b, 0, a, "", ""])
    return out


def source_of(obj: Path) -> str:
    """The source file an object of the modern build was compiled from."""
    parts = obj.parts
    for i in range(len(parts) - 1, -1, -1):
        if parts[i] == "src":
            return "/".join(parts[i:]).removesuffix(".obj")
    return obj.name


def function_address(sym: str, file: str, marked: dict[str, set[int]], symbols: dict[str, int]) -> str:
    """The address of the function's `// FUNCTION:` marker in its file, else its data/symbols.csv address."""
    name = base_name(sym)
    found = marked.get(name) or marked.get(name.split("::")[-1])
    if found and len(found) == 1:
        return "0x%x" % next(iter(found))
    address = address_of(sym, symbols)
    return "0x%x" % address if address is not None else ""


def refs_of(obj, globals_: Globals, symbols: dict[str, int]) -> dict[tuple[int, str], list[Ref]]:
    """Each function of an object (section, name) -> the references it makes to globals."""
    out: dict[tuple[int, str], list[Ref]] = {}
    for sec in obj.secs:
        if not sec.is_code or not sec.relocs:
            continue
        names = sorted((s.value, s.name) for s in obj.syms.values()
                       if s.section == sec.index and s.sclass in (2, 3) and not s.name.startswith("."))
        for offset, index, kind in sec.relocs:
            if kind not in (REL_DIR32, REL_REL32) or index not in obj.syms or not names:
                continue
            address = address_of(obj.syms[index].name, symbols)
            if address is None or not GLOBAL_LO <= address < GLOBAL_HI:
                continue
            (addend,) = struct.unpack_from("<i", sec.data, offset)
            row = globals_.exact(address) or globals_.before(address)
            if row is None or not row.address <= address < row.end:
                continue
            at = bisect.bisect_right([v for v, _ in names], offset) - 1
            function = names[max(at, 0)][1]
            out.setdefault((sec.index, function), []).append(
                Ref(row, addend + address - row.address, use_of(sec.data, offset)))
    return out


def generate(objects: list[Path]) -> list[list]:
    """The rows of the objects, sorted and without duplicates."""
    globals_ = load_globals()
    symbols = load_known()[0]
    rows = set()
    for path in objects:
        file = source_of(path)
        src = ROOT / file
        marked: dict[str, set[int]] = {}
        if src.exists():
            for address, name in annotations(src):
                marked.setdefault(name, set()).add(address)
                marked.setdefault(name.split("::")[-1], set()).add(address)
        obj = parse(path)
        for (_, sym), refs in refs_of(obj, globals_, symbols).items():
            address = function_address(sym, file, marked, symbols)
            for row in rows_of(base_name(sym), address, file, refs, globals_):
                rows.add(tuple(row))
    return sorted((list(r) for r in rows), key=lambda r: (r[1] == "", int(r[1], 16) if r[1] else 0, r[0], r[3],
                                                          r[4], r[5], r[6]))


def render(rows: list[list]) -> str:
    """The CSV text."""
    buf = io.StringIO()
    writer = csv.writer(buf, lineterminator="\n")
    writer.writerow(COLUMNS)
    writer.writerows(rows)
    return buf.getvalue()


def response(arg: str) -> list[Path]:
    """The object files an argument names: a path, or @file listing them."""
    if arg.startswith("@"):
        return [Path(line.strip()) for line in Path(arg[1:]).read_text().splitlines() if line.strip()]
    return [Path(arg)]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--objects", nargs="+", required=True, help="object files, or @file listing them")
    parser.add_argument("--out", type=Path, default=OUT)
    parser.add_argument("--check", action="store_true", help="fail if the file differs from what the tool generates")
    args = parser.parse_args()
    text = render(generate([p for a in args.objects for p in response(a)]))
    if args.check:
        if not args.out.exists() or args.out.read_text() != text:
            print("layoutrefs: %s is out of date; run uv run tools/layoutrefs.py --objects @build/modern/objects.rsp"
                  % args.out, file=sys.stderr)
            sys.exit(1)
        print("layoutrefs: %s is up to date" % args.out)
        return
    args.out.write_text(text)
    kinds = {}
    for line in text.splitlines()[1:]:
        kinds[line.split(",")[3]] = kinds.get(line.split(",")[3], 0) + 1
    print("layoutrefs: %d rows written to %s %s" % (len(text.splitlines()) - 1, args.out, kinds))


if __name__ == "__main__":
    main()

"""List every pointer from the shared state block to memory outside it, found at run time, and class each one.

    uv run tools/blockpointers.py                    # play the suite with -blockscan, write data/block_pointers.csv
    uv run tools/blockpointers.py --from DIR...      # build from blockptrs files already collected (no Wine)
    uv run tools/blockpointers.py --out F            # write F instead (a dry run; class and note kept from the data file)
    uv run tools/blockpointers.py --check            # CI: the file itself is valid (classes, notes, order)
    uv run tools/blockpointers.py --unclassified     # list the rows with no class
    uv run tools/test_blockpointers.py               # tests, on hand-written scans, maps and rows

This is the list for #285. The decompilation's types do not mark every pointer, so the game finds
them: run with `-blockscan` (tools/playtest.py --blockscan), a test-mode run walks every live
allocation of the block and the `SimGlobals` struct at the end of each tick and writes blockptrs.txt
(reference/README.md, "Block pointers") with each aligned word that points into a live heap
allocation outside the block. This tool plays the whole scenario suite (the scenarios of
reference/scenarios/, as tools/reference.py does, quirks off), or takes the files of an earlier
run (`--from DIR...`, files named `<scenario>.txt`, which `reference.py --check --save-blockptrs DIR`
writes), resolves the allocation sites with the linker map and merges the results.

Names. A site is a return address in the exe. The map (build/modern/TotalA.map) gives the function
that contains it, and the tag the caller passed adds `: <tag>`. A function with several sites of one
tag has them numbered `#2`, `#3` in address order. Address 0 is `unknown`. The holder `SimGlobals`
is named `SimGlobals.<field>`, the field of src/state/sim_globals.h that holds the word (from the
record layout clang prints for a probe that includes the header), and the offset is then inside that
field; with no layout the holder is `SimGlobals` and the offset is the struct's.

Rows, one per (holder, target), sorted by holder, then target:

  holder, target  the sites named above
  offsets         where in the holder the pointers sit. The offsets of one pair that form an
                  arithmetic progression of 3 or more (a field in each element of an array) are
                  `start+stride*k` (k from 0, unbounded: more elements in another run still match);
                  the others are listed (`0x10 0x24`)
  scenarios       in how many scenarios the pair was seen
  ticks           the ticks it was seen in, summed over the scenarios
  target_offsets  `0x0` when the word always pointed at the start of the target, else `0xa-0x40`
  class           outside (load-time data that stays outside the block), moves-in (sim state that
                  should be allocated in the block) or not-a-pointer (an integer that looks like an
                  address; it shows in few runs or at varying target offsets, and needs a note)
  note            free text

`class` and `note` are kept by regeneration, by (holder, target). `reference.py --check` plays the
scan on and fails on a pair or an offset this file does not cover.
"""

import argparse
import bisect
import csv
import difflib
import io
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "data/block_pointers.csv"
DEFAULT_EXE = ROOT / "build/modern/TotalA.exe"
COLUMNS = ["holder", "offsets", "target", "scenarios", "ticks", "target_offsets", "class", "note"]
KEPT = ("class", "note")
CLASSES = ("outside", "moves-in", "not-a-pointer")
SCAN_MAGIC = "bt-blockptrs 1"
SCAN_COLUMNS = ["holder", "holder_tag", "offset", "target", "target_tag", "ticks", "target_offset_min",
                "target_offset_max"]
MIN_RUN = 3
SIM_GLOBALS = "SimGlobals"
SIM_GLOBALS_HEADER = "src/state/sim_globals.h"
HEX = r"0x[0-9a-f]+"
OFFSET_TOKEN = re.compile(r"^(%s)(?:\+(%s)\*k)?$" % (HEX, HEX))
TARGET_OFFSETS = re.compile(r"^%s(?:-%s)?$" % (HEX, HEX))
LAYOUT_FLAG = ["-Xclang", "-fdump-record-layouts-complete"]


@dataclass
class ScanRow:
    """One line of a blockptrs.txt: a word of a holder allocation that points into a target allocation."""
    scenario: str
    holder: str
    holder_tag: str
    offset: int
    target: str
    target_tag: str
    ticks: int
    target_min: int
    target_max: int


@dataclass
class Scan:
    """A parsed blockptrs.txt: the commit and tick count of the run and its rows."""
    scenario: str
    commit: str
    ticks: int
    rows: list[ScanRow] = field(default_factory=list)


def parse_scan(text: str, scenario: str) -> Scan:
    """The rows of a blockptrs.txt; raises ValueError on a file that does not follow the format."""
    lines = text.splitlines()
    if not lines or lines[0].strip() != SCAN_MAGIC:
        raise ValueError("%s: not a %r file" % (scenario, SCAN_MAGIC))
    header: dict[str, str] = {}
    rows: list[ScanRow] = []
    for number, line in enumerate(lines[1:], 2):
        if not line.strip():
            continue
        cells = line.split("\t")
        if cells[0] in ("commit", "ticks") and len(cells) == 2:
            header[cells[0]] = cells[1]
            continue
        if cells == SCAN_COLUMNS:
            continue
        if len(cells) != len(SCAN_COLUMNS):
            raise ValueError("%s:%d: %d columns, expected %d" % (scenario, number, len(cells), len(SCAN_COLUMNS)))
        try:
            rows.append(ScanRow(scenario, cells[0], cells[1], int(cells[2], 16), cells[3], cells[4],
                                int(cells[5]), int(cells[6], 16), int(cells[7], 16)))
        except ValueError:
            raise ValueError("%s:%d: a number does not parse: %r" % (scenario, number, line)) from None
    if "ticks" not in header:
        raise ValueError("%s: no ticks line" % scenario)
    return Scan(scenario, header.get("commit", "-"), int(header["ticks"]), rows)


def read_scans(paths: list[Path]) -> list[Scan]:
    """The scans of the files, and of the *.txt files of the directories, each named by its file's stem."""
    files: list[Path] = []
    for path in paths:
        files += sorted(path.glob("*.txt")) if path.is_dir() else [path]
    if not files:
        raise ValueError("no blockptrs files in %s" % ", ".join(str(p) for p in paths))
    return [parse_scan(p.read_text(), p.stem) for p in files]


@dataclass
class CodeMap:
    """The functions of a linker map by address, for naming the function that holds a return address."""
    addresses: list[int]
    names: list[str]

    def function(self, address: int) -> str:
        """The function that contains the instruction before the return address, or its hex address."""
        i = bisect.bisect_right(self.addresses, address - 1) - 1
        return self.names[i] if i >= 0 else "0x%08x" % address


MAP_LINE = re.compile(r"^\s*0001:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8,16})\s")
SKIPPED = ("$", "?catch$", "___ehhandler$")


def demangle(name: str) -> str:
    """The plain name of a decorated one: `Class::method`, `function`; other names are left alone."""
    if name.startswith("?"):
        m = re.match(r"\?([^@?]+)@(?:\?\$([^@]+)@|([^@?]+)@)?", name)
        if m:
            owner = m.group(2) or m.group(3)
            return "%s::%s" % (owner, m.group(1)) if owner else m.group(1)
        return name
    return re.sub(r"@\d+$", "", name[1:] if name.startswith("_") else name)


def parse_map(text: str) -> CodeMap:
    """The code symbols of a linker map. Of several names at one address the first by (FUN_ last, name) wins."""
    by_address: dict[int, list[str]] = {}
    for line in text.splitlines():
        m = MAP_LINE.match(line)
        if not m or m.group(1).startswith(SKIPPED):
            continue
        by_address.setdefault(int(m.group(2), 16), []).append(demangle(m.group(1)))
    addresses = sorted(by_address)
    names = [min(by_address[a], key=lambda n: (n.startswith("FUN_"), n)) for a in addresses]
    return CodeMap(addresses, names)


class Namer:
    """Names allocation sites: function (and tag), with repeats of one name numbered in address order."""

    def __init__(self, code: CodeMap, sites):
        """`sites` is every (address, tag) the names will be asked for."""
        self.code = code
        self.numbers: dict[tuple[int, str], int] = {}
        groups: dict[tuple[str, str], list[int]] = {}
        for address, tag in set(sites):
            if address:
                groups.setdefault((code.function(address), tag), []).append(address)
        for (_, tag), members in groups.items():
            for n, address in enumerate(sorted(members), 1):
                self.numbers[(address, tag)] = n

    def name(self, site: str, tag: str) -> str:
        """The name of a site given as `0x%08x` (or SimGlobals, which has no tag)."""
        if site == SIM_GLOBALS:
            return SIM_GLOBALS
        address = int(site, 16)
        if address == 0:
            return "unknown"
        text = self.code.function(address)
        number = self.numbers.get((address, "" if tag == "-" else tag), 1)
        if tag != "-":
            text += ": " + tag
        return text + (" #%d" % number if number > 1 else "")


def site_pairs(scans: list[Scan]):
    """Every (address, tag) of a holder or target in the scans (the holder SimGlobals excluded)."""
    for scan in scans:
        for r in scan.rows:
            for site, tag in ((r.holder, r.holder_tag), (r.target, r.target_tag)):
                if site != SIM_GLOBALS:
                    yield int(site, 16), "" if tag == "-" else tag


@dataclass
class Member:
    """A top-level member of SimGlobals and its byte range."""
    name: str
    start: int
    end: int


def simglobals_members(layout_text: str) -> list[Member] | None:
    """The members of SimGlobals in a clang record-layout dump, or None when it has none."""
    import stateclasses
    found = stateclasses.layout_fields(layout_text, SIM_GLOBALS)
    if not found:
        return None
    return [Member(f.name, f.start, f.end) for f in found[1] if not f.bits]


def load_members(build_dir: Path) -> list[Member] | None:
    """The members of SimGlobals from clang-cl on a probe that includes its header, or None when that fails."""
    import rwsets
    try:
        _, argv, directory = rwsets.compile_commands(build_dir, [])[0]
        i = argv.index("--")
        dump = argv.index("-ast-dump=json")
        flags = argv[:dump - 1] + argv[dump + 1:i]
        with tempfile.TemporaryDirectory() as tmp:
            probe = Path(tmp) / "probe.cpp"
            probe.write_text('#include "%s"\nstatic_assert(sizeof(SimGlobals) > 0, "");\n'
                             % (ROOT / SIM_GLOBALS_HEADER))
            run = subprocess.run(flags + LAYOUT_FLAG + ["--", str(probe)], cwd=directory, capture_output=True,
                                 text=True, errors="replace")
        return simglobals_members(run.stdout)
    except (OSError, IndexError, subprocess.SubprocessError, ValueError) as e:
        sys.stderr.write("blockpointers: no SimGlobals layout (%s); its holders are named SimGlobals+0x<off>\n" % e)
        return None


def holder_name(row: ScanRow, namer: Namer, members: list[Member] | None) -> tuple[str, int]:
    """The holder's name and the offset inside it; SimGlobals words are named by field with a field-relative offset."""
    if row.holder != SIM_GLOBALS:
        return namer.name(row.holder, row.holder_tag), row.offset
    for m in members or []:
        if m.start <= row.offset < max(m.end, m.start + 1):
            return "%s.%s" % (SIM_GLOBALS, m.name), row.offset - m.start
    return SIM_GLOBALS + ("+0x%x" % row.offset if members is None else ""), (0 if members is None else row.offset)


@dataclass
class Resolved:
    """A scanned word with its holder and target named."""
    scenario: str
    holder: str
    offset: int
    target: str
    ticks: int
    target_min: int
    target_max: int


def resolve(scans: list[Scan], code: CodeMap, members: list[Member] | None) -> list[Resolved]:
    """Every scanned row with names."""
    namer = Namer(code, site_pairs(scans))
    out = []
    for scan in scans:
        for r in scan.rows:
            holder, offset = holder_name(r, namer, members)
            if holder == SIM_GLOBALS + "+0x%x" % r.offset:
                offset = 0
            out.append(Resolved(scan.scenario, holder, offset, namer.name(r.target, r.target_tag), r.ticks,
                                r.target_min, r.target_max))
    return out


def fold(offsets: set[int]) -> list[tuple[int, int | None]]:
    """The offsets as (start, stride) runs: progressions of MIN_RUN or more, else single (start, None) entries.

    Greedy from the smallest remaining offset: the stride (smallest wins a tie) that reaches the most
    consecutive members of the set, taken as a run if it reaches MIN_RUN.
    """
    left = sorted(offsets)
    out: list[tuple[int, int | None]] = []
    while left:
        start = left[0]
        members = set(left)
        best_stride, best_length = 0, 1
        for other in left[1:]:
            stride = other - start
            length = 1
            while start + stride * length in members:
                length += 1
            if length > best_length:
                best_stride, best_length = stride, length
        if best_length >= MIN_RUN:
            out.append((start, best_stride))
            taken = {start + best_stride * i for i in range(best_length)}
        else:
            out.append((start, None))
            taken = {start}
        left = [o for o in left if o not in taken]
    return out


def offsets_text(offsets: set[int]) -> str:
    """The offsets column: runs as `start+stride*k`, the rest as plain offsets."""
    return " ".join("0x%x" % s if d is None else "0x%x+0x%x*k" % (s, d) for s, d in fold(offsets))


def target_offsets_text(low: int, high: int) -> str:
    return "0x%x" % low if low == high else "0x%x-0x%x" % (low, high)


def merge(rows: list[Resolved]) -> list[dict]:
    """One row per (holder, target) with no class or note, sorted by holder then target."""
    groups: dict[tuple[str, str], list[Resolved]] = {}
    for r in rows:
        groups.setdefault((r.holder, r.target), []).append(r)
    out = []
    for (holder, target), members in groups.items():
        ticks = 0
        per_scenario: dict[str, int] = {}
        for r in members:
            per_scenario[r.scenario] = max(per_scenario.get(r.scenario, 0), r.ticks)
        ticks = sum(per_scenario.values())
        out.append({"holder": holder, "offsets": offsets_text({r.offset for r in members}), "target": target,
                    "scenarios": str(len(per_scenario)), "ticks": str(ticks),
                    "target_offsets": target_offsets_text(min(r.target_min for r in members),
                                                          max(r.target_max for r in members)),
                    "class": "", "note": ""})
    return sorted(out, key=row_order)


def row_order(row: dict) -> tuple:
    """The sort key of a row: holder, then target."""
    return row["holder"], row["target"]


def render(rows: list[dict]) -> str:
    out = io.StringIO()
    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(COLUMNS)
    writer.writerows([[row[c] for c in COLUMNS] for row in rows])
    return out.getvalue()


def read_rows(path: Path = OUT) -> list[dict]:
    """The rows of data/block_pointers.csv, or none when it does not exist."""
    if not path.exists():
        return []
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def merge_kept(rows: list[dict], old: list[dict]) -> list[dict]:
    """The rows with the class and note the old file had for the same (holder, target)."""
    kept = {(r["holder"], r["target"]): r for r in old}
    for row in rows:
        before = kept.get((row["holder"], row["target"]))
        if before:
            for column in KEPT:
                row[column] = before[column]
    return rows


def differences(have: list[dict], want: list[dict]) -> list[str]:
    """Unified diff lines of the rows with class and note left out."""
    def lines(rows):
        return [",".join(r[c] for c in COLUMNS if c not in KEPT) for r in rows]
    return list(difflib.unified_diff(lines(have), lines(want), "data/block_pointers.csv", "regenerated",
                                     lineterm="", n=0))


def check_rows(text: str) -> list[str]:
    """What is wrong with the text of data/block_pointers.csv: columns, values, classes, notes, order."""
    reader = csv.reader(io.StringIO(text))
    table = list(reader)
    if not table or table[0] != COLUMNS:
        return ["the header is not %s" % ",".join(COLUMNS)]
    problems = []
    previous = None
    for number, cells in enumerate(table[1:], 2):
        if len(cells) != len(COLUMNS):
            problems.append("line %d: %d columns, expected %d" % (number, len(cells), len(COLUMNS)))
            continue
        row = dict(zip(COLUMNS, cells))
        where = "line %d (%s -> %s)" % (number, row["holder"], row["target"])
        if not row["holder"] or not row["target"]:
            problems.append("line %d: holder and target must be set" % number)
        offsets = row["offsets"].split()
        if not offsets or not all(OFFSET_TOKEN.match(o) for o in offsets):
            problems.append("%s: offsets %r are not `0x10`, `0x10+0x24*k` entries" % (where, row["offsets"]))
        if not row["scenarios"].isdigit() or not row["ticks"].isdigit():
            problems.append("%s: scenarios and ticks must be numbers" % where)
        if not TARGET_OFFSETS.match(row["target_offsets"]):
            problems.append("%s: target_offsets %r is not `0x0` or `0xa-0x40`" % (where, row["target_offsets"]))
        if row["class"] not in CLASSES:
            problems.append("%s: class %r is not one of %s" % (where, row["class"], ", ".join(CLASSES)))
        if row["class"] == "not-a-pointer" and not row["note"].strip():
            problems.append("%s: a not-a-pointer row needs a note" % where)
        key = row_order(row)
        if previous is not None and key <= previous:
            problems.append("%s: rows must be sorted by holder then target, without repeats" % where)
        previous = key
    return problems


def offset_covered(offset: int, tokens: list[str]) -> bool:
    """Whether the offset is one of the entries of an offsets column."""
    for token in tokens:
        m = OFFSET_TOKEN.match(token)
        if not m:
            continue
        start = int(m.group(1), 16)
        if m.group(2) is None:
            if offset == start:
                return True
        else:
            stride = int(m.group(2), 16)
            if stride and offset >= start and (offset - start) % stride == 0:
                return True
    return False


def uncovered(scanned: list[Resolved], rows: list[dict]) -> list[Resolved]:
    """The scanned words whose (holder, target) pair or offset the rows do not list, one per distinct word."""
    listed: dict[tuple[str, str], list[str]] = {}
    for r in rows:
        listed.setdefault((r["holder"], r["target"]), []).extend(r["offsets"].split())
    out = []
    seen = set()
    for s in scanned:
        tokens = listed.get((s.holder, s.target))
        if tokens is not None and offset_covered(s.offset, tokens):
            continue
        key = (s.scenario, s.holder, s.offset, s.target)
        if key not in seen:
            seen.add(key)
            out.append(s)
    return out


def describe_uncovered(missing: list[Resolved], rows: list[dict]) -> list[str]:
    """One line per word the file does not cover, with the reason."""
    pairs = {(r["holder"], r["target"]) for r in rows}
    lines = []
    for s in missing:
        why = "offset not listed" if (s.holder, s.target) in pairs else "pair not listed"
        lines.append("%s: %s offset 0x%x -> %s (%s)" % (s.scenario, s.holder, s.offset, s.target, why))
    return lines


def check_scans(texts: dict[str, str | None], map_path: Path, build_dir: Path,
                rows: list[dict] | None = None) -> list[str]:
    """The problems of a gate run's blockptrs.txt files against data/block_pointers.csv, as lines."""
    problems = ["%s: no blockptrs.txt (did the exe run with -blockscan?)" % n for n, t in texts.items() if t is None]
    scans = []
    for name, text in texts.items():
        if text is None:
            continue
        try:
            scans.append(parse_scan(text, name))
        except ValueError as e:
            problems.append(str(e))
    if not scans:
        return problems
    if not map_path.is_file():
        return problems + ["%s: no linker map to name the sites with" % map_path]
    code = parse_map(map_path.read_text(errors="replace"))
    needs_layout = any(r.holder == SIM_GLOBALS for s in scans for r in s.rows)
    members = load_members(build_dir) if needs_layout else []
    listed = read_rows() if rows is None else rows
    missing = uncovered(resolve(scans, code, members), listed)
    return problems + describe_uncovered(missing, listed)


def default_build_dir() -> Path:
    """The compile database directory for the SimGlobals layout (build/rwsets)."""
    import rwsets
    return rwsets.DEFAULT_BUILD_DIR


def play_suite(exe: Path, game_dir: str | None, parallel: int) -> dict[str, str | None]:
    """Play every scenario with the scan on; each scenario's blockptrs.txt text (None when it wrote none)."""
    import reference
    scans: dict[str, str | None] = {}
    reference.play_all(exe, reference.scenario_files(), game_dir, parallel, scans=scans)
    return scans


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if the data file is not valid (CI)")
    parser.add_argument("--unclassified", action="store_true", help="list the rows with no class")
    parser.add_argument("--from", dest="sources", nargs="+", type=Path, metavar="DIR",
                        help="build from these blockptrs files or directories of <scenario>.txt, playing nothing")
    parser.add_argument("--exe", type=Path, default=DEFAULT_EXE, help="the exe to play (default build/modern/TotalA.exe)")
    parser.add_argument("--map", type=Path, help="the linker map (default: the exe's)")
    parser.add_argument("--game-dir", help="the game's install (default: Steam's)")
    parser.add_argument("--parallel", type=int, metavar="N", help="scenarios played at once")
    parser.add_argument("--build-dir", type=Path, help="compile database for the SimGlobals layout (default build/rwsets)")
    parser.add_argument("--out", type=Path, default=OUT, help="write the rows here instead of the data file")
    args = parser.parse_args()
    if args.check:
        problems = check_rows(OUT.read_text()) if OUT.exists() else ["%s does not exist" % OUT]
        for p in problems[:40]:
            print(p)
        if problems:
            print("blockpointers: %d problems in data/block_pointers.csv" % len(problems))
            sys.exit(1)
        print("blockpointers: ok (%d rows)" % len(read_rows()))
        return
    old = read_rows()
    if args.unclassified:
        empty = [r for r in old if not r["class"]]
        bad = [r for r in old if r["class"] and r["class"] not in CLASSES]
        for r in bad:
            print("bad class %r: %s -> %s" % (r["class"], r["holder"], r["target"]))
        for r in empty[:20]:
            print("no class: %s -> %s (%s scenarios, %s ticks)" % (r["holder"], r["target"], r["scenarios"], r["ticks"]))
        print("blockpointers: %d of %d rows have no class" % (len(empty), len(old)))
        sys.exit(1 if empty or bad else 0)
    map_path = args.map or args.exe.with_suffix(".map")
    if args.sources:
        scans = read_scans(args.sources)
    else:
        import reference
        texts = play_suite(args.exe.resolve(), args.game_dir, args.parallel or reference.default_parallel())
        absent = [n for n, t in texts.items() if t is None]
        if absent:
            sys.exit("blockpointers: no blockptrs.txt from: " + ", ".join(absent))
        scans = [parse_scan(t, n) for n, t in sorted(texts.items())]
    if not map_path.is_file():
        sys.exit("blockpointers: no linker map at %s (--map)" % map_path)
    commits = {s.commit for s in scans}
    if len(commits) > 1:
        print("warning: the scans come from different commits: " + ", ".join(sorted(commits)))
    members = load_members(args.build_dir or default_build_dir()) \
        if any(r.holder == SIM_GLOBALS for s in scans for r in s.rows) else []
    code = parse_map(map_path.read_text(errors="replace"))
    merged = merge_kept(merge(resolve(scans, code, members)), old)
    now = {(r["holder"], r["target"]) for r in merged}
    lost = [r for r in old if (r["class"] or r["note"]) and (r["holder"], r["target"]) not in now]
    args.out.write_text(render(merged))
    print("%d rows from %d scans" % (len(merged), len(scans)))
    if lost:
        print("warning: %d rows with a class or note no longer exist and were dropped" % len(lost))
        for r in lost[:20]:
            print("  %s -> %s: %s %s" % (r["holder"], r["target"], r["class"], r["note"]))


if __name__ == "__main__":
    main()

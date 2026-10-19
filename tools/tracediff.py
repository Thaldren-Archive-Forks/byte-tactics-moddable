"""Find where two builds' game logic first differs, on one reference scenario.

    uv run tools/tracediff.py A/TotalA.exe B/TotalA.exe --scenario reference/scenarios/skirmish2.txt
    uv run tools/tracediff.py --traces a/trace.txt b/trace.txt
    uv run tools/tracediff.py --dumps a/dump.txt b/dump.txt --at 900 units

Runs both exes on the scenario under Wine (tools/playtest.py --testmode), compares
the trace.txt each writes and reports the first tick and phase whose hashes differ.
It then runs both again with `--testdump <tick>` and prints every field of that
phase that differs, with its value in A and in B. Set WINEARCH=win64 for Wine.

    first divergence: tick 900, phase units
      phase function: UpdateAllUnits (FUNCTION: 0x48ad30)
      first differing field: units[12].health: a=0x0064 b=0x0063
      all 1 differing fields:
        units[12].health: a=0x0064 b=0x0063

The row `world` is the tables hashed once per tick (game scalars, map, line of sight, pathfinder),
after the last phase's row. A dump holds them after every phase, so the fields of a `world` divergence
are read from the last phase's lines.

`--traces` compares existing trace files without running, and reports the first
divergence only. `--dumps` diffs the phase of two existing dump files.

`--reference a|b` says which side's dump came from the reference build, whose leaf names predate the
renames on moddable. Its names are translated to the modern ones before diffing, using the tables of
src/harness/trace_schema.h in this tree and on `--reference-ref` (default origin/reference), matched
by table and offset. A leaf with the same offset and size in both is renamed. Leaves that were split
or merged are compared once, as the bytes they cover, under the modern names joined by "+". Without
`--reference` the names are compared as they are.

`--poison-b` runs B with `playtest.py --poison` (new allocations filled with 0xCD), A as usual.

`--ignore-view-seed` accepts traces whose headers differ only in view_seed (any
other header difference is still refused). In run mode, give `--scenario` twice
to run A on the first file and B on the second.

Exit status: 0 if the traces are identical, 1 if they differ, 2 on a usage
error, a failed run or traces that cannot be compared (different seeds or
scenario).
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path
from typing import NamedTuple

from traceschema import TRACE_FORMAT

ROOT = Path(__file__).resolve().parent.parent
PHASES = ("net", "units", "projectiles", "explosions", "players", "features", "wind", "meteors", "world")
PHASE_FUNCTIONS = {
    "net": ("HandleNetPackets", 0x453D40),
    "units": ("UpdateAllUnits", 0x48AD30),
    "projectiles": ("UpdateProjectiles", 0x49B720),
    "explosions": ("UpdateExplosions", 0x420F30),
    "players": ("UpdatePlayers", 0x464F80),
    "features": ("UpdateFeatures", 0x424050),
    "wind": ("UpdateWind", 0x490C40),
    "meteors": ("UpdateMeteors", 0x437DE0),
}
DUMP_PHASE = {"world": "meteors"}
SCHEMA_PATH = "src/harness/trace_schema.h"
DEFAULT_REFERENCE_REF = "origin/reference"
FIELD_TABLE = re.compile(r"static const TraceField (k\w+Fields)\[\] = \{\n(.*?)\n\};", re.S)
FIELD_ROW = re.compile(r'\{ "([^"]*)", 0x([0-9a-f]+), (\d+), (TF_\w+),')
ORDER_TABLES = ("Order", "OrderPosY")
FX_TABLES = ("ApproachRadius", "RingApproach", "PointMarker", "PathOrder", "PathOrderPosY", "PathOrderHeading",
             "PathOrderPiece", "PathOrderFollow", "AirManeuverOrder", "AirManeuverOrderHeading")
TIMER_TABLES = ("AssaultTimer", "EscortTimer", "SpatialTimer", "SquadTimer")
CHANNEL_TABLES = ("ScriptChannel", "ScriptChannelDelay", "ScriptChannelWait", "ScriptChannelPiece")
GOAL_TABLES = ("PatrolGoal", "AiSearchGoal", "PackedGoal", "LiteGoalState")
CONDITION_TABLES = ("MissionCondition", "ConditionNumUnits", "ConditionNumLeft")
PATH_TABLES = [
    (r"units\[\d+\]\.(?:list|list2)\[\d+\]", ORDER_TABLES),
    (r"units\[\d+\]\.motion\.goal", GOAL_TABLES),
    (r"units\[\d+\]\.motion", ("UnitMotion",)),
    (r"units\[\d+\]\.script\.channels\[\d+\]", CHANNEL_TABLES),
    (r"units\[\d+\]\.script\.pieces\[\d+\]", ("ScriptPiece",)),
    (r"units\[\d+\]\.script\.transforms\[\d+\]", ("PieceTransform",)),
    (r"units\[\d+\]\.script", ("ScriptState",)),
    (r"units\[\d+\]", ("Unit",)),
    (r"orderFx\[\d+\]", FX_TABLES),
    (r"projectiles\[\d+\]", ("Projectile",)),
    (r"explodedPieces\[\d+\]\.rec", ("PieceRec",)),
    (r"explodedPieces\[\d+\]", ("ExplodedPiece",)),
    (r"debris\[\d+\]", ("Debris",)),
    (r"pieces\[\d+\]\.faces\[\d+\]", ("Face",)),
    (r"pieces\[\d+\]", ("Piece",)),
    (r"players\[\d+\]\.ai", ("PlayerAI",)),
    (r"players\[\d+\]\.squadManager\.timers\[\d+\]", TIMER_TABLES),
    (r"players\[\d+\]\.squadManager", ("SquadManager",)),
    (r"players\[\d+\]\.squads\[\d+\]", ("Squad",)),
    (r"players\[\d+\]\.econ", ("UnitResources",)),
    (r"players\[\d+\]", ("Player",)),
    (r"features\[\d+\]", ("SpotObject", "SpotAnim")),
    (r"spatial\[\d+\]|spatial\.overflow", ("Rec",)),
    (r"eyes\[\d+\]", ("Eye",)),
    (r"pathfinder\.pool\[\d+\]", ("Node",)),
    (r"pathfinder", ("Pathfinder", "PathfinderSearch")),
    (r"moveClasses\[\d+\]", ("MoveClass",)),
    (r"conditions\.(?:victory|defeat)\[\d+\]", CONDITION_TABLES),
    (r"conditions", ("MissionConditions",)),
    (r"mission", ("MissionData", "MissionStartRest")),
    (r"weapons\[\d+\]", ("WeaponDefData",)),
    (r"defs\[\d+\]", ("UnitDefData",)),
    (r"wind", ("Wind",)),
    (r"game", ("GameScalar",)),
]
COMPILED_PATH_TABLES = [(re.compile(f"({stem})\\.(.*)", re.S), [f"k{t}Fields" for t in tables])
                        for stem, tables in PATH_TABLES]
COMPARED_HEADERS = ("scenario", "game_seed", "crt_seed", "view_seed")


class Error(Exception):
    pass


def parse_trace(path: Path) -> tuple[dict[str, str], list[tuple[int, str, str]]]:
    """The header lines as a dict, and the (tick, phase, hash) lines."""
    header: dict[str, str] = {}
    rows: list[tuple[int, str, str]] = []
    for n, line in enumerate(path.read_text().splitlines(), 1):
        words = line.split()
        if n == 1:
            if words != ["bt-trace", str(TRACE_FORMAT)]:
                raise Error(f"{path}: not a bt-trace {TRACE_FORMAT} file")
        elif len(words) == 3 and words[0].isdigit() and words[1] in PHASES:
            rows.append((int(words[0]), words[1], words[2]))
        elif rows or not words:
            raise Error(f"{path}:{n}: unexpected line")
        else:
            header[words[0]] = line[len(words[0]):].strip()
    return header, rows


def check_headers(a: dict[str, str], b: dict[str, str], ignore_view_seed: bool = False) -> None:
    keys = [k for k in COMPARED_HEADERS if not (ignore_view_seed and k == "view_seed")]
    bad = [f"{k}: a={a.get(k)} b={b.get(k)}" for k in keys if a.get(k) != b.get(k)]
    if bad:
        raise Error("the traces are not of the same run, " + "; ".join(bad))


def first_divergence(a: list[tuple[int, str, str]], b: list[tuple[int, str, str]]) -> tuple[int, str] | None:
    for x, y in zip(a, b):
        if x != y:
            return (x[0], x[1]) if x[:2] == y[:2] else min((x[:2], y[:2]), key=lambda t: (t[0], PHASES.index(t[1])))
    if len(a) != len(b):
        return (a if len(a) > len(b) else b)[min(len(a), len(b))][:2]
    return None


def parse_dump(path: Path) -> dict[tuple[int, str], dict[str, str]]:
    """Path -> value for each (tick, phase), paths in file order."""
    out: dict[tuple[int, str], dict[str, str]] = {}
    for n, line in enumerate(path.read_text().splitlines(), 1):
        if n == 1:
            if line.split() != ["bt-dump", str(TRACE_FORMAT)]:
                raise Error(f"{path}: not a bt-dump {TRACE_FORMAT} file")
            continue
        m = re.fullmatch(r"(\d+) (\w+) (\S+) (.*)", line)
        if not m:
            raise Error(f"{path}:{n}: unexpected line")
        out.setdefault((int(m[1]), m[2]), {})[m[3]] = m[4]
    return out


def diff_dumps(a: Path, b: Path, tick: int, phase: str, reference: str | None = None,
               maps: dict[str, TableMap] | None = None) -> list[tuple[str, str, str]]:
    """The (path, a value, b value) of each field of that phase that differs; "-" if absent.

    With reference "a" or "b" (and the maps from build_maps), that side's names become the modern ones."""
    phase = DUMP_PHASE.get(phase, phase)
    fa = parse_dump(a).get((tick, phase), {})
    fb = parse_dump(b).get((tick, phase), {})
    if reference:
        if maps is None:
            raise Error("--reference needs the schema maps")
        fa = translate_phase(fa, maps, reference == "a")
        fb = translate_phase(fb, maps, reference == "b")
    if not fa and not fb:
        raise Error(f"no tick {tick} phase {phase} in either dump")
    return [(k, fa.get(k, "-"), fb.get(k, "-")) for k in [*fa, *(k for k in fb if k not in fa)]
            if fa.get(k) != fb.get(k)]


class Leaf(NamedTuple):
    name: str
    offset: int
    size: int

    @property
    def key(self) -> str:
        return self.name or f"+{self.offset:#x}"

    @property
    def end(self) -> int:
        return self.offset + self.size


class Group(NamedTuple):
    key: str
    reference: tuple[Leaf, ...]
    modern: tuple[Leaf, ...]


class TableMap(NamedTuple):
    renames: dict[str, str]
    reference_groups: dict[str, Group]
    modern_groups: dict[str, Group]


def parse_leaf_tables(text: str) -> dict[str, list[Leaf]]:
    """The leaves of each kXxxFields table of a trace_schema.h."""
    return {m[1]: [Leaf(r[1], int(r[2], 16), int(r[3])) for r in FIELD_ROW.finditer(m[2])]
            for m in FIELD_TABLE.finditer(text)}


def overlapping_groups(reference: list[Leaf], modern: list[Leaf]) -> list[Group]:
    """The runs of leaves that cover the same bytes in the two tables but have no leaf in common."""
    both = {(leaf.offset, leaf.size) for leaf in reference} & {(leaf.offset, leaf.size) for leaf in modern}
    loose = sorted([(leaf, True) for leaf in reference if (leaf.offset, leaf.size) not in both] +
                   [(leaf, False) for leaf in modern if (leaf.offset, leaf.size) not in both],
                   key=lambda t: (t[0].offset, t[0].end))
    runs: list[list[tuple[Leaf, bool]]] = []
    end = -1
    for item in loose:
        if runs and item[0].offset < end:
            runs[-1].append(item)
            end = max(end, item[0].end)
        else:
            runs.append([item])
            end = item[0].end
    groups = []
    for run in runs:
        ref = tuple(leaf for leaf, is_ref in run if is_ref)
        mod = tuple(leaf for leaf, is_ref in run if not is_ref)
        if ref and mod:
            groups.append(Group("/".join(leaf.key for leaf in mod), ref, mod))
    return groups


def table_map(reference: list[Leaf], modern: list[Leaf]) -> TableMap:
    by_place = {(leaf.offset, leaf.size): leaf for leaf in modern}
    renames = {leaf.key: by_place[leaf.offset, leaf.size].key for leaf in reference
               if (leaf.offset, leaf.size) in by_place}
    groups = overlapping_groups(reference, modern)
    return TableMap(renames,
                    {leaf.key: g for g in groups for leaf in g.reference},
                    {leaf.key: g for g in groups for leaf in g.modern})


def build_maps(reference_text: str, modern_text: str) -> dict[str, TableMap]:
    """A TableMap for each table the two trace_schema.h share, matched by table and offset."""
    ref, mod = parse_leaf_tables(reference_text), parse_leaf_tables(modern_text)
    return {name: table_map(ref[name], mod[name]) for name in ref if name in mod}


def value_bytes(value: str) -> str | None:
    """The memory-order hex of a dumped scalar or byte run, None for a pointer."""
    if value.startswith("hex:"):
        return value[4:]
    if re.fullmatch(r"0x(?:[0-9a-f]{2})+", value):
        digits = value[2:]
        return "".join(digits[i:i + 2] for i in range(len(digits) - 2, -2, -2))
    return None


def group_value(leaves: tuple[Leaf, ...], values: dict[str, str]) -> str:
    present = [values[leaf.key] for leaf in leaves if leaf.key in values]
    as_bytes = [value_bytes(v) for v in present]
    if all(b is not None for b in as_bytes):
        return "hex:" + "".join(as_bytes)
    return ",".join(present)


def locate_leaf(path: str, maps: dict[str, TableMap], from_reference: bool) -> tuple[str, str | Group] | None:
    """(stem, new leaf name or Group) for a dumped path whose leaf the maps know."""
    for pattern, tables in COMPILED_PATH_TABLES:
        m = pattern.fullmatch(path)
        if not m:
            continue
        for table in tables:
            tm = maps.get(table)
            if tm is None:
                continue
            groups = tm.reference_groups if from_reference else tm.modern_groups
            if m[2] in groups:
                return m[1], groups[m[2]]
            if from_reference and m[2] in tm.renames:
                return m[1], tm.renames[m[2]]
        return None
    return None


def translate_phase(fields: dict[str, str], maps: dict[str, TableMap], from_reference: bool) -> dict[str, str]:
    """One phase's path -> value, with the reference's names changed to the modern ones.

    The leaves of a Group become one path, valued with their bytes; the modern side gets the same."""
    out: dict[str, str] = {}
    grouped: dict[str, tuple[Group, dict[str, str]]] = {}
    for path, value in fields.items():
        found = locate_leaf(path, maps, from_reference)
        if found is None:
            out[path] = value
        elif isinstance(found[1], Group):
            stem, group = found
            key = f"{stem}.{group.key}"
            if key not in grouped:
                grouped[key] = (group, {})
                out[key] = ""
            grouped[key][1][path[len(stem) + 1:]] = value
        else:
            out[f"{found[0]}.{found[1]}"] = value
    for key, (group, values) in grouped.items():
        out[key] = group_value(group.reference if from_reference else group.modern, values)
    return out


def read_reference_schema(ref: str) -> str:
    r = subprocess.run(["git", "-C", str(ROOT), "show", f"{ref}:{SCHEMA_PATH}"], capture_output=True, text=True)
    if r.returncode != 0:
        raise Error(f"cannot read {SCHEMA_PATH} on {ref} (try git fetch origin reference): {r.stderr.strip()}")
    return r.stdout


def load_maps(ref: str) -> dict[str, TableMap]:
    return build_maps(read_reference_schema(ref), (ROOT / SCHEMA_PATH).read_text())


def describe_phase(phase: str) -> str:
    """The function a phase's hash follows, from RunGameSteps (src/ingame/dialogs_493340.cpp)."""
    if phase not in PHASE_FUNCTIONS:
        return "none (the once-per-tick tables, hashed after UpdateMeteors)"
    name, address = PHASE_FUNCTIONS[phase]
    return f"{name} (FUNCTION: {address:#x})"


def print_diff(diffs: list[tuple[str, str, str]]) -> None:
    """The first differing field on its own line, then all of them."""
    if not diffs:
        print("  no field differs in the dumps")
        return
    path, va, vb = diffs[0]
    print(f"  first differing field: {path}: a={va} b={vb}")
    print(f"  all {len(diffs)} differing fields:")
    for path, va, vb in diffs:
        print(f"    {path}: a={va} b={vb}")


def print_divergence(tick: int, phase: str) -> None:
    print(f"first divergence: tick {tick}, phase {phase}")
    print(f"  phase function: {describe_phase(phase)}")


def play(exe: Path, scenario: Path, game_dir: str | None, dump: int | None, timeout: float | None = None,
         poison: bool = False) -> Path:
    """Run the exe on the scenario with playtest.py; the run's directory."""
    cmd = [sys.executable, str(ROOT / "tools/playtest.py"), "--exe", str(exe), "--testmode", str(scenario)]
    if game_dir:
        cmd += ["--game-dir", game_dir]
    if dump is not None:
        cmd += ["--testdump", str(dump)]
    if timeout is not None:
        cmd += ["--testmode-timeout", f"{timeout:g}"]
    if poison:
        cmd.append("--poison")
    r = subprocess.run(cmd, capture_output=True, text=True)
    sys.stderr.write(r.stderr)
    m = re.search(r"^logs and screenshots: (.+)/$", r.stdout, re.M)
    if r.returncode != 0 or not m:
        raise Error(f"playtest.py failed for {exe} (exit {r.returncode}):\n{r.stdout}")
    runs = [d for d in (ROOT / m[1]).iterdir() if d.is_dir()]
    if len(runs) != 1:
        raise Error(f"expected one run in {m[1]}")
    return runs[0]


def need(run: Path, name: str) -> Path:
    if not (run / name).is_file():
        raise Error(f"{run} has no {name}: does the exe write traces?")
    return run / name


def compare(args: argparse.Namespace) -> int:
    exes = [Path(e).resolve() for e in args.exe]
    scenarios = [Path(s).resolve() for s in args.scenario]
    scenarios *= 2 if len(scenarios) == 1 else 1
    poison = [False, args.poison_b]
    runs = [play(e, s, args.game_dir, None, None, p) for e, s, p in zip(exes, scenarios, poison)]
    (ha, ra), (hb, rb) = (parse_trace(need(r, "trace.txt")) for r in runs)
    check_headers(ha, hb, args.ignore_view_seed)
    div = first_divergence(ra, rb)
    if not div:
        print(f"traces identical ({len(ra)} lines)")
        return 0
    tick, phase = div
    print_divergence(tick, phase)
    runs = [play(e, s, args.game_dir, tick, None, p) for e, s, p in zip(exes, scenarios, poison)]
    maps = load_maps(args.reference_ref) if args.reference else None
    print_diff(diff_dumps(need(runs[0], "dump.txt"), need(runs[1], "dump.txt"), tick, phase, args.reference, maps))
    return 1


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("exe", nargs="*", help="the two exes, A then B")
    ap.add_argument("--scenario", metavar="FILE", action="append",
                    help="a reference/ scenario file (twice: A's, then B's)")
    ap.add_argument("--ignore-view-seed", action="store_true",
                    help="accept traces whose headers differ only in view_seed")
    ap.add_argument("--game-dir", help="the game's install (default: Steam's)")
    ap.add_argument("--poison-b", action="store_true",
                    help="run B with new game allocations filled with 0xCD (as reference.py --check --poison does)")
    ap.add_argument("--reference", choices=("a", "b"),
                    help="the side whose dump came from the reference build: its leaf names are translated to the modern ones")
    ap.add_argument("--reference-ref", default=DEFAULT_REFERENCE_REF, metavar="REF",
                    help=f"the git ref whose {SCHEMA_PATH} names the reference's leaves (default {DEFAULT_REFERENCE_REF})")
    ap.add_argument("--traces", nargs=2, metavar=("A", "B"), help="compare two trace.txt files, without running")
    ap.add_argument("--dumps", nargs=2, metavar=("A", "B"), help="diff two dump.txt files, with --at")
    ap.add_argument("--at", nargs=2, metavar=("TICK", "PHASE"), help="--dumps: the tick and phase to diff")
    args = ap.parse_args()
    modes = [bool(args.traces), bool(args.dumps), bool(args.exe)]
    if sum(modes) != 1:
        ap.error("give two exes with --scenario, or --traces A B, or --dumps A B --at TICK PHASE")
    if args.exe and (len(args.exe) != 2 or not args.scenario):
        ap.error("two exes and --scenario are required")
    if args.exe and len(args.scenario) > 2:
        ap.error("--scenario takes at most two files")
    if args.reference and args.traces:
        ap.error("--reference applies to --dumps and to running two exes")
    for s in args.scenario or []:
        if args.exe and not Path(s).is_file():
            ap.error(f"{s} is not a file")
    if args.dumps and (not args.at or not args.at[0].isdigit() or args.at[1] not in PHASES):
        ap.error(f"--dumps needs --at TICK PHASE, phase one of {' '.join(PHASES)}")
    try:
        if args.traces:
            (ha, ra), (hb, rb) = (parse_trace(Path(t)) for t in args.traces)
            check_headers(ha, hb, args.ignore_view_seed)
            div = first_divergence(ra, rb)
            if not div:
                print(f"traces identical ({len(ra)} lines)")
                return 0
            print_divergence(*div)
            return 1
        if args.dumps:
            maps = load_maps(args.reference_ref) if args.reference else None
            diffs = diff_dumps(Path(args.dumps[0]), Path(args.dumps[1]), int(args.at[0]), args.at[1],
                               args.reference, maps)
            print(f"tick {args.at[0]}, phase {args.at[1]}")
            print(f"  phase function: {describe_phase(args.at[1])}")
            print_diff(diffs)
            return 1 if diffs else 0
        return compare(args)
    except (Error, OSError) as e:
        print(f"tracediff: {e}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())

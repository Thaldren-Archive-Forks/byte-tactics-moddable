"""Group the functions into subsystems and count the calls and shared sim state between each pair.

    uv run tools/coupling.py                 # writes data/subsystems.csv (keeping override) and data/coupling.csv
    uv run tools/coupling.py --check         # fail if either file differs from what the tool regenerates
    uv run tools/coupling.py --summary [S..] # print entry points, imports and sim state per subsystem
    uv run tools/coupling.py --seams         # fail on any direct call between sim subsystems, or from game into one
    uv run tools/test_coupling.py            # tests, on hand-written rows

This is the evidence for #128: each subsystem issue (#9 to #18) starts from a draft
interface, and CI shows every new call or shared field between subsystems as a change to
data/coupling.csv. It reads data/rwsets.csv, data/hazard_edges.csv and
data/state_classes.csv and compiles nothing.

Subsystems. Every function of data/rwsets.csv (a `function` row: name and file) is in one of
SUBSYSTEMS. Overloads and same-named functions in several files are separate rows here,
but the walks below are by name, as in hazards.py.

data/subsystems.csv columns: function, file, subsystem, rule, override. `subsystem` is
`override` when that is set and else the answer of the first rule below that applies;
`rule` is that rule (`name:detail`) even when an override replaced its answer, so the
file shows what the hand set differs from. Regeneration keeps `override` by (function,
file) and rewrites the rest; an override that no longer names a function is an error.

Rules, first match wins:

  folder        src/harness is harness; src/seams is seams; src/debug, src/util, src/runtime and src/data are platform
  draw          a name that starts (after `Class::`) with Draw, Render or Blit is presentation, unless
                a phase walk reaches it (see phase)
  library       _Construct, _Destroy, operator new and delete, Unguarded_insert and Copy_backward (the
                library code the compiler instantiated in a game file) are platform
  file          a path prefix of FILES decides: a file or folder that holds one subsystem's code
  phase         for a function in a folder that holds several subsystems (src/game, src/map,
                the presentation folders), the one phase root whose walk reaches it, if only
                one does, by PHASE_SUBSYSTEMS. HandleNetPackets does not count: it dispatches
                into every subsystem, so what it reaches says nothing about the owner.
  presentation  src/graphics, src/gui, src/frontend, src/ingame and src/sound, unless a phase
                walk reaches the function: a function the sim runs is never presentation
  name          the first pattern of NAMES that matches the qualified function name (a
                presentation pattern never matches a function a phase walk reaches)
  default       the folder's subsystem (FOLDER_DEFAULT), else game

The phase walk is stateclasses.Walks: each root of hazards.ROOTS over the `call` rows and
the hand-resolved edges, one root at a time, stopping at presentation code (every function
of the presentation folders, the main loop and the frame handlers). So a helper in a
presentation folder that the sim calls (src/ingame/control_panel.cpp's AddBuildProgress,
AddRepairProgress and the other sim code docs/state.md names) is not reached by a walk and
is set by `override`. StepGafSequence is the one function of a presentation folder a walk
reaches (stateclasses.SIM_IN_PRESENTATION_DIRS) and its phase rule gives it a subsystem.
src/harness is not a phase and is no one's callee, so a walk never reaches it.

Coupling. data/coupling.csv has columns from, to, kind, target, count, sorted by from, to,
kind, target. Kinds:

  call          a `call` row of a function of `from` whose callee is a function of `to`;
                target is the callee, count the functions of `from` that call it. A callee
                name defined in several subsystems is `from`'s own if one of them is, else
                the first by name. A call into code outside data/rwsets.csv (a library
                function) is not a row.
  indirect      the same for the (caller, target) pairs of data/hazard_edges.csv, the calls
                through a function pointer or a virtual method resolved by hand
  shared_write  a sim item (class `sim` or `mixed`) that `from` writes and `to` reads or
                writes, `from` != `to`; target is the item and count the functions of `to`
                that read or write it. A pair of subsystems that both write an item has a
                row each way.

Items are the rows of data/state_classes.csv that a `global_*` target of data/rwsets.csv
joins (stateclasses.Items.join: the same name, or a DAT_ address), with that row's class
(or its proposal where the class is empty), and the fields `Type::field` of `field_*`
rows, which have no row of their own: their class is the proposal of stateclasses.classify
on the phases and presentation code that access them, by type as in stateclasses.py. A
`Game::name` field is the name the source gave it, and each source file declares its own
partial view of struct Game, so two names for one offset are two items here and the
bytes of the regions are not looked at (that needs the layouts only a clang run gives).
A write is a `*_write` row; an address taken or a pointer passed to a non-const parameter
counts as a read, because the callee's own rows show what it writes. A field of an anonymous
type (`<anon>::x`) is the same name for unrelated structs and is left out, and so are
the other accesses (deref_*), which name no item. Types shared by everything (Vec3, Point,
Fixed) give every subsystem that touches them a row: the limit of evidence by type.

Seams. The sim subsystems (SIM_SUBSYSTEMS) call each other only through the seams of
interfaces/seams.h (#8), and there is no list of exceptions. --seams reads data/coupling.csv, takes
every `call` row whose `from` and `to` are both sim subsystems, or whose `from` is `game` and whose
`to` is a sim subsystem, and fails on each one, listing it. An `indirect` row, a shared_write row and a
call to or from presentation, network, harness, seams or platform code is not an edge. src/seams holds
the code that fills the seam tables; it is the `seams` subsystem, so the direct calls it makes into the
sim are not edges, and the function addresses it takes (`fn_ref` rows) are not `call` rows in any case.

--summary prints, for each subsystem (or the ones named): its entry points (the phase roots
it holds, and the functions other subsystems call, with the number of callers), its imports
(the functions it calls in other subsystems, by subsystem), and the sim items it writes and
reads.
"""

import argparse
import csv
import difflib
import io
import re
import sys
import textwrap
from collections import defaultdict
from pathlib import Path

import hazards
import stateclasses as sc
from rwsets import ROOT

RWSETS = ROOT / "data/rwsets.csv"
EDGES = ROOT / "data/hazard_edges.csv"
STATE_CLASSES = ROOT / "data/state_classes.csv"
SUBSYSTEMS_CSV = ROOT / "data/subsystems.csv"
COUPLING_CSV = ROOT / "data/coupling.csv"
SUBSYSTEM_COLUMNS = ["function", "file", "subsystem", "rule", "override"]
COUPLING_COLUMNS = ["from", "to", "kind", "target", "count"]
SIM_SUBSYSTEMS = ["wind_meteors", "economy", "features", "ai", "pathfinding", "weapons", "targeting", "los_radar",
                  "orders", "units", "players", "map"]
SUBSYSTEMS = SIM_SUBSYSTEMS + ["network", "game", "harness", "seams", "presentation", "platform"]
PHASE_SUBSYSTEMS = {"UpdateAllUnits": "units", "UpdateProjectiles": "weapons",
                    "UpdateExplosions": "weapons", "UpdatePlayers": "players", "UpdateFeatures": "features",
                    "StepAllGafSequences": "game", "UpdateWind": "wind_meteors", "UpdateMeteors": "wind_meteors",
                    "UpdateResourceSharing": "economy"}
FOLDER_FIXED = [("src/harness/", "harness"), ("src/seams/", "seams"), ("src/debug/", "platform"), ("src/util/", "platform"),
                ("src/runtime/", "platform"), ("src/data/", "platform")]
DISPATCH_ROOTS = {"HandleNetPackets"}
DRAW = re.compile(r"(^|::)(Draw|Render|Blit)")
LIBRARY = re.compile(r"^(_Construct|_Destroy|operator new|operator delete|Unguarded_insert|Copy_backward)$")
FILES = [("src/ai/pathfind", "pathfinding"), ("src/ai/", "ai"), ("src/network/", "network"),
         ("src/weapons/", "weapons"), ("src/orders/", "orders"),
         ("src/map/features", "features"), ("src/map/line_of_sight", "los_radar"),
         ("src/map/los_tables", "los_radar"), ("src/map/radar", "los_radar"), ("src/map/meteors", "wind_meteors"),
         ("src/map/terrain", "map"), ("src/map/map_", "map"), ("src/map/plot_map", "map"),
         ("src/game/economy", "economy"), ("src/game/share_commands", "economy"),
         ("src/game/players_4658e0", "los_radar"), ("src/game/players_465ac0", "los_radar"),
         ("src/game/players", "players"), ("src/game/victory", "players"), ("src/game/statistics", "players"),
         ("src/game/data_files", "platform"), ("src/game/cd_check", "platform")]
SEVERAL_SUBSYSTEMS = ("src/game/", "src/map/")
PRESENTATION_FOLDERS = sc.PRESENTATION_DIRS
NAMES = [("presentation", r"^(CMemoryCache|UnitTable)::|WorldToScreen"), ("platform", r"^(TdfFile|TdfFieldVector|StringRef|OvrFile)::"),
         ("map", r"PassMap"), ("pathfinding", r"Pathfind"), ("targeting", r"Target"),
         ("weapons", r"Weapon|Projectile|Explosion|Explode|Ballistic|Damage|Paralyze|Detonate"),
         ("los_radar", r"LineOfSight|LOS|Radar|Sonar|Eyeball|Fog|Cloak|Visible|Explored|IsSeen|Sight|Jam"),
         ("features", r"Feature|Burn"), ("wind_meteors", r"Wind|Meteor"),
         ("economy", r"Economy|Resource|Energy|Metal|Income|Storage|Spend|Share"),
         ("ai", r"\bAI\b|PlayerAI|Squad|Scout"), ("orders", r"Order"),
         ("players", r"Player|Victory|MissionCondition"), ("map", r"Terrain|Footprint|Cell|Plotmap"),
         ("network", r"Packet|Sync")]
NAME_PATTERNS = [(subsystem, re.compile(pattern)) for subsystem, pattern in NAMES]
FOLDER_DEFAULT = {"src/ai/": "ai", "src/units/": "units", "src/orders/": "orders", "src/weapons/": "weapons",
                  "src/network/": "network", "src/game/": "game", "src/map/": "map"}
BIG = 1 << 40


def rule_of(function: str, file: str, reached: dict[str, set[str]]) -> tuple[str, str]:
    """The (subsystem, rule) of a function by the rules in the docstring, without an override.

    `reached` maps a function name to the phase roots whose walks reach it.
    """
    for prefix, subsystem in FOLDER_FIXED:
        if file.startswith(prefix):
            return subsystem, "folder:" + prefix
    roots = reached.get(function, set())
    if DRAW.search(function) and not roots:
        return "presentation", "draw"
    if LIBRARY.search(function):
        return "platform", "library"
    for prefix, subsystem in FILES:
        if file.startswith(prefix):
            return subsystem, "file:" + prefix
    several = file.startswith(SEVERAL_SUBSYSTEMS) or file.startswith(PRESENTATION_FOLDERS) \
        or not any(file.startswith(p) for p in FOLDER_DEFAULT)
    decisive = roots - DISPATCH_ROOTS
    if several and len(decisive) == 1:
        root = next(iter(decisive))
        return PHASE_SUBSYSTEMS[root], "phase:" + root
    if file.startswith(PRESENTATION_FOLDERS) and not roots:
        return "presentation", "presentation"
    for subsystem, pattern in NAME_PATTERNS:
        found = pattern.search(function)
        if found and not (subsystem == "presentation" and roots):
            return subsystem, "name:" + found.group(0)
    for prefix, subsystem in FOLDER_DEFAULT.items():
        if file.startswith(prefix):
            return subsystem, "default:" + prefix
    return "game", "default"


def functions_of(rows: list[list[str]]) -> list[tuple[str, str]]:
    """The distinct (function, file) pairs of the `function` rows, by address, then name, then file."""
    first: dict[tuple[str, str], int] = {}
    for address, function, file, kind, _ in rows:
        if kind == "function":
            key = (function, file)
            at = int(address, 16) if address else BIG
            first[key] = min(first.get(key, BIG), at)
    return sorted(first, key=lambda k: (first[k], k))


def phase_roots(walks) -> dict[str, set[str]]:
    """Each function name mapped to the phase roots whose walk reaches it, one walk per root."""
    reached: dict[str, set[str]] = defaultdict(set)
    for root in hazards.ROOTS:
        for name in sc.reach(walks.graph, [root], walks.stops):
            reached[name].add(root)
    return reached


def build_subsystems(rows: list[list[str]], reached: dict[str, set[str]], overrides: dict[tuple, str]) -> list[dict]:
    """The rows of data/subsystems.csv: every function with its rule's answer and the kept override."""
    out = []
    for function, file in functions_of(rows):
        subsystem, rule = rule_of(function, file, reached)
        override = overrides.get((function, file), "")
        out.append({"function": function, "file": file, "subsystem": override or subsystem, "rule": rule,
                    "override": override})
    return out


def render(columns: list[str], rows: list[dict]) -> str:
    out = io.StringIO()
    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(columns)
    writer.writerows([[row[c] for c in columns] for row in rows])
    return out.getvalue()


def read_csv(path: Path) -> list[dict]:
    if not path.exists():
        return []
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def read_overrides(path: Path = SUBSYSTEMS_CSV) -> dict[tuple, str]:
    """The (function, file) -> subsystem overrides of a subsystems file, or none when it does not exist."""
    out = {}
    for row in read_csv(path):
        if row["override"]:
            if row["override"] not in SUBSYSTEMS:
                sys.exit("coupling: %s: override %r of %s is not a subsystem" % (path.name, row["override"],
                                                                                  row["function"]))
            out[(row["function"], row["file"])] = row["override"]
    return out


def stale_overrides(overrides: dict[tuple, str], rows: list[dict]) -> list[tuple]:
    """The overrides that name no function of the rows."""
    have = {(r["function"], r["file"]) for r in rows}
    return [key for key in overrides if key not in have]


def subsystem_maps(subsystems: list[dict]) -> tuple[dict[tuple, str], dict[str, list[str]]]:
    """The subsystem of each (function, file), and the sorted subsystems of each function name."""
    of = {(r["function"], r["file"]): r["subsystem"] for r in subsystems}
    names: dict[str, set[str]] = defaultdict(set)
    for (function, _), subsystem in of.items():
        names[function].add(subsystem)
    return of, {name: sorted(subs) for name, subs in names.items()}


def callee_subsystem(caller: str, targets: list[str] | None) -> str | None:
    """The subsystem a call goes to, or None when it stays in the caller's or leaves the source."""
    if not targets or caller in targets:
        return None
    return targets[0]


def call_counts(rows: list[list[str]], edges: list[tuple[str, str]], of: dict, names: dict) -> dict[tuple, int]:
    """(from, to, kind, target) -> number of calling functions, for `call` rows and the hand-resolved edges."""
    counts: dict[tuple, int] = defaultdict(int)
    for _, function, file, kind, target in rows:
        if kind == "call":
            to = callee_subsystem(of[(function, file)], names.get(target))
            if to:
                counts[(of[(function, file)], to, "call", target)] += 1
    callers: dict[str, list[tuple]] = defaultdict(list)
    for function, file in of:
        callers[function].append((function, file))
    for caller, target in edges:
        for key in callers.get(caller, ()):
            to = callee_subsystem(of[key], names.get(target))
            if to:
                counts[(of[key], to, "indirect", target)] += 1
    return counts


def seam_edges(coupling: list[dict]) -> set[tuple[str, str, str]]:
    """The (from, to, target) of each direct call between sim subsystems, or from `game` into one."""
    sim = set(SIM_SUBSYSTEMS)
    return {(r["from"], r["to"], r["target"]) for r in coupling
            if r["kind"] == "call" and r["to"] in sim and (r["from"] in sim or r["from"] == "game")}


def check_seams(coupling: list[dict]) -> tuple[list[str], str]:
    """The failure lines, and the one-line summary when there are none."""
    edges = sorted(seam_edges(coupling))
    lines = ["seams: direct call %s -> %s: %s" % edge for edge in edges]
    if edges:
        lines.append("seams: a call between sim subsystems must go through a seam (interfaces/seams.h)")
    return lines, "seams: 0 direct calls between sim subsystems"


def item_accesses(rows: list[list[str]], items: sc.Items) -> tuple[dict, dict]:
    """The accesses by item key: name-level (for the class of a field) and by (function, file) with a write flag."""
    by_name: dict[tuple, sc.Access] = defaultdict(sc.Access)
    by_function: dict[tuple, dict[tuple, bool]] = defaultdict(dict)
    for _, function, file, kind, target in rows:
        category, _, mode = kind.partition("_")
        if category == "global" and mode:
            key = items.join(target)
        elif category == "field" and mode and not target.startswith("<anon>"):
            key = ("field", target)
        else:
            continue
        if key is None:
            continue
        by_name[key].add(function, mode)
        written = by_function[key].get((function, file), False)
        by_function[key][(function, file)] = written or mode == "write"
    return by_name, by_function


def item_classes(by_name: dict, state_rows: list[dict], walks) -> dict[tuple, str]:
    """The class of each item: the reviewed or proposed class of its state_classes row, else the field's proposal."""
    known = {(r["kind"], r["name"]): r["class"] or r["proposed"] for r in state_rows}
    out = {}
    for key, access in by_name.items():
        if key[0] == "field":
            out[key] = sc.classify(sc.evidence(access, walks))[0]
        elif key in known:
            out[key] = known[key]
    return out


def shared_counts(by_function: dict, classes: dict, of: dict) -> tuple[dict, dict, dict]:
    """(from, to, "shared_write", item) -> functions of `to`, and each subsystem's sim items written and read."""
    counts: dict[tuple, int] = defaultdict(int)
    wrote: dict[str, set[str]] = defaultdict(set)
    read: dict[str, set[str]] = defaultdict(set)
    for key, accessors in by_function.items():
        if classes.get(key) not in ("sim", "mixed"):
            continue
        name = key[1]
        writers, touching = set(), defaultdict(int)
        for function, written in accessors.items():
            subsystem = of[function]
            touching[subsystem] += 1
            (wrote if written else read)[subsystem].add(name)
            if written:
                writers.add(subsystem)
        for writer in writers:
            for subsystem, n in touching.items():
                if subsystem != writer:
                    counts[(writer, subsystem, "shared_write", name)] += n
    return counts, wrote, read


class Result:
    """The coupling rows and the sim items each subsystem reads and writes."""

    def __init__(self, counts: dict, wrote: dict, read: dict):
        self.rows = [{"from": f, "to": t, "kind": k, "target": target, "count": str(n)}
                     for (f, t, k, target), n in sorted(counts.items())]
        self.wrote, self.read = wrote, read


def couple(rows: list[list[str]], edges: list[tuple[str, str]], subsystems: list[dict], state_rows: list[dict],
           globals_rows: list[dict], walks) -> Result:
    """The coupling of the rows under a subsystem assignment."""
    of, names = subsystem_maps(subsystems)
    counts = call_counts(rows, edges, of, names)
    by_name, by_function = item_accesses(rows, sc.Items(rows, globals_rows))
    classes = item_classes(by_name, state_rows, walks)
    shared, wrote, read = shared_counts(by_function, classes, of)
    counts.update(shared)
    return Result(counts, wrote, read)


def load():
    """Everything the tool reads: the rwsets rows, the walks, the edges, the state classes and the globals."""
    rows, graph = hazards.load_graph()
    walks = sc.Walks(rows, graph)
    walks.graph = graph
    edges = hazards.read_edges(EDGES, graph.functions)
    return rows, walks, edges, read_csv(STATE_CLASSES), sc.read_globals()


def generate(overrides: dict[tuple, str]) -> tuple[list[dict], Result, list[list[str]], list[tuple]]:
    rows, walks, edges, state_rows, globals_rows = load()
    subsystems = build_subsystems(rows, phase_roots(walks), overrides)
    return subsystems, couple(rows, edges, subsystems, state_rows, globals_rows, walks), rows, edges


def diff(name: str, have: str, want: str) -> list[str]:
    return list(difflib.unified_diff(have.splitlines(), want.splitlines(), name, "regenerated", lineterm="", n=0))


def listing(names, indent: str) -> str:
    """The sorted names on wrapped lines."""
    return textwrap.fill(", ".join(sorted(names)) or "none", 118, initial_indent=indent, subsequent_indent=indent)


def summary(result: Result, subsystems: list[dict], wanted: list[str]) -> str:
    """Per subsystem: its entry points, its imports and the sim items it writes and reads."""
    callers: dict[str, dict[str, int]] = defaultdict(lambda: defaultdict(int))
    imports: dict[str, dict[str, set]] = defaultdict(lambda: defaultdict(set))
    for r in result.rows:
        if r["kind"] != "shared_write":
            callers[r["to"]][r["target"]] += int(r["count"])
            imports[r["from"]][r["to"]].add(r["target"])
    sizes: dict[str, int] = defaultdict(int)
    roots: dict[str, list[str]] = defaultdict(list)
    of = {}
    for r in subsystems:
        sizes[r["subsystem"]] += 1
        of.setdefault(r["function"], r["subsystem"])
    for root in hazards.ROOTS:
        if root in of:
            roots[of[root]].append(root)
    out = []
    for subsystem in wanted or SUBSYSTEMS:
        out.append("%s: %d functions" % (subsystem, sizes[subsystem]))
        out.append("  entry points, phase roots: %s" % (", ".join(roots[subsystem]) or "none"))
        called = callers[subsystem]
        out.append("  entry points, called from other subsystems, with the callers (%d):" % len(called))
        out.append(listing(["%s(%d)" % (n, c) for n, c in called.items()], "    "))
        total = sum(len(v) for v in imports[subsystem].values())
        out.append("  imports (%d functions of %d subsystems):" % (total, len(imports[subsystem])))
        for to, targets in sorted(imports[subsystem].items(), key=lambda x: (-len(x[1]), x[0])):
            out.append("    to %s (%d):" % (to, len(targets)))
            out.append(listing(targets, "      "))
        for label, items in (("sim items written", result.wrote[subsystem]),
                             ("sim items only read", result.read[subsystem] - result.wrote[subsystem])):
            out.append("  %s (%d):" % (label, len(items)))
            out.append(listing(items, "    "))
    return "\n".join(out)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if data/subsystems.csv or data/coupling.csv differs")
    parser.add_argument("--summary", nargs="*", metavar="SUBSYSTEM",
                        help="print entry points, imports and sim state of the subsystems (default all)")
    parser.add_argument("--seams", action="store_true",
                        help="fail on any direct call between sim subsystems, or from game into one")
    args = parser.parse_args()
    if args.seams:
        coupling = read_csv(COUPLING_CSV)
        lines, ok = check_seams(coupling)
        if lines:
            print("\n".join(lines))
            sys.exit(1)
        print(ok)
        return
    overrides = read_overrides()
    subsystems, result, rows, _ = generate(overrides)
    bad = stale_overrides(overrides, subsystems)
    if bad:
        for function, file in bad:
            print("coupling: override of %s (%s) names no function of data/rwsets.csv" % (function, file))
        sys.exit(1)
    texts = {SUBSYSTEMS_CSV: render(SUBSYSTEM_COLUMNS, subsystems), COUPLING_CSV: render(COUPLING_COLUMNS, result.rows)}
    if args.summary is not None:
        unknown = [s for s in args.summary if s not in SUBSYSTEMS]
        if unknown:
            sys.exit("coupling: not a subsystem: %s" % ", ".join(unknown))
        print(summary(result, subsystems, args.summary))
        return
    if args.check:
        failed = False
        for path, text in texts.items():
            have = path.read_text() if path.exists() else ""
            lines = diff(path.name, have, text)
            if lines:
                failed = True
                print("\n".join(lines[:40]))
                if len(lines) > 40:
                    print("... %d more lines" % (len(lines) - 40))
                print("coupling: data/%s is out of date; run `uv run tools/coupling.py` and commit the result" % path.name)
        if failed:
            sys.exit(1)
        print("coupling: up to date (%d functions, %d coupling rows)" % (len(subsystems), len(result.rows)))
        return
    for path, text in texts.items():
        path.write_text(text)
    print("%d functions, %d coupling rows" % (len(subsystems), len(result.rows)))


if __name__ == "__main__":
    main()

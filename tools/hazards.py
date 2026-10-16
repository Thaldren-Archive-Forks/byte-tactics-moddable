"""List the places in game logic whose behaviour could depend on addresses, uninitialised memory or time.

    uv run tools/hazards.py                 # writes data/hazards.csv, keeping verdicts and notes
    uv run tools/hazards.py --check         # fail if the candidates differ from data/hazards.csv
    uv run tools/hazards.py --unchecked     # fail if a row of data/hazards.csv has no verdict
    uv run tools/hazards.py --build-dir D   # use D/compile_commands.json (default build/rwsets)
    uv run tools/hazards.py --jobs N        # clang-cl processes at once (default 4)
    uv run tools/test_hazards.py            # tests, on hand-written functions

A different allocator, the WASM build or machine load changes heap addresses, the
contents of fresh memory and timing. This tool lists every place in game logic that
could notice, as candidates for a hand check (#126). It does not decide: the `verdict`
column of data/hazards.csv is filled in by hand with `real` or `false_positive`, and
`note` says why.

Game logic is every function reachable through the `call` rows of data/rwsets.csv from
the roots below (the tick roots), or from the load roots (see Load). Overloads share a name in rwsets.csv, so a walk reaches all of them. A
virtual or function-pointer call has no `call` row, so the walk also follows the edges of
data/hazard_edges.csv (columns caller, target, reason): one row per calling function and
function it can reach through a virtual method, a function pointer or a table of them,
resolved by hand from the vtables and from the assignments to the pointer, with the
file:line of the evidence in `reason`. A call into code outside the source (a COM method,
a function from GetProcAddress) has no edge. Both names of a row must be functions of
data/rwsets.csv, and the tool fails on a stale row. Each such call is still an
`indirect_call` row of data/hazards.csv, whose verdict says where it can go. The roots are the functions RunGameSteps (0x495490) calls in its phases that
change or feed the simulation:

  HandleNetPackets      phase 0: applies received orders and unit state
  UpdateAllUnits, UpdateProjectiles, UpdateExplosions, UpdatePlayers, UpdateFeatures,
  StepAllGafSequences, UpdateWind, UpdateMeteors      phases 1 to 7
  UpdateResourceSharing the resource-sharing step after the phases

Load. LOAD_ROOTS are LoadMatch (0x497180), which builds the simulation state when a match
starts (BuildAllPassMaps, for one), and AssignStartPositions (0x4568c0), which only frame
handlers call and which draws the start slots. They are walked over the same `call` rows
and edges, and like the tick walk the load walk has no stop at presentation code: the hand
check marks the hazards of the helpers that only draw or play a sound false_positive. The
`phase` column of a row is `tick` when the walk from the tick roots reaches its function,
else `load`; a function both walks reach is `tick`.

Left out: HarnessBeforeStep, HarnessPhase and HarnessAfterStep (the trace harness
reads the state, it is not the game), AccumulateProfileTime (profiling), UpdateCameraFollow,
UpdateParticles and UpdateBlink (presentation), SendAllQueued (sends what the step
produced, changes nothing in the state), ExpireOldestMessage, ExpireEyeballs and the
empty post-step hooks (UI). Because the walk is by name over the call graph, game logic
also contains helpers that only play a sound or print a message; the hand check marks
the hazards in those false_positive, and #125's classes check the result.

The source is the clang syntax tree rwsets.py reads (one clang-cl run per file, which
also gives the warnings below). Kinds, one row per place:

  ptr_compare      `<`, `>`, `<=`, `>=` or `-` with a pointer on both sides. `==`, `!=` and
                   a comparison with a null pointer are not hazards. target: `op Type*`.
  ptr_to_int       a cast with castKind PointerToIntegral, in any spelling (C cast,
                   reinterpret_cast, functional cast). C++ has no implicit conversion of a
                   pointer to an integer, so arithmetic on a pointer value (hashing,
                   masking, sorting keys) always shows up as this cast. The cast of a
                   string literal or of a function is also a row: its address is stable
                   within one build but not between builds. target: `from -> to`. Not
                   found: a pointer read through a union or `*(int*)&p`.
  uninit           clang's -Wuninitialized, -Wsometimes-uninitialized and
                   -Wconditional-uninitialized warnings, mapped to the function whose
                   source lines contain them. target: `variable (warning name)`.
  heap_uncleared   an allocation whose result is not cleared in the same function.
                   Allocators: `new` and `new[]` (but not `new T()`, `new T{}` or any other
                   initialiser, nor a class with a constructor of its own, which is taken to
                   set its fields), operator new, malloc, realloc, GlobalAlloc and LocalAlloc
                   (unless the flags constant has GMEM_ZEROINIT, 0x40), GlobalAllocFixed,
                   HeapAlloc (unless HEAP_ZERO_MEMORY 8) and the game's own GameAlloc,
                   GameRealloc and PoolAlloc, which wrap malloc and GlobalAlloc in
                   debug_lib.cpp. calloc and VirtualAlloc return zeroed memory and are
                   skipped. Cleared means a call of memset, ZeroMemory, FillMemory,
                   bzero, or a function or method whose name has zero, clear, wipe,
                   memset or memfill in it, where the first argument (memset family) or
                   any argument mentions the variable or field the result was stored in
                   (or any clearing call at all, when the result is not stored in one).
                   The order of the calls is not checked. An allocation that is returned
                   directly is a wrapper and is not a row, its callers are. target:
                   `allocator` or `new Type`.
  time_read        a call of timeGetTime, GetTickCount, QueryPerformanceCounter,
                   QueryPerformanceFrequency, GetSystemTime, GetSystemTimeAsFileTime,
                   GetLocalTime, time, clock, localtime or _ftime, or of the source's
                   wrappers of them: GetTicks and GetMilliseconds (display.cpp),
                   GetTimeSeconds (debug_lib_4e1e50.cpp), and UpdateTimers and AddTimer,
                   which run the timer table by the clock. target: the function called.
  thread_global    a global that code running on another thread writes (global_write or
                   global_maybe_written in rwsets.csv, reached through `call` rows from a
                   thread entry point) and a game-logic function reads. Thread entry
                   points are the functions passed to CreateThread, _beginthread,
                   _beginthreadex, StartThread (the game's wrapper of _beginthread),
                   timeSetEvent, SetTimer, CreateTimerQueueTimer, QueueUserAPC, the
                   DirectPlay and DirectSound enumerations (EnumSessions, EnumPlayers,
                   EnumConnections, EnumAddress, EnumAddressTypes, DirectPlayEnumerate*,
                   DirectSoundEnumerate*) and the game's HAPINET_enum* wrappers of them.
                   A callback that DirectPlay runs on the calling thread is listed too,
                   since which thread runs it is the library's choice. One row per
                   (global, reading function) while there are few; see THREAD_ROW_LIMIT.
                   target: `global <- entry points`.
  asm              a function with inline assembly (rwsets.py does not look inside it).
                   target: `inline`.
  indirect_call    a call through a function pointer or a virtual method, with the target
                   text rwsets.csv has for it. A hand check says where it can go.

data/hazards.csv columns: address, function, file, line, kind, target, verdict, note, phase,
sorted like data/rwsets.csv (address, function, file, kind, target, then line). A row is
identified by (function, kind, target, number of the row among those of the function with
the same kind and target, in line order), not by its line or phase, so a regeneration keeps
the verdict and note of a row when source lines move or its function changes phase. The candidates, lines and phases included, must match the committed
file in --check, which ignores verdict and note.
"""

import argparse
import csv
import difflib
import io
import re
import subprocess
import sys
import time
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

import rwsets
from rwsets import ROOT, Analyzer, Walker, is_pointer, record_name, strip, strip_parens, type_text

RWSETS = ROOT / "data/rwsets.csv"
EDGES = ROOT / "data/hazard_edges.csv"
HAZARDS = ROOT / "data/hazards.csv"
COLUMNS = ["address", "function", "file", "line", "kind", "target", "verdict", "note", "phase"]
PHASES = ("tick", "load")
VERDICTS = ("", "real", "false_positive")
ROOTS = ["HandleNetPackets", "UpdateAllUnits", "UpdateProjectiles", "UpdateExplosions", "UpdatePlayers",
         "UpdateFeatures", "StepAllGafSequences", "UpdateWind", "UpdateMeteors", "UpdateResourceSharing"]
LOAD_ROOTS = ["LoadMatch", "AssignStartPositions"]
THREAD_ROW_LIMIT = 400
WARNING_FLAGS = ["-Wuninitialized", "-Wsometimes-uninitialized", "-Wconditional-uninitialized"]
TIME_CALLS = {"timeGetTime", "GetTickCount", "QueryPerformanceCounter", "QueryPerformanceFrequency",
              "GetSystemTime", "GetSystemTimeAsFileTime", "GetLocalTime", "time", "clock", "localtime",
              "_ftime", "GetTicks", "GetMilliseconds", "GetTimeSeconds", "UpdateTimers", "AddTimer"}
THREAD_CALLS = {"CreateThread", "_beginthread", "_beginthreadex", "StartThread", "timeSetEvent", "SetTimer",
                "CreateTimerQueueTimer", "QueueUserAPC", "EnumSessions", "EnumPlayers", "EnumConnections",
                "EnumAddress", "EnumAddressTypes", "DirectPlayEnumerate", "DirectPlayEnumerateA",
                "DirectPlayEnumerateW", "DirectSoundEnumerate", "DirectSoundEnumerateA",
                "DirectSoundEnumerateW"}
THREAD_CALL_PREFIX = "HAPINET_enum"
ALLOCATORS = {"malloc": None, "realloc": None, "operator new": None, "operator new[]": None,
              "GameAlloc": None, "GameRealloc": None, "PoolAlloc": None,
              "GlobalAlloc": (0, 0x40), "GlobalAllocFixed": None, "LocalAlloc": (0, 0x40),
              "HeapAlloc": (1, 0x8)}
MEMSET_FAMILY = {"memset", "ZeroMemory", "FillMemory", "bzero"}
CLEARING = re.compile(r"zero|clear|wipe|memset|memfill", re.I)
WRAPPING = ("ImplicitCastExpr", "CStyleCastExpr", "ParenExpr", "CXXStaticCastExpr", "CXXReinterpretCastExpr",
            "CXXFunctionalCastExpr")
WARNING = re.compile(r"^(.+?)(?:\((\d+)(?:,\d+)?\)|:(\d+):\d+)\s*:?\s*warning: (.*) \[(-W[^\],]+)[^\]]*\]\s*$")
VARIABLE = re.compile(r"(?:variable|field) '([^']+)'")


def const_value(node: dict) -> int | None:
    """The value of an integer constant expression of literals, `|` and `+`, or None."""
    node = strip(node)
    if node["kind"] == "IntegerLiteral":
        return int(node["value"])
    if node["kind"] == "BinaryOperator" and node["opcode"] in ("|", "+"):
        a, b = (const_value(c) for c in node["inner"])
        if a is not None and b is not None:
            return a | b if node["opcode"] == "|" else a + b
    return None


def callee_name(node: dict) -> str | None:
    """The name of the function or method a call expression calls by name, else None."""
    inner = node.get("inner")
    if not inner:
        return None
    callee = strip(inner[0])
    if callee["kind"] == "DeclRefExpr" and callee["referencedDecl"]["kind"] in rwsets.FUNCTION_KINDS:
        return callee["referencedDecl"]["name"]
    if callee["kind"] == "MemberExpr":
        return callee.get("name")
    return None


def keys_in(node: dict) -> set:
    """The variables (by declaration) and fields (by name) an expression mentions."""
    found = set()
    stack = [node]
    while stack:
        n = stack.pop()
        kind = n.get("kind")
        if kind in rwsets.UNEVALUATED:
            continue
        if kind == "DeclRefExpr" and n["referencedDecl"]["kind"] == "VarDecl":
            found.add(("var", n["referencedDecl"]["id"]))
        elif kind == "MemberExpr":
            found.add(("member", n.get("name")))
        stack.extend(n.get("inner", ()))
    return found


def key_of(node: dict):
    """The variable or field an lvalue names, or None."""
    node = strip(node)
    if node["kind"] == "DeclRefExpr" and node["referencedDecl"]["kind"] == "VarDecl":
        return ("var", node["referencedDecl"]["id"])
    if node["kind"] == "MemberExpr":
        return ("member", node.get("name"))
    return None


def holder_of(node: dict, parents: list[dict]):
    """Where an allocation's result goes: a key, "return", or None when it is not stored."""
    child = node
    for parent in reversed(parents):
        kind = parent["kind"]
        if kind in WRAPPING:
            child = parent
        elif kind == "BinaryOperator" and parent["opcode"] == "=" and parent["inner"][1] is child:
            return key_of(parent["inner"][0])
        elif kind == "VarDecl":
            return ("var", parent["id"])
        elif kind == "ReturnStmt":
            return "return"
        else:
            return None
    return None


def allocation(node: dict) -> str | None:
    """The target of a row if the node is an allocation that is not zeroed or initialised, else None."""
    kind = node["kind"]
    if kind == "CXXNewExpr":
        if "initStyle" in node or any(c["kind"] == "CXXConstructExpr" and not c["ctorType"]["qualType"].endswith("noexcept")
                                      for c in node.get("inner", ())):
            return None
        pointee = record_name(node["type"])
        return "new %s%s" % (pointee, "[]" if node.get("isArray") else "")
    if kind == "CallExpr":
        name = callee_name(node)
        if name in ALLOCATORS:
            flags = ALLOCATORS[name]
            if flags:
                args = node["inner"][1:]
                value = const_value(args[flags[0]]) if len(args) > flags[0] else None
                if value is not None and value & flags[1]:
                    return None
            return name
    return None


def clears_in(node: dict) -> list[set]:
    """For each clearing call in a function body, the variables and fields its arguments mention."""
    found = []
    stack = [node]
    while stack:
        n = stack.pop()
        stack.extend(n.get("inner", ()))
        if n.get("kind") not in ("CallExpr", "CXXMemberCallExpr"):
            continue
        name = callee_name(n)
        if not name or not (name in MEMSET_FAMILY or CLEARING.search(name)):
            continue
        callee = strip(n["inner"][0])
        args = n["inner"][1:]
        if name in MEMSET_FAMILY:
            args = args[:1]
        elif callee["kind"] == "MemberExpr":
            args = args + callee.get("inner", [])
        found.append(set().union(*(keys_in(a) for a in args)) if args else set())
    return found


def scan_function(node: dict) -> list[tuple[str, str, int]]:
    """The (kind, target, line) hazards in one function definition, in source order."""
    events: list[tuple[str, str, int]] = []
    allocations: list[tuple[str, int, object]] = []
    clears = clears_in(node)
    parents: list[dict] = []

    def walk(n: dict) -> None:
        kind = n.get("kind")
        if kind is None or kind in rwsets.UNEVALUATED:
            return
        line = n.get("_line", 0)
        inner = n.get("inner", [])
        if kind == "BinaryOperator" and n["opcode"] in ("<", ">", "<=", ">=", "-") and len(inner) == 2:
            a, b = inner
            if is_pointer(a.get("type", {})) and is_pointer(b.get("type", {})) and \
                    not any(strip(x).get("kind") == "IntegerLiteral" or x.get("castKind") == "NullToPointer"
                            for x in (a, b)):
                events.append(("ptr_compare", "%s %s*" % (n["opcode"], record_name(a["type"])), line))
        elif n.get("castKind") == "PointerToIntegral":
            events.append(("ptr_to_int", "%s -> %s" % (type_text(inner[0].get("type", {})), type_text(n["type"])),
                           line))
        elif kind in ("MSAsmStmt", "GCCAsmStmt"):
            events.append(("asm", "inline", line))
        elif kind in ("CallExpr", "CXXMemberCallExpr"):
            name = callee_name(n)
            if name in TIME_CALLS and strip(n["inner"][0])["kind"] == "DeclRefExpr":
                events.append(("time_read", name, line))
            if name in THREAD_CALLS or (name or "").startswith(THREAD_CALL_PREFIX):
                for arg in n["inner"][1:]:
                    for entry in function_refs(arg):
                        events.append(("thread_entry", entry, line))
        target = allocation(n)
        if target:
            holder = holder_of(n, parents)
            if holder != "return":
                allocations.append((target, line, holder))
        parents.append(n)
        for child in inner:
            walk(child)
        parents.pop()

    for child in node.get("inner", ()):
        if child["kind"].endswith("Stmt") or child["kind"] == "CXXCtorInitializer":
            walk(child)
    for target, line, holder in allocations:
        cleared = any(holder in keys for keys in clears) if holder else bool(clears)
        if not cleared:
            events.append(("heap_uncleared", target, line))
    return events


def function_refs(node: dict) -> list[str]:
    """The functions an expression names, anywhere in it (a callback argument, cast or not)."""
    found = []
    stack = [node]
    while stack:
        n = stack.pop()
        if n.get("kind") == "DeclRefExpr" and n["referencedDecl"]["kind"] in rwsets.FUNCTION_KINDS:
            found.append(n["referencedDecl"]["name"])
        stack.extend(n.get("inner", ()))
    return found


class LineWalker(Walker):
    """The rwsets walker, remembering the line and node of each indirect call it records."""

    def __init__(self, unit: Analyzer, name: str):
        super().__init__(unit, name)
        self.line = 0
        self.node = ""
        self.indirect: dict[tuple[str, str], tuple[str, int]] = {}
        unit.last_walker = self

    def call(self, node: dict) -> None:
        saved = self.line, self.node
        self.line, self.node = node.get("_line", 0), node.get("id", "")
        try:
            super().call(node)
        finally:
            self.line, self.node = saved

    def record(self, kind: str, target: str) -> None:
        super().record(kind, target)
        if kind == "indirect_call":
            self.indirect[(self.node, target)] = (target, self.line)


class HazardAnalyzer(Analyzer):
    """The rwsets analyzer, keeping for each function its range and its hazards."""

    walker_class = LineWalker

    def function(self, node: dict, name: str) -> None:
        before = len(self.functions)
        super().function(node, name)
        if len(self.functions) == before:
            return
        fn = self.functions[-1]
        fn["end"] = node["_end"]
        events = scan_function(node)
        events += [("indirect_call", target, line) for target, line in self.last_walker.indirect.values()]
        fn["events"] = sorted(events, key=lambda e: e[2])


def scan(job: tuple[Path, list[str], str]) -> list[dict]:
    """Run clang-cl on one file and return its functions with hazards, the uninit warnings included."""
    root, argv, directory = job
    unit, stderr = rwsets.analyse_unit((root, argv, directory), HazardAnalyzer)
    by_file: dict[str, list[dict]] = defaultdict(list)
    for fn in unit.functions:
        by_file[fn["file"]].append(fn)
    for line in stderr.splitlines():
        m = WARNING.match(line)
        if not m or m.group(5) not in WARNING_FLAGS:
            continue
        path = Path(m.group(1))
        rel = unit.relative(str(path if path.is_absolute() else Path(directory) / path))
        number = int(m.group(2) or m.group(3))
        variable = VARIABLE.search(m.group(4))
        for fn in by_file.get(rel, ()):
            if fn["start"] <= number <= fn["end"]:
                target = "%s (%s)" % (variable.group(1) if variable else "?", m.group(5)[2:])
                fn["events"].append(("uninit", target, number))
    for fn in unit.functions:
        fn["events"].sort(key=lambda e: e[2])
    return [{key: fn[key] for key in ("address", "function", "file", "start", "events")}
            for fn in unit.functions]


def scan_argv(argv: list[str]) -> list[str]:
    """The AST-dump command line with the uninitialised-use warnings switched on."""
    i = argv.index("--")
    return argv[:i] + WARNING_FLAGS + argv[i:]


def scan_source(path: Path, root: Path) -> list[dict]:
    """The functions of one source file outside the build, compiled with no headers (for tests)."""
    argv = ["clang-cl", "--target=i686-pc-windows-msvc", "/nologo", "-TP", "-fms-compatibility",
            "-fms-extensions", "-fsyntax-only", "-Xclang", "-ast-dump=json", "--", str(path)]
    return scan((root, scan_argv(argv), str(root)))


class Graph:
    """The parts of data/rwsets.csv the walk and the thread check need, with the hand-resolved edges."""

    def __init__(self, rows: list[list[str]], edges: list[tuple[str, str]] = ()):
        self.calls: dict[str, set[str]] = defaultdict(set)
        self.functions: set[str] = set()
        self.reads: dict[str, set[str]] = defaultdict(set)
        self.writes: dict[str, set[str]] = defaultdict(set)
        for address, function, file, kind, target in rows:
            if kind == "function":
                self.functions.add(function)
            if kind == "call":
                self.calls[function].add(target)
            elif kind == "global_read":
                self.reads[function].add(target)
            elif kind in ("global_write", "global_maybe_written"):
                self.writes[function].add(target)
        for caller, target in edges:
            self.calls[caller].add(target)

    def reach(self, roots) -> set[str]:
        """Every function named by a `call` row on a path from the roots, the roots included."""
        seen = set(roots)
        todo = list(seen)
        while todo:
            for callee in self.calls.get(todo.pop(), ()):
                if callee not in seen:
                    seen.add(callee)
                    todo.append(callee)
        return seen


def read_edges(path: Path, functions: set[str]) -> list[tuple[str, str]]:
    """The (caller, target) pairs of data/hazard_edges.csv; fail on a name that is not a function."""
    if not path.exists():
        return []
    with path.open(newline="") as f:
        rows = list(csv.reader(f))
    if rows[0] != ["caller", "target", "reason"]:
        sys.exit("hazards: %s must have the columns caller,target,reason" % path.name)
    bad = [r for r in rows[1:] if len(r) != 3 or r[0] not in functions or r[1] not in functions]
    for r in bad:
        print("hazards: %s: %r is not a caller and a target that are both functions of data/rwsets.csv" % (path.name, r))
    if bad:
        sys.exit(1)
    return [(r[0], r[1]) for r in rows[1:]]


def read_rwsets(text: str) -> list[list[str]]:
    """The rows of data/rwsets.csv, without the header."""
    return list(csv.reader(io.StringIO(text)))[1:]


def load_graph() -> tuple[list[list[str]], Graph]:
    """The rows of data/rwsets.csv and the call graph with the hand-resolved edges of data/hazard_edges.csv."""
    rows = read_rwsets(RWSETS.read_text())
    return rows, Graph(rows, read_edges(EDGES, {r[1] for r in rows if r[3] == "function"}))


def candidates(functions: list[dict], graph: Graph, roots=ROOTS, load_roots=LOAD_ROOTS) -> list[list]:
    """The hazard rows [address, function, file, line, kind, target, phase] for the game logic of the graph."""
    ticking = graph.reach(roots)
    logic = ticking | graph.reach(load_roots)

    def phase(name: str) -> str:
        return "tick" if name in ticking else "load"

    entries = defaultdict(set)
    for fn in functions:
        for kind, target, line in fn["events"]:
            if kind == "thread_entry":
                entries[target].add(fn["function"])
    entry_names = set(entries)
    writers: dict[str, set[str]] = defaultdict(set)
    for entry in entry_names:
        for name in graph.reach([entry]):
            for glob in graph.writes.get(name, ()):
                writers[glob].add(entry)
    rows = []
    for fn in functions:
        if fn["function"] not in logic:
            continue
        head = [fn["address"], fn["function"], fn["file"]]
        for kind, target, line in fn["events"]:
            if kind != "thread_entry":
                rows.append(head + [line, kind, target, phase(fn["function"])])
    threaded = defaultdict(list)
    for fn in functions:
        if fn["function"] in logic:
            for glob in graph.reads.get(fn["function"], ()):
                if glob in writers:
                    threaded[glob].append(fn)
    if sum(len(v) for v in threaded.values()) <= THREAD_ROW_LIMIT:
        for glob, readers in threaded.items():
            for fn in readers:
                rows.append([fn["address"], fn["function"], fn["file"], fn["start"], "thread_global",
                             "%s <- %s" % (glob, "+".join(sorted(writers[glob]))), phase(fn["function"])])
    else:
        for glob, readers in threaded.items():
            fn = min(readers, key=lambda f: (f["address"] == "", f["address"], f["function"]))
            rows.append([fn["address"], fn["function"], fn["file"], fn["start"], "thread_global",
                         "%s <- %s" % (glob, "+".join(sorted(writers[glob]))), phase(fn["function"])])
    return [list(r) for r in sorted(set(map(tuple, rows)), key=order)]


def order(row) -> tuple:
    """The sort key of a row: like rwsets.csv, then the line."""
    address, function, file, line, kind, target = row[:6]
    return (0, int(address, 16)) if address else (1, 0), function, file, kind, target, int(line)


def occurrences(rows) -> list[tuple]:
    """For each row (in order), its identity (function, kind, target, occurrence number)."""
    count: dict[tuple, int] = defaultdict(int)
    keys = []
    for row in sorted(rows, key=order):
        base = (row[1], row[4], row[5])
        keys.append(base + (count[base],))
        count[base] += 1
    return keys


def merge_verdicts(rows: list[list], old: list[list[str]]) -> list[list]:
    """The rows with the verdict and note the old file had for the same (function, kind, target, n)."""
    kept = {key: (row[6], row[7]) for key, row in zip(occurrences(old), sorted(old, key=order))}
    ordered = sorted(rows, key=order)
    return [list(row[:6]) + list(kept.get(key, ("", ""))) + [row[6]]
            for key, row in zip(occurrences(ordered), ordered)]


def render(rows: list[list]) -> str:
    out = io.StringIO()
    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(COLUMNS)
    writer.writerows(rows)
    return out.getvalue()


def read_hazards(path: Path = HAZARDS) -> list[list]:
    """The rows of data/hazards.csv with the line as a number, or none when it does not exist."""
    if not path.exists():
        return []
    with path.open(newline="") as f:
        rows = list(csv.reader(f))[1:]
    return [r[:3] + [int(r[3])] + r[4:8] + (r[8:9] or [""]) for r in rows]


def generate(build_dir: Path, jobs: int) -> tuple[list[list], int]:
    """The candidate rows for the whole of src/, and the number of files compiled."""
    commands = rwsets.compile_commands(build_dir, [])
    with ProcessPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(scan, [(ROOT, scan_argv(argv), directory) for _, argv, directory in commands],
                                chunksize=4))
    functions = {}
    for result in results:
        for fn in result:
            functions.setdefault((fn["function"], fn["file"], fn["start"]), fn)
    graph = load_graph()[1]
    return candidates(list(functions.values()), graph), len(commands)


def key_lines(rows: list[list], phase_at: int) -> list[str]:
    """The candidate part of each row (verdict and note left out) as text, in order; `phase_at` is the phase column."""
    return [",".join(map(str, r[:6] + [r[phase_at]])) for r in sorted(rows, key=order)]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if the candidates differ from data/hazards.csv")
    parser.add_argument("--unchecked", action="store_true", help="fail if a row has no verdict")
    parser.add_argument("--build-dir", type=Path, default=rwsets.DEFAULT_BUILD_DIR)
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    sys.setrecursionlimit(100000)
    old = read_hazards()
    if args.unchecked:
        bad = [r for r in old if r[6] not in VERDICTS]
        empty = [r for r in old if r[6] == ""]
        for r in bad:
            print("bad verdict %r: %s %s %s" % (r[6], r[1], r[4], r[5]))
        for r in empty[:20]:
            print("no verdict: %s:%s %s %s %s" % (r[2], r[3], r[1], r[4], r[5]))
        print("hazards: %d of %d rows have no verdict" % (len(empty), len(old)))
        sys.exit(1 if empty or bad else 0)
    started = time.time()
    rows, files = generate(args.build_dir, args.jobs)
    elapsed = time.time() - started
    if args.check:
        have, want = key_lines(old, 8), key_lines(rows, 6)
        if have != want:
            diff = list(difflib.unified_diff(have, want, "data/hazards.csv", "regenerated", lineterm="", n=0))
            print("\n".join(diff[:40]))
            if len(diff) > 40:
                print("... %d more lines" % (len(diff) - 40))
            print("hazards: data/hazards.csv is out of date; run `uv run tools/hazards.py` and commit the result")
            sys.exit(1)
        print("hazards: up to date (%d files, %d rows, %.0fs)" % (files, len(rows), elapsed))
        return
    merged = merge_verdicts(rows, old)
    now = set(occurrences(merged))
    lost = [r for key, r in zip(occurrences(old), sorted(old, key=order)) if (r[6] or r[7]) and key not in now]
    HAZARDS.write_text(render(merged))
    print("%d rows, %d files, %.0fs" % (len(merged), files, elapsed))
    if lost:
        print("warning: %d rows with a verdict or note no longer match a candidate and were dropped" % len(lost))
        for r in lost[:20]:
            print("  %s %s %s %s: %s %s" % (r[2], r[1], r[4], r[5], r[6], r[7]))


if __name__ == "__main__":
    main()

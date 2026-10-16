"""List the C runtime and float operations the game's own functions use.

    uv run tools/crtcalls.py            # writes data/crt_calls.csv, prints totals
    uv run tools/crtcalls.py --quiet    # only write the file
    uv run tools/crtcalls.py --check    # fail if data/game_logic.csv is out of date

Also writes data/game_logic.csv (address,function,file, sorted by address):
every function classed `game` below, which tools/determinismlint.py reads in
CI, where there is no toolchain. --check regenerates it in memory, writes
nothing and exits 1 when the committed file differs.

Reads the matching build's objects (every game and gap source under src/,
compiled and cached the way tools/progress.py does), not the source text, so
compiler helpers such as _ftol, _allmul, _alldiv and _chkstk show up. For
every annotated function it records:

  call    a relocation that resolves to a library function (kind `library` in
          data/functions.csv, or the compiler helper _chkstk), counted once
          per caller and callee; Win32 and DirectX stubs (`_Name@N`) are left out
  inline  a decoded instruction that converts or computes in the FPU: fist,
          fistp, fsqrt, fsin, fcos, fptan, fpatan, fyl2x, fyl2xp1, f2xm1,
          frndint, fscale, fprem, fprem1, fxtract and the control word
          instructions (fldcw, fnstcw)

Code is decoded by recursive descent from the entry, following jumps and
switch tables, so data placed in the code section is not read as code.

Each caller is classed `game` when it is reachable from the eight game-logic
phase calls of RunGameSteps, from LoadMatch, or from a function in
EXTRA_ROOTS (game-logic functions that only frame handlers call), and `presentation` otherwise.
Edges are the direct calls and jumps, every function whose address a function
takes, and for a data symbol that a function references (a vtable, an order
handler table), every function pointer in the data up to the next symbol, read
from orig/TotalA.exe. A callee no object defines by name is read from the
original at the same offset of the same function. Indirect calls that none of
this resolves are counted in the totals and are a limitation.

InitCommands (called by LoadMatch) registers the console and cheat command
tables, which pull a lot in; what only they reach is marked in the notes.

Columns of data/crt_calls.csv: address,function,callee,kind,file,class,notes,
sorted by address, then callee. The notes column holds the number of sites
(`sites=N`), the operand width of an inline conversion, `address taken` for a
library function whose address is taken, `via console commands only`, and a
`conflict:` note where the class of a `rand` site disagrees with the call-site
tables of #52 (BuildExplosionFrame, which #52 keeps on rand although it is
presentation, was listed as one until the maintainer ruled it game logic;
MAINTAINER_RAND records that). A `srand` row can be classed game by hand the same way (MAINTAINER_SRAND: GameMain seeds the main thread's game stream). Callers of HarnessViewRand that game logic
reaches are only printed.
"""

import argparse
import bisect
import collections
import csv
import io
import re
import struct
import sys
from pathlib import Path
from types import SimpleNamespace

import capstone

from check import Original, base_name, load_symbols
from coff import REL_I386_DIR32, REL_I386_REL32, parse_object
from check import select_function
from progress import compile_cached
from sources import ROOT, annotations, is_gap_source, sources

OUT = ROOT / "data/crt_calls.csv"
GAME_LOGIC = ROOT / "data/game_logic.csv"
FUNCTIONS = ROOT / "data/functions.csv"
PHASES = ("HandleNetPackets", "UpdateAllUnits", "UpdateProjectiles", "UpdateExplosions",
          "UpdatePlayers", "UpdateFeatures", "UpdateWind", "UpdateMeteors")
LOAD_ROOT = "LoadMatch"
EXTRA_ROOTS: dict[int, str] = {
    0x4568c0: "sets start positions; called from MainLoopTick and LoadingScreenFrame, which aren't roots",
}
COMPILER_HELPERS = {"__chkstk"}
STACK_PROBE = "_chkstk"
WIN32_STUB = re.compile(r"^_\w+@\d+$")
FPU_INLINE = {"fist", "fistp", "fsqrt", "fsin", "fcos", "fptan", "fpatan", "fsincos", "fyl2x", "fyl2xp1",
              "f2xm1", "frndint", "fscale", "fprem", "fprem1", "fxtract", "fldcw", "fnstcw", "fstcw"}
IMMEDIATE = re.compile(r"^(0x[0-9a-f]+|\d+)$")
WIDTH = {"word": "16-bit", "dword": "32-bit", "qword": "64-bit"}

PRESENTATION_RAND = {
    0x4737c0, 0x474880, 0x474b00, 0x474df0, 0x474fc0, 0x4751c0, 0x475340, 0x475600, 0x472ab0,
    0x473d50, 0x41c6f0, 0x47fd70, 0x4cdb40, 0x425b80, 0x478790, 0x478e80, 0x47bd70, 0x418310,
    0x450380, 0x44fc10,
}
MAINTAINER_RAND: dict[int, str] = {
    0x420d20: "#52 lists it as presentation; the maintainer later decided on #52 and #72 that it is game logic on rand",
}
MAINTAINER_SRAND: dict[int, str] = {
    0x49e830: "GameMain seeds the main thread's game rand stream, so the srand is game logic although only presentation reaches it",
}
GAME_RAND = {0x490c40, 0x437d60, 0x437de0, 0x438070, 0x423c50, 0x49be60, 0x497180, 0x4568c0, 0x490360} | set(
    MAINTAINER_RAND)
INLINED_RAND = {0x424050: "inlined Rand, which #52 lists under 0x423c50"}


def load_functions() -> tuple[dict[int, dict], list[int]]:
    with FUNCTIONS.open() as fh:
        funcs = {int(r["address"], 16): r for r in csv.DictReader(fh)}
    return funcs, sorted(funcs)


def containing(addr: int, funcs: dict, starts: list[int]) -> int | None:
    i = bisect.bisect_right(starts, addr) - 1
    if i < 0:
        return None
    start = starts[i]
    return start if addr < start + int(funcs[start]["size"]) else None


def decode_function(sec, lo: int, hi: int, labels: dict[str, int], linear: bool = False):
    """Recursive-descent decode of sec.data[lo:hi].

    A gap function is hand-written assembly with several entry points, so
    with linear set the decode goes on past a return.

    Returns (instructions, covered offsets, the indirect calls and jumps).
    """
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.syntax = capstone.CS_OPT_SYNTAX_INTEL
    code = sec.data
    reloc_at = {r.offset: r for r in sec.relocs if lo <= r.offset < hi}
    todo = [lo]
    todo += [labels[r.symbol] for r in reloc_at.values() if r.symbol in labels and lo <= labels[r.symbol] < hi]
    seen: set[int] = set()
    insns = []
    covered: set[int] = set()
    indirect: list[str] = []
    while todo:
        off = todo.pop()
        while lo <= off < hi and off not in seen:
            got = list(md.disasm(code[off:min(hi, off + 15)], off, count=1))
            if not got:
                if linear:
                    off += 1
                    continue
                break
            ins = got[0]
            seen.add(off)
            insns.append(ins)
            covered.update(range(off, off + ins.size))
            mnem = ins.mnemonic
            if mnem.startswith("j") or mnem == "call":
                if IMMEDIATE.match(ins.op_str):
                    target = int(ins.op_str, 0)
                    if mnem != "call" and lo <= target < hi:
                        todo.append(target)
                else:
                    syms = [reloc_at[o].symbol for o in range(off, off + ins.size) if o in reloc_at]
                    if not any(x in labels or x.startswith("__imp__") for x in syms):
                        indirect.append("memory" if "[" in ins.op_str else "register")
            if mnem in ("ret", "jmp", "int3", "ud2", "hlt") and not linear:
                break
            off += ins.size
    return insns, covered, indirect


def original_target(orig: Original, function: int, offset: int) -> int | None:
    """The target of the call or jump whose rel32 field is at offset in the
    original's copy of a function, or None if the bytes there are no call."""
    raw = orig.read(function + offset - 1, 5)
    if len(raw) < 5 or raw[0] not in (0xE8, 0xE9):
        return None
    (rel,) = struct.unpack_from("<i", raw, 1)
    return (function + offset + 4 + rel) & 0xFFFFFFFF


def table_targets(orig: Original, addr: int, nxt: int, funcs) -> list[int]:
    """The function pointers in the data at addr, up to the next symbol (at most 16 KiB)."""
    size = min(nxt, addr + 0x4000) - addr
    raw = orig.read(addr, size - size % 4)
    return [v for (v,) in struct.iter_unpack("<I", raw) if v in funcs]


def analyze() -> SimpleNamespace:
    """Decode every game and gap function and find which ones game logic reaches.

    Returns a namespace: infos (address to function info), game_reach (the
    game-logic addresses), core, parent, rows, edges, by_addr, by_name, funcs
    and the counters unresolved, from_original and uncovered.
    """
    funcs, starts = load_functions()
    symbols = load_symbols()
    by_addr: dict[int, str] = {}
    for name, a in sorted(symbols.items(), key=lambda kv: (kv[1], kv[0])):
        by_addr.setdefault(a, name)
    sym_addrs = sorted(set(symbols.values()))
    orig = Original()

    infos: dict[int, dict] = {}
    defined: dict[str, int] = {}
    for src in sources("game", "gap"):
        obj_path, log = compile_cached(src)
        if not obj_path:
            sys.exit(f"{src}: does not compile\n{log}")
        obj = parse_object(obj_path.read_bytes(), obj_path.name)
        for address, qualname in annotations(src):
            picked, err = select_function(obj, None, qualname)
            if not picked:
                sys.exit(f"{src} {address:#x}: {err}")
            name, sec, lo, hi = picked
            labels = {s.name: s.value for s in obj.symbols if s.section == sec.index and s.name.startswith("$L")}
            infos[address] = {"name": name, "sec": sec, "lo": lo, "hi": hi, "labels": labels,
                              "file": src.relative_to(ROOT).as_posix(), "qual": qualname,
                              "gap": is_gap_source(src)}
            defined.setdefault(name, address)

    by_base: dict[str, set[int]] = collections.defaultdict(set)
    for name, a in defined.items():
        by_base[base_name(name)].add(a)

    rows: dict[tuple[int, str, str], dict] = {}
    edges: dict[int, set[int]] = collections.defaultdict(set)
    table_cache: dict[int, list[int]] = {}
    unresolved: collections.Counter = collections.Counter()
    from_original: collections.Counter = collections.Counter()
    uncovered = 0

    def library_name(addr: int, fn: int) -> str:
        """The readable name of a library function; an entry inside a larger
        library block (_allmul inside _ftol's block) keeps its own."""
        raw = re.sub(r"^zlib [\d.]+: ", "", funcs[fn]["name"])
        name = by_addr.get(addr) or by_addr.get(fn) or (base_name(raw) if raw else f"FUN_{fn:08x}")
        m = re.match(r"\?(\w+)@\?\$(\w+)@", funcs[fn]["name"])
        if m and name.startswith("std::"):
            name = f"std::{m.group(2)}::{m.group(1)}"
        return STACK_PROBE if name in ("_alloca_probe", "_chkstk") else name

    def add_row(addr, callee, kind, note=""):
        row = rows.setdefault((addr, callee, kind), {"sites": 0, "notes": set()})
        row["sites"] += 1
        if note:
            row["notes"].add(note)

    for address, info in sorted(infos.items()):
        sec, lo, hi = info["sec"], info["lo"], info["hi"]
        insns, covered, indirect = decode_function(sec, lo, hi, info["labels"], info["gap"])
        info["indirect"] = indirect
        for ins in insns:
            if ins.mnemonic in FPU_INLINE:
                width = next((WIDTH[w] for w in WIDTH if ins.op_str.startswith(w)), "")
                add_row(address, ins.mnemonic, "inline", width)
        for r in sec.relocs:
            if not (lo <= r.offset < hi):
                continue
            if r.offset not in covered:
                if r.symbol not in info["labels"]:
                    uncovered += 1
                continue
            sym = r.symbol
            is_ref = r.type == REL_I386_DIR32
            if sym.startswith(("__imp__", "_$", "$")) or sym.startswith("??_C@"):
                continue
            target = defined.get(sym)
            if target is not None:
                edges[address].add(target)
                continue
            if WIN32_STUB.match(sym):
                continue
            addr = symbols.get(base_name(sym))
            if addr is None:
                if sym in COMPILER_HELPERS:
                    add_row(address, STACK_PROBE, "call")
                elif by_base.get(base_name(sym)):
                    edges[address].update(by_base[base_name(sym)])
                elif r.type == REL_I386_REL32 and not sym.startswith(("?DAT_", "?Harness")):
                    at = original_target(orig, address, r.offset - lo)
                    fn = containing(at, funcs, starts) if at else None
                    if fn is not None and funcs[fn]["kind"] in ("game", "gap") and fn in infos:
                        edges[address].add(fn)
                        from_original[sym] += 1
                    elif fn is not None and funcs[fn]["kind"] == "library":
                        add_row(address, library_name(at, fn), "call")
                        from_original[sym] += 1
                    else:
                        unresolved[sym] += 1
                continue
            fn = containing(addr, funcs, starts)
            if fn is None:
                continue
            kind = funcs[fn]["kind"]
            if kind in ("game", "gap"):
                if fn in infos:
                    edges[address].add(fn)
                else:
                    unresolved[sym] += 1
            elif kind == "library":
                add_row(address, library_name(addr, fn), "call", "address taken" if is_ref else "")
        for r in sec.relocs:
            if not (lo <= r.offset < hi) or r.offset not in covered or r.type != REL_I386_DIR32:
                continue
            addr = symbols.get(base_name(r.symbol))
            if addr is None or containing(addr, funcs, starts) is not None or addr in funcs:
                continue
            if addr not in table_cache:
                i = bisect.bisect_right(sym_addrs, addr)
                nxt = sym_addrs[i] if i < len(sym_addrs) else addr + 4096
                table_cache[addr] = table_targets(orig, addr, nxt, funcs)
            for t in table_cache[addr]:
                if t in infos:
                    edges[address].add(t)
                else:
                    fn = containing(t, funcs, starts)
                    if fn is not None and funcs[fn]["kind"] == "library":
                        add_row(address, library_name(t, fn), "call", "address taken")

    by_name = collections.defaultdict(list)
    for a, info in infos.items():
        by_name[info["qual"]].append(a)
    for n in PHASES + (LOAD_ROOT, "InitCommands"):
        if len(by_name[n]) != 1:
            sys.exit(f"cannot find exactly one function named {n}: {by_name[n]}")
    for a in EXTRA_ROOTS:
        if a not in infos:
            sys.exit(f"extra root {a:#x} is not a decoded game function")
    roots = [by_name[n][0] for n in PHASES + (LOAD_ROOT,)] + sorted(EXTRA_ROOTS)
    parent: dict[int, int | None] = {a: None for a in roots}
    queue = collections.deque(roots)
    while queue:
        a = queue.popleft()
        for b in sorted(edges.get(a, ())):
            if b not in parent:
                parent[b] = a
                queue.append(b)
    game_reach = set(parent)
    console_root = by_name["InitCommands"][0]
    core: set[int] = set(roots)
    queue = collections.deque(roots)
    while queue:
        a = queue.popleft()
        for b in edges.get(a, ()) if a != console_root else ():
            if b not in core:
                core.add(b)
                queue.append(b)
    return SimpleNamespace(infos=infos, game_reach=game_reach, core=core, parent=parent, rows=rows,
                           edges=edges, by_addr=by_addr, by_name=by_name, funcs=funcs,
                           unresolved=unresolved, from_original=from_original, uncovered=uncovered)


def game_logic_csv(g: SimpleNamespace) -> str:
    """The text of data/game_logic.csv for an analysis."""
    out = io.StringIO()
    w = csv.writer(out, lineterminator="\n")
    w.writerow(["address", "function", "file"])
    for address in sorted(g.game_reach):
        info = g.infos[address]
        w.writerow([f"{address:#x}", g.by_addr.get(address) or base_name(info["name"]), info["file"]])
    return out.getvalue()


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--quiet", action="store_true", help="only write the file")
    ap.add_argument("--check", action="store_true",
                    help="write nothing; exit 1 if data/game_logic.csv is out of date")
    ap.add_argument("--why", action="append", default=[], metavar="ADDR",
                    help="print the shortest chain from a game-logic root to a function")
    args = ap.parse_args()

    g = analyze()
    if args.check:
        fresh = game_logic_csv(g)
        if not GAME_LOGIC.exists() or GAME_LOGIC.read_text() != fresh:
            print(f"{GAME_LOGIC.relative_to(ROOT)} is out of date: run uv run tools/crtcalls.py")
            sys.exit(1)
        print(f"{GAME_LOGIC.relative_to(ROOT)} is up to date")
        return
    GAME_LOGIC.write_text(game_logic_csv(g), newline="")
    infos, rows, parent, game_reach, core = g.infos, g.rows, g.parent, g.game_reach, g.core
    by_addr, unresolved, from_original, uncovered = g.by_addr, g.unresolved, g.from_original, g.uncovered
    for want in args.why:
        a = int(want, 16)
        chain = []
        while a in parent:
            chain.append(a)
            a = parent[a]
            if a is None:
                break
        print(" <- ".join(f"{x:#x} {by_addr.get(x, '')}" for x in chain) or f"{want}: not reachable")

    view_callers: collections.Counter = collections.Counter()
    for address, info in infos.items():
        for r in info["sec"].relocs:
            if info["lo"] <= r.offset < info["hi"] and base_name(r.symbol) == "HarnessViewRand":
                view_callers[address] += 1
    conflicts: list[str] = []
    out_rows = []
    for (address, callee, kind), data in rows.items():
        info = infos[address]
        cls = "game" if address in game_reach else "presentation"
        notes = [f"sites={data['sites']}"] + sorted(data["notes"])
        if address in game_reach and address not in core:
            notes.append("via console commands only")
        if callee == "srand" and address in MAINTAINER_SRAND:
            cls = "game"
            notes.append(MAINTAINER_SRAND[address])
        if callee == "rand":
            msg = None
            if cls == "presentation":
                if address in GAME_RAND:
                    msg = "#52 keeps it in game logic, but only presentation reaches it"
                else:
                    msg = "presentation by reachability and not in the tables of #52"
            elif address in INLINED_RAND:
                notes.append(INLINED_RAND[address])
            elif address not in GAME_RAND:
                msg = "game logic by reachability and not in the tables of #52"
            if msg:
                notes.append("conflict: " + msg)
                conflicts.append(f"{address:#x} {info['file']}: {msg}")
        fname = by_addr.get(address) or base_name(info["name"])
        out_rows.append({"address": f"{address:#x}", "function": fname, "callee": callee, "kind": kind,
                         "file": info["file"], "class": cls, "notes": "; ".join(notes), "_a": address})
    view_in_game = sorted(a for a in view_callers if a in game_reach)
    out_rows.sort(key=lambda r: (r["_a"], r["callee"], r["kind"]))
    with OUT.open("w", newline="") as fh:
        w = csv.DictWriter(fh, ["address", "function", "callee", "kind", "file", "class", "notes"],
                           lineterminator="\n", extrasaction="ignore")
        w.writeheader()
        w.writerows(out_rows)

    if args.quiet:
        return
    game_rows = [r for r in out_rows if r["class"] == "game"]
    print(f"{len(infos)} functions, {len(game_reach)} reachable as game logic, "
          f"{len(infos) - len(game_reach)} presentation")
    print(f"game-logic functions reached only through the console command tables: "
          f"{len(game_reach - core)} of {len(game_reach)}")
    print(f"rows: {len(out_rows)} ({len(game_rows)} game, {len(out_rows) - len(game_rows)} presentation)")
    per = collections.Counter(r["callee"] for r in game_rows)
    print("game-logic functions per callee:")
    for callee, n in sorted(per.items()):
        print(f"  {callee}: {n}")
    sites = collections.Counter()
    for r in game_rows:
        sites[r["callee"]] += int(re.search(r"sites=(\d+)", r["notes"]).group(1))
    print("game-logic sites per callee:", dict(sorted(sites.items())))
    seen_kinds = collections.Counter(k for a, i in infos.items() if a in game_reach for k in i["indirect"])
    print("indirect calls and jumps in game-logic functions (not through switch tables): "
          + ", ".join(f"{k} {v}" for k, v in sorted(seen_kinds.items())))
    print(f"call targets taken from the original's bytes: {sum(from_original.values())}")
    print(f"unresolved call relocations: {sum(unresolved.values())} ({len(unresolved)} symbols)")
    print(f"relocations outside decoded code: {uncovered}")
    print(f"HarnessViewRand callers reached from game logic: {len(view_in_game)}: "
          + ", ".join(f"{a:#x}" for a in view_in_game))
    print("unresolved symbols:", ", ".join(f"{k} x{v}" for k, v in sorted(unresolved.items())))
    print(f"rand conflicts: {len(conflicts)}")
    for c in conflicts:
        print("  " + c)


if __name__ == "__main__":
    main()

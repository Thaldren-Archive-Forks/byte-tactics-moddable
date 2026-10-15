"""Find where two builds' game logic first differs, on one reference scenario.

    uv run tools/tracediff.py A/TotalA.exe B/TotalA.exe --scenario reference/scenarios/skirmish2.txt
    uv run tools/tracediff.py --traces a/trace.txt b/trace.txt
    uv run tools/tracediff.py --dumps a/dump.txt b/dump.txt --at 900 units

Runs both exes on the scenario under Wine (tools/playtest.py --testmode), compares
the trace.txt each writes and reports the first tick and phase whose hashes differ.
It then runs both again with `--testdump <tick>` and prints every field of that
phase that differs, with its value in A and in B. Set WINEARCH=win64 for Wine.

    first divergence: tick 900, phase units
      units[12].health: a=0x0064 b=0x0063

`--traces` compares existing trace files without running, and reports the first
divergence only. `--dumps` diffs the phase of two existing dump files.

Exit status: 0 if the traces are identical, 1 if they differ, 2 on a usage
error, a failed run or traces that cannot be compared (different seeds or
scenario).
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PHASES = ("net", "units", "projectiles", "explosions", "players", "features", "wind", "meteors")
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
            if words != ["bt-trace", "1"]:
                raise Error(f"{path}: not a bt-trace 1 file")
        elif len(words) == 3 and words[0].isdigit() and words[1] in PHASES:
            rows.append((int(words[0]), words[1], words[2]))
        elif rows or not words:
            raise Error(f"{path}:{n}: unexpected line")
        else:
            header[words[0]] = line[len(words[0]):].strip()
    return header, rows


def check_headers(a: dict[str, str], b: dict[str, str]) -> None:
    bad = [f"{k}: a={a.get(k)} b={b.get(k)}" for k in COMPARED_HEADERS if a.get(k) != b.get(k)]
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
            if line.split() != ["bt-dump", "1"]:
                raise Error(f"{path}: not a bt-dump 1 file")
            continue
        m = re.fullmatch(r"(\d+) (\w+) (\S+) (.*)", line)
        if not m:
            raise Error(f"{path}:{n}: unexpected line")
        out.setdefault((int(m[1]), m[2]), {})[m[3]] = m[4]
    return out


def diff_dumps(a: Path, b: Path, tick: int, phase: str) -> list[tuple[str, str, str]]:
    """The (path, a value, b value) of each field of that phase that differs; "-" if absent."""
    fa = parse_dump(a).get((tick, phase), {})
    fb = parse_dump(b).get((tick, phase), {})
    if not fa and not fb:
        raise Error(f"no tick {tick} phase {phase} in either dump")
    return [(k, fa.get(k, "-"), fb.get(k, "-")) for k in [*fa, *(k for k in fb if k not in fa)]
            if fa.get(k) != fb.get(k)]


def print_diff(diffs: list[tuple[str, str, str]]) -> None:
    for path, va, vb in diffs:
        print(f"  {path}: a={va} b={vb}")
    if not diffs:
        print("  no field differs in the dumps")


def play(exe: Path, scenario: Path, game_dir: str | None, dump: int | None) -> Path:
    """Run the exe on the scenario with playtest.py; the run's directory."""
    cmd = [sys.executable, str(ROOT / "tools/playtest.py"), "--exe", str(exe), "--testmode", str(scenario)]
    if game_dir:
        cmd += ["--game-dir", game_dir]
    if dump is not None:
        cmd += ["--testdump", str(dump)]
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
    scenario = Path(args.scenario).resolve()
    runs = [play(e, scenario, args.game_dir, None) for e in exes]
    (ha, ra), (hb, rb) = (parse_trace(need(r, "trace.txt")) for r in runs)
    check_headers(ha, hb)
    div = first_divergence(ra, rb)
    if not div:
        print(f"traces identical ({len(ra)} lines)")
        return 0
    tick, phase = div
    print(f"first divergence: tick {tick}, phase {phase}")
    runs = [play(e, scenario, args.game_dir, tick) for e in exes]
    print_diff(diff_dumps(need(runs[0], "dump.txt"), need(runs[1], "dump.txt"), tick, phase))
    return 1


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("exe", nargs="*", help="the two exes, A then B")
    ap.add_argument("--scenario", metavar="FILE", help="a reference/ scenario file")
    ap.add_argument("--game-dir", help="the game's install (default: Steam's)")
    ap.add_argument("--traces", nargs=2, metavar=("A", "B"), help="compare two trace.txt files, without running")
    ap.add_argument("--dumps", nargs=2, metavar=("A", "B"), help="diff two dump.txt files, with --at")
    ap.add_argument("--at", nargs=2, metavar=("TICK", "PHASE"), help="--dumps: the tick and phase to diff")
    args = ap.parse_args()
    modes = [bool(args.traces), bool(args.dumps), bool(args.exe)]
    if sum(modes) != 1:
        ap.error("give two exes with --scenario, or --traces A B, or --dumps A B --at TICK PHASE")
    if args.exe and (len(args.exe) != 2 or not args.scenario):
        ap.error("two exes and --scenario are required")
    if args.exe and not Path(args.scenario).is_file():
        ap.error(f"{args.scenario} is not a file")
    if args.dumps and (not args.at or not args.at[0].isdigit() or args.at[1] not in PHASES):
        ap.error(f"--dumps needs --at TICK PHASE, phase one of {' '.join(PHASES)}")
    try:
        if args.traces:
            (ha, ra), (hb, rb) = (parse_trace(Path(t)) for t in args.traces)
            check_headers(ha, hb)
            div = first_divergence(ra, rb)
            if not div:
                print(f"traces identical ({len(ra)} lines)")
                return 0
            print(f"first divergence: tick {div[0]}, phase {div[1]}")
            return 1
        if args.dumps:
            diffs = diff_dumps(Path(args.dumps[0]), Path(args.dumps[1]), int(args.at[0]), args.at[1])
            print(f"tick {args.at[0]}, phase {args.at[1]}")
            print_diff(diffs)
            return 1 if diffs else 0
        return compare(args)
    except (Error, OSError) as e:
        print(f"tracediff: {e}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())

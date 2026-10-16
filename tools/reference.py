"""Build a test-mode exe from any commit of this repository.

    uv run tools/reference.py HEAD
    uv run tools/reference.py a1b2c3d
    uv run tools/reference.py HEAD --traces     # write reference/traces/
    uv run tools/reference.py HEAD --check      # compare against reference/traces/
    uv run tools/reference.py HEAD --check --quick    # a fixed subset, not the gate
    uv run tools/reference.py --check --exe build/x/TotalA.exe

Checks the commit out in build/reference/<ref>, links it with its own tools
(the conformity harness is part of its src/), and leaves TotalA.exe,
SMACKW32.DLL and commit.txt there. It then runs checkall over every annotated
function and reports which ones no longer MATCH, against the allowlist in the
commit's own reference/changed.txt. Exits 1 if the build fails or a function
outside the allowlist does not MATCH.

`--traces` then plays every scenario in reference/scenarios/ (`--parallel N` at a
time, default half the CPUs and at most 6, under Wine through tools/playtest.py) and writes reference/traces/<scenario>.txt:
the header of the run's trace.txt and one digest per tick (tools/tracedigest.py).
`--check` plays the same scenarios and compares the digests with the stored ones.
On the first difference it builds the reference from the commit the trace
records, prints tools/tracediff.py's report for that scenario and exits 1. A
missing or extra trace or scenario exits 1. `--exe` checks a given exe instead
of building the ref. `--quick` restricts `--check` to the scenarios named in QUICK.
Set WINEARCH=win64 for Wine; `--game-dir` names the game's
install.
"""

import argparse
import csv
import os
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import playtest
import tracediff
import tracedigest

ROOT = Path(__file__).resolve().parent.parent
OUTPUTS = ("TotalA.exe", "SMACKW32.DLL", "commit.txt")
SCENARIOS = ROOT / "reference/scenarios"
TRACES = ROOT / "reference/traces"
MAX_PARALLEL = 6
QUICK = ("eight-full-moon", "eight-greenhaven", "duel-coast-to-coast", "duel-great-divide", "orders-move-attack")
RUN_SECONDS_BASE = 120
RUN_TICKS_PER_SECOND = 40
RUN_ATTEMPTS = 2
REF_ENV = {k: v for k, v in os.environ.items() if k != "VIRTUAL_ENV"}


def git(*args: str, cwd: Path = ROOT, check: bool = True, capture: bool = False) -> subprocess.CompletedProcess:
    return subprocess.run(["git", "-C", str(cwd), *args], check=check, text=True,
                          capture_output=capture)


def fail(message: str) -> None:
    sys.exit(f"reference: {message}")


def resolve(ref: str) -> str:
    """The commit a ref names."""
    proc = git("rev-parse", "--verify", "-q", f"{ref}^{{commit}}", check=False, capture=True)
    if proc.returncode:
        fail(f"{ref} names no commit")
    return proc.stdout.strip()


def registered_worktrees() -> set[Path]:
    out = git("worktree", "list", "--porcelain", capture=True).stdout
    return {Path(line[len("worktree "):]).resolve() for line in out.splitlines() if line.startswith("worktree ")}


def prepare_worktree(directory: Path, sha: str) -> None:
    if directory.resolve() in registered_worktrees() and not (directory / ".git").exists():
        removed = git("worktree", "remove", "--force", str(directory), check=False, capture=True)
        if removed.returncode and directory.resolve() in registered_worktrees():
            sys.stderr.write(removed.stderr)
            fail(f"cannot repair the stale worktree registration for {directory}")
    if (directory / ".git").exists():
        git("checkout", "-q", "-f", "--detach", sha, cwd=directory)
        git("clean", "-fdq", cwd=directory)
    else:
        directory.parent.mkdir(parents=True, exist_ok=True)
        git("worktree", "add", "-q", "--detach", str(directory), sha)

    link = directory / "toolchain"
    link.unlink(missing_ok=True)
    link.symlink_to(ROOT / "toolchain")
    (directory / "orig").mkdir(exist_ok=True)
    (directory / "build").mkdir(exist_ok=True)
    for name, target in (("orig/TotalA.exe", ROOT / "orig/TotalA.exe"),
                         ("build/ghidra", ROOT / "build/ghidra")):
        if target.exists():
            link = directory / name
            link.unlink(missing_ok=True)
            link.symlink_to(target)


def build(directory: Path, sha: str) -> None:
    proc = subprocess.run(["uv", "run", "tools/link.py", "--carve"], cwd=directory, env=REF_ENV)
    if proc.returncode:
        fail(f"tools/link.py --carve failed ({proc.returncode})")
    for name in ("TotalA.exe", "SMACKW32.DLL"):
        shutil.copyfile(directory / "build/link" / name, directory / name)
    (directory / "commit.txt").write_text(sha + "\n")


def run_checkall(directory: Path, addresses: list[str]) -> dict[int, str]:
    proc = subprocess.run(["uv", "run", "tools/checkall.py", *addresses], cwd=directory, env=REF_ENV,
                          stdout=subprocess.PIPE, text=True)
    results = {int(a, 16): "no result from checkall" for a in addresses}
    for line in proc.stdout.splitlines():
        if line.strip():
            results[int(line.split()[0], 16)] = line
    return results


def is_match(line: str) -> bool:
    return line.endswith("->  MATCH")


def checkall(directory: Path) -> dict[int, str]:
    with open(directory / "data/progress.csv", newline="") as f:
        addresses = [row["address"] for row in csv.DictReader(f)]
    return run_checkall(directory, addresses)


def read_allowlist(directory: Path) -> dict[int, str]:
    path = directory / "reference/changed.txt"
    if not path.is_file():
        fail(f"{path.relative_to(directory)} is missing from {directory.relative_to(ROOT)}")
    allowed = {}
    for line in path.read_text().splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            address, _, name = line.partition(" ")
            allowed[int(address, 16)] = name.strip()
    return allowed


def label(address: int, name: str) -> str:
    return f"{address:#x} {name}".rstrip()


def clock(seconds: float) -> str:
    m, s = divmod(int(round(seconds)), 60)
    return f"{m}:{s:02d}"


def scenario_files() -> dict[str, Path]:
    return {p.stem: p for p in sorted(SCENARIOS.glob("*.txt"))}


def default_parallel() -> int:
    """Half the CPUs, at least 1 and at most MAX_PARALLEL."""
    return max(1, min(MAX_PARALLEL, (os.cpu_count() or 2) // 2))


def scenario_ticks(scenario: Path) -> int:
    """The scenario's `ticks` value, or a tracediff.Error if it has none that parses."""
    for line in scenario.read_text().splitlines():
        words = line.split()
        if words and words[0] == "ticks":
            try:
                return int(words[1], 0)
            except (IndexError, ValueError):
                break
    raise tracediff.Error(f"{scenario}: the scenario has no valid ticks line")


def play_scenario(exe: Path, scenario: Path, game_dir: str | None) -> tuple[dict[str, str], list[tuple[int, str]], float]:
    """Run the scenario under Wine: the trace header, the digest of each tick, the seconds taken."""
    ticks = scenario_ticks(scenario)
    timeout = RUN_SECONDS_BASE + ticks / RUN_TICKS_PER_SECOND
    t0 = time.time()
    for attempt in range(RUN_ATTEMPTS):
        try:
            run = tracediff.play(exe, scenario, game_dir, None, timeout)
            break
        except tracediff.Error as e:
            if attempt == RUN_ATTEMPTS - 1:
                raise
            print(f"{scenario.stem}: run failed, trying again: {last_lines(e)}", flush=True)
    elapsed = time.time() - t0
    try:
        header, rows = tracediff.parse_trace(tracediff.need(run, "trace.txt"))
    finally:
        shutil.rmtree(run.parent, ignore_errors=True)
    for key in ("game_seed", "crt_seed", "view_seed"):
        if key not in header:
            raise tracediff.Error(f"{scenario}: the trace has no {key} line")
    return header, tracedigest.digests(rows), elapsed


def last_lines(error: Exception, count: int = 1) -> str:
    """The last `count` non-empty lines of an error's message, each cut to 200 characters."""
    lines = [line[:200] for line in str(error).splitlines() if line.strip()]
    return "\n    ".join(lines[-count:]) or type(error).__name__


def play_all(exe: Path, scenarios: dict[str, Path], game_dir: str | None, parallel: int) -> dict[str, tuple]:
    """Every scenario's play_scenario result, `parallel` at a time and longest first; exits 1 if one fails to run.

    Each run has its own session directory, Wine prefix, game directory, X display and audio sink
    (tools/playtest.py names them by its pid), so runs share nothing but the template prefix,
    which they only hardlink.
    """
    playtest.template_prefix()
    results: dict[str, tuple] = {}
    failures: list[str] = []

    def one(name: str) -> None:
        try:
            results[name] = play_scenario(exe, scenarios[name], game_dir)
        except (tracediff.Error, OSError) as e:
            failures.append(f"{name}: failed to run, playtest.py ended with:\n    {last_lines(e, 5)}")

    def longest_first(name: str) -> int:
        try:
            return -scenario_ticks(scenarios[name])
        except tracediff.Error:
            return 0

    with ThreadPoolExecutor(parallel) as pool:
        list(pool.map(one, sorted(scenarios, key=longest_first)))
    if failures:
        fail("scenarios failed to run:\n" + "\n".join(failures))
    return results


def write_traces(sha: str, exe: Path, game_dir: str | None, parallel: int) -> None:
    scenarios = scenario_files()
    if not scenarios:
        fail(f"no scenarios in {SCENARIOS.relative_to(ROOT)}")
    t0 = time.time()
    results = play_all(exe, scenarios, game_dir, parallel)
    rendered = {}
    for name, (header, digests, elapsed) in results.items():
        if not digests or any(k not in header for k in ("game_seed", "crt_seed", "view_seed")):
            fail(f"{name}: the run produced no complete trace; the stored traces are untouched")
        header = {**header, "commit": sha, "scenario": str(scenarios[name].relative_to(ROOT))}
        rendered[name] = tracedigest.render(header, digests)
    TRACES.mkdir(parents=True, exist_ok=True)
    for old in TRACES.glob("*.txt"):
        old.unlink()
    for name, (header, digests, elapsed) in results.items():
        (TRACES / f"{name}.txt").write_text(rendered[name])
        print(f"wrote {(TRACES / f'{name}.txt').relative_to(ROOT)}: {len(digests)} ticks, {clock(elapsed)}")
    print(f"{len(results)} traces, commit {sha}, total {clock(time.time() - t0)}")


def first_difference(stored: list[tuple[int, str]], got: list[tuple[int, str]]) -> str | None:
    """A description of the first tick at which the digests differ, or None."""
    for a, b in zip(stored, got):
        if a != b:
            return f"tick {a[0]}: stored {a[1]}, got {b[1]}"
    if len(stored) != len(got):
        longer, which = (stored, "stored") if len(stored) > len(got) else (got, "got")
        return f"tick {longer[min(len(stored), len(got))][0]}: only the {which} trace has it"
    return None


def check_traces(exe: Path, game_dir: str | None, parallel: int, quick: bool = False) -> None:
    scenarios = scenario_files()
    stored = {p.stem: p for p in sorted(TRACES.glob("*.txt"))} if TRACES.is_dir() else {}
    if quick:
        missing = [n for n in QUICK if n not in scenarios or n not in stored]
        if missing:
            fail("the quick subset names scenarios without both a scenario and a trace: " + ", ".join(missing))
        scenarios = {n: scenarios[n] for n in QUICK}
        stored = {n: stored[n] for n in QUICK}
    problems = [f"scenario {n} has no trace in {TRACES.relative_to(ROOT)}" for n in scenarios if n not in stored]
    problems += [f"trace {n} has no scenario in {SCENARIOS.relative_to(ROOT)}" for n in stored if n not in scenarios]
    if not scenarios or problems:
        fail("\n".join(problems) or f"no scenarios in {SCENARIOS.relative_to(ROOT)}")
    expected = {}
    for name, path in stored.items():
        try:
            expected[name] = tracedigest.parse_digest(path)
        except tracediff.Error as e:
            fail(str(e))

    t0 = time.time()
    results = play_all(exe, scenarios, game_dir, parallel)
    wall = time.time() - t0
    bad: list[str] = []
    for name in scenarios:
        header, digests, elapsed = results[name]
        want_header, want = expected[name]
        diff = first_difference(want, digests)
        seeds = [k for k in ("game_seed", "crt_seed", "view_seed") if want_header.get(k) != header[k]]
        if seeds:
            diff = diff or "the scenario's seeds differ from the stored trace: " + ", ".join(seeds)
        print(f"{'DIFF' if diff else 'ok':4} {name} ({len(digests)} ticks, {clock(elapsed)})" + (f" {diff}" if diff else ""))
        if diff:
            bad.append(name)
    print(f"{len(scenarios) - len(bad)} of {len(scenarios)} scenarios equal, total {clock(wall)}")
    if not bad:
        if quick:
            print("quick subset passed; this is not the gate, a pull request needs the full --check")
        return

    name = bad[0]
    sha = expected[name][0].get("commit", "unknown")
    print(f"\ntracediff for {name}, against the reference at {sha}:", flush=True)
    if resolve_quiet(sha) is None:
        fail(f"{name}: the trace's commit {sha} is not in this repository")
    ref_dir = build_cached(sha)
    cmd = [sys.executable, str(ROOT / "tools/tracediff.py"), str(ref_dir / "TotalA.exe"), str(exe),
           "--scenario", str(scenarios[name])]
    if game_dir:
        cmd += ["--game-dir", game_dir]
    subprocess.run(cmd)
    sys.exit(1)


def resolve_quiet(ref: str) -> str | None:
    proc = git("rev-parse", "--verify", "-q", f"{ref}^{{commit}}", check=False, capture=True)
    return proc.stdout.strip() if proc.returncode == 0 else None


def build_cached(sha: str) -> Path:
    """build/reference/<sha>, built if it holds no exe of that commit."""
    directory = ROOT / "build/reference" / sha
    stamp = directory / "commit.txt"
    if not (directory / "TotalA.exe").is_file() or not stamp.is_file() or stamp.read_text().strip() != sha:
        for name in OUTPUTS:
            (directory / name).unlink(missing_ok=True)
        prepare_worktree(directory, sha)
        build(directory, sha)
    return directory


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("ref", nargs="?", help="branch, tag or commit to build")
    ap.add_argument("--traces", action="store_true", help="play every scenario and write reference/traces/")
    ap.add_argument("--check", action="store_true", help="play every scenario and compare with reference/traces/")
    ap.add_argument("--quick", action="store_true",
                    help="--check: play only the fixed quick subset (not the gate)")
    ap.add_argument("--exe", metavar="PATH", help="--check: check this exe instead of building the ref")
    ap.add_argument("--game-dir", help="--traces, --check: the game's install (default: Steam's)")
    ap.add_argument("--parallel", type=int, default=default_parallel(), metavar="N",
                    help="--traces, --check: scenarios played at once (default: half the CPUs, at most 6)")
    args = ap.parse_args()
    if args.parallel < 1:
        ap.error("--parallel takes 1 or more")
    ref = args.ref
    if args.traces == args.check and (args.traces or args.exe):
        ap.error("give one of --traces and --check")
    if args.quick and not args.check:
        ap.error("--quick needs --check; it cannot be used with --traces")
    if args.exe and not args.check:
        ap.error("--exe needs --check")
    if args.exe and not Path(args.exe).is_file():
        ap.error(f"{args.exe} is not a file")
    if not args.exe and not ref:
        ap.error("give a ref to build")
    if args.exe:
        check_traces(Path(args.exe).resolve(), args.game_dir, args.parallel, args.quick)
        return

    sha = resolve(ref)
    directory = ROOT / "build/reference" / ref
    for name in OUTPUTS:
        (directory / name).unlink(missing_ok=True)

    prepare_worktree(directory, sha)
    allowed = read_allowlist(directory)
    build(directory, sha)
    results = checkall(directory)

    matching = {a for a, line in results.items() if is_match(line)}
    changed = sorted(a for a in allowed if a in results and a not in matching)
    unchanged = sorted(a for a in allowed if a in matching)
    unlisted = sorted(a for a in results if a not in matching and a not in allowed)

    print(f"reference {ref} @ {sha}: {directory.relative_to(ROOT)}/TotalA.exe")
    print(f"checkall: {len(results)} functions, {len(matching)} MATCH")
    print("  allowlisted, changed:   " + (", ".join(label(a, allowed[a]) for a in changed) or "none"))
    print("  allowlisted, unchanged: " + (", ".join(label(a, allowed[a]) for a in unchanged) or "none"))
    if unlisted:
        print("  not allowlisted, changed:")
        for a in unlisted:
            print(f"    {results[a]}")
        sys.exit(1)
    print("  not allowlisted, changed: none")
    if args.traces:
        write_traces(sha, directory / "TotalA.exe", args.game_dir, args.parallel)
    elif args.check:
        check_traces(directory / "TotalA.exe", args.game_dir, args.parallel, args.quick)


if __name__ == "__main__":
    main()

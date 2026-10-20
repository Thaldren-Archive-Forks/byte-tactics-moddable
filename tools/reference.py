"""The conformity gate: play the scenarios and compare their digests with reference/traces/.

    uv run tools/reference.py --check --exe build/modern/TotalA.exe    # the gate
    uv run tools/reference.py --check --exe build/modern/TotalA.exe --quick    # a fixed subset, not the gate
    uv run tools/reference.py --traces          # rewrite reference/traces/ from the reference branch

`--check --exe` plays every scenario in reference/scenarios/ (`--parallel N` at a
time, default half the CPUs and at most 6, under Wine through tools/playtest.py)
and compares the digest of each tick (tools/tracedigest.py) with the stored one.
On the first difference it builds the reference from the commit the trace
records, prints tools/tracediff.py's report for that scenario and exits 1. A
missing or extra trace or scenario exits 1. `--quick` restricts `--check` to the
scenarios named in QUICK.

Every scenario is played with new game allocations zeroed (the game's own -memset=0 switch, through
tools/playtest.py --testmode). `--check --poison` fills them with 0xCD instead (-memset=0xcdcdcdcd); a
table that reads uninitialised bytes may then differ from the stored digests, and the report lists those.

`--check --exe <exe> --quirks <spec>` plays every scenario with `quirks <spec>` added and prints, per
scenario, equal, differs (the first differing tick and phase) or failed. A scenario that differs is
played again with every flag off and compared with that run's phases. A difference is not a failure:
the exit status is 1 only when a scenario failed, or its all-off rerun failed or differed from the
stored trace. It builds no reference and is not the gate.

`--traces` checks out origin/reference in build/reference/branch, runs that
branch's own `tools/reference.py HEAD --traces` there (it links the matching
build with the original compiler and plays the scenarios), and copies the
traces it wrote into reference/traces/.
Set WINEARCH=win64 for Wine; `--game-dir` names the game's
install.
"""

import argparse
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
RUN_TICKS_PER_SECOND = 25
RUN_ATTEMPTS = 2
RETRY_DIALOG = "dialog-directx"


def reference_env(environ: dict[str, str], root: Path) -> dict[str, str]:
    env = {k: v for k, v in environ.items() if k != "VIRTUAL_ENV"}
    pin = root / ".python-version"
    if pin.is_file() and "UV_PYTHON" not in env:
        env["UV_PYTHON"] = pin.read_text().strip()
    return env


REF_ENV = reference_env(os.environ, ROOT)


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


def difference(want: tuple[dict[str, str], list[tuple[int, str]]], header: dict[str, str],
               digests: list[tuple[int, str]]) -> str | None:
    """Why a run differs from the stored (header, digests), or None when it equals them."""
    want_header, want_digests = want
    diff = first_difference(want_digests, digests)
    seeds = [k for k in ("game_seed", "crt_seed", "view_seed") if want_header.get(k) != header[k]]
    if seeds:
        diff = diff or "the scenario's seeds differ from the stored trace: " + ", ".join(seeds)
    return diff


def keep_run(run: Path, scenario: str, suffix: str = "") -> Path:
    """Copy the run's trace.txt (and dump.txt, if any) to build/scratch/check-<scenario>-<time>/."""
    kept = ROOT / "build/scratch" / f"check-{scenario}{suffix}-{time.strftime('%Y%m%d-%H%M%S')}"
    kept.mkdir(parents=True, exist_ok=True)
    for name in ("trace.txt", "dump.txt"):
        if (run / name).is_file():
            shutil.copy2(run / name, kept / name)
    return kept


def should_retry(message: str) -> bool:
    """Whether a failed run is tried again: an exe crash or the DirectX init dialog, not a hang or another dialog.

    playtest.py names an error dialog's kind (dialog-directx, dialog-crt, ...) in the failure. The DirectX
    box is only classified so before the first tick; the runtime error (R6025) is a real fault.
    """
    if "still runs after" in message:
        return False
    if "dialog-" in message:
        return RETRY_DIALOG in message
    return True


def play_scenario(exe: Path, scenario: Path, game_dir: str | None,
                  want: tuple[dict[str, str], list[tuple[int, str]]] | None = None, poison: bool = False,
                  keep_always: bool = False) -> tuple[dict[str, str], list[tuple[int, str]], float, Path | None]:
    """Run the scenario under Wine: the trace header, the digest of each tick, the seconds taken.

    With `want` (the stored header and digests), a run that differs is kept: the last element is the
    directory holding its trace.txt, otherwise None. With `keep_always` the run is kept whatever it did.

    A run whose exe crashes (a nonzero exit) is tried once more, since an exe fault at startup
    was seen in 1 of 300 runs, and so is one that stopped on the DirectX initialisation dialog before
    tick 1 (startup under load). A run that is still going at the timeout is a hang, and a run that
    stopped on any other error dialog (R6025 and the like) is a game fault: both fail at once.
    """
    ticks = scenario_ticks(scenario)
    timeout = RUN_SECONDS_BASE + ticks / RUN_TICKS_PER_SECOND
    t0 = time.time()
    for attempt in range(RUN_ATTEMPTS):
        try:
            run = tracediff.play(exe, scenario, game_dir, None, timeout, poison)
            break
        except tracediff.Error as e:
            if attempt == RUN_ATTEMPTS - 1 or not should_retry(str(e)):
                raise
            print(f"{scenario.stem}: run failed, trying again: {last_lines(e)}", flush=True)
    elapsed = time.time() - t0
    kept = None
    try:
        header, rows = tracediff.parse_trace(tracediff.need(run, "trace.txt"))
        for key in ("game_seed", "crt_seed", "view_seed"):
            if key not in header:
                raise tracediff.Error(f"{scenario}: the trace has no {key} line")
        digests = tracedigest.digests(rows)
        if keep_always or (want is not None and difference(want, header, digests)):
            kept = keep_run(run, scenario.stem, "-alloff" if keep_always else "")
    finally:
        shutil.rmtree(run.parent, ignore_errors=True)
    return header, digests, elapsed, kept


def last_lines(error: Exception, count: int = 1) -> str:
    """The last `count` non-empty lines of an error's message, each cut to 200 characters."""
    lines = [line[:200] for line in str(error).splitlines() if line.strip()]
    return "\n    ".join(lines[-count:]) or type(error).__name__


def play_all(exe: Path, scenarios: dict[str, Path], game_dir: str | None, parallel: int,
             expected: dict[str, tuple] | None = None, poison: bool = False,
             failed: dict[str, str] | None = None, keep_always: bool = False) -> dict[str, tuple]:
    """Every scenario's play_scenario result, `parallel` at a time and longest first; exits 1 if one fails to run.

    With `failed` (a dict), a scenario that fails to run is recorded there as name -> message and play_all
    returns without exiting. With `keep_always`, every run's trace.txt is kept (see play_scenario).

    Each run has its own session directory, Wine prefix, game directory, X display and audio sink
    (tools/playtest.py names them by its pid), so runs share nothing but the template prefix,
    which they only hardlink. With `expected`, a scenario that differs keeps its trace (see play_scenario).
    """
    playtest.template_prefix()
    results: dict[str, tuple] = {}
    failures: list[str] = []

    def one(name: str) -> None:
        try:
            results[name] = play_scenario(exe, scenarios[name], game_dir, (expected or {}).get(name), poison, keep_always)
        except (tracediff.Error, OSError) as e:
            if failed is not None:
                failed[name] = last_lines(e, 5)
            else:
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


def with_steps_per_frame(scenarios: dict[str, Path], steps: int, directory: Path) -> dict[str, Path]:
    """Copies of the scenarios in `directory`, each ending with a steps_per_frame line (a later line replaces an earlier one)."""
    directory.mkdir(parents=True, exist_ok=True)
    copies = {}
    for name, path in scenarios.items():
        text = path.read_text()
        copies[name] = directory / path.name
        copies[name].write_text(text + ("" if text.endswith("\n") else "\n") + f"steps_per_frame {steps}\n")
    return copies


def with_quirks(scenarios: dict[str, Path], spec: str, directory: Path) -> dict[str, Path]:
    """Copies of the scenarios in `directory`, each with a `quirks <spec>` line before its first order line (else at the end)."""
    directory.mkdir(parents=True, exist_ok=True)
    copies = {}
    for name, path in scenarios.items():
        lines = path.read_text().splitlines()
        at = next((i for i, line in enumerate(lines) if line.split()[:1] == ["order"]), len(lines))
        lines.insert(at, f"quirks {spec}")
        copies[name] = directory / path.name
        copies[name].write_text("\n".join(lines) + "\n")
    return copies


def first_difference(stored: list[tuple[int, str]], got: list[tuple[int, str]]) -> str | None:
    """A description of the first tick at which the digests differ, or None."""
    for a, b in zip(stored, got):
        if a != b:
            return f"tick {a[0]}: stored {a[1]}, got {b[1]}"
    if len(stored) != len(got):
        longer, which = (stored, "stored") if len(stored) > len(got) else (got, "got")
        return f"tick {longer[min(len(stored), len(got))][0]}: only the {which} trace has it"
    return None


def check_traces(exe: Path, game_dir: str | None, parallel: int, quick: bool = False, poison: bool = False,
                 steps_per_frame: int | None = None, quirks: str | None = None) -> None:
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

    played = scenarios
    scratch = ROOT / "build/scratch" / f"steps-per-frame-{os.getpid()}"
    if steps_per_frame is not None:
        played = with_steps_per_frame(scenarios, steps_per_frame, scratch)
    base = played
    if quirks is not None:
        played = with_quirks(played, quirks, scratch / "quirks")
    off_results: dict[str, tuple] = {}
    off_failed: dict[str, str] = {}
    failed: dict[str, str] = {}
    t0 = time.time()
    try:
        results = play_all(exe, played, game_dir, parallel, expected, poison, failed if quirks is not None else None)
        if quirks is not None:
            differing = [n for n in scenarios if n in results and n not in failed and difference(expected[n], *results[n][:2])]
            if differing:
                off_results = play_all(exe, {n: base[n] for n in differing}, game_dir, parallel, expected, False, off_failed, True)
    finally:
        shutil.rmtree(scratch, ignore_errors=True)
    wall = time.time() - t0
    if quirks is not None:
        report_quirks(scenarios, expected, results, failed, quirks, wall, off_results, off_failed)
        return
    bad: list[str] = []
    for name in scenarios:
        header, digests, elapsed, kept = results[name]
        diff = difference(expected[name], header, digests)
        where = f" [kept: {kept.relative_to(ROOT)}]" if kept else ""
        print(f"{'DIFF' if diff else 'ok':4} {name} ({len(digests)} ticks, {clock(elapsed)})" + (f" {diff}{where}" if diff else ""))
        if diff:
            bad.append(name)
    print(f"{len(scenarios) - len(bad)} of {len(scenarios)} scenarios equal, total {clock(wall)}")
    if not bad:
        if quick:
            print("quick subset passed; this is not the gate, a pull request needs the full --check")
        return

    name = bad[0]
    sha = expected[name][0].get("commit", "unknown")
    print(f"\ntracediff for {name}: a = the reference at {sha}, b = {exe}", flush=True)
    if resolve_quiet(sha) is None:
        fail(f"{name}: the trace's commit {sha} is not in this repository")
    ref_dir = build_cached(sha)
    subprocess.run(tracediff_command(ref_dir / "TotalA.exe", exe, scenarios[name], poison, game_dir))
    sys.exit(1)


def tracediff_command(ref_exe: Path, exe: Path, scenario: Path, poison: bool, game_dir: str | None) -> list[str]:
    cmd = [sys.executable, str(ROOT / "tools/tracediff.py"), str(ref_exe), str(exe),
           "--scenario", str(scenario), "--reference", "a"]
    if poison:
        cmd.append("--poison-b")
    if game_dir:
        cmd += ["--game-dir", game_dir]
    return cmd


def shown(path: Path) -> Path:
    """The path relative to the repository when it is inside it."""
    try:
        return path.relative_to(ROOT)
    except ValueError:
        return path


def quirk_divergence(kept: Path, off_kept: Path) -> str | None:
    """The first tick and phase at which two kept runs' trace.txt differ, with the phase's function, or None."""
    _, rows = tracediff.parse_trace(kept / "trace.txt")
    _, off_rows = tracediff.parse_trace(off_kept / "trace.txt")
    found = tracediff.first_divergence(off_rows, rows)
    if found is None:
        return None
    return f"tick {found[0]}, phase {found[1]} ({tracediff.describe_phase(found[1])})"


def report_quirks(scenarios: dict[str, Path], expected: dict[str, tuple], results: dict[str, tuple],
                  failed: dict[str, str], spec: str, wall: float,
                  off_results: dict[str, tuple], off_failed: dict[str, str]) -> None:
    """Print one line per scenario (equal, differs at a tick and phase, or failed) and a total; exits 1 only if one failed.

    A scenario that differs from the stored trace is played again with every flag off (`off_results`,
    `off_failed`), whose phases the quirks run is compared with. Both runs' directories are kept. A
    difference is not a failure, but a rerun that fails or that differs from the stored trace is.
    """
    equal = differ = bad = 0
    for name in scenarios:
        lines = failed.get(name)
        if lines is not None:
            bad += 1
            print(f"FAIL {name}: {lines}")
            continue
        header, digests, elapsed, kept = results[name]
        diff = difference(expected[name], header, digests)
        if not diff:
            equal += 1
            print(f"equal   {name} ({len(digests)} ticks, {clock(elapsed)})")
            continue
        if name in off_failed:
            bad += 1
            print(f"FAIL {name}: the all-off rerun failed: {off_failed[name]}")
            continue
        off_header, off_digests, _, off_kept = off_results[name]
        off_diff = difference(expected[name], off_header, off_digests)
        if off_diff:
            bad += 1
            print(f"FAIL {name}: all-off run differs from the stored trace, {off_diff} [kept: {shown(off_kept)}]")
            continue
        differ += 1
        divergence = None
        try:
            divergence = quirk_divergence(kept, off_kept)
        except (tracediff.Error, OSError, ValueError) as e:
            divergence = f"{diff} (no phase: {last_lines(e)})"
        print(f"differs {name} ({len(digests)} ticks, {clock(elapsed)}) {divergence or diff}"
              f" [kept: {shown(kept)}, all-off: {shown(off_kept)}]")
    print(f"quirks {spec}: {equal} equal, {differ} differ, {bad} failed of {len(scenarios)} scenarios, total {clock(wall)}")
    if bad:
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


def reference_branch() -> Path:
    git("fetch", "-q", "origin", "reference", check=False)
    directory = ROOT / "build/reference/branch"
    prepare_worktree(directory, resolve("origin/reference"))
    return directory


def name_mismatch(produced: set[str], expected: set[str]) -> str:
    """Empty when the sets are equal, else a message naming the missing and the extra names."""
    if produced == expected:
        return ""
    missing = ", ".join(sorted(expected - produced)) or "none"
    extra = ", ".join(sorted(produced - expected)) or "none"
    return f"the traces do not match moddable's scenarios; missing: {missing}; extra: {extra}"


def write_traces(game_dir: str | None, parallel: int) -> None:
    directory = reference_branch()
    branch_scenarios = directory / "reference/scenarios"
    shutil.rmtree(branch_scenarios, ignore_errors=True)
    shutil.copytree(SCENARIOS, branch_scenarios)
    cmd = ["uv", "run", "tools/reference.py", "HEAD", "--traces", "--parallel", str(parallel)]
    if game_dir:
        cmd += ["--game-dir", game_dir]
    if subprocess.run(cmd, cwd=directory, env=REF_ENV).returncode:
        fail("tools/reference.py --traces failed on the reference branch")
    produced = sorted((directory / "reference/traces").glob("*.txt"))
    if not produced:
        fail("the reference branch wrote no traces; reference/traces/ is untouched")
    mismatch = name_mismatch({p.stem for p in produced}, set(scenario_files()))
    if mismatch:
        fail(f"{mismatch}; reference/traces/ is untouched")
    TRACES.mkdir(parents=True, exist_ok=True)
    for old in TRACES.glob("*.txt"):
        old.unlink()
    for path in produced:
        shutil.copyfile(path, TRACES / path.name)
    print(f"{len(produced)} traces copied to {TRACES.relative_to(ROOT)}")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--traces", action="store_true", help="rewrite reference/traces/ from the reference branch")
    ap.add_argument("--check", action="store_true", help="play every scenario and compare with reference/traces/")
    ap.add_argument("--quick", action="store_true",
                    help="--check: play only the fixed quick subset (not the gate)")
    ap.add_argument("--poison", action="store_true",
                    help="--check: play the scenarios with new game allocations filled with 0xCD; list the bytes whose starting value reaches hashed state; a difference is expected for real uninitialised reads")
    ap.add_argument("--quirks", metavar="SPEC",
                    help="--check: play every scenario with `quirks SPEC` added and report which equal the stored traces and which differ, with the first tick and phase (a differing scenario is rerun with all flags off; a difference is not a failure; no reference build); not the gate")
    ap.add_argument("--exe", metavar="PATH", help="--check: the exe to check (build/modern/TotalA.exe)")
    ap.add_argument("--steps-per-frame", type=int, metavar="N",
                    help="--check: play every scenario with steps_per_frame N (1 to 8) and compare with the stored traces")
    ap.add_argument("--game-dir", help="--traces, --check: the game's install (default: Steam's)")
    ap.add_argument("--parallel", type=int, default=default_parallel(), metavar="N",
                    help="--traces, --check: scenarios played at once (default: half the CPUs, at most 6)")
    args = ap.parse_args()
    if args.parallel < 1:
        ap.error("--parallel takes 1 or more")
    if args.traces == args.check:
        ap.error("give one of --traces and --check")
    if args.poison and not args.check:
        ap.error("--poison needs --check")
    if args.quick and not args.check:
        ap.error("--quick needs --check; it cannot be used with --traces")
    if args.steps_per_frame is not None and not args.check:
        ap.error("--steps-per-frame needs --check")
    if args.steps_per_frame is not None and not 1 <= args.steps_per_frame <= 8:
        ap.error("--steps-per-frame takes 1 to 8")
    if args.quirks is not None and not args.check:
        ap.error("--quirks needs --check")
    if args.quirks is not None and not args.quirks.strip():
        ap.error("--quirks takes a spec, such as recommended or '3.1c +flag'")
    if args.quirks is not None and args.poison:
        ap.error("--quirks cannot be combined with --poison")
    if args.exe and not args.check:
        ap.error("--exe needs --check")
    if args.traces:
        write_traces(args.game_dir, args.parallel)
        return
    if not args.exe:
        ap.error("--check needs --exe: the gate is the modern build, --exe build/modern/TotalA.exe")
    if not Path(args.exe).is_file():
        ap.error(f"{args.exe} is not a file")
    check_traces(Path(args.exe).resolve(), args.game_dir, args.parallel, args.quick, args.poison, args.steps_per_frame,
                 args.quirks)


if __name__ == "__main__":
    main()

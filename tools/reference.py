"""Build a test-mode exe from any git ref with the conformity harness applied.

    uv run tools/reference.py main
    uv run tools/reference.py a1b2c3d

Checks the ref out in build/reference/<ref>, applies reference/harness.patch
and copies reference/harness.cpp into it, links it with that ref's own tools,
and leaves TotalA.exe, SMACKW32.DLL and commit.txt there. It then runs checkall
over every annotated function and reports which ones no longer MATCH, against
the allowlist in reference/changed.txt. Exits 1 if the patch does not apply,
the build fails, or a function outside the allowlist does not MATCH.
"""

import argparse
import csv
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REFERENCE = ROOT / "reference"
OUTPUTS = ("TotalA.exe", "SMACKW32.DLL", "commit.txt")
REF_ENV = {k: v for k, v in os.environ.items() if k != "VIRTUAL_ENV"}


def git(*args: str, cwd: Path = ROOT, check: bool = True, capture: bool = False) -> subprocess.CompletedProcess:
    return subprocess.run(["git", "-C", str(cwd), *args], check=check, text=True,
                          capture_output=capture)


def fail(message: str) -> None:
    sys.exit(f"reference: {message}")


def resolve(ref: str) -> str:
    """The commit a ref names, trying origin/<ref> when there is no local one."""
    found = []
    for name in (ref, f"origin/{ref}"):
        proc = git("rev-parse", "--verify", "-q", f"{name}^{{commit}}", check=False, capture=True)
        found.append(proc.stdout.strip() if proc.returncode == 0 else None)
    local, remote = found
    if local and remote and local != remote and not ref.startswith("origin/"):
        print(f"reference: warning: {ref} is {local[:12]} but origin/{ref} is {remote[:12]}; building {ref}",
              file=sys.stderr)
    if local or remote:
        return local or remote
    fail(f"{ref} names no commit")


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


def apply_harness(directory: Path, ref: str, sha: str) -> None:
    patch = REFERENCE / "harness.patch"
    source = REFERENCE / "harness.cpp"
    for path in (patch, source):
        if not path.is_file():
            fail(f"{path.relative_to(ROOT)} is missing")
    check = git("apply", "--check", str(patch), cwd=directory, check=False, capture=True)
    if check.returncode:
        sys.stderr.write(check.stderr)
        fail(f"reference/harness.patch does not apply to {ref} ({sha})")
    git("apply", str(patch), cwd=directory)
    target = directory / "src/harness/harness.cpp"
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)


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
    results = run_checkall(directory, addresses)
    # TODO(#59): drop this retry once checkall's parallel compile is fixed.
    retry = [f"{a:#x}" for a, line in results.items() if not is_match(line)]
    if retry:
        results.update(run_checkall(directory, retry))
    return results


def read_allowlist() -> dict[int, str]:
    path = REFERENCE / "changed.txt"
    if not path.is_file():
        fail(f"{path.relative_to(ROOT)} is missing")
    allowed = {}
    for line in path.read_text().splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            address, _, name = line.partition(" ")
            allowed[int(address, 16)] = name.strip()
    return allowed


def label(address: int, name: str) -> str:
    return f"{address:#x} {name}".rstrip()


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("ref", help="branch, tag or commit to build")
    ref = ap.parse_args().ref

    sha = resolve(ref)
    directory = ROOT / "build/reference" / ref
    for name in OUTPUTS:
        (directory / name).unlink(missing_ok=True)

    prepare_worktree(directory, sha)
    apply_harness(directory, ref, sha)
    allowed = read_allowlist()
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


if __name__ == "__main__":
    main()

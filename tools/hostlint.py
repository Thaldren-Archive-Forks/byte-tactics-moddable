"""Fail when a file includes a platform, host or presentation header that data/host_exceptions.csv does not list.

    python3 tools/hostlint.py                  # check the repository
    python3 tools/hostlint.py --open-issues    # also check that each row's issue is open
    python3 tools/hostlint.py --root DIR       # check another tree

Platform headers are windows.h, shlobj.h, imagehlp.h, mmsystem.h, winsock*.h, the
DirectX headers (ddraw, dsound, dplay, dplobby, dinput, d3d*, dxguid) and SDL.
data/host_exceptions.csv (file,header,issue) lists the pairs that remain, each
with the open issue that removes it. A row whose file no longer includes its
header is stale, so the list only shrinks.

The game-logic rule: a game-logic file is a file with any function in data/game_logic.csv
(its file column). Presentation headers are the headers under PRESENTATION_DIRS of
tools/stateclasses.py. Host headers are interfaces/host.h and anything under src/host/.
Each quoted or angle include of a game-logic file is resolved against the including file's
directory, include/ and the repository root (the directories the build searches), and a
resolved host or presentation header is listed by its path from the root, for example
src/sound/sound.h. A game-logic file that includes one without a row fails, and so does a
data/game_logic.csv row naming a file that does not exist. Prints one line per problem and
exits 1 if there are any. Standard library only.
"""

import argparse
import csv
import json
import os
import re
import posixpath
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from stateclasses import PRESENTATION_DIRS  # noqa: E402

EXCEPTIONS = "data/host_exceptions.csv"
REPO = "ozgb/byte-tactics-moddable"
SUFFIXES = {".c", ".cpp", ".h", ".hpp", ".inc", ".cc"}
SKIP_DIRS = {".git", "build", "toolchain", "orig"}
HOST_DIR = "src/host"
HOST_HEADER = "interfaces/host.h"
GAME_LOGIC = "data/game_logic.csv"
INCLUDE_DIRS = ("include", "")
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)
PLATFORM = re.compile(
    r"^(windows|shlobj|imagehlp|mmsystem|winsock2?|ddraw|dsound|dplay|dplobby|dinput|dxguid|d3d\w*)\.h$"
    r"|^sdl\d*(\.h$|/)",
    re.IGNORECASE,
)


def platform_includes(root):
    """Yield (relative path, header) for each platform include outside the host, headers lowercased."""
    for dirpath, dirnames, filenames in os.walk(root):
        rel_dir = os.path.relpath(dirpath, root).replace(os.sep, "/")
        rel_dir = "" if rel_dir == "." else rel_dir
        dirnames[:] = sorted(d for d in dirnames if d not in SKIP_DIRS and (rel_dir + "/" + d).lstrip("/") != HOST_DIR)
        for name in sorted(filenames):
            if os.path.splitext(name)[1].lower() not in SUFFIXES:
                continue
            path = (rel_dir + "/" + name).lstrip("/")
            with open(os.path.join(root, path), encoding="latin-1") as f:
                text = f.read()
            for header in sorted({m.group(1).lower() for m in INCLUDE.finditer(text) if PLATFORM.match(m.group(1))}):
                yield path, header


def game_logic_files(root):
    """The sorted files named by data/game_logic.csv, empty when it is absent."""
    path = os.path.join(root, GAME_LOGIC)
    if not os.path.exists(path):
        return []
    with open(path, newline="") as f:
        return sorted({r["file"] for r in csv.DictReader(f)})


def boundary_header(resolved):
    return resolved == HOST_HEADER or resolved.startswith(HOST_DIR + "/") or resolved.startswith(PRESENTATION_DIRS)


def resolve_include(root, file, name):
    """Return the repository path an include of file names, or None when no include directory has it."""
    for base in (posixpath.dirname(file), *INCLUDE_DIRS):
        candidate = posixpath.normpath(posixpath.join(base, name))
        if not candidate.startswith("..") and os.path.isfile(os.path.join(root, candidate)):
            return candidate
    return None


def boundary_includes(root, files):
    """Yield (file, resolved header) for each host or presentation include of the existing files."""
    for file in files:
        path = os.path.join(root, file)
        if not os.path.isfile(path):
            continue
        with open(path, encoding="latin-1") as f:
            text = f.read()
        for name in sorted({m.group(1) for m in INCLUDE.finditer(text)}):
            resolved = resolve_include(root, file, name)
            if resolved and boundary_header(resolved):
                yield file, resolved


def read_exceptions(root):
    path = os.path.join(root, EXCEPTIONS)
    if not os.path.exists(path):
        return []
    with open(path, newline="") as f:
        return [(r["file"], r["header"].lower(), r["issue"]) for r in csv.DictReader(f)]


def gh_issue_state(number, gh):
    """Return the state of the issue (OPEN or CLOSED), or None when gh cannot say."""
    cmd = [*gh.split(), "issue", "view", str(number), "-R", REPO, "--json", "state"]
    try:
        out = subprocess.run(cmd, capture_output=True, text=True, check=True).stdout
        return json.loads(out)["state"]
    except (OSError, subprocess.CalledProcessError, ValueError, KeyError):
        return None


def lint(root, open_issues=False, gh="gh"):
    problems = []
    rows = read_exceptions(root)
    listed = set()
    for file, header, issue in rows:
        if not re.fullmatch(r"#[0-9]+", issue):
            problems.append(f"{EXCEPTIONS}: {file} {header}: issue {issue!r} is not #<number>")
        listed.add((file, header))
    found = set(platform_includes(root))
    logic = game_logic_files(root)
    for file in logic:
        if not os.path.isfile(os.path.join(root, file)):
            problems.append(f"{GAME_LOGIC}: names {file}, which does not exist, remove its rows")
    found |= set(boundary_includes(root, logic))
    for file, header in sorted(found - listed):
        problems.append(f"{file}: includes {header} and is not in {EXCEPTIONS}")
    for file, header, _ in rows:
        if (file, header) not in found:
            reason = "no longer includes it" if os.path.exists(os.path.join(root, file)) else "no longer exists"
            problems.append(f"{EXCEPTIONS}: stale row {file} {header}: the file {reason}, remove the row")
    if open_issues:
        states = {}
        for _, _, issue in rows:
            if re.fullmatch(r"#[0-9]+", issue) and issue not in states:
                states[issue] = gh_issue_state(issue[1:], gh)
        for issue, state in sorted(states.items()):
            if state != "OPEN":
                problems.append(f"{EXCEPTIONS}: issue {issue} is {state or 'not readable'}, not open")
    return problems


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--root", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
    parser.add_argument("--open-issues", action="store_true", help="also check that each listed issue is open")
    parser.add_argument("--gh", default="gh", help="the gh command to run (default gh)")
    args = parser.parse_args(argv)
    problems = lint(args.root, args.open_issues, args.gh)
    for line in problems:
        print(line)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

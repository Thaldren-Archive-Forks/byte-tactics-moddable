"""Report nondeterminism that a change adds to game-logic functions.

    python3 tools/determinismlint.py [RANGE]     # default origin/moddable...HEAD
    python3 tools/determinismlint.py --all       # every line of every listed function

Checks the lines RANGE adds (git diff -U0, as tools/commentlint.py does)
inside a `// FUNCTION: 0x...` block, which runs from its marker to the next
marker or the end of the file. A block is checked when its address is listed
in data/game_logic.csv (written from the matching
build's reachability, by the reference branch's tools/crtcalls.py), or when the marker line itself is added in a file under
GAME_LOGIC_DIRS, so a new function is covered before the csv is regenerated.

Flagged, outside comments and string literals:

  * calls to wall-clock time: GetTicks, GetTickCount, timeGetTime, time,
    QueryPerformanceCounter, host_time_ms, host_time_counter;
  * calls to the presentation random stream, HarnessViewRand;
  * any use of std::unordered_map, unordered_set and their multi variants,
    whose iteration order is unspecified;
  * `<`, `>`, `<=`, `>=` between two plain identifiers that are both declared
    as pointers in the same function and not redeclared as anything else.
    Anything less certain is not reported.

ALLOWLIST holds the sites that exist today, keyed by function address and
callee, each with the issue that removes it; an allowlisted site never fails.
Prints file:line: <function> 0x<addr>: <what> and exits 1 if there are any
findings. Standard library only.
"""

import argparse
import csv
import re
import sys
from pathlib import Path

from commentlint import C_SUFFIXES, DEFAULT_RANGE, RAW_OPEN, RAW_PREFIX, added_lines, git, new_text

GAME_LOGIC_CSV = "data/game_logic.csv"
GAME_LOGIC_DIRS = ("src/units/", "src/weapons/", "src/ai/", "src/orders/", "src/map/", "src/game/")

ARRAY_BOUND = "compares the ends of a walk over one array, which is ordered by position"
VIEW_ONLY = "draws a presentation effect; reached from game logic through a call that does not feed state (revisit in #22)"
QUIRK_ONLY = "3.1c's clock read behind a quirk flag that is off by default; the default path reads the game tick"
NET_TIME = "3.1c network timing and timeouts, not hashed state (revisit in #21)"
UI_TIME = "3.1c widget animation and repeat timing, not hashed state (revisit in #22)"
LOAD_TIME = "3.1c load-time or console-command clock read, not hashed state (revisit in #19)"
TIMER_LIB = "the host timer itself or a diagnostic report (revisit in #24)"

ALLOWLIST: dict[tuple[int, str], str] = {
    (0x4071f0, "pointer"): ARRAY_BOUND,
    (0x415fa0, "GetTicks"): NET_TIME,
    (0x416150, "GetTicks"): NET_TIME,
    (0x417600, "GetTicks"): LOAD_TIME,
    (0x4181d0, "HarnessViewRand"): VIEW_ONLY,
    (0x418310, "HarnessViewRand"): VIEW_ONLY,
    (0x41c640, "HarnessViewRand"): VIEW_ONLY,
    (0x424890, "pointer"): ARRAY_BOUND,
    (0x44fc10, "HarnessViewRand"): VIEW_ONLY,
    (0x450380, "HarnessViewRand"): VIEW_ONLY,
    (0x450a10, "GetTicks"): NET_TIME,
    (0x453640, "GetTicks"): NET_TIME,
    (0x453c20, "GetTicks"): NET_TIME,
    (0x453d40, "GetTicks"): NET_TIME,
    (0x4565a0, "host_time_ms"): NET_TIME,
    (0x4624a0, "GetTicks"): NET_TIME,
    (0x462ae0, "GetTicks"): NET_TIME,
    (0x466dc0, "pointer"): ARRAY_BOUND,
    (0x467440, "pointer"): ARRAY_BOUND,
    (0x472ab0, "HarnessViewRand"): VIEW_ONLY,
    (0x474880, "HarnessViewRand"): VIEW_ONLY,
    (0x474df0, "HarnessViewRand"): VIEW_ONLY,
    (0x4751c0, "HarnessViewRand"): VIEW_ONLY,
    (0x475340, "HarnessViewRand"): VIEW_ONLY,
    (0x475600, "HarnessViewRand"): VIEW_ONLY,
    (0x47bd70, "HarnessViewRand"): VIEW_ONLY,
    (0x47fd70, "HarnessViewRand"): VIEW_ONLY,
    (0x4833b0, "pointer"): ARRAY_BOUND,
    (0x486e80, "pointer"): ARRAY_BOUND,
    (0x486ed0, "pointer"): ARRAY_BOUND,
    (0x486f10, "pointer"): ARRAY_BOUND,
    (0x4876c0, "pointer"): ARRAY_BOUND,
    (0x488810, "pointer"): ARRAY_BOUND,
    (0x488960, "pointer"): ARRAY_BOUND,
    (0x48a490, "GetTicks"): QUIRK_ONLY,
    (0x48ad30, "pointer"): ARRAY_BOUND,
    (0x48d790, "pointer"): ARRAY_BOUND,
    (0x4917d0, "GetTicks"): LOAD_TIME,
    (0x4948e0, "GetTicks"): UI_TIME,
    (0x497180, "host_time_counter"): LOAD_TIME,
    (0x497180, "time"): LOAD_TIME,
    (0x49be60, "HarnessViewRand"): VIEW_ONLY,
    (0x4a2e40, "GetTicks"): UI_TIME,
    (0x4a3780, "GetTicks"): UI_TIME,
    (0x4a4440, "GetTicks"): UI_TIME,
    (0x4a4660, "GetTicks"): UI_TIME,
    (0x4a51d0, "GetTicks"): UI_TIME,
    (0x4a6ae0, "GetTicks"): UI_TIME,
    (0x4a81e0, "GetTicks"): UI_TIME,
    (0x4a9b90, "GetTicks"): UI_TIME,
    (0x4a9fd0, "GetTicks"): UI_TIME,
    (0x4b6340, "host_time_ms"): TIMER_LIB,
    (0x4b6340, "GetTicks"): TIMER_LIB,
    (0x4b63f0, "host_time_ms"): TIMER_LIB,
    (0x4b6560, "host_time_ms"): TIMER_LIB,
    (0x4b88d0, "pointer"): ARRAY_BOUND,
    (0x4baff0, "pointer"): ARRAY_BOUND,
    (0x4bff20, "pointer"): ARRAY_BOUND,
    (0x4c3e40, "pointer"): ARRAY_BOUND,
    (0x4c4340, "pointer"): ARRAY_BOUND,
    (0x4cdb40, "HarnessViewRand"): VIEW_ONLY,
    (0x4d0f60, "pointer"): ARRAY_BOUND,
    (0x4ded60, "time"): TIMER_LIB,
}

CLOCK_CALLS = ("GetTicks", "GetTickCount", "timeGetTime", "time", "QueryPerformanceCounter", "host_time_ms",
               "host_time_counter")
VIEW_STREAM_CALLS = ("HarnessViewRand",)
CALL = re.compile(r"(?<![\w])(" + "|".join(CLOCK_CALLS + VIEW_STREAM_CALLS) + r")\s*\(")
UNORDERED = re.compile(r"\b(unordered_(?:multi)?(?:map|set))\b")
MARKER = re.compile(r"^\s*//\s*FUNCTION:\s*(0x[0-9a-fA-F]+)(?:\s+(\S+))?\s*$")
KEYWORDS = {"return", "case", "else", "goto", "delete", "sizeof", "new", "throw", "typedef", "using"}
POINTER_DECL = re.compile(r"(?m)^[ \t]*(?:const[ \t]+|struct[ \t]+|unsigned[ \t]+)*\w+(?:::\w+)*[ \t]*\*+[ \t]*(\w+)[ \t]*(?=[;=,)\[])")
PARAM_DECL = re.compile(r"[(,][ \t]*(?:const[ \t]+|struct[ \t]+|unsigned[ \t]+)*\w+(?:::\w+)*[ \t]*\*+[ \t]*(\w+)[ \t]*(?=[,)\[])")
COMPARE = re.compile(r"\b([A-Za-z_]\w*)[ \t]*(<=|>=|<|>)[ \t]*([A-Za-z_]\w*)\b")


def mask_c(text):
    """text with comments and string and char literals blanked, newlines kept."""
    out = list(text)
    i, n = 0, len(text)

    def blank(a, b):
        for k in range(a, b):
            if out[k] != "\n":
                out[k] = " "

    while i < n:
        c, two = text[i], text[i:i + 2]
        if two == "//":
            j = i + 2
            while j < n and not (text[j] == "\n" and text[j - 1] != "\\"):
                j += 1
            blank(i, j)
            i = j
        elif two == "/*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            blank(i, j)
            i = j
        elif c == '"' or (c == "'" and not (i and text[i - 1].isdigit())):
            m = RAW_OPEN.match(text, i) if c == '"' and RAW_PREFIX.search(text, max(0, i - 3), i) else None
            end = text.find(")" + m.group(1) + '"', m.end()) if m else -1
            if m:
                j = n if end < 0 else end + len(m.group(1)) + 2
            else:
                j = i + 1
                while j < n and text[j] != c and text[j] != "\n":
                    j += 2 if text[j] == "\\" else 1
                j = min(j + 1, n)
            blank(i + 1, max(i + 1, j - 1))
            i = j
        else:
            i += 1
    return "".join(out)


def load_game_logic(root):
    """{address: function name} from data/game_logic.csv, empty if absent."""
    path = Path(root) / GAME_LOGIC_CSV
    if not path.exists():
        return {}
    with path.open() as fh:
        return {int(r["address"], 16): r["function"] for r in csv.DictReader(fh)}


def blocks_of(text):
    """[(address, name or None, first line, last line)] for each FUNCTION block."""
    lines = text.splitlines()
    marks = [(i + 1, int(m.group(1), 16), m.group(2))
             for i, row in enumerate(lines) if (m := MARKER.match(row))]
    out = []
    for k, (line, addr, name) in enumerate(marks):
        last = marks[k + 1][0] - 1 if k + 1 < len(marks) else len(lines)
        out.append((addr, name, line, last))
    return out


def pointer_names(code):
    """Names declared as pointers in code and never declared as anything else."""
    pointers = set(POINTER_DECL.findall(code)) | set(PARAM_DECL.findall(code))
    plain = set()
    for m in re.finditer(r"(?<![\w*&])(\w+)[ \t]+&?[ \t]*(\w+)[ \t]*(?=[;=,)\[])", code):
        if m.group(1) not in KEYWORDS:
            plain.add(m.group(2))
    return pointers - plain


def certain_compare(code_line, pointers):
    """The pointer ordering comparisons on a line whose operands are both declared pointers."""
    found = []
    for m in COMPARE.finditer(code_line):
        left, op, right = m.groups()
        if left not in pointers or right not in pointers:
            continue
        before = code_line[:m.start()].rstrip()
        after = code_line[m.end():].lstrip()
        if before and not (before[-1] in "(;,{}?:" or before.endswith(("&&", "||"))
                           or (before[-1] == "=" and before[-2:-1] not in "=!<>" )):
            continue
        if after and not (after[0] in ");,?" or after.startswith(("&&", "||"))):
            continue
        found.append(f"{left} {op} {right}")
    return found


def check_block(masked, first, last, lines):
    """[(line, callee)] for the checked lines first..last of a block, masked is the whole block."""
    pointers = pointer_names("\n".join(masked))
    found = []
    for ln in lines:
        row = masked[ln - first]
        for m in CALL.finditer(row):
            before = row[:m.start()].rstrip()
            if before.endswith((".", "->")):
                continue
            if before.endswith("::") and not before[:-2].rstrip().endswith("std"):
                if before[:-2].rstrip()[-1:].isalnum() or before[:-2].rstrip()[-1:] == "_":
                    continue
            found.append((ln, m.group(1)))
        for m in UNORDERED.finditer(row):
            found.append((ln, m.group(1)))
        for text in certain_compare(row, pointers):
            found.append((ln, f"pointer ordering {text}"))
    return found


def describe(callee):
    if callee in VIEW_STREAM_CALLS:
        return f"{callee}() draws the presentation random stream"
    if callee in CLOCK_CALLS:
        return f"{callee}() reads wall-clock time"
    if callee.startswith("unordered_"):
        return f"{callee} has no defined iteration order"
    return f"{callee} compares pointers, an order that varies between runs"


def allowed(addr, callee):
    return (addr, callee) in ALLOWLIST or (addr, callee.split(" ")[0]) in ALLOWLIST


def lint(root, rev_range=None, everything=False):
    """[(path, line, function, address, what)] for the added lines, or every line with everything."""
    listed = load_game_logic(root)
    if everything:
        paths = [p for p in git(root, "ls-files").splitlines() if Path(p).suffix in C_SUFFIXES]
        lines_by_path = {}
    else:
        lines_by_path = added_lines(root, rev_range)
        paths = sorted(p for p in lines_by_path if Path(p).suffix in C_SUFFIXES)
    findings = []
    for path in paths:
        text = (Path(root) / path).read_text(errors="replace") if everything \
            else new_text(root, rev_range, path)
        added = lines_by_path.get(path, set())
        if not everything and not added:
            continue
        masked_lines = mask_c(text).splitlines()
        for addr, name, first, last in blocks_of(text):
            new_block = not everything and first in added and path.startswith(GAME_LOGIC_DIRS)
            if addr not in listed and not new_block:
                continue
            checked = range(first, last + 1) if everything else sorted(added & set(range(first, last + 1)))
            for ln, callee in check_block(masked_lines[first - 1:last], first, last, checked):
                if not allowed(addr, callee):
                    findings.append((path, ln, listed.get(addr) or name or "?", addr, describe(callee)))
    return sorted(set(findings))


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("range", nargs="?", default=DEFAULT_RANGE)
    ap.add_argument("--all", action="store_true", help="check every line of every listed function")
    ap.add_argument("--show-allowed", action="store_true", help="with --all, ignore ALLOWLIST")
    args = ap.parse_args(argv)
    root = git(".", "rev-parse", "--show-toplevel").strip()
    if args.show_allowed:
        ALLOWLIST.clear()
    findings = lint(root, args.range, args.all)
    for path, line, func, addr, what in findings:
        print(f"{path}:{line}: {func} {addr:#x}: {what}")
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())

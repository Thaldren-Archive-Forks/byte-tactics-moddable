"""Check that every sim heap allocation site allocates from the sim block.

    uv run tools/simalloc.py            # fail if a sim heap site uses a non-sim allocator
    uv run tools/simalloc.py --quiet    # print only the summary line
    uv run tools/test_simalloc.py       # tests, on hand-written sources

This enforces #184 on the rows of data/state_classes.csv with kind `heap` and class
`sim`. Each row names an allocating function (`Function: allocator type-or-tag`) and
its address. The function is found by its `// FUNCTION:` marker (tools/sources.py);
a row with no address is found by the function's name. A row fails when

  * the function body allocates the row's type or tag without the sim block:
    `new T` or `new T[` without `(kSim)`, `operator new(`, or a call of GameAlloc,
    GameAllocIgnoreTag, GameAllocShared, GameCalloc or GameRealloc (a call that names
    the row's tag is the site; if no call names it, every call of the family is); or
  * the body has no sim allocation for the row: `new (kSim) T`, or SimAlloc/SimRealloc.
    A row the 3.1c binary inlined from a constructor or helper has none in its own
    body, so the calls of the body are followed, two levels deep, to functions defined
    in the same file or in the headers it includes, and a sim allocation there counts.
    A non-sim allocation there fails the row too.

A call that names no sim row's tag is skipped when it is no sim site: its literal tag is
the tag of a non-sim row (platform, presentation, mixed or view) of the same function and
address, or its tag is not a literal and the pointer it returns is freed in the same
function (a temporary). A call that names a sim row's tag is never skipped.

tools/stateclasses.py needs a clang build and the compile database. This check reads
source text only, so it runs in seconds without a build, which is why it is a tool of
its own. data/simalloc_allow.csv lists rows to skip (name, reason).
"""

import argparse
import csv
import re
import sys
from pathlib import Path

import sources

ROOT = sources.ROOT
STATE_CLASSES = ROOT / "data" / "state_classes.csv"
ALLOW = ROOT / "data" / "simalloc_allow.csv"
DEPTH = 2

C_FAMILY = re.compile(r"\b(GameAlloc\w*|GameCalloc|GameRealloc\w*)\s*\(")
OPERATOR_NEW = re.compile(r"\boperator\s+new(?:\s*\[\s*\])?\s*\(")
SIM_CALL = re.compile(r"\bSim(?:Alloc(?:Tagged)?|Realloc)\s*\(")
INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)
CALL = re.compile(r"\b([A-Za-z_~][\w]*(?:::~?[A-Za-z_]\w*)*)\s*\(")
KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "new", "delete", "catch", "operator"}
ROW_NAME = re.compile(r"^(?P<function>.+?): (?P<allocator>new\[\]|new|operator new|\w+)(?: (?P<what>.*?))?(?: #\d+)?$")


class Unit:
    """A source file with its comments blanked, and the same text with strings blanked too."""

    def __init__(self, path: Path, display: str, text: str | None = None):
        self.path = path
        self.display = display
        raw = path.read_text(errors="replace") if text is None else text
        self.code, self.bare = blank(raw)
        self.starts = [0]
        for i, c in enumerate(self.code):
            if c == "\n":
                self.starts.append(i + 1)

    def line_of(self, offset: int) -> int:
        lo, hi = 0, len(self.starts)
        while hi - lo > 1:
            mid = (lo + hi) // 2
            if self.starts[mid] <= offset:
                lo = mid
            else:
                hi = mid
        return lo + 1

    def line_text(self, line: int) -> str:
        end = self.starts[line] if line < len(self.starts) else len(self.code)
        return self.code[self.starts[line - 1]:end].strip()


def blank(text: str) -> tuple[str, str]:
    """The text with comments blanked, and with strings and character literals blanked as well."""
    code, bare = [], []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        two = text[i:i + 2]
        if two == "//" or two == "/*":
            end = text.find("\n", i) if two == "//" else text.find("*/", i)
            end = n if end < 0 else (end if two == "//" else end + 2)
            for ch in text[i:end]:
                code.append(ch if ch == "\n" else " ")
                bare.append(ch if ch == "\n" else " ")
            i = end
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            j = min(j + 1, n)
            code.append(text[i:j])
            bare.append(c + " " * (j - i - 2) + c if j - i >= 2 else text[i:j])
            i = j
        else:
            code.append(c)
            bare.append(c)
            i += 1
    return "".join(code), "".join(bare)


class Function:
    def __init__(self, unit: Unit, name: str, start: int, end: int):
        self.unit, self.name, self.start, self.end = unit, name, start, end

    @property
    def body(self) -> str:
        return self.unit.code[self.start:self.end]


def body_end(unit: Unit, start: int) -> int | None:
    """The offset after the brace that closes the first brace block at or after start."""
    depth = 0
    for i in range(start, len(unit.bare)):
        c = unit.bare[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i + 1
        elif c == ";" and depth == 0:
            return None
    return None


def marked_functions(unit: Unit) -> list[tuple[int, str, Function]]:
    """(address, name, function) for each // FUNCTION: marker of a file."""
    raw = unit.path.read_text(errors="replace") if unit.path.exists() else ""
    names = sources.annotations(unit.path) if raw else []
    out = []
    markers = [m for m in re.finditer(r"^\s*//\s*FUNCTION:\s*(0x[0-9a-fA-F]+)", raw, re.M)]
    for m, (_, name) in zip(markers, names):
        end = body_end(unit, m.end())
        if end is not None:
            out.append((int(m.group(1), 16), name, Function(unit, name, m.end(), end)))
    return out


class Index:
    def __init__(self, units: list[Unit]):
        self.units = units
        self.by_address: dict[int, list[Function]] = {}
        self.by_name: dict[str, list[Function]] = {}
        self.headers: dict[Path, Unit] = {}
        for unit in units:
            for address, name, function in marked_functions(unit):
                self.by_address.setdefault(address, []).append(function)
                self.by_name.setdefault(name, []).append(function)
        seen = set()
        for unit in units:
            for header in self.included(unit):
                if header.path in seen:
                    continue
                seen.add(header.path)
                for address, name, function in marked_functions(header):
                    self.by_address.setdefault(address, []).append(function)
                    self.by_name.setdefault(name, []).append(function)

    def named(self, name: str) -> list[Function]:
        found = list(self.by_name.get(name, []))
        for key, functions in self.by_name.items():
            if key.endswith("::" + name):
                found += functions
        if not found:
            for unit in self.units:
                found += definitions(unit, name)
        return found

    def included(self, unit: Unit) -> list[Unit]:
        found = []
        for m in INCLUDE.finditer(unit.path.read_text(errors="replace") if unit.path.exists() else ""):
            for base in (unit.path.parent, ROOT / "src", ROOT / "include", ROOT):
                path = (base / m.group(1)).resolve()
                if path.exists():
                    if path not in self.headers:
                        try:
                            display = str(path.relative_to(ROOT))
                        except ValueError:
                            display = str(path)
                        self.headers[path] = Unit(path, display)
                    found.append(self.headers[path])
                    break
        return found


def definitions(unit: Unit, name: str) -> list[Function]:
    """The bodies the file defines for a function name (the last part of a qualified name counts)."""
    last = name.split("::")[-1]
    pattern = re.compile(r"(?<![\w.>])(?:[\w:]*::)?" + re.escape(last) + r"\s*\(")
    found = []
    for m in pattern.finditer(unit.bare):
        depth, i = 1, m.end()
        while i < len(unit.bare) and depth:
            depth += {"(": 1, ")": -1}.get(unit.bare[i], 0)
            i += 1
        rest = re.match(r"\s*(?:const\s*)?(?::[^{;]*)?\{", unit.bare[i:])
        if rest:
            end = body_end(unit, i + rest.end() - 1)
            if end:
                found.append(Function(unit, name, m.start(), end))
    return found


def type_pattern(what: str) -> str:
    return r"\s+".join(re.escape(p) for p in what.split())


def plain_new(body: str, row: dict) -> list[int]:
    if not row["what"]:
        return []
    t = type_pattern(row["what"])
    return [m.start() for m in re.finditer(r"\bnew\s+(?:struct\s+|class\s+)?" + t + r"\b", body)]


def non_sim_calls(body: str, row: dict, others: frozenset = frozenset()) -> list[int]:
    if row["allocator"] in ("new", "new[]"):
        return plain_new(body, row)
    if row["allocator"] == "operator new":
        return [m.start() for m in OPERATOR_NEW.finditer(body)]
    calls = [m for m in C_FAMILY.finditer(body)]
    tag = row["what"]
    named = [m for m in calls if tag and tag in call_text(body, m.end())]
    if named:
        return [m.start() for m in named]
    return [m.start() for m in calls if not other_site(body, m, others)]


def other_site(body: str, call: re.Match, others: set) -> bool:
    """Whether a call that names no sim row's tag is not a sim site.

    It is not when its literal tag is the tag of a non-sim row of the same function, or when
    its tag is not a literal and the pointer it returns is freed in the same function (a
    temporary, which holds no state between ticks).
    """
    args = call_text(body, call.end())
    literal = re.match(r'\s*"((?:[^"\\]|\\.)*)"', args)
    if literal:
        return literal.group(1) in others
    line_start = body.rfind("\n", 0, call.start()) + 1
    target = re.search(r"(\w+)\s*=\s*(?:\([^()]*\)\s*)?$", body[line_start:call.start()])
    if not target:
        return False
    return bool(re.search(r"\b(?:GameFree\w*|free)\s*\(\s*" + re.escape(target.group(1)) + r"\s*\)", body))


def call_text(body: str, start: int) -> str:
    depth, i = 1, start
    while i < len(body) and depth:
        depth += {"(": 1, ")": -1}.get(body[i], 0)
        i += 1
    return body[start:i]


def sim_allocates(body: str, row: dict) -> bool:
    if row["allocator"] in ("new", "new[]"):
        if not row["what"]:
            return False
        return bool(re.search(r"\bnew\s*\(\s*kSim\s*\)\s*(?:struct\s+|class\s+)?" + type_pattern(row["what"]) + r"\b", body))
    return bool(SIM_CALL.search(body))


def helpers(function: Function, index: Index, depth: int, seen: set) -> list[Function]:
    """The functions the body calls that are defined in its file or its headers, to the given depth."""
    found = []
    units = [function.unit] + index.included(function.unit)
    for m in CALL.finditer(function.body):
        name = m.group(1)
        if name in KEYWORDS or name.split("::")[-1] in KEYWORDS:
            continue
        for unit in units:
            for d in definitions(unit, name):
                key = (str(d.unit.path), d.start)
                if key in seen or (d.unit is function.unit and d.start <= function.start < d.end):
                    continue
                seen.add(key)
                found.append(d)
                if depth > 1:
                    found += helpers(d, index, depth - 1, seen)
    return found


def parse_row(row: dict) -> dict | None:
    m = ROW_NAME.match(row["name"])
    if not m:
        return None
    return {"name": row["name"], "address": row["address"].strip(), "function": m.group("function"),
            "allocator": m.group("allocator"), "what": (m.group("what") or "").strip()}


def check_row(row: dict, index: Index, others: frozenset = frozenset()) -> list[str]:
    """The failures of one row: a line each, empty when the row allocates from the sim block."""
    functions = index.by_address.get(int(row["address"], 16), []) if row["address"] else index.named(row["function"])
    if not functions:
        return [f"{row['function']}: no function found for row '{row['name']}'"]
    out = []
    for function in functions:
        unit = function.unit
        bad = non_sim_calls(function.body, row, others)
        if bad:
            line = unit.line_of(function.start + bad[0])
            out.append(f"{unit.display}:{line} {row['function']}: {unit.line_text(line)}")
            continue
        if sim_allocates(function.body, row):
            continue
        near = helpers(function, index, DEPTH, set())
        helper_bad = [(h, o) for h in near for o in non_sim_calls(h.body, row, others)]
        if helper_bad:
            h, o = helper_bad[0]
            line = h.unit.line_of(h.start + o)
            out.append(f"{h.unit.display}:{line} {row['function']}: {h.unit.line_text(line)}")
            continue
        if any(sim_allocates(h.body, row) for h in near):
            continue
        line = unit.line_of(function.start)
        want = f"new (kSim) {row['what']}" if row["allocator"] in ("new", "new[]") else "SimAlloc"
        out.append(f"{unit.display}:{line} {row['function']}: no {want} in the function or its helpers")
    return out


def sim_rows(path: Path = STATE_CLASSES) -> list[dict]:
    with path.open(newline="") as f:
        rows = [r for r in csv.DictReader(f) if r["kind"] == "heap" and r["class"] == "sim"]
    return [p for p in map(parse_row, rows) if p]


def other_tags(path: Path = STATE_CLASSES) -> dict[tuple[str, str], frozenset]:
    """The tags of the non-sim heap rows of each (function, address)."""
    found: dict[tuple[str, str], set] = {}
    with path.open(newline="") as f:
        for r in csv.DictReader(f):
            if r["kind"] == "heap" and r["class"] != "sim":
                p = parse_row(r)
                if p and p["what"]:
                    found.setdefault((p["function"], p["address"]), set()).add(p["what"])
    return {k: frozenset(v) for k, v in found.items()}


def read_allow(path: Path = ALLOW) -> set[str]:
    if not path.exists():
        return set()
    with path.open(newline="") as f:
        return {r["name"] for r in csv.DictReader(f)}


def run(rows: list[dict], index: Index, allow: set[str],
        others: dict | None = None) -> tuple[int, list[str]]:
    """(rows checked, failure lines), each failing row once, deduplicated by line."""
    failures, seen = [], set()
    checked = 0
    for row in rows:
        if row["name"] in allow:
            continue
        checked += 1
        for line in check_row(row, index, (others or {}).get((row["function"], row["address"]), frozenset())):
            if line not in seen:
                seen.add(line)
                failures.append(line)
    return checked, failures


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--quiet", action="store_true", help="print only the summary line")
    args = ap.parse_args()
    units = [Unit(p, sources.relative(p)) for p in sources.source_files()]
    rows = sim_rows()
    checked, failures = run(rows, Index(units), read_allow(), other_tags())
    if failures:
        if not args.quiet:
            print("\n".join(failures))
        print(f"simalloc: {len(failures)} failing lines in {checked} sim heap sites", file=sys.stderr)
        sys.exit(1)
    print(f"simalloc: {checked} sim heap sites, all use the sim block")


if __name__ == "__main__":
    main()

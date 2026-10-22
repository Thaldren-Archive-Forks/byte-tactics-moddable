"""Generate static_asserts on the sizes and field offsets of the types the state block holds (#348).

    uv run tools/layoutasserts.py            # write src/state/layout_asserts.cpp
    uv run tools/layoutasserts.py --write    # the same
    uv run tools/layoutasserts.py --check    # fail if the file is stale
    uv run tools/test_layoutasserts.py       # tests, on canned clang layout text

The types are Game and the types tools/typedupes.py's Scope.closure reaches from the sim and mixed
rows of data/state_classes.csv, each defined in a header (a type defined only inside a .cpp has no
header to include, so it is left out). One translation unit includes every defining header. The
sizes and offsets are those clang prints with `-Xclang -fdump-record-layouts-complete` for that
unit, packing and anonymous members included. Bit-fields, anonymous members, base classes, vptrs
and fields whose offsetof does not compile (private members) get no assert.
"""

from __future__ import annotations

import argparse
import difflib
import os
import re
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from pathlib import Path

import refcheck
import rwsets
import stateclasses
import typedupes
from rwsets import ROOT

OUT = ROOT / "src" / "state" / "layout_asserts.cpp"
OUT_TA = ROOT / "src" / "state" / "layout_asserts_ta_types.cpp"
OUT_DIR = OUT.parent
BUILD_DIR = ROOT / "build" / "modern"
LAYOUT_FLAG = stateclasses.LAYOUT_FLAG
HEADER_SUFFIXES = (".h", ".hpp")
UNIT_NAME = "unit.cpp"
FORMAT_FLAGS = ["-ferror-limit=0", "-Xclang", "-fdiagnostics-format", "-Xclang", "clang"]
IDENTIFIER = re.compile(r"^[A-Za-z_]\w*$")
RECORD_HEAD = re.compile(r"^ *0 \| (?:struct|class|union) (\S+?)( \(empty\))?$", re.M)
SIZE_LINE = re.compile(r"\[sizeof=(\d+)")
DIAGNOSTIC = re.compile(r"^(?P<file>.*?):(?P<line>\d+):\d+: error: (?P<text>.*)$", re.M)
SYSTEM_INCLUDES = ["<stddef.h>", "<stdio.h>", "<string.h>", "<stdlib.h>", "<math.h>", "<vector>", "<memory>",
                   '"../state/block.h"', '"../state/sim_allocator.h"', '"win_types.h"']
FIRST_HEADERS = ["src/util/vec3.h"]
SELF_ASSERTED = {"SimGlobals"}
PRELUDE = ['#pragma clang diagnostic ignored "-Winvalid-offsetof"', "#pragma pack(push, 1)"]


@dataclass
class Record:
    """The size of a record and its named, byte-addressed members, as clang laid them out."""
    name: str
    size: int
    fields: list[tuple[str, int]] = field(default_factory=list)


def parse_records(text: str, wanted: set[str]) -> dict[str, Record]:
    """The records of a clang layout dump named in `wanted`, with their plain named members."""
    heads = list(RECORD_HEAD.finditer(text))
    out: dict[str, Record] = {}
    for i, m in enumerate(heads):
        name = m.group(1)
        if name not in wanted or name in out:
            continue
        end = heads[i + 1].start() if i + 1 < len(heads) else len(text)
        chunk = text[m.start():end]
        size = SIZE_LINE.search(chunk)
        if not size:
            continue
        found = None if m.group(2) else stateclasses.layout_fields(chunk, name)
        members: list[tuple[str, int]] = []
        seen: set[str] = set()
        for f in (found[1] if found else []):
            if f.bits or not IDENTIFIER.match(f.name) or f.name in seen:
                continue
            seen.add(f.name)
            members.append((f.name, f.start))
        out[name] = Record(name, int(size.group(1)), members)
    return out


def emit(includes: list[str], records: list[Record]) -> str:
    """The text of the translation unit: the includes, then a size assert and an offset assert per member."""
    lines = ["#include " + inc for inc in SYSTEM_INCLUDES] + PRELUDE + ['#include "%s"' % inc for inc in includes] + [""]
    for r in sorted(records, key=lambda r: r.name):
        lines.append('static_assert(sizeof(%s) == %d, "");' % (r.name, r.size))
        for name, offset in r.fields:
            lines.append('static_assert(offsetof(%s, %s) == %d, "");' % (r.name, name, offset))
    return "\n".join(lines) + "\n"


def drop_failing(text: str, diagnostics: str) -> tuple[str, int, list[str]]:
    """The text without the lines clang rejected, how many went, and the assertion failures (kept in)."""
    lines = text.split("\n")
    bad: set[int] = set()
    failed: list[str] = []
    for m in DIAGNOSTIC.finditer(diagnostics):
        if os.path.basename(m.group("file")) != UNIT_NAME:
            continue
        n = int(m.group("line")) - 1
        if n >= len(lines) or not lines[n].startswith("static_assert"):
            continue
        if "static assertion failed" in m.group("text") or "static_assert failed" in m.group("text"):
            failed.append("%s: %s" % (lines[n], m.group("text")))
        else:
            bad.add(n)
    return "\n".join(l for i, l in enumerate(lines) if i not in bad), len(bad), failed


def is_template(files: dict, d: dict) -> bool:
    """Whether the definition follows a `template <...>` line."""
    lines = files[d["file"]].text.split("\n")
    return d["line"] >= 2 and lines[d["line"] - 2].strip().startswith("template")


def block_types(files=None, by_name=None) -> tuple[list[str], dict[str, str], list[str]]:
    """(header names, name -> defining header, cpp-only names) for the types refcheck says the block holds."""
    sys.setrecursionlimit(100000)
    files = files if files is not None else typedupes.collect()
    by_name = by_name if by_name is not None else typedupes.definitions(files)
    names = refcheck.block_structs(files, by_name, stateclasses.read_rows(), stateclasses.read_globals())
    homes: dict[str, str] = {}
    cpp_only: list[str] = []
    for name in sorted(names - SELF_ASSERTED):
        ds = [d for d in by_name.get(name, []) if not d.get("local") and not is_template(files, d)]
        if not ds:
            continue
        headers = sorted({d["file"] for d in ds if d["file"].endswith(HEADER_SUFFIXES)})
        if headers:
            homes[name] = headers[0]
        else:
            cpp_only.append(name)
    return sorted(homes), homes, cpp_only


TA_TYPES = "include/ta_types.h"


def split_units(homes: dict[str, str]) -> dict[Path, dict[str, str]]:
    """The output files and the types each holds: ta_types.h redefines types other headers define, so it is apart."""
    return {
        OUT: {n: h for n, h in homes.items() if h != TA_TYPES},
        OUT_TA: {n: h for n, h in homes.items() if h == TA_TYPES},
    }


def include_order(homes: dict[str, str]) -> list[str]:
    """The defining headers as include paths from src/state."""
    paths = sorted(set(homes.values()), key=lambda p: (p not in FIRST_HEADERS, p))
    return [os.path.relpath(ROOT / p, OUT_DIR).replace(os.sep, "/") for p in paths]


def clang_flags(build_dir: Path) -> tuple[list[str], str]:
    """The game's clang-cl flags (no source, no output, no AST dump) and the directory to run in."""
    _, argv, directory = rwsets.compile_commands(build_dir, [])[0]
    i = argv.index("--")
    dump = argv.index("-ast-dump=json")
    return argv[:dump - 1] + argv[dump + 1:i] + ["-I" + str(OUT_DIR)], directory


def run_clang(flags: list[str], directory: str, text: str, layouts: bool) -> subprocess.CompletedProcess:
    """Compile `text` with -fsyntax-only, printing record layouts when asked."""
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / UNIT_NAME
        path.write_text(text)
        return subprocess.run(flags + (LAYOUT_FLAG if layouts else []) + FORMAT_FLAGS + ["--", str(path)],
                              cwd=directory, capture_output=True, text=True, errors="replace")


def settle(text: str, flags: list[str], directory: str, run=run_clang) -> tuple[str, int]:
    """The text without the asserts clang cannot compile (private members), and how many went."""
    dropped = 0
    for _ in range(3):
        result = run(flags, directory, text, False)
        text2, n, failed = drop_failing(text, result.stderr.replace("\r", ""))
        if failed:
            raise SystemExit("layoutasserts: clang disagrees with its own layout:\n" + "\n".join(failed[:10]))
        if not n:
            return text, dropped
        text, dropped = text2, dropped + n
    return text, dropped


def generate(build_dir: Path, run=run_clang) -> tuple[dict[Path, str], dict]:
    """(output file -> its text, a summary of what the files cover and what they leave out)."""
    names, homes, cpp_only = block_types()
    flags, directory = clang_flags(build_dir)
    texts: dict[Path, str] = {}
    info = {"types": 0, "asserts": 0, "dropped": 0, "cpp_only": cpp_only, "missing": []}
    for path, part in split_units(homes).items():
        includes = include_order(part)
        probe = run(flags, directory, emit(includes, []), True)
        errors = DIAGNOSTIC.findall(probe.stderr)
        if errors or probe.returncode:
            raise SystemExit("layoutasserts: the headers of %s do not compile together:\n%s" % (
                path.relative_to(ROOT), "\n".join(probe.stderr.splitlines()[:20])))
        records = parse_records(probe.stdout, set(part))
        info["missing"] += [n for n in part if n not in records]
        text, dropped = settle(emit(includes, list(records.values())), flags, directory, run)
        texts[path] = text
        info["types"] += len(records)
        info["asserts"] += text.count("static_assert")
        info["dropped"] += dropped
    return texts, info


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--write", action="store_true", help="write src/state/layout_asserts.cpp (the default)")
    parser.add_argument("--check", action="store_true", help="fail if src/state/layout_asserts.cpp is stale")
    parser.add_argument("--build-dir", type=Path, default=BUILD_DIR)
    args = parser.parse_args()
    started = time.time()
    texts, info = generate(args.build_dir)
    summary = "%d types, %d asserts, %d members dropped, %d left out (defined only in a .cpp), %d without a layout" % (
        info["types"], info["asserts"], info["dropped"], len(info["cpp_only"]), len(info["missing"]))
    if args.check:
        stale = False
        for path, text in texts.items():
            have = path.read_text() if path.exists() else ""
            if have != text:
                stale = True
                sys.stdout.writelines(list(difflib.unified_diff(have.splitlines(True), text.splitlines(True),
                                                                "committed", "fresh", n=0))[:40])
                print("layoutasserts: %s is out of date; run `uv run tools/layoutasserts.py --write` and commit it" %
                      path.relative_to(ROOT))
        if stale:
            sys.exit(1)
        print("layoutasserts: up to date (%s), %.0fs" % (summary, time.time() - started))
        return
    for path, text in texts.items():
        path.write_text(text)
    print("layoutasserts: wrote %s (%s), %.0fs" % (", ".join(str(p.relative_to(ROOT)) for p in texts), summary,
                                                  time.time() - started))
    print("layoutasserts: left out: " + " ".join(info["cpp_only"]))
    if info["missing"]:
        print("layoutasserts: no layout: " + " ".join(info["missing"]))


if __name__ == "__main__":
    main()

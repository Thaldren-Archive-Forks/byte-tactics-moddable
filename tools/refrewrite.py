"""Rewrite raw pointer fields to Ref<T>, for the rows of data/ref_exceptions.csv.

    uv run tools/refrewrite.py --issue '#349'
    uv run tools/refrewrite.py --struct Unit --struct Order --dry-run
    uv run tools/refrewrite.py --struct Unit --exceptions build/scratch/348/x.csv --dry-run

Selects the rows of kind `pointer` that match --issue or any --struct (a row
matching either is taken), finds each field's declaration with clang-cl
(`-Xclang -ast-dump=json`, with the file's command from the compile database)
and splices `Unit* carrier;` into `Ref<Unit> carrier;`, changing nothing else
in the file. Each file in the row's `file` column is rewritten (several files
are space-separated). Adds the include of interfaces/ref.h when it is missing
and drops the rewritten rows from the exceptions file.

Handled: `T *a`, `T* a`, `struct T* a`, `const T* a` (Ref<const T>), arrays
(`T* a[4]`), `T *a, *b;` (split into one declaration each), and `T** a`
(Ref<T*>, reported). Skipped and reported, never changed: fields declared
through macros, function pointers, `T* const a`, typedef'd pointers and
anything the spelling does not show as a plain pointer declarator.

--dry-run prints a unified diff and writes nothing.
"""

import argparse
import csv
import difflib
import io
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

import rwsets
from sources import ROOT

EXCEPTIONS = ROOT / "data/ref_exceptions.csv"
REF_HEADER = "interfaces/ref.h"
BUILD_DIR = ROOT / "build/modern"
COLUMNS = ["struct", "field", "kind", "file", "issue"]

PLAIN_FLAGS = ["clang-cl", "--target=i686-pc-windows-msvc", "/nologo", "-TP", "-fms-compatibility",
               "-fms-extensions", "-fsyntax-only", "-Xclang", "-ast-dump=json"]

POINTER_PREFIX = re.compile(r"^(?P<base>.*?)(?P<ptr>(?:\*\s*)+)$", re.S)
STAR_ONLY = re.compile(r"^\s*,\s*(?P<ptr>(?:\*\s*)+)$")


@dataclass
class FieldInfo:
    name: str
    file: str
    begin: int
    end: int
    name_off: int
    qual_type: str
    macro: bool


@dataclass
class Result:
    rewritten: list[tuple[str, str, str]] = field(default_factory=list)
    skipped: list[tuple[str, str, str, str]] = field(default_factory=list)
    notes: list[str] = field(default_factory=list)


def real(path: str, directory: str) -> str:
    return os.path.realpath(os.path.join(directory, path))


def macro_loc(loc: dict) -> bool:
    return "spellingLoc" in loc or "expansionLoc" in loc


def bare_locs(loc: dict):
    if "spellingLoc" in loc or "expansionLoc" in loc:
        for key in ("spellingLoc", "expansionLoc"):
            if key in loc:
                yield loc[key]
    else:
        yield loc


def dump_fields(ast: dict, main_file: str, directory: str, wanted: dict[str, set[str]]) -> dict:
    """{(struct, real file): [FieldInfo]} for each wanted struct name and the files it may be defined in."""
    found: dict[tuple[str, str], list[FieldInfo]] = {}
    current = [real(main_file, directory)]

    def track(loc: dict) -> None:
        for bare in bare_locs(loc):
            if "file" in bare:
                current[0] = real(bare["file"], directory)

    stack = [(ast, None)]
    while stack:
        node, owner = stack.pop()
        loc = node.get("loc")
        if loc:
            track(loc)
        file_here = current[0]
        begin = end = None
        macro = bool(loc and macro_loc(loc))
        rng = node.get("range")
        if rng:
            begin_loc, end_loc = rng.get("begin", {}), rng.get("end", {})
            macro = macro or macro_loc(begin_loc) or macro_loc(end_loc)
            track(begin_loc)
            begin = next(bare_locs(begin_loc), {}).get("offset")
            file_begin = current[0]
            track(end_loc)
            bare_end = next(bare_locs(end_loc), {})
            if "offset" in bare_end:
                end = bare_end["offset"] + bare_end.get("tokLen", 0)
            file_here = file_begin
        kind = node.get("kind")
        child_owner = None
        if kind in ("RecordDecl", "CXXRecordDecl") and node.get("completeDefinition") and not node.get("isImplicit"):
            name = node.get("name")
            if name in wanted and file_here in wanted[name]:
                child_owner = (name, file_here)
            elif not name and owner:
                child_owner = owner
        elif kind == "FieldDecl" and owner and loc is not None:
            bare_loc = next(bare_locs(loc), {})
            if "offset" in bare_loc and begin is not None and end is not None and "name" in node:
                found.setdefault(owner, []).append(FieldInfo(
                    node["name"], file_here, begin, end, bare_loc["offset"],
                    node.get("type", {}).get("qualType", ""), macro))
        if kind == "FieldDecl":
            continue
        inner = node.get("inner") or []
        for child in reversed(inner):
            stack.append((child, child_owner))
    return found


def run_clang(argv: list[str], directory: str) -> dict:
    proc = subprocess.run(argv, cwd=directory, capture_output=True, text=True, errors="replace")
    if not proc.stdout.strip():
        raise SystemExit(f"clang-cl produced no AST for {argv[-1]}:\n{proc.stderr[-2000:]}")
    return json.loads(proc.stdout)


def tu_for(path: str, build_dir: Path) -> tuple[list[str], str, str]:
    """(argv, directory, main file) of a translation unit that defines `path` (itself, or one that includes it)."""
    rel = os.path.relpath(path, ROOT)
    if not (build_dir / "compile_commands.json").exists():
        raise SystemExit(f"{build_dir}/compile_commands.json is missing; configure build/modern first")
    if path.endswith((".cpp", ".c")):
        commands = rwsets.compile_commands(build_dir, [rel])
        if commands:
            return commands[0][1], commands[0][2], commands[0][1][-1]
    base = os.path.basename(path)
    pattern = re.compile(r'#\s*include\s*["<][^">]*' + re.escape(base) + r'[">]')
    commands = rwsets.compile_commands(build_dir, [])
    for source, argv, directory in commands:
        if pattern.search((ROOT / source).read_text(errors="replace")):
            return argv, directory, argv[-1]
    if commands:
        source, argv, directory = commands[0]
        return argv[:-2] + ["-include", path, "--", argv[-1]], directory, argv[-1]
    raise SystemExit(f"no translation unit for {rel}")


def split_group(text: str, group: list[FieldInfo]):
    """The declarators of one declaration: (base, [(ptr, name, suffix, field)]) or a reason string."""
    group = sorted(group, key=lambda f: f.name_off)
    first = group[0]
    head = POINTER_PREFIX.match(text[first.begin:first.name_off])
    if not head:
        return "not a plain pointer declarator"
    base = head.group("base").rstrip()
    out = []
    ptr = head.group("ptr")
    for i, info in enumerate(group):
        if i:
            gap = STAR_ONLY.match(text[group[i - 1].end:info.name_off])
            ptr = gap.group("ptr") if gap else ""
        suffix = text[info.name_off + len(info.name):info.end]
        out.append((ptr, info.name, suffix, info))
    return base, out


def ref_type(base: str, ptr: str) -> str:
    levels = ptr.count("*")
    return f"Ref<{base}{'*' * (levels - 1)}>"


def skip_reason(info: FieldInfo, ptr: str) -> str | None:
    if info.macro:
        return "declared through a macro"
    if "(" in info.qual_type:
        return "function pointer"
    if "*" not in info.qual_type:
        return "not a pointer type"
    if not ptr:
        return "typedef'd pointer"
    if re.search(r"\b(const|volatile)\b", ptr):
        return "qualified pointer"
    return None


def plan_edits(text: str, infos: list[FieldInfo], names: set[str], struct: str, path: str, result: Result):
    """(start, end, replacement) edits for the wanted fields among `infos`, and the names rewritten."""
    edits = []
    done = set()
    groups: dict[int, list[FieldInfo]] = {}
    for info in infos:
        groups.setdefault(info.begin, []).append(info)
    for begin, group in groups.items():
        group = sorted(group, key=lambda f: f.name_off)
        if not any(f.name in names for f in group):
            continue
        if any(f.macro for f in group):
            for f in group:
                if f.name in names:
                    result.skipped.append((struct, f.name, path, "declared through a macro"))
            continue
        split = split_group(text, group)
        if isinstance(split, str):
            for f in group:
                if f.name in names:
                    result.skipped.append((struct, f.name, path, split))
            continue
        base, decls = split
        ok = {}
        for ptr, name, suffix, info in decls:
            if name not in names:
                continue
            reason = skip_reason(info, ptr)
            if reason:
                result.skipped.append((struct, name, path, reason))
            else:
                ok[name] = ptr
        if not ok:
            continue
        for name, ptr in ok.items():
            if ptr.count("*") > 1:
                result.notes.append(f"{path}: {struct}::{name} is a pointer to pointer, now {ref_type(base, ptr)}")
        if len(decls) == 1:
            ptr = decls[0][0]
            tail = text[begin:group[0].name_off][len(base):]
            last_star = tail.rindex("*") + 1
            after = tail[last_star:]
            gap = "" if after else " "
            edits.append((begin, begin + len(base) + last_star, ref_type(base, ptr) + gap))
            done.update(ok)
            continue
        last = max(f.end for f in group)
        semi = text.index(";", last)
        line_start = text.rfind("\n", 0, begin) + 1
        indent = re.match(r"[ \t]*", text[line_start:begin]).group(0)
        parts = []
        for ptr, name, suffix, info in decls:
            if name in ok:
                parts.append(f"{ref_type(base, ptr)} {name}{suffix};")
            else:
                parts.append(f"{base} {ptr}{name}{suffix};")
        edits.append((begin, semi + 1, ("\n" + indent).join(parts)))
        done.update(ok)
    return edits, done


def apply_edits(text: str, edits: list[tuple[int, int, str]]) -> str:
    for start, end, replacement in sorted(edits, reverse=True):
        text = text[:start] + replacement + text[end:]
    return text


def add_include(text: str, path: Path, header: Path) -> str:
    relative = os.path.relpath(header, path.parent).replace(os.sep, "/")
    if re.search(r'#\s*include\s*["<][^">]*\bref\.h[">]', text):
        return text
    line = f'#include "{relative}"'
    newline = "\r\n" if "\r\n" in text else "\n"
    includes = list(re.finditer(r"^[ \t]*#\s*include[^\n]*\n", text, re.M))
    if includes:
        at = includes[-1].end()
    else:
        pragma = re.search(r"^[ \t]*#\s*pragma once[^\n]*\n", text, re.M)
        if pragma:
            return text[:pragma.end()] + newline + line + newline + text[pragma.end():]
        guard = re.search(r"^#\s*define\s+\w+[^\n]*\n", text, re.M)
        at = guard.end() if guard and text.lstrip().startswith("#ifndef") else 0
    if at and not text[:at].endswith("\n"):
        line = newline + line
    return text[:at] + line + newline + text[at:]


def read_rows(path: Path) -> tuple[list[str], list[dict]]:
    lines = path.read_text().splitlines(True)
    rows = []
    for number, line in enumerate(lines):
        if number == 0:
            continue
        values = next(csv.reader([line]))
        rows.append(dict(zip(COLUMNS, values), line=number))
    return lines, rows


def select(rows: list[dict], issue: str | None, structs: list[str]) -> list[dict]:
    return [r for r in rows if r["kind"] == "pointer"
            and ((issue and r["issue"] == issue) or r["struct"] in structs)]


def rewrite(rows: list[dict], build_dir: Path, root: Path, plain: bool, header: Path):
    """(new text per file, rows fully rewritten, Result)."""
    result = Result()
    wants: dict[str, dict[str, set[str]]] = {}
    for row in rows:
        for file in row["file"].split():
            wants.setdefault(str(root / file), {}).setdefault(row["struct"], set()).add(row["field"])
    texts = {path: Path(path).read_text(encoding="latin-1", newline="") for path in wants}
    edits: dict[str, list] = {path: [] for path in wants}
    done: dict[tuple[str, str, str], bool] = {}
    pending = dict(wants)
    while pending:
        path = next(iter(pending))
        if plain:
            argv, directory, main = PLAIN_FLAGS + ["--", path], str(root), path
        else:
            argv, directory, main = tu_for(path, build_dir)
        covered = {path: pending[path]}
        wanted: dict[str, set[str]] = {}
        for p, structs in covered.items():
            for struct in structs:
                wanted.setdefault(struct, set()).add(os.path.realpath(p))
        found = dump_fields(run_clang(argv, directory), main, directory, wanted)
        for p, structs in covered.items():
            for struct, names in sorted(structs.items()):
                infos = found.get((struct, os.path.realpath(p)), [])
                if not infos:
                    for name in sorted(names):
                        result.skipped.append((struct, name, os.path.relpath(p, root), "struct definition not found"))
                    continue
                have = {i.name for i in infos}
                for name in sorted(names - have):
                    result.skipped.append((struct, name, os.path.relpath(p, root), "field not found"))
                rel = os.path.relpath(p, root)
                new, rewritten = plan_edits(texts[p], infos, names & have, struct, rel, result)
                edits[p].extend(new)
                for name in rewritten:
                    done[(struct, name, p)] = True
        for p in covered:
            del pending[p]
    new_texts = {}
    for path, text in texts.items():
        if edits[path]:
            changed = apply_edits(text, edits[path])
            new_texts[path] = add_include(changed, Path(path), header)
    dropped = []
    for row in rows:
        files = [str(root / f) for f in row["file"].split()]
        if all(done.get((row["struct"], row["field"], f)) for f in files):
            dropped.append(row)
            for f in files:
                result.rewritten.append((row["struct"], row["field"], os.path.relpath(f, root)))
    return new_texts, dropped, result


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--issue", help="rewrite the pointer rows of this issue, such as '#349'")
    parser.add_argument("--struct", action="append", default=[], help="rewrite the pointer rows of this struct")
    parser.add_argument("--exceptions", type=Path, default=EXCEPTIONS)
    parser.add_argument("--build-dir", type=Path, default=BUILD_DIR)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--plain", action="store_true", help="compile each file alone, with no compile database")
    parser.add_argument("--dry-run", action="store_true", help="print a unified diff and write nothing")
    args = parser.parse_args(argv)
    if not args.issue and not args.struct:
        parser.error("give --issue or --struct")

    lines, rows = read_rows(args.exceptions)
    chosen = select(rows, args.issue, args.struct)
    if not chosen:
        print("no pointer rows selected")
        return 0
    root = args.root.resolve()
    header = root / REF_HEADER
    new_texts, dropped, result = rewrite(chosen, args.build_dir, root, args.plain, header)

    drop_lines = {row["line"] for row in dropped}
    new_csv = "".join(line for number, line in enumerate(lines) if number not in drop_lines)
    outputs = {str(args.exceptions): (args.exceptions.read_text(), new_csv)}
    for path, text in new_texts.items():
        outputs[path] = (Path(path).read_text(encoding="latin-1", newline=""), text)

    if args.dry_run:
        for path, (old, new) in sorted(outputs.items()):
            sys.stdout.writelines(difflib.unified_diff(
                old.splitlines(True), new.splitlines(True),
                f"a/{os.path.relpath(path, root)}", f"b/{os.path.relpath(path, root)}"))
    else:
        for path, (old, new) in outputs.items():
            if old != new:
                if path in new_texts:
                    Path(path).write_bytes(new.encode("latin-1"))
                else:
                    Path(path).write_text(new)

    summary = io.StringIO()
    print(f"rewritten: {len(result.rewritten)} field(s) in {len(new_texts)} file(s); "
          f"skipped: {len(result.skipped)}; rows dropped: {len(dropped)}", file=summary)
    for struct, name, file in result.rewritten:
        print(f"  rewritten {struct}::{name} ({file})", file=summary)
    for struct, name, file, reason in result.skipped:
        print(f"  skipped {struct}::{name} ({file}): {reason}", file=summary)
    for note in result.notes:
        print(f"  note: {note}", file=summary)
    sys.stderr.write(summary.getvalue())
    return 0


if __name__ == "__main__":
    sys.exit(main())

"""Compare the interface headers with a baseline and fail on a change that has no version bump.

    uv run tools/abidiff.py                      # baseline: newest interfaces-v* tag, else the merge base with origin/moddable
    uv run tools/abidiff.py --base REF           # baseline: the headers at REF
    uv run tools/abidiff.py --report-standins    # list the int*, void* and char* in slot signatures
    uv run tools/test_abidiff.py                 # tests, on synthetic header pairs

For each interfaces/*.h that has `<NAME>_VERSION_MAJOR` and `_MINOR` macros, the tool compares the
header with the one at the baseline and prints one line per interface. It exits non-zero when one
fails. Headers without version macros (seams.h) are skipped.

What is compared, per struct or class the header defines:

  slots    the function-pointer fields of a struct and the virtual methods of a class: name, order,
           full signature and calling convention
  layout   the member offsets, member types and size, for a struct with data members or a class with
           a vtable pointer, bases or data members (a pure function table is covered by its slots)
  version  the `_VERSION_MAJOR` and `_VERSION_MINOR` macros

Every change is classed, and the class decides which bump it needs:

  additive  a slot or a field appended at the end, a new struct, or a stand-in retype (below):
            needs a minor bump, or a major bump
  breaking  a slot removed, reordered, inserted before the end or retyped: needs a major bump and an
            adapter; until #9 any major bump with a change fails with `adapter required (#9)`
  layout    a field that moves or changes size: fails whatever the bump

A bump with nothing changed is fine. A version that goes down fails.

A stand-in retype is a parameter or return type that was exactly `int*`, `void*` or `char*` (const
and volatile included) and is now a pointer to a struct, class or union. It has the same size and goes
in the same register or stack slot, so it is additive. Any other retype is breaking: one struct pointer
for another, a different width, a value for a pointer, a different calling convention. Each stand-in
retype is printed as its own line.

Layouts and signatures come from clang-cl (LLVM 22.1.8, as cmake/clang-cl-i686.cmake) with the flags
of the modern build, on a one-line translation unit that includes the header: `-Xclang
-fdump-record-layouts-complete` for the layouts and `-Xclang -ast-dump=json` for the slot types. No build
directory is needed.

The baseline headers are read with `git show REF:interfaces/F.h` into a temporary directory. REF is
the newest `interfaces-v*` tag; until one exists, the merge base of HEAD and origin/moddable.
"""

import argparse
import json
import re
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASE_BRANCH = "origin/moddable"
CLANG = ["clang-cl", "--target=i686-pc-windows-msvc", "/nologo", "-TP", "-fms-compatibility",
         "-fms-extensions", "-fms-compatibility-version=11.00", "-fsyntax-only",
         "-Xclang", "-fdump-record-layouts-complete", "-Xclang", "-ast-dump=json"]
MAJOR = re.compile(r"^\s*#\s*define\s+(\w+)_VERSION_MAJOR\s+(\d+)", re.M)
MINOR = re.compile(r"^\s*#\s*define\s+(\w+)_VERSION_MINOR\s+(\d+)", re.M)
CALLCONV = re.compile(r"\s*__attribute__\(\((\w+)\)\)\s*$")
LAYOUT_START = re.compile(r"^\*\*\* Dumping AST Record Layout\s*$", re.M)
LAYOUT_MEMBER = re.compile(r"^\s*(\d+(?::\d+-\d+)?)\s*\|(\s*)(.*?)\s*$")
LAYOUT_SIZE = re.compile(r"^\s*\|\s*\[sizeof=(\d+), align=(\d+)")
STAND_IN = re.compile(r"(?:(?:const|volatile) )*(?:int|void|char) \*(?: (?:const|volatile))*")
STRUCT_POINTER = re.compile(r"(?:(?:const|volatile) )*([A-Za-z_]\w*) \*(?: (?:const|volatile))*")
JSON_START = re.compile(r'^\{\n  "id"', re.M)


@dataclass(frozen=True)
class Signature:
    ret: str
    params: tuple
    conv: str

    def text(self) -> str:
        return "%s (%s) %s" % (self.ret, ", ".join(self.params), self.conv or "cdecl")


@dataclass
class Interface:
    name: str = ""
    major: int = 0
    minor: int = 0
    slots: dict = field(default_factory=dict)
    layouts: dict = field(default_factory=dict)
    records: set = field(default_factory=set)


@dataclass
class Change:
    kind: str
    text: str


@dataclass
class Result:
    name: str
    ok: bool
    summary: str
    details: list = field(default_factory=list)


def split_top(text: str) -> list[str]:
    """The comma-separated parts of a parameter list, ignoring commas inside brackets."""
    parts, depth, start = [], 0, 0
    for i, c in enumerate(text):
        if c in "<([":
            depth += 1
        elif c in ">)]":
            depth -= 1
        elif c == "," and depth == 0:
            parts.append(text[start:i].strip())
            start = i + 1
    if text.strip():
        parts.append(text[start:].strip())
    return parts


def parse_signature(text: str, is_pointer: bool) -> Signature:
    """A Signature from clang's type text: `R (*)(P, Q) __attribute__((cc))`, or `R (P, Q)` for a method."""
    conv = ""
    found = CALLCONV.search(text)
    if found:
        conv = found.group(1)
        text = text[:found.start()]
    opener = "(*)(" if is_pointer else " ("
    head = text.index(opener)
    ret = text[:head].strip()
    body = text[head + len(opener):]
    depth = 1
    for i, c in enumerate(body):
        depth += (c == "(") - (c == ")")
        if depth == 0:
            body = body[:i]
            break
    return Signature(ret, tuple(split_top(body)), conv)


def is_function_pointer(qual: str) -> bool:
    return "(*)(" in qual


def clang_dump(header: Path, scratch: Path) -> tuple[dict, str]:
    """clang-cl's syntax tree and record layouts of a translation unit that includes the header."""
    scratch.mkdir(parents=True, exist_ok=True)
    unit = scratch / "unit.cpp"
    unit.write_text('#include "%s"\n' % header.as_posix())
    run = subprocess.run(CLANG + ["--", str(unit)], capture_output=True, text=True, errors="replace")
    found = JSON_START.search(run.stdout)
    if run.returncode != 0 or found is None:
        raise SystemExit("%s: clang-cl failed\n%s" % (header, run.stderr[-2000:]))
    return json.loads(run.stdout[found.start():]), run.stderr + run.stdout[:found.start()]


def parse_layouts(text: str) -> dict:
    """Record name -> (members, size, align), from the -fdump-record-layouts-complete text."""
    out = {}
    for block in LAYOUT_START.split(text)[1:]:
        block = block.split("\n\n")[0]
        members, size, name = [], None, None
        for line in block.split("\n"):
            size_line = LAYOUT_SIZE.match(line)
            if size_line:
                size = (int(size_line.group(1)), int(size_line.group(2)))
                continue
            found = LAYOUT_MEMBER.match(line)
            if not found:
                continue
            if name is None:
                name = re.sub(r"^(struct|class|union) ", "", found.group(3))
                continue
            members.append((found.group(1), len(found.group(2)), found.group(3)))
        if name is not None and size is not None:
            out[name] = (tuple(members), size[0], size[1])
    return out


def load_interface(header: Path, scratch: Path) -> Interface:
    """The versions, slots and layouts of one interface header."""
    text = header.read_text()
    major, minor = MAJOR.search(text), MINOR.search(text)
    iface = Interface(name=header.stem)
    if major is None or minor is None:
        return iface
    iface.major, iface.minor = int(major.group(2)), int(minor.group(2))
    tree, layout_text = clang_dump(header, scratch)
    layouts = parse_layouts(layout_text)
    for node in tree["inner"]:
        if node.get("isImplicit") or node["kind"] != "CXXRecordDecl" or "name" not in node:
            continue
        iface.records.add(node["name"])
        if not node.get("completeDefinition"):
            continue
        slots, data = [], False
        for member in node.get("inner", []):
            kind, qual = member["kind"], member.get("type", {}).get("qualType", "")
            if kind == "FieldDecl" and is_function_pointer(qual):
                slots.append((member["name"], parse_signature(qual, True)))
            elif kind == "FieldDecl":
                data = True
            elif kind == "CXXMethodDecl" and member.get("virtual"):
                slots.append((member["name"], parse_signature(qual, False)))
        has_vtable = any(m["kind"] == "CXXMethodDecl" and m.get("virtual") for m in node.get("inner", []))
        iface.slots[node["name"]] = slots
        if data or has_vtable or node.get("bases"):
            iface.layouts[node["name"]] = layouts.get(node["name"])
    return iface


def is_stand_in_retype(old: str, new: str, records: set) -> bool:
    """Whether an `int*`, `void*` or `char*` became a pointer to a struct, class or union."""
    target = STRUCT_POINTER.fullmatch(new)
    return bool(STAND_IN.fullmatch(old)) and target is not None and target.group(1) in records


def compare_signature(where: str, old: Signature, new: Signature, records: set) -> list[Change]:
    if old == new:
        return []
    if old.conv != new.conv or len(old.params) != len(new.params):
        return [Change("breaking", "%s: %s -> %s" % (where, old.text(), new.text()))]
    out = []
    pairs = [("return", old.ret, new.ret)] + [("param %d" % (i + 1), a, b)
                                              for i, (a, b) in enumerate(zip(old.params, new.params))]
    for label, a, b in pairs:
        if a == b:
            continue
        kind = "standin" if is_stand_in_retype(a, b, records) else "breaking"
        out.append(Change(kind, "%s %s: %s -> %s" % (where, label, a, b)))
    return out


def compare_slots(record: str, old: list, new: list, records: set) -> list[Change]:
    out = []
    old_names, new_names = [n for n, _ in old], [n for n, _ in new]
    old_by, new_by = dict(old), dict(new)
    for name in old_names:
        if name not in new_by:
            out.append(Change("breaking", "slot removed: %s.%s" % (record, name)))
    common_old = [n for n in old_names if n in new_by]
    common_new = [n for n in new_names if n in old_by]
    if common_old != common_new:
        out.append(Change("breaking", "slots reordered in %s" % record))
    tail = new_names[len(old_names):] if new_names[:len(old_names)] == old_names else None
    for name in new_names:
        if name in old_by:
            continue
        if tail is not None and name in tail:
            out.append(Change("additive", "new slot: %s.%s" % (record, name)))
        else:
            out.append(Change("breaking", "slot inserted before the end: %s.%s" % (record, name)))
    for name in common_old:
        out += compare_signature("%s.%s" % (record, name), old_by[name], new_by[name], records)
    return out


def compare_layout(record: str, old: tuple, new: tuple) -> list[Change]:
    if old is None or new is None:
        return [Change("layout", "no layout from clang for %s" % record)] if old != new else []
    old_members, old_size, _ = old
    new_members, new_size, _ = new
    if new_members[:len(old_members)] != old_members or new_size < old_size:
        return [Change("layout", "field moved or changed size in %s" % record)]
    if new_members != old_members:
        return [Change("additive", "field appended to %s" % record)]
    return []


def compare_interfaces(old: Interface, new: Interface) -> list[Change]:
    """Every change from old to new, each of kind additive, standin, breaking or layout."""
    changes = []
    for record in old.slots:
        if record not in new.slots:
            changes.append(Change("breaking", "struct removed: %s" % record))
    for record, slots in new.slots.items():
        if record not in old.slots:
            changes.append(Change("additive", "new struct: %s" % record))
        else:
            changes += compare_slots(record, old.slots[record], slots, new.records)
    for record, layout in new.layouts.items():
        if record in old.layouts:
            changes += compare_layout(record, old.layouts[record], layout)
    return changes


def judge(old: Interface, new: Interface, changes: list[Change]) -> Result:
    """The verdict on the changes, given the version bump."""
    kinds = {c.kind for c in changes}
    details = [c.text if c.kind != "standin" else "stand-in retype, " + c.text for c in changes]
    major_up, minor_up = new.major > old.major, new.minor > old.minor
    versions = "%d.%d -> %d.%d" % (old.major, old.minor, new.major, new.minor)

    def fail(reason: str) -> Result:
        return Result(new.name, False, "FAIL (%s)" % reason, details)

    if (new.major, new.minor) < (old.major, old.minor):
        return fail("version went down, %s" % versions)
    if "layout" in kinds:
        return fail("a field moved or changed size, which no version bump allows")
    if not changes:
        bump = ", " + bump_text(old, new) if (old.major, old.minor) != (new.major, new.minor) else ""
        return Result(new.name, True, "ok (no change%s)" % bump)
    if major_up:
        return fail("adapter required (#9)")
    if "breaking" in kinds:
        return fail("breaking change needs a major bump, version is %s" % versions)
    if not minor_up:
        return fail("additive change needs a minor bump, version is %s" % versions)
    return Result(new.name, True, "ok (additive, %s)" % bump_text(old, new), details)


def bump_text(old: Interface, new: Interface) -> str:
    if new.major != old.major:
        return "major %d->%d" % (old.major, new.major)
    return "minor %d->%d" % (old.minor, new.minor)


def diff_headers(old_path: Path, new_path: Path, scratch: Path) -> Result:
    """The verdict for one interface header against its baseline."""
    name = new_path.stem if new_path.exists() else old_path.stem
    if not old_path.exists():
        return Result(name, True, "ok (new interface)")
    if not new_path.exists():
        return Result(name, False, "FAIL (interface removed)")
    old = load_interface(old_path, scratch / "old")
    new = load_interface(new_path, scratch / "new")
    if not new.major and not new.minor and not old.major and not old.minor:
        return Result(name, True, "skipped (no version macros)")
    return judge(old, new, compare_interfaces(old, new))


def diff_dirs(old_dir: Path, new_dir: Path) -> list[Result]:
    """The verdicts for every header in either directory, sorted by name."""
    names = sorted({p.name for p in old_dir.glob("*.h")} | {p.name for p in new_dir.glob("*.h")})
    with tempfile.TemporaryDirectory(prefix="abidiff-") as tmp:
        def one(name: str) -> Result:
            return diff_headers(old_dir / name, new_dir / name, Path(tmp) / name)
        with ThreadPoolExecutor() as pool:
            return list(pool.map(one, names))


def git(*args: str) -> str:
    run = subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True)
    if run.returncode != 0:
        raise SystemExit("git %s failed\n%s" % (" ".join(args), run.stderr.strip()))
    return run.stdout


def baseline_ref() -> str:
    """The newest interfaces-v* tag, else the merge base of HEAD and the base branch."""
    tags = git("tag", "-l", "interfaces-v*", "--sort=-v:refname").split()
    return tags[0] if tags else git("merge-base", "HEAD", BASE_BRANCH).strip()


def extract_baseline(ref: str, target: Path) -> None:
    """Write interfaces/*.h as they were at ref into target."""
    for path in git("ls-tree", "--name-only", ref, "interfaces/").split():
        if path.endswith(".h"):
            (target / Path(path).name).write_text(git("show", "%s:%s" % (ref, path)))


def standins(directory: Path) -> list[str]:
    """One line for each int*, void* and char* in a slot signature of the headers in directory."""
    out = []
    with tempfile.TemporaryDirectory(prefix="abidiff-") as tmp:
        for header in sorted(directory.glob("*.h")):
            iface = load_interface(header, Path(tmp) / header.stem)
            for record, slots in iface.slots.items():
                for slot, sig in slots:
                    pairs = [("return", sig.ret)] + [("param %d" % (i + 1), p) for i, p in enumerate(sig.params)]
                    out += ["%s.%s.%s %s: %s" % (header.stem, record, slot, label, text)
                            for label, text in pairs if STAND_IN.fullmatch(text)]
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--base", metavar="REF", help="baseline ref (default: newest interfaces-v* tag, else the merge base with %s)" % BASE_BRANCH)
    parser.add_argument("--report-standins", action="store_true", help="list every int*, void* and char* in a slot signature")
    args = parser.parse_args()
    if args.report_standins:
        lines = standins(ROOT / "interfaces")
        print("\n".join(lines))
        print("%d int*, void* or char* in slot signatures" % len(lines))
        return
    ref = args.base or baseline_ref()
    with tempfile.TemporaryDirectory(prefix="abidiff-base-") as tmp:
        extract_baseline(ref, Path(tmp))
        results = diff_dirs(Path(tmp), ROOT / "interfaces")
    print("baseline: %s" % ref)
    for result in results:
        print("%s: %s" % (result.name, result.summary))
        for line in result.details:
            print("    " + line)
    sys.exit(0 if all(r.ok for r in results) else 1)


if __name__ == "__main__":
    main()

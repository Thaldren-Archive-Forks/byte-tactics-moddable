"""List free __stdcall, __cdecl and __fastcall declarations whose function is a member function.

    uv run tools/callconvscan.py            # prints every hit
    uv run tools/callconvscan.py --check    # exits 1 if there is a hit that is not in KNOWN

A caller that declares a member function (thiscall: `this` in ecx) as a free
function passes `this` on the stack, or not at all. MSVC 5 may still emit the
same bytes, because ecx happens to hold the object. clang does not: it leaves
ecx to chance, so the callee reads garbage as `this`.

A function counts as a member function when any of these holds:
  - data/symbols.csv names it `Class::Name`;
  - data/progress.csv gives it a decorated member name (`?f@C@@QAE...`);
  - its first instructions read ecx before anything writes it (the original
    exe in orig/TotalA.exe), which finds methods the sources spell as free
    functions, such as 0x490080.

Each declaration or definition of a free function with an explicit calling
convention is resolved to an address by the name in data/symbols.csv (or
FUN_xxxxxxxx), or by a `// FUNCTION:` marker above a definition. With --names, a
name that is the leaf of a member function is also reported: it may be an
unrelated function that shares the name. A __fastcall function that reads edx
too, or takes no stack arguments, is not a hit: clang passes it as MSVC does.
"""

import argparse
import csv
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SYMBOLS = ROOT / "data" / "symbols.csv"
PROGRESS = ROOT / "data" / "progress.csv"
FUNCTIONS = ROOT / "data" / "functions.csv"
EXE = ROOT / "orig" / "TotalA.exe"

MEMBER_DECORATION = re.compile(r"^\?[^?@]+(?:@[^?@]+)*@@[ABEFIJMNQRUV]AE")
DECL = re.compile(r"^\s*(?!typedef|return|//)[^;{}()=]*\b(__stdcall|__cdecl|__fastcall)\s+(\w+)\s*\(")
IMPLICIT = re.compile(r"^(?!typedef|return|static|extern \"C\"|class|struct|union|enum|using|template|#)"
                      r"(?:unsigned\s+|const\s+)*[A-Za-z_][\w:<>]*[\s\*&]+(\w+)\s*\([^;{}]*\)\s*;")
FUNCTION_MARKER = re.compile(r"^\s*//\s*FUNCTION:\s*(0x[0-9a-fA-F]+)")
FUN_NAME = re.compile(r"^FUN_([0-9a-fA-F]{8})$")
ECX = {"ecx", "cx", "cl", "ch"}
EDX = {"edx", "dx", "dl", "dh"}

KNOWN: set[tuple[str, str]] = set()


def reads_ecx_first(insns, regs=ECX) -> bool:
    """True when the first use of ecx (or of `regs`) in the straight-line prologue is a read.

    `push ecx` is skipped (compilers use it to reserve a stack slot), while
    `xor ecx, ecx` and `or ecx, -1` count as writes.
    """
    for insn in insns:
        if insn.mnemonic in ("call", "ret", "retn", "jmp", "loop") or insn.mnemonic.startswith("j"):
            return False
        if insn.mnemonic == "push" and insn.op_str == "ecx" and regs is ECX:
            continue
        reads, writes = insn.regs_access()
        names_read = {insn.reg_name(r) for r in reads}
        names_write = {insn.reg_name(r) for r in writes}
        operands = insn.op_str.replace(" ", "").split(",")
        if insn.mnemonic in ("xor", "sub") and len(set(operands)) == 1 and operands[0] in regs:
            return False
        if insn.mnemonic == "or" and operands[0] in regs and operands[1] in ("-1", "0xffffffff"):
            return False
        if names_read & regs:
            return True
        if names_write & regs:
            return False
    return False


def thiscall_by_asm(functions: list[tuple[int, int]]) -> tuple[set[int], set[int]]:
    """The functions that read ecx first, and those among them that also read edx first (true fastcall)."""
    import capstone
    import pefile

    pe = pefile.PE(str(EXE))
    base = pe.OPTIONAL_HEADER.ImageBase
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    found, fastcall = set(), set()
    for address, size in functions:
        code = pe.get_data(address - base, min(size, 48))
        if reads_ecx_first(md.disasm(code, address)):
            found.add(address)
            if reads_ecx_first(md.disasm(code, address), EDX):
                fastcall.add(address)
    return found, fastcall


def load_tables():
    names: dict[str, list[int]] = defaultdict(list)
    leaves: dict[str, list[int]] = defaultdict(list)
    members: set[int] = set()
    with open(SYMBOLS, newline="") as f:
        for row in csv.DictReader(f):
            address, name = int(row["address"], 16), row["name"]
            if "::" in name:
                members.add(address)
                leaves[name.rsplit("::", 1)[1]].append(address)
            else:
                names[name].append(address)
    with open(PROGRESS, newline="") as f:
        for row in csv.DictReader(f):
            if MEMBER_DECORATION.match(row["symbol"]):
                members.add(int(row["address"], 16))
    functions = []
    stack_args = {}
    with open(FUNCTIONS, newline="") as f:
        for row in csv.DictReader(f):
            if row["kind"] == "game":
                functions.append((int(row["address"], 16), int(row["size"])))
                stack_args[int(row["address"], 16)] = int(row["params"])
    return names, leaves, members, functions, stack_args


def declarations(root: Path, implicit: bool = False):
    """Yield (path, line, convention, name, marker address or None) for each free declaration."""
    for sub in ("src", "include"):
        for path in sorted((root / sub).rglob("*")):
            if path.suffix not in (".cpp", ".h", ".hpp", ".c"):
                continue
            marker = None
            for number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
                m = FUNCTION_MARKER.match(line)
                if m:
                    marker = int(m.group(1), 16)
                    continue
                if implicit and not DECL.match(line):
                    mi = IMPLICIT.match(line)
                    if mi and mi.group(1) not in ("sizeof", "if", "while", "for", "switch"):
                        yield path.relative_to(root), number, "implicit", mi.group(1), None
                m = DECL.match(line)
                if m and "::" not in line.split("(")[0]:
                    yield path.relative_to(root), number, m.group(1), m.group(2), marker
                if line.strip():
                    marker = None if m else marker if line.lstrip().startswith("//") else None


def scan(root: Path = ROOT, asm: bool = True, implicit: bool = False, names_only: bool = False):
    names, leaves, members, functions, stack_args = load_tables()
    thiscalls = set(members)
    fastcalls: set[int] = set()
    if asm:
        found, fastcalls = thiscall_by_asm(functions)
        thiscalls |= found
    hits = []
    for path, line, conv, name, marker in declarations(root, implicit):
        addresses = set(names.get(name, []))
        m = FUN_NAME.match(name)
        if m:
            addresses.add(int(m.group(1), 16))
        if marker is not None:
            addresses.add(marker)
        for address in sorted(addresses & thiscalls):
            if conv == "__fastcall" and address not in members and (address in fastcalls
                                                                    or stack_args.get(address) == 0):
                continue
            why = "member" if address in members else "reads ecx"
            hits.append((path, line, conv, name, address, why))
        if names_only and name in leaves and not addresses & thiscalls:
            for address in leaves[name]:
                hits.append((path, line, conv, name, address, "name only"))
    return hits


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="exit 1 on a hit not in KNOWN")
    parser.add_argument("--implicit", action="store_true",
                        help="also scan declarations with no calling convention (stdcall under /Gz)")
    parser.add_argument("--names", action="store_true",
                        help="also list names that are only the leaf of a member (many false hits)")
    parser.add_argument("--no-asm", action="store_true", help="skip the ecx scan of orig/TotalA.exe")
    args = parser.parse_args()
    hits = scan(asm=not args.no_asm, implicit=args.implicit, names_only=args.names)
    for path, line, conv, name, address, why in hits:
        print(f"{path}:{line}: {conv} {name} -> 0x{address:x} ({why})")
    print(f"{len(hits)} hits", file=sys.stderr)
    new = [h for h in hits if (str(h[0]), h[3]) not in KNOWN]
    return 1 if args.check and new else 0


if __name__ == "__main__":
    sys.exit(main())

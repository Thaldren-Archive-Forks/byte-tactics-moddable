"""Map every caller's spelling of a function or global to the one name defined at its address.

    uv run tools/aliases.py --out aliases.obj --objects @objects.rsp --libs A.LIB B.LIB

The tree is one function per file, so every caller declares its callees and the
game's globals with its own struct names and parameter types, and the same
address has many mangled spellings. This writes a COFF object of weak externals
(the mechanism OLDNAMES.LIB uses for its C runtime aliases) that maps each
spelling to the definition at that address, and each C++-mangled use of a C
runtime function (tolower, sprintf, ...) to the library's symbol. Both the
matching link (tools/link.py) and the modern CMake build link it.
"""

import argparse
import csv
import re
import struct
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path

from check import ROOT, base_name
from linkcheck import (Demangle, MEMBER_STATIC, archive_symbols, data_symbol, load_known, read_object,
                       type_size)
from linkcheck import address_of as matching_address_of

GLOBALS = ROOT / "data/globals.csv"
CLANG_RUNTIME = {"___CxxFrameHandler3": "___CxxFrameHandler"}
IMPORT_SPELLINGS = {"_SmackSoundEnable@4": "_SmackSimulate@4",
                    "__imp__SmackSoundEnable@4": "__imp__SmackSimulate@4"}
ENTRY = re.compile(r"^\s*//\s*ENTRY:\s*(0x[0-9a-fA-F]+)(?:\s+(\S+))?")

IMAGE_SYM_CLASS_EXTERNAL = 2
IMAGE_SYM_CLASS_WEAK_EXTERNAL = 105
IMAGE_WEAK_EXTERN_SEARCH_ALIAS = 3

GLOBAL_LO, GLOBAL_HI = 0x4FC000, 0x52D000


def address_of(sym: str, symbols: dict[str, int]) -> int | None:
    """The address a symbol names (tools/linkcheck.py's), where a std::pair of
    a const key is the pair data/symbols.csv names without the const (`$$CB`)."""
    found = matching_address_of(sym, symbols)
    if found is None and "$$CB" in sym:
        found = matching_address_of(sym.replace("$$CB", ""), symbols)
    return found


def leaf_address(name: str, symbols: dict[str, int]) -> int | None:
    """The address of a namespaced global whose leaf name is in symbols.csv:
    `?g_game@Build@@3PAUGame@1@A` -> symbols['g_game']. The type differs, so
    the mangled spelling is not in symbols.csv, but the address is the same
    global and the weak external only cares about the name."""
    if not name.startswith("?"):
        return None
    return symbols.get(name[1:].split("@", 1)[0])


def build_aliases(objects: list[Path], symbols: dict[str, int], data_addr: dict[int, str],
                  libs: dict[str, str], out: Path, extra: dict[str, str] | None = None,
                  winmain_addr: int | None = None, data_fallback: bool = False,
                  strip_underscore: bool = False, thunks: dict[int, str] | None = None,
                  placeholder_methods: bool = False,
                  annotated: dict[int, tuple[str, str]] | None = None,
                  overloads: dict[str, set[int]] | None = None) -> dict[str, str]:
    """Write the COFF object that maps caller spellings to definitions."""
    infos = [read_object(str(o), o.read_bytes()) for o in objects]
    defined = {n for info in infos for n in info.defs}
    funcs_at = functions_at(infos, symbols, annotated)
    for addr, name in (thunks or {}).items():
        funcs_at.setdefault(addr, []).append(name)

    lib_by_base: dict[str, str] = {}
    for n in libs:
        if not n.startswith("?"):
            lib_by_base.setdefault(base_name(n), n)

    by_member: dict[tuple[str, str], list[str]] = defaultdict(list)
    for name in defined:
        parts = split_member(name)
        if parts:
            by_member[(parts[0], parts[2])].append(name)

    absolute: dict[str, int] = {}
    aliases: dict[str, str] = dict(extra or {})
    if winmain_addr is not None and funcs_at.get(winmain_addr) and "_WinMain@16" not in defined:
        aliases.setdefault("_WinMain@16", prefer(funcs_at[winmain_addr]))
    for info in infos:
        for name, is_func in info.refs.items():
            if name in defined or name in aliases or name in libs:
                continue
            addr = address_of(name, symbols)
            if not is_func:
                is_func = untyped_function(name, addr, funcs_at, data_addr)
            if is_func:
                targets = funcs_at.get(addr) if addr is not None else None
                if overloads is not None and (chosen := overload_target(name, overloads, funcs_at)):
                    targets = [chosen]
                if targets:
                    aliases[name] = prefer(targets)
                elif placeholder_methods and (target := placeholder_method(name, funcs_at, by_member)):
                    aliases[name] = target
                elif strip_underscore and base_name(name).lstrip("_") in lib_by_base:
                    aliases[name] = lib_by_base[base_name(name).lstrip("_")]
                elif base_name(name) in lib_by_base and lib_by_base[base_name(name)] != name:
                    aliases[name] = lib_by_base[base_name(name)]
            else:
                if addr is None and data_fallback:
                    addr = leaf_address(name, symbols)
                if addr == 0 and name not in data_addr.values():
                    absolute[name] = 0
                elif addr is not None and addr in data_addr and data_addr[addr] != name:
                    aliases[name] = data_addr[addr]
    write_object(aliases, out, absolute)
    return aliases


def defined_addresses(obj: Path, symbols: dict[str, int]) -> dict[int, str]:
    """address -> the data symbol an object defines there (its exact COFF name)."""
    info = read_object(str(obj), obj.read_bytes())
    out: dict[int, str] = {}
    for name, d in info.defs.items():
        if d.code:
            continue
        addr = address_of(name, symbols)
        if addr is not None and GLOBAL_LO <= addr < GLOBAL_HI:
            out[addr] = name
    return out


def global_rows() -> dict[int, tuple[str, int, str, str]]:
    """address -> (name, size, section, init-hex) for every globals.csv row
    that link/ has to define: not one a data file or a tree file defines, nor
    one inside another global, nor the runtime library's (_tls_index)."""
    out = {}
    if not GLOBALS.exists():
        return out
    with GLOBALS.open() as fh:
        for r in csv.DictReader(fh):
            if r.get("defined") or r.get("kind") == "library":
                continue
            out[int(r["address"], 16)] = (f"DAT_{int(r['address'], 16):08x}", int(r["size"]),
                                          r["section"], r["init"])
    return out


def write_extra_data(extra: dict[int, tuple[str, int, str, str]], out: Path) -> None:
    """Write the C++ source that defines each global in extra as a byte array
    named DAT_<address>, with the initial value the original holds."""
    exe = None
    lines = ['extern "C" {']
    for addr in sorted(extra):
        _, size, section, init = extra[addr]
        name = f"DAT_{addr:08x}"
        if section == ".bss":
            bytes_ = b""
        elif len(init) >= 2 * size:
            bytes_ = bytes.fromhex(init)[:size].rstrip(b"\0")
        else:
            if exe is None:
                from check import Original
                exe = Original()
            bytes_ = exe.read(addr, size).rstrip(b"\0")
        if section == ".bss" or not bytes_:
            lines.append(f"unsigned char {name}[{max(size, 1)}];")
        else:
            body = ", ".join(f"0x{b:02x}" for b in bytes_)
            lines.append(f"unsigned char {name}[{max(size, 1)}] = {{ {body}{', 0' if size > len(bytes_) else ''} }};")
    lines.append("}")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines) + "\n")


def data_extents(objects: list[Path], symbols: dict[str, int]) -> list[tuple[int, int, str]]:
    """(address, size, symbol) of every global the objects define at a known
    address: a global runs from its symbol to the next one in its section."""
    from place import parse
    out = []
    for path in objects:
        obj = parse(path)
        for name, sym in obj.externals.items():
            sec = obj.secs[sym.section - 1]
            if sec.is_code or name.startswith(("??_C@", "__real@")):
                continue
            addr = address_of(name, symbols)
            if addr is not None and GLOBAL_LO <= addr < GLOBAL_HI:
                lo, hi = sec.slice_at(sym.value)
                out.append((addr, hi - lo, name))
    return sorted(out)


def vtable_addresses(symbols: dict[str, int]) -> dict[int, str]:
    """A class's vtable the compiler emits (??_7...) is the one definition of
    its address."""
    return {addr: name for name, addr in symbols.items() if name.startswith("??_7")}


def extra_globals(objects: list[Path], symbols: dict[str, int]) -> dict[int, tuple[str, int, str, str]]:
    """The globals data/globals.csv lists that no object defines and none
    contains."""
    extents = data_extents(objects, symbols)
    have = {a for a, _, _ in extents} | set(vtable_addresses(symbols))
    rows = {a: r for a, r in global_rows().items() if GLOBAL_LO <= a < GLOBAL_HI}
    inside = lambda a: any(lo <= a < lo + max(size, 1) for lo, size, _ in extents)
    return {a: r for a, r in rows.items() if a not in have and not inside(a)}


PLACEHOLDER_CLASS = re.compile(r"^(?:Class|Struct|Elem|Vec|List|Node|Lists)_([0-9a-f]{8})$")


def split_member(name: str) -> tuple[str, str, str] | None:
    """(member, class, signature) of a mangled member function name, with
    `ctor` and `dtor` for the constructor and destructor."""
    m = re.match(r"^\?([^@?]+)@([^@]+)@@(Q.*)$", name)
    if m:
        return m.group(1), m.group(2), m.group(3)
    m = re.match(r"^\?\?([01])([^@]+)@@(Q.*)$", name)
    if m:
        return ("ctor" if m.group(1) == "0" else "dtor"), m.group(2), m.group(3)
    return None


def placeholder_method(name: str, funcs_at: dict[int, list[str]],
                       by_member: dict[tuple[str, str], list[str]]) -> str | None:
    """The definition of a method a caller reached through a placeholder
    class (Class_00435a20::LoadMissionByName, where the file that defines it
    calls the class Mission): the definition at the address the placeholder
    holds, else the only definition with the same member and signature."""
    parts = split_member(name)
    if parts is None:
        return None
    member, cls, signature = parts
    m = PLACEHOLDER_CLASS.match(cls)
    if m is None:
        return None
    at = [n for n in funcs_at.get(int(m.group(1), 16), ())
          if (p := split_member(n)) and p[0] == member and p[2] == signature]
    if at:
        return prefer(at)
    others = [n for n in by_member.get((member, signature), ()) if split_member(n)[1] != cls]
    return others[0] if len(others) == 1 else None


def import_thunks(libs: dict[str, str]) -> dict[int, str]:
    """address -> the library symbol of each `jmp [import]` thunk in the
    original's gap region of import stubs (0x49f710): the linker generates
    those, so the modern link takes the names from the import libraries."""
    import pefile
    with FUNCTIONS.open() as fh:
        regions = [(int(r["address"], 16), int(r["size"])) for r in csv.DictReader(fh)
                   if r["kind"] == "gap" and r["address"] == "0x49f710"]
    pe = pefile.PE(str(EXE))
    base = pe.OPTIONAL_HEADER.ImageBase
    imported = {i.address: i.name.decode() for e in pe.DIRECTORY_ENTRY_IMPORT for i in e.imports if i.name}
    by_name: dict[str, str] = {}
    for name in libs:
        m = re.match(r"^_([A-Za-z0-9_]+)(@\d+)?$", name)
        if m:
            by_name.setdefault(m.group(1), name)
    out: dict[int, str] = {}
    for start, size in regions:
        code = pe.get_data(start - base, size)
        for off in range(0, size - 5, 6):
            if code[off:off + 2] == b"\xff\x25":
                (slot,) = struct.unpack_from("<I", code, off + 2)
                if slot in imported and imported[slot] in by_name:
                    out[start + off] = by_name[imported[slot]]
    return out


def functions_at(infos, symbols: dict[str, int],
                 annotated: dict[int, tuple[str, str]] | None = None) -> dict[int, list[str]]:
    """address -> the code symbols the objects define there: where
    data/progress.csv gives a definition's address (one of several overloads
    that share a name in data/symbols.csv), that one; else the name's."""
    at_address = {name: addr for addr, (_, name) in (annotated or {}).items()}
    funcs_at: dict[int, list[str]] = defaultdict(list)
    for info in infos:
        for name, d in info.defs.items():
            if d.code:
                a = at_address.get(name)
                if a is None:
                    a = address_of(name, symbols)
                if a is not None and name not in funcs_at[a]:
                    funcs_at[a].append(name)
    return funcs_at


def overload_target(name: str, overloads: dict[str, set[int]], funcs_at: dict[int, list[str]]) -> str | None:
    """The definition a call to an overloaded name reaches, when the overloads
    (data/aliases.csv lists each one's address) take different numbers of
    argument bytes and exactly one takes the caller's."""
    if "::" not in (base := base_name(name)) and not name.startswith("??0"):
        return None
    candidates = overloads.get(base)
    if not candidates or len(candidates) < 2:
        return None
    wanted = stub_cleanup_bytes(name)
    matching = [d for a in sorted(candidates) for d in funcs_at.get(a, ())
                if stub_cleanup_bytes(d) == wanted and split_member(d) and split_member(name)
                and split_member(d)[0] == split_member(name)[0]]
    return matching[0] if len(matching) == 1 else None


def overloaded_names(symbols: dict[str, int]) -> dict[str, set[int]]:
    """name -> every address data/symbols.csv and data/aliases.csv give it."""
    from check import load_aliases
    out = {name: set(addrs) for name, addrs in load_aliases().items()}
    for name, addrs in out.items():
        if name in symbols:
            addrs.add(symbols[name])
    return out


def alias_names(symbols: dict[str, int]) -> None:
    """Add the names data/aliases.csv gives one address each (a view name for
    a definition, a duplicate copy) to symbols."""
    from check import load_aliases
    for name, addrs in load_aliases().items():
        if len(addrs) == 1 and name not in symbols:
            symbols[name] = next(iter(addrs))


def untyped_function(name: str, addr: int | None, funcs_at: dict[int, list[str]],
                     data_addr: dict[int, str]) -> bool:
    """Whether an undefined name that carries no COFF function type (clang-cl
    leaves it off; Visual C++ sets it) is a function: by the definition at its
    address, or else by its mangling (`@@2` and `@@3` mark data, `@N` ends a
    __stdcall C name)."""
    if addr is not None and addr in data_addr and addr not in funcs_at:
        return False
    if addr is not None and addr in funcs_at:
        return True
    if name.startswith("?"):
        return not re.search(r"@@[23]", name)
    return bool(re.search(r"@\d+$", name))


REL_I386_DIR32 = 0x06
REL_I386_REL32 = 0x14
IMAGE_SCN_LNK_NRELOC_OVFL = 0x01000000


def static_names(symbols: dict[str, int]) -> dict[str, int]:
    """name -> address of the file statics data/symbols.csv names `X$S<n>`
    (the compiler's number for a static X), which other files reach as an
    extern."""
    return {n.split("$S", 1)[0]: a for n, a in symbols.items() if re.search(r"\$S\d+$", n)}


def promote_statics(data: bytes, names: set[str]) -> tuple[bytes, list[str]]:
    """The object with each file-static data symbol whose leaf name is in
    names made external, and the names it defined."""
    buf = bytearray(data)
    _, nsects, _, symptr, nsyms, opthdr, _ = struct.unpack_from("<HHIIIHH", buf, 0)
    strtab = bytes(buf[symptr + nsyms * 18:])
    found = []
    i = 0
    while i < nsyms:
        at = symptr + i * 18
        raw = bytes(buf[at:at + 18])
        value, secnum, typ, sclass, naux = struct.unpack_from("<IhHBB", raw, 8)
        if sclass == 3 and secnum > 0 and (typ & 0x30) != 0x20:
            if raw[:4] == b"\0\0\0\0":
                (off,) = struct.unpack_from("<I", raw, 4)
                name = strtab[off:strtab.index(b"\0", off)].decode("latin-1")
            else:
                name = raw[:8].split(b"\0", 1)[0].decode("latin-1")
            if not name.startswith(".") and base_name(name) in names:
                buf[at + 16] = 2
                found.append(name)
        i += 1 + naux
    return bytes(buf), found


def interior_references(objects: list[Path], symbols: dict[str, int],
                        extents: list[tuple[int, int, str]]) -> dict[str, tuple[str, int]]:
    """name -> (container, offset) for every undefined global some object
    names that lies inside another global: a field or an entry the code reaches
    by its own symbol (DAT_00512d80 is DAT_00512c80 + 0x100)."""
    containers = {a for a, _, _ in extents}
    out: dict[str, tuple[str, int]] = {}
    for path in objects:
        info = read_object(str(path), path.read_bytes())
        for name in info.refs:
            if name in out or name.startswith("__imp_"):
                continue
            addr = address_of(name, symbols)
            if addr is None or not GLOBAL_LO <= addr < GLOBAL_HI or addr in containers:
                continue
            for lo, size, container in extents:
                if lo < addr < lo + size:
                    out[name] = (container, addr - lo)
                    break
    return out


def patch_object(data: bytes, interior: dict[str, tuple[str, int]]) -> bytes | None:
    """The object with each reference to an interior global pointed at its
    container, the offset added to the stored addend; None if it has none."""
    buf = bytearray(data)
    _, nsects, _, symptr, nsyms, opthdr, _ = struct.unpack_from("<HHIIIHH", buf, 0)
    strtab_at = symptr + nsyms * 18
    strings = bytearray()
    (strtab_size,) = struct.unpack_from("<I", buf, strtab_at)
    renamed: dict[int, int] = {}
    i = 0
    while i < nsyms:
        at = symptr + i * 18
        raw = bytes(buf[at:at + 18])
        value, secnum, typ, sclass, naux = struct.unpack_from("<IhHBB", raw, 8)
        if raw[:4] == b"\0\0\0\0":
            (off,) = struct.unpack_from("<I", raw, 4)
            end = buf.index(b"\0", strtab_at + off)
            name = bytes(buf[strtab_at + off:end]).decode("latin-1")
        else:
            name = raw[:8].split(b"\0", 1)[0].decode("latin-1")
        if secnum == 0 and sclass == 2 and name in interior:
            container, offset = interior[name]
            encoded = container.encode("latin-1")
            if len(encoded) <= 8:
                field = encoded + b"\0" * (8 - len(encoded))
            else:
                field = b"\0\0\0\0" + struct.pack("<I", strtab_size + len(strings))
                strings += encoded + b"\0"
            buf[at:at + 8] = field
            renamed[i] = offset
        i += 1 + naux
    if not renamed:
        return None
    for s in range(nsects):
        hdr = 20 + opthdr + s * 40
        name = bytes(buf[hdr:hdr + 8]).rstrip(b"\0")
        size, rawptr, relptr, _, nrel, _, chars = struct.unpack_from("<IIIIHHI", buf, hdr + 16)
        if name.startswith(b".debug") or not rawptr:
            continue
        if chars & IMAGE_SCN_LNK_NRELOC_OVFL:
            raise SystemExit("aliases: relocation overflow sections are not supported")
        for r in range(nrel):
            roff, symidx, rtype = struct.unpack_from("<IIH", buf, relptr + r * 10)
            if symidx in renamed:
                if rtype not in (REL_I386_DIR32, REL_I386_REL32):
                    raise SystemExit(f"aliases: relocation type {rtype:#x} against an interior global")
                (addend,) = struct.unpack_from("<i", buf, rawptr + roff)
                struct.pack_into("<i", buf, rawptr + roff, addend + renamed[symidx])
    buf += strings
    struct.pack_into("<I", buf, strtab_at, strtab_size + len(strings))
    return bytes(buf)


def entry_points(source: Path) -> list[tuple[int, str]]:
    """(address, symbol) of every `// ENTRY:` label in a gap source: an entry
    in the middle of hand-written assembly, public as _FUN_<address> unless the
    comment names it."""
    out = []
    for line in source.read_text(errors="replace").splitlines():
        m = ENTRY.match(line)
        if m:
            address = int(m.group(1), 16)
            out.append((address, m.group(2) or f"_FUN_{address:08x}"))
    return out


def entry_offset(original: bytes, compiled: bytes, upto: int) -> int | None:
    """The offset in compiled of the instruction the original has at offset
    upto: the two instruction streams are aligned by mnemonic, since the
    compiler's assembler picks other encodings (and a `__clang__` branch of
    the source may spend more instructions on one)."""
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    ours = [(size, mnemonic) for _, size, mnemonic, _ in md.disasm_lite(compiled, 0)]
    theirs = [(size, mnemonic) for _, size, mnemonic, _ in md.disasm_lite(original, 0)]
    count = 0
    at = 0
    for size, _ in theirs:
        if at == upto:
            break
        at += size
        count += 1
    if at != upto:
        return None
    from difflib import SequenceMatcher
    matcher = SequenceMatcher(None, [m for _, m in theirs], [m for _, m in ours], autojunk=False)
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal" and i1 <= count < i2:
            target = j1 + count - i1
            return sum(size for size, _ in ours[:target])
    return None


def add_entry_symbols(data: bytes, entries: list[tuple[int, str]], symbols: dict[str, int]) -> bytes:
    """The object with a public symbol at each entry: in the section of the
    function holding it, at the instruction the original has at that address.
    The compiler's assembler may encode an instruction differently from the
    original's, so the offset is found by counting instructions, not bytes."""
    import pefile
    buf = bytearray(data)
    _, nsects, _, symptr, nsyms, opthdr, _ = struct.unpack_from("<HHIIIHH", buf, 0)
    strtab = bytearray(buf[symptr + nsyms * 18:])
    sections = []
    for sidx in range(nsects):
        hdr = 20 + opthdr + sidx * 40
        size, rawptr = struct.unpack_from("<II", buf, hdr + 16)
        sections.append((rawptr, size))
    functions: list[tuple[int, int, int]] = []
    i = 0
    while i < nsyms:
        raw = bytes(buf[symptr + i * 18:symptr + i * 18 + 18])
        value, secnum, typ, sclass, naux = struct.unpack_from("<IhHBB", raw, 8)
        if sclass == 2 and secnum > 0:
            if raw[:4] == b"\0\0\0\0":
                (off,) = struct.unpack_from("<I", raw, 4)
                name = bytes(strtab[off:strtab.index(b"\0", off)]).decode("latin-1")
            else:
                name = raw[:8].split(b"\0", 1)[0].decode("latin-1")
            addr = address_of(name, symbols)
            if addr is not None and (typ & 0x30) == 0x20:
                functions.append((addr, secnum, value))
        i += 1 + naux
    pe = pefile.PE(str(EXE))
    base = pe.OPTIONAL_HEADER.ImageBase
    new = bytearray()
    added = 0
    for address, name in entries:
        holder = max((f for f in functions if f[0] <= address), default=None)
        if holder is None:
            continue
        start, secnum, value = holder
        rawptr, size = sections[secnum - 1]
        original = pe.get_data(start - base, max(address - start, 1) + 4096)
        offset = entry_offset(original[:address - start + 64], bytes(buf[rawptr + value:rawptr + size]),
                              address - start)
        if offset is None:
            print(f"aliases: entry {address:#x} is not on an instruction both builds share in the function "
                  f"at {start:#x}", file=sys.stderr)
            continue
        encoded = name.encode("latin-1")
        if len(encoded) <= 8:
            field = encoded.ljust(8, b"\0")
        else:
            field = b"\0\0\0\0" + struct.pack("<I", len(strtab))
            strtab += encoded + b"\0"
        new += field + struct.pack("<IhHBB", value + offset, secnum, 0x20, 2, 0)
        added += 1
    if not added:
        return data
    struct.pack_into("<I", strtab, 0, len(strtab))
    struct.pack_into("<I", buf, 12, nsyms + added)
    return bytes(buf[:symptr + nsyms * 18]) + bytes(new) + bytes(strtab)


def data_kind(sec: Section) -> str:
    """The output section a piece of data belongs to."""
    if sec.chars & IMAGE_SCN_CNT_UNINIT:
        return ".bss"
    return ".data" if sec.chars & IMAGE_SCN_MEM_WRITE else ".rdata"


FILLER_CHARS = {".data": 0xC0100040, ".rdata": 0x40100040, ".bss": 0xC0100080}


@dataclass
class DataPiece:
    """A global that sits alone in a section of its object."""
    symbol: int
    name: str
    address: int
    size: int
    kind: str


def data_pieces_of(data: bytes, symbols: dict[str, int]) -> list[DataPiece]:
    """The globals of an object that have an address in the original and sit
    in a section of their own (clang-cl /Gw)."""
    _, secs, syms = read(data)
    owners: dict[int, int] = defaultdict(int)
    for sym in syms:
        if sym.section > 0 and not (sym.sclass == IMAGE_SYM_CLASS_STATIC and sym.name.startswith(".")):
            owners[sym.section] += 1
    out = []
    for index, sym in enumerate(syms):
        if sym.section <= 0 or sym.sclass != 2 or sym.value or owners[sym.section] != 1:
            continue
        sec = secs[sym.section - 1]
        if sec.chars & IMAGE_SCN_CNT_CODE or sec.name not in (".data", ".rdata", ".bss"):
            continue
        addr = address_of(sym.name, symbols)
        if addr is not None and GLOBAL_LO <= addr < GLOBAL_HI:
            out.append(DataPiece(index, sym.name, addr, sec.size, data_kind(sec)))
    return out


def piece_priority(piece: DataPiece, from_source: bool) -> int:
    """Which of several definitions at one address is the global: the
    compiler's own vtable, then the tree's source, then the generated data."""
    if piece.name.startswith("??_7"):
        return 3
    return 2 if from_source else 1


def name_data_sections(data: bytes, keep: set[int], drop: set[int],
                       pieces: dict[str, dict[int, int]]) -> bytes:
    """The object with the pieces in keep in sections named `<kind>$<address>`
    (LINK sorts the parts of a grouped section by the name after the `$`,
    which lays the game's data out in the original's order), and the pieces
    in drop, other definitions of an address, made undefined so that the
    bridge points their callers at the one definition. pieces[kind][address]
    collects the sizes."""
    chars, secs, syms = read(data)
    for index, sym in enumerate(syms):
        if index not in keep and index not in drop:
            continue
        sec = secs[sym.section - 1]
        if index in drop:
            sec.chars |= IMAGE_SCN_LNK_REMOVE
            sec.chars &= ~IMAGE_SCN_LNK_COMDAT
            sym.section, sym.value = 0, 0
            continue
        addr = address_of(sym.name, SYMBOLS_FOR_NAMING)
        kind = data_kind(sec)
        pieces.setdefault(kind, {})[addr] = max(pieces.get(kind, {}).get(addr, 0), sec.size)
        align = 1 << (((sec.chars >> 20) & 0xF) - 1) if (sec.chars >> 20) & 0xF else 16
        while addr % align:
            align //= 2
        sec.chars = sec.chars & ~0x00F00000 | (align.bit_length() << 20)
        sec.name = f"{kind}${addr:08x}"
    position, raw = {}, 0
    for pos, sym in enumerate(syms):
        position[raw] = pos
        raw += 1 + len(sym.aux)
    for sec in secs:
        for reloc in sec.relocs:
            reloc[1] = position[reloc[1]]
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        write(chars, secs, syms, Path(tmp) / "o.obj")
        return (Path(tmp) / "o.obj").read_bytes()


SYMBOLS_FOR_NAMING: dict[str, int] = {}


def layout_filler(pieces: dict[str, dict[int, int]], out: Path) -> int:
    """An object of the bytes between the pieces, so that every piece is as
    far from its neighbours as in the original: the original's bytes in
    .data and .rdata, zeros in .bss. Each kind starts at the page its first
    piece is on, so alignments hold. Returns the number of pieces that
    overlap the next one (their neighbour is not where the original has it)."""
    import pefile
    pe = pefile.PE(str(EXE))
    base = pe.OPTIONAL_HEADER.ImageBase

    def original(addr: int, size: int) -> bytes:
        try:
            return pe.get_data(addr - base, size)
        except Exception:
            return bytes(size)

    secs: list[Section] = []
    overlaps = 0
    for kind, found in pieces.items():
        end = min(found) & ~0xFFF
        first = True
        for addr in sorted(found):
            gap = addr - end
            if gap < 0:
                overlaps += 1
            elif 0 < gap <= 0x10000 or first and gap < 0x1000:
                body = b"" if kind == ".bss" else original(end, gap)
                chars = FILLER_CHARS[kind]
                if first:
                    chars = chars & ~0x00F00000 | (13 << 20)
                secs.append(Section(f"{kind}${end:08x}", chars, body, gap))
            first = False
            end = max(end, addr + found[addr])
    write(0, secs, [], out)
    return overlaps


def wanted_initialisers() -> dict[str, list[str]]:
    """source file -> the symbols of its functions the original's C runtime
    runs before WinMain (`_$E<n>` initialisers, and ordinary functions the
    tree spells otherwise)."""
    with PROGRESS.open() as fh:
        rows = {int(r["address"], 16): (r["file"], r["symbol"]) for r in csv.DictReader(fh)}
    wanted: dict[str, list[str]] = defaultdict(list)
    for fn in original_initialisers():
        if fn in rows:
            wanted[rows[fn][0]].append(rows[fn][1])
    return wanted


def drop_initialisers(data: bytes) -> bytes:
    """The object with its .CRT$XCU sections (the compiler's dynamic
    initialisers) removed from the link."""
    buf = bytearray(data)
    _, nsects, _, _, _, opthdr, _ = struct.unpack_from("<HHIIIHH", buf, 0)
    for s in range(nsects):
        hdr = 20 + opthdr + s * 40
        if bytes(buf[hdr:hdr + 8]).rstrip(b"\0") == b".CRT$XCU":
            (chars,) = struct.unpack_from("<I", buf, hdr + 36)
            struct.pack_into("<I", buf, hdr + 36, chars | IMAGE_SCN_LNK_REMOVE)
    return bytes(buf)


def patch_objects(objects: list[Path], symbols: dict[str, int], out_dir: Path,
                  source_of=None) -> list[Path]:
    """Copies of the objects under out_dir (named for their position) with
    interior globals pointed at their containers; the unchanged ones are
    copied too, so the link takes one directory."""
    out_dir.mkdir(parents=True, exist_ok=True)
    statics = static_names(symbols)
    for leaf, addr in statics.items():
        symbols.setdefault(leaf, addr)
    promoted: dict[str, list[Path]] = defaultdict(list)
    staged = out_dir / "static"
    staged.mkdir(exist_ok=True)
    staged_paths = []
    for n, path in enumerate(objects):
        data, found = promote_statics(path.read_bytes(), set(statics))
        target = staged / f"{n:04d}_{path.name}"
        target.write_bytes(data)
        staged_paths.append(target)
        for name in found:
            promoted[base_name(name)].append(path)
    for leaf, paths in promoted.items():
        if len(paths) > 1:
            print(f"aliases: static {leaf} is defined in {len(paths)} files", file=sys.stderr)
    originals = objects
    objects = staged_paths
    extents = data_extents(objects, symbols)
    interior = interior_references(objects, symbols, extents)
    result = []
    patched = 0
    wanted = wanted_initialisers() if source_of else {}
    kept = dropped = 0
    candidates: dict[int, list[tuple[int, int, int]]] = defaultdict(list)
    targets: dict[int, Path] = {}
    pieces: dict[str, dict[int, int]] = {}
    SYMBOLS_FOR_NAMING.clear()
    SYMBOLS_FOR_NAMING.update(symbols)
    for n, (path, stage) in enumerate(zip(originals, objects)):
        data = stage.read_bytes()
        new = patch_object(data, interior)
        source = source_of(path) if source_of else None
        entries = entry_points(source) if source is not None else []
        if entries:
            new = add_entry_symbols(new if new is not None else data, entries, symbols)
        patched += new is not None
        if source is not None:
            relative = str(source.relative_to(ROOT))
            if not any(sym.startswith("_$E") for sym in wanted.get(relative, ())):
                before = new if new is not None else data
                new = drop_initialisers(before)
                dropped += new != before
            else:
                kept += 1
            for symbol in wanted.get(relative, ()):
                if not symbol.startswith("_$E"):
                    entry = out_dir / f"{n:04d}_init_{symbol[:24].replace('?', '_').replace('@', '_')}.obj"
                    write_init_object(symbol, entry)
                    result.append(entry)
        target = out_dir / f"{n:04d}_{path.name}"
        final = new if new is not None else data
        target.write_bytes(final)
        found = data_pieces_of(final, symbols)
        for piece in found:
            candidates[piece.address].append((piece_priority(piece, source is not None), n, piece.symbol))
        result.append(target)
        targets[n] = target
    chosen = {addr: max(options, key=lambda o: (o[0], -o[1])) for addr, options in candidates.items()}
    for n, target in targets.items():
        keep = {sym for choice in chosen.values() for _, who, sym in [choice] if who == n}
        drop = {sym for addr, options in candidates.items() for option in options
                for _, who, sym in [option] if who == n and option != chosen[addr]}
        if keep or drop:
            target.write_bytes(name_data_sections(target.read_bytes(), keep, drop, pieces))
    overlaps = layout_filler(pieces, out_dir / "layout.obj")
    result.append(out_dir / "layout.obj")
    ordered = sum(len(found) for found in pieces.values())
    print(f"aliases: {ordered:,} globals ordered as in the original, {overlaps} larger than the space "
          f"the original left", file=sys.stderr)
    print(f"aliases: {len(interior)} interior globals pointed at their containers in {patched} objects; "
          f"initialisers kept in {kept} files, dropped from {dropped}", file=sys.stderr)
    return result


def prefer(names: list[str]) -> str:
    """A real symbols.csv name over a FUN_/DAT_ placeholder."""
    real = [n for n in names if not re.match(r"^(FUN|DAT|PTR|LAB)_[0-9a-f]{8}$", base_name(n))]
    return sorted(real or names)[0]


def write_object(aliases: dict[str, str], out: Path, absolute: dict[str, int] | None = None) -> None:
    strings = bytearray()
    str_index: dict[str, int] = {}

    def name_field(name: str) -> bytes:
        raw = name.encode("latin-1")
        if len(raw) <= 8:
            return raw + b"\0" * (8 - len(raw))
        if name not in str_index:
            str_index[name] = 4 + len(strings)
            strings.extend(raw + b"\0")
        return b"\0\0\0\0" + struct.pack("<I", str_index[name])

    syms = bytearray()
    index = 0
    for alias, target in aliases.items():
        syms += name_field(target) + struct.pack("<IhHBB", 0, 0, 0x20, IMAGE_SYM_CLASS_EXTERNAL, 0)
        index += 1
        syms += name_field(alias) + struct.pack("<IhHBB", 0, 0, 0x20, IMAGE_SYM_CLASS_WEAK_EXTERNAL, 1)
        index += 1
        syms += struct.pack("<II", index - 2, IMAGE_WEAK_EXTERN_SEARCH_ALIAS) + b"\0" * 10
        index += 1

    for name, value in (absolute or {}).items():
        syms += name_field(name) + struct.pack("<IhHBB", value, -1, 0, IMAGE_SYM_CLASS_EXTERNAL, 0)
        index += 1

    strtab = struct.pack("<I", 4 + len(strings)) + bytes(strings)
    nsects, symptr, nsyms = 1, 20 + 40, index
    header = struct.pack("<HHIIIHH", 0x14C, nsects, 0, symptr, nsyms, 0, 0)
    section = b"aliases\0" + struct.pack("<IIIIIIHHI", 0, 0, 0, 0, 0, 0, 0, 0, 0x40000040)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(header + section + bytes(syms) + strtab)




def stub_cleanup_bytes(name: str) -> int | None:
    """The bytes a __stdcall/__thiscall stub must pop, 0 for __cdecl and
    __fastcall, or None when the argument size cannot be read from the name."""
    m = re.search(r"@(\d+)$", name)
    if m:
        return int(m.group(1))
    if not name.startswith("?"):
        return 0
    try:
        d = Demangle(name[1:])
        if d.s.startswith("?"):
            d.i = 1
            code = d.take()
            if code == "_":
                code += d.take()
            if code == "$":
                return None
            if d.s[d.i] != "@":
                d.qualified()
            if d.s[d.i] == "@":
                d.i += 1
        else:
            d.qualified()
        access = d.take()
        if access in "YZ":
            kind = "global"
        elif access in MEMBER_STATIC:
            kind = "static"
        else:
            kind = "member"
            d.take()
        cc = d.take()
        if d.s[d.i] == "@":
            d.i += 1
        else:
            if d.s.startswith("?A", d.i):
                d.i += 2
            d.type()
        args = []
        while d.i < len(d.s) and d.s[d.i] not in "@Z":
            if d.s[d.i] == "X" and not args:
                d.i += 1
                break
            args.append(d.arg())
        sizes = [type_size(a) for a in args]
        if any(s is None for s in sizes):
            return None
        total = sum((s + 3) & ~3 for s in sizes)
        if kind == "member" and cc == "G":
            total += 4
        if cc in ("G", "E"):
            return total
        return 0
    except (ValueError, IndexError, KeyError):
        return None


def stub_bytes(name: str) -> bytes:
    """`xor eax, eax` then a return, cleaning the stack for callee-cleans."""
    cleanup = stub_cleanup_bytes(name)
    if cleanup:
        return b"\x31\xc0\xc2" + struct.pack("<H", cleanup)
    return b"\x31\xc0\xc3"


def stub_data_size(name: str) -> int:
    _, t = data_symbol(name)
    size = type_size(t)
    return size if size and size > 0 else 4


def write_stub_object(funcs: list[str], data: list[str], out: Path) -> None:
    """A COFF object defining each name: a ret stub in .text, a zero array in
    .data. Written by hand so any mangled spelling can be defined."""
    strings = bytearray()
    str_index: dict[str, int] = {}

    def name_field(name: str) -> bytes:
        raw = name.encode("latin-1")
        if len(raw) <= 8:
            return raw + b"\0" * (8 - len(raw))
        if name not in str_index:
            str_index[name] = 4 + len(strings)
            strings.extend(raw + b"\0")
        return b"\0\0\0\0" + struct.pack("<I", str_index[name])

    text = bytearray()
    text_defs: list[tuple[str, int]] = []
    for name in sorted(funcs):
        text_defs.append((name, len(text)))
        text.extend(stub_bytes(name))
    dat = bytearray()
    data_defs: list[tuple[str, int]] = []
    for name in sorted(data):
        data_defs.append((name, len(dat)))
        dat.extend(b"\0" * stub_data_size(name))

    sections = []
    if text_defs:
        sections.append((b".text\0\0\0", bytes(text), 0x60500020, text_defs, 0x20))
    if data_defs:
        sections.append((b".data\0\0\0", bytes(dat), 0xC0000040, data_defs, 0x00))

    hdr_size = 20 + 40 * len(sections)
    raw_off = hdr_size
    section_headers = bytearray()
    raws = bytearray()
    for sname, raw, chars, _, _ in sections:
        section_headers += sname + struct.pack("<IIIIIIHHI", 0, 0, len(raw), raw_off, 0, 0, 0, 0, chars)
        raws += raw
        raw_off += len(raw)
    nsyms = sum(len(defs) for *_, defs, _ in sections)
    symptr = raw_off

    symtab = bytearray()
    for sec_index, (_, _, _, defs, symtype) in enumerate(sections, start=1):
        for name, value in defs:
            symtab += name_field(name) + struct.pack("<IhHBB", value, sec_index, symtype,
                                                     IMAGE_SYM_CLASS_EXTERNAL, 0)
    strtab = struct.pack("<I", 4 + len(strings)) + bytes(strings)
    header = struct.pack("<HHIIIHH", 0x14C, len(sections), 0, symptr, nsyms, 0, 0)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(header + bytes(section_headers) + bytes(raws) + bytes(symtab) + strtab)


def unresolved_names(objects: list[Path], alias_keys: set[str], libs: dict[str, str],
                     classify=None) -> dict[str, bool]:
    """The names LINK would still report LNK2001 for: referenced by some
    object, defined by none, not aliased and not in a library. classify, when
    given, says whether an untyped reference is a function."""
    defined: set[str] = set()
    refs: dict[str, bool] = {}
    for obj in objects:
        info = read_object(str(obj), obj.read_bytes())
        defined |= set(info.defs)
        for name, is_func in info.refs.items():
            if classify is not None and not is_func:
                is_func = classify(name)
            refs[name] = refs.get(name, False) or is_func
    return {n: f for n, f in refs.items()
            if n not in defined and n not in alias_keys and n not in libs}


IMAGE_SCN_LNK_REMOVE = 0x800
EXE = ROOT / "orig/TotalA.exe"
PROGRESS = ROOT / "data/progress.csv"
FUNCTIONS = ROOT / "data/functions.csv"


def original_initialisers() -> list[int]:
    """The functions the original's C runtime calls before WinMain to construct
    its global objects: the entries of its __xc_a..__xc_z table, which the
    runtime's _cinit passes to _initterm (the second pair of pushes)."""
    import pefile
    pe = pefile.PE(str(EXE))
    base = pe.OPTIONAL_HEADER.ImageBase
    with FUNCTIONS.open() as fh:
        cinit = next(int(r["address"], 16) for r in csv.DictReader(fh) if r["name"] == "__cinit")
    code = pe.get_data(cinit - base, 64)
    pushes = [struct.unpack_from("<I", code, i + 1)[0] for i in range(len(code) - 5) if code[i] == 0x68]
    xc_a, xc_z = pushes[3], pushes[2]
    out = []
    for at in range(xc_a, xc_z, 4):
        (fn,) = struct.unpack("<I", pe.get_data(at - base, 4))
        if fn:
            out.append(fn)
    return out


def write_init_object(symbol: str, out: Path) -> None:
    """A COFF object whose .CRT$XCU section holds one pointer to symbol."""
    strings = bytearray()

    def name_field(name: str) -> bytes:
        raw = name.encode("latin-1")
        if len(raw) <= 8:
            return raw + b"\0" * (8 - len(raw))
        off = 4 + len(strings)
        strings.extend(raw + b"\0")
        return b"\0\0\0\0" + struct.pack("<I", off)

    raw_off = 20 + 40
    reloc_off = raw_off + 4
    sym_off = reloc_off + 10
    header = struct.pack("<HHIIIHH", 0x14C, 1, 0, sym_off, 2, 0, 0)
    section = b".CRT$XCU" + struct.pack("<IIIIIIHHI", 0, 0, 4, raw_off, reloc_off, 0, 1, 0, 0xC0300040)
    reloc = struct.pack("<IIH", 0, 1, 0x06)
    syms = name_field(".CRT$XCU") + struct.pack("<IhHBB", 0, 1, 0, 3, 0)
    syms += name_field(symbol) + struct.pack("<IhHBB", 0, 0, 0x20, IMAGE_SYM_CLASS_EXTERNAL, 0)
    strtab = struct.pack("<I", 4 + len(strings)) + bytes(strings)
    out.write_bytes(header + section + b"\0\0\0\0" + reloc + syms + strtab)


IMAGE_SCN_CNT_CODE = 0x20
IMAGE_SCN_CNT_UNINIT = 0x80
IMAGE_SCN_LNK_COMDAT = 0x1000
IMAGE_SCN_MEM_WRITE = 0x80000000
REL_DIR32, REL_DIR32NB = 0x06, 0x07
IMAGE_SYM_CLASS_STATIC = 3


@dataclass
class Section:
    name: str
    chars: int
    data: bytes
    size: int
    relocs: list = field(default_factory=list)


@dataclass
class Symbol:
    name: str
    value: int
    section: int
    type: int
    sclass: int
    aux: list = field(default_factory=list)


def read(data: bytes) -> tuple[int, list[Section], list[Symbol]]:
    machine, nsects, stamp, symptr, nsyms, opthdr, chars = struct.unpack_from("<HHIIIHH", data, 0)
    strtab = data[symptr + nsyms * 18:]

    def long_name(raw: bytes) -> str:
        if raw[:4] == b"\0\0\0\0":
            off = struct.unpack_from("<I", raw, 4)[0]
            return strtab[off:strtab.index(b"\0", off)].decode("latin-1")
        return raw[:8].split(b"\0", 1)[0].decode("latin-1")

    secs = []
    for s in range(nsects):
        hdr = data[20 + opthdr + s * 40: 60 + opthdr + s * 40]
        name = hdr[:8].split(b"\0", 1)[0].decode("latin-1")
        if name.startswith("/"):
            off = int(name[1:])
            name = strtab[off:strtab.index(b"\0", off)].decode("latin-1")
        size, rawptr, relptr, _, nrel, _, schars = struct.unpack_from("<IIIIHHI", hdr, 16)
        body = data[rawptr:rawptr + size] if rawptr else b""
        relocs = [list(struct.unpack_from("<IIH", data, relptr + r * 10)) for r in range(nrel)]
        secs.append(Section(name, schars, body, size, relocs))
    syms: list[Symbol] = []
    i = 0
    while i < nsyms:
        raw = data[symptr + i * 18: symptr + i * 18 + 18]
        value, secnum, typ, sclass, naux = struct.unpack_from("<IhHBB", raw, 8)
        aux = [data[symptr + (i + 1 + k) * 18: symptr + (i + 2 + k) * 18] for k in range(naux)]
        syms.append(Symbol(long_name(raw), value, secnum, typ, sclass, aux))
        i += 1 + naux
    return chars, secs, syms


def write(chars: int, secs: list[Section], syms: list[Symbol], out: Path) -> None:
    strings = bytearray()

    def string(name: str) -> int:
        off = 4 + len(strings)
        strings.extend(name.encode("latin-1") + b"\0")
        return off

    index, n = [], 0
    for s in syms:
        index.append(n)
        n += 1 + len(s.aux)
    pos = 20 + 40 * len(secs)
    headers, bodies = bytearray(), bytearray()
    for sec in secs:
        raw = sec.name.encode("latin-1")
        name_field = raw.ljust(8, b"\0") if len(raw) <= 8 else f"/{string(sec.name)}".encode().ljust(8, b"\0")
        relocs = b"".join(struct.pack("<IIH", off, index[symidx], rtype) for off, symidx, rtype in sec.relocs)
        rawptr = pos if sec.data else 0
        bodies += sec.data
        pos += len(sec.data)
        relptr = pos if relocs else 0
        bodies += relocs
        pos += len(relocs)
        headers += name_field + struct.pack("<IIIIIIHHI", 0, 0, sec.size, rawptr, relptr, 0,
                                            len(sec.relocs), 0, sec.chars)
    symtab = bytearray()
    for s in syms:
        raw = s.name.encode("latin-1")
        name_field = raw.ljust(8, b"\0") if len(raw) <= 8 else b"\0\0\0\0" + struct.pack("<I", string(s.name))
        symtab += name_field + struct.pack("<IhHBB", s.value, s.section, s.type, s.sclass, len(s.aux))
        for a in s.aux:
            symtab += a
    head = struct.pack("<HHIIIHH", 0x14C, len(secs), 0, pos, n, 0, chars)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(head + bytes(headers) + bytes(bodies) + bytes(symtab)
                    + struct.pack("<I", 4 + len(strings)) + bytes(strings))


def drop_unreferenced_undefined(secs: list[Section], syms: list[Symbol]) -> list[Symbol]:
    """The symbols with the undefined externals no relocation (and no weak
    external) names removed: LINK counts an undefined name in an object's
    symbol table as a reference to it. The relocations in secs are
    renumbered for the new table, in list positions as write() takes them."""
    raw_of, index = [], 0
    for sym in syms:
        raw_of.append(index)
        index += 1 + len(sym.aux)
    position = {raw: pos for pos, raw in enumerate(raw_of)}
    used = {reloc[1] for sec in secs for reloc in sec.relocs}
    for sym in syms:
        if sym.sclass == IMAGE_SYM_CLASS_WEAK_EXTERNAL and sym.aux:
            used.add(struct.unpack_from("<I", sym.aux[0], 0)[0])
    kept = [sym for raw, sym in zip(raw_of, syms)
            if not (sym.section == 0 and sym.sclass == 2 and raw not in used)]
    new_raw, index = {}, 0
    for raw, sym in zip(raw_of, syms):
        if not (sym.section == 0 and sym.sclass == 2 and raw not in used):
            new_raw[raw] = index
            index += 1 + len(sym.aux)
    new_position = {}
    pos = 0
    for raw, sym in zip(raw_of, syms):
        if raw in new_raw:
            new_position[raw] = pos
            pos += 1
    for sec in secs:
        for reloc in sec.relocs:
            reloc[1] = new_position[reloc[1]]
    for sym in kept:
        if sym.sclass == IMAGE_SYM_CLASS_WEAK_EXTERNAL and sym.aux:
            aux = bytearray(sym.aux[0])
            struct.pack_into("<I", aux, 0, new_raw[struct.unpack_from("<I", aux, 0)[0]])
            sym.aux[0] = bytes(aux)
    return kept


def prune_dead(paths: list[Path], aliases: dict[str, str], roots: tuple[str, ...]) -> int:
    """Mark every COMDAT section no live section reaches as removed from the
    link, so the references it holds (explicit instantiations of every member
    of a class, the dynamic initialiser of a global whose initialiser the
    original does not run) are not reported as unresolved: LINK reports an
    undefined name even when only dead code uses it. Roots are the sections
    that are not COMDAT and the names in roots (what the C runtime calls).
    Returns the number of sections removed."""
    objs = [read(p.read_bytes()) for p in paths]
    raws = []
    for _, _, syms in objs:
        raw, index = {}, 0
        for sym in syms:
            raw[index] = sym
            index += 1 + len(sym.aux)
        raws.append(raw)
    definitions: dict[str, list[tuple[int, int]]] = defaultdict(list)
    names_in: dict[tuple[int, int], list[str]] = defaultdict(list)
    follows: dict[tuple[int, int], list[tuple[int, int]]] = defaultdict(list)
    for oi, (_, secs, syms) in enumerate(objs):
        for sym in syms:
            if sym.section > 0 and sym.sclass == 2:
                definitions[sym.name].append((oi, sym.section))
                names_in[(oi, sym.section)].append(sym.name)
            if sym.sclass == IMAGE_SYM_CLASS_STATIC and sym.value == 0 and sym.section > 0 and sym.aux \
                    and sym.name.startswith("."):
                number, selection = struct.unpack_from("<HB", sym.aux[0], 12)
                if selection == 5 and number:
                    follows[(oi, number)].append((oi, sym.section))
    live: set[tuple[int, int]] = set()
    work: list[tuple[int, int]] = []

    def reach(key):
        if key not in live:
            live.add(key)
            work.append(key)

    def reach_name(name: str) -> None:
        seen = set()
        while name in aliases and name not in seen:
            seen.add(name)
            name = aliases[name]
        for key in definitions.get(name, ()):
            reach(key)

    for oi, (_, secs, _) in enumerate(objs):
        for si, sec in enumerate(secs, start=1):
            if sec.chars & IMAGE_SCN_LNK_REMOVE or sec.name == ".llvm_addrsig":
                continue
            if not sec.chars & IMAGE_SCN_LNK_COMDAT or sec.name.startswith((".CRT$", ".tls")):
                reach((oi, si))
    for name in roots:
        reach_name(name)
    while work:
        oi, si = work.pop()
        for key in follows.get((oi, si), ()):
            reach(key)
        for name in names_in.get((oi, si), ()):
            for key in definitions[name]:
                reach(key)
        for _, symidx, _ in objs[oi][1][si - 1].relocs:
            sym = raws[oi].get(symidx)
            if sym is None:
                continue
            if sym.section > 0:
                reach((oi, sym.section))
            elif sym.section == 0:
                reach_name(sym.name)
                if sym.sclass == IMAGE_SYM_CLASS_WEAK_EXTERNAL and sym.aux:
                    fallback = raws[oi].get(struct.unpack_from("<I", sym.aux[0], 0)[0])
                    if fallback is not None and fallback.section > 0:
                        reach((oi, fallback.section))
    removed = 0
    for oi, path in enumerate(paths):
        chars, secs, syms = objs[oi]
        dead = {si for si, sec in enumerate(secs, start=1)
                if (oi, si) not in live and sec.chars & IMAGE_SCN_LNK_COMDAT
                and not sec.chars & IMAGE_SCN_LNK_REMOVE}
        for sec in secs:
            if sec.name == ".llvm_addrsig":
                sec.chars |= IMAGE_SCN_LNK_REMOVE
        dead |= set()
        placed = {si for si in dead if re.fullmatch(r"\.(data|rdata|bss)\$[0-9a-f]{8}", secs[si - 1].name)}
        for si in dead:
            sec = secs[si - 1]
            sec.relocs = []
            if si not in placed:
                sec.data, sec.size = b"", 0
        for sym in syms:
            if sym.section in dead - placed and sym.aux and sym.sclass == IMAGE_SYM_CLASS_STATIC:
                aux = bytearray(sym.aux[0])
                struct.pack_into("<IHH", aux, 0, 0, 0, 0)
                sym.aux[0] = bytes(aux)
        for sec in secs:
            if sec.name == ".llvm_addrsig":
                sec.chars |= IMAGE_SCN_LNK_REMOVE
                sec.data, sec.size, sec.relocs = b"", 0, []
        syms = drop_unreferenced_undefined(secs, syms)
        write(chars, secs, syms, path)
        removed += len(dead)
    return removed


def response(arg: str) -> list[Path]:
    """The paths in @file (one per line), or the single path arg."""
    if arg.startswith("@"):
        return [Path(l.strip()) for l in Path(arg[1:]).read_text().splitlines() if l.strip()]
    return [Path(arg)]


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--objects", nargs="*", default=[], help="object files, or @file listing them")
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--libs", nargs="*", default=[], help="libraries the link takes names from")
    ap.add_argument("--extra-data", action="store_true",
                    help="write C++ defining the globals no object defines, instead of the alias object")
    ap.add_argument("--patch-dir", type=Path, default=None,
                    help="first copy the objects here with interior globals pointed at their containers, "
                         "and write the copies' paths to DIR/objects.rsp")
    ap.add_argument("--import-def", type=Path, default=None, metavar="LIB",
                    help="instead write the module-definition file of an old-style import library (its "
                         "__imp_ names), for llvm-dlltool -k; --objects is ignored")
    ap.add_argument("--winmain", type=lambda v: int(v, 0), default=None,
                    help="alias _WinMain@16 to the function at this address")
    args = ap.parse_args()
    if args.import_def is not None:
        names = sorted(n[len("__imp_"):] for n in archive_symbols(args.import_def) if n.startswith("__imp_"))
        lines = [f"LIBRARY {args.import_def.stem}.dll", "EXPORTS"]
        lines += [f"    {n[1:] if n.startswith('_') else n}" for n in names]
        args.out.write_text("\n".join(lines) + "\n")
        print(f"aliases: {len(names)} imports of {args.import_def.name} in {args.out}", file=sys.stderr)
        return
    objects = [p for a in args.objects for p in response(a)]
    symbols, _, annotated = load_known()
    alias_names(symbols)
    if args.extra_data:
        extra = extra_globals(objects, symbols)
        write_extra_data(extra, args.out)
        print(f"aliases: {len(extra)} globals defined in {args.out}", file=sys.stderr)
        return
    if args.patch_dir is not None:
        def source_of(path: Path) -> Path | None:
            parts = path.parts
            if "src" not in parts:
                return None
            source = ROOT / Path(*parts[parts.index("src"):]).with_suffix("")
            return source if source.suffix == ".cpp" and source.exists() else None

        objects = patch_objects(objects, symbols, args.patch_dir, source_of)
        (args.patch_dir / "objects.rsp").write_text("\n".join(str(o) for o in objects) + "\n")
    libs: dict[str, str] = {}
    for lib in args.libs:
        path = Path(lib)
        names = list(read_object(lib, path.read_bytes()).defs) if path.suffix.lower() == ".obj" \
            else archive_symbols(path)
        for name in names:
            libs.setdefault(name, path.name)
    data_addr = {a: n for a, _, n in data_extents(objects, symbols)}
    data_addr.update({a: n for a, n in vtable_addresses(symbols).items() if a not in data_addr})
    extra = {new: old for new, old in {**CLANG_RUNTIME, **IMPORT_SPELLINGS}.items() if old in libs}
    aliases = build_aliases(objects, symbols, data_addr, libs, args.out, extra=extra,
                            winmain_addr=args.winmain, data_fallback=True, strip_underscore=True,
                            thunks=import_thunks(libs), placeholder_methods=True, annotated=annotated,
                            overloads=overloaded_names(symbols))
    print(f"aliases: {len(aliases):,} aliases for {len(objects):,} objects", file=sys.stderr)
    if args.patch_dir is not None:
        removed = prune_dead([p for p in objects if p.name != "layout.obj"], aliases, ("_WinMain@16",))
        print(f"aliases: {removed:,} sections no live section reaches left out of the link", file=sys.stderr)


if __name__ == "__main__":
    main()

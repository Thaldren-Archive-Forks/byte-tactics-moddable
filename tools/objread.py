"""The COFF and name-mangling readers the modern build's tools/aliases.py uses."""

import bisect
import csv
import hashlib
import re
import struct
from dataclasses import dataclass, field
from pathlib import Path

from sources import ROOT, base_name, load_symbols

FUNCTIONS = ROOT / "data/functions.csv"
PROGRESS = ROOT / "data/progress.csv"
SCN_CODE = 0x20
IMAGE_SCN_LNK_COMDAT = 0x1000
IMAGE_SCN_CNT_CODE = 0x20
REL_I386_DIR32 = 0x06
PLACEHOLDER = re.compile(r"(?:DAT|FUN|PTR|LAB)_([0-9a-f]{8})$")


@dataclass
class Definition:
    name: str
    code: bool
    section: str
    comdat: int
    size: int
    fingerprint: str


@dataclass
class ObjectInfo:
    src: str
    defs: dict[str, Definition] = field(default_factory=dict)
    refs: dict[str, bool] = field(default_factory=dict)
    libs: list[str] = field(default_factory=list)
    addends: dict[str, tuple[int, int]] = field(default_factory=dict)


def read_object(src: str, data: bytes) -> ObjectInfo:
    """The external symbols of one COFF object (coff.py skips the auxiliary
    records, which hold each COMDAT section's selection)."""
    info = ObjectInfo(src)
    _, nsects, _, symptr, nsyms, opthdr, _ = struct.unpack_from("<HHIIIHH", data, 0)
    strtab = data[symptr + nsyms * 18:]

    def name_of(raw: bytes) -> str:
        if raw[:4] == b"\0\0\0\0":
            (off,) = struct.unpack_from("<I", raw, 4)
            return strtab[off:strtab.index(b"\0", off)].decode("latin-1")
        return raw[:8].split(b"\0", 1)[0].decode("latin-1")

    sections = []
    for s in range(nsects):
        hdr = data[20 + opthdr + s * 40: 20 + opthdr + s * 40 + 40]
        sname = hdr[:8].split(b"\0", 1)[0].decode("latin-1")
        size, rawptr, relptr, _, nrel, _, chars = struct.unpack_from("<IIIIHHI", hdr, 16)
        sections.append((sname, size, rawptr, relptr, nrel, chars))
        if sname == ".drectve" and rawptr:
            text = data[rawptr:rawptr + size].decode("latin-1")
            info.libs += [m.upper() for m in re.findall(r"-defaultlib:(\S+)", text, re.I)]

    names: dict[int, str] = {}
    selection: dict[int, int] = {}
    externals = []
    i = 0
    while i < nsyms:
        raw = data[symptr + i * 18: symptr + i * 18 + 18]
        value, secnum, typ, sclass, naux = struct.unpack_from("<IhHBB", raw, 8)
        name = name_of(raw)
        names[i] = name
        if sclass == 3 and naux and secnum > 0 and value == 0 and name.startswith("."):
            aux = data[symptr + (i + 1) * 18: symptr + (i + 2) * 18]
            if sections[secnum - 1][5] & IMAGE_SCN_LNK_COMDAT:
                selection[secnum] = aux[14]
        elif sclass == 2:
            externals.append((name, value, secnum, typ))
        i += 1 + naux

    def fingerprint(secnum: int) -> str:
        sname, size, rawptr, relptr, nrel, _ = sections[secnum - 1]
        body = bytearray(data[rawptr:rawptr + size]) if rawptr else bytearray(size)
        targets = []
        for r in range(nrel):
            off, symidx, rtype = struct.unpack_from("<IIH", data, relptr + r * 10)
            target = names.get(symidx, "?")
            targets.append(f"{off}:{rtype}:{'<local>' if target.startswith(('$', '.')) else target}")
            if rtype == REL_I386_DIR32:
                continue
            body[off:off + 4] = b"\0\0\0\0"
        return hashlib.sha1(bytes(body) + "|".join(targets).encode()).hexdigest()[:16]

    data_symbols = set()
    for name, value, secnum, typ in externals:
        if secnum > 0:
            sname, size, _, _, _, chars = sections[secnum - 1]
            sel = selection.get(secnum, 0)
            info.defs[name] = Definition(name, bool(chars & IMAGE_SCN_CNT_CODE), sname, sel, size,
                                         fingerprint(secnum) if sel else "")
            if not chars & IMAGE_SCN_CNT_CODE:
                data_symbols.add(name)
        elif secnum == 0 and value:
            info.defs[name] = Definition(name, False, ".bss", 0, value, "common")
            data_symbols.add(name)
        elif secnum == 0:
            is_func = (typ & 0x30) == 0x20
            info.refs[name] = is_func
            if not is_func:
                data_symbols.add(name)

    for sname, size, rawptr, relptr, nrel, _ in sections:
        if not rawptr:
            continue
        for r in range(nrel):
            off, symidx, rtype = struct.unpack_from("<IIH", data, relptr + r * 10)
            target = names.get(symidx)
            if rtype != REL_I386_DIR32 or target not in data_symbols or off + 4 > size:
                continue
            (addend,) = struct.unpack_from("<i", data, rawptr + off)
            lo, hi = info.addends.get(target, (addend, addend))
            info.addends[target] = (min(lo, addend), max(hi, addend))
    return info


def archive_symbols(path: Path) -> list[str]:
    """The public symbols a .lib offers, from its first linker member."""
    data = path.read_bytes()
    if data[:8] != b"!<arch>\n" or data[8:24].rstrip() != b"/":
        return []
    size = int(data[56:66])
    body = data[68:68 + size]
    (count,) = struct.unpack_from(">I", body, 0)
    return [n.decode("latin-1") for n in body[4 + 4 * count:].split(b"\0")[:count]]


PRIMS = {"C": "signed char", "D": "char", "E": "unsigned char", "F": "short", "G": "unsigned short",
         "H": "int", "I": "unsigned int", "J": "long", "K": "unsigned long", "M": "float",
         "N": "double", "O": "long double", "X": "void", "_J": "__int64", "_K": "unsigned __int64",
         "_N": "bool", "_W": "wchar_t"}


PRIM_SIZE = {"signed char": 1, "char": 1, "unsigned char": 1, "short": 2, "unsigned short": 2,
             "int": 4, "unsigned int": 4, "long": 4, "unsigned long": 4, "float": 4, "double": 8,
             "long double": 8, "__int64": 8, "unsigned __int64": 8, "bool": 1, "wchar_t": 2}


CVS = {"A": "", "B": "const", "C": "volatile", "D": "const volatile"}
CALLCONV = {"A": "__cdecl", "E": "__thiscall", "G": "__stdcall", "I": "__fastcall"}
RECORDS = {"U": "struct", "V": "class", "T": "union"}


class Demangle:
    def __init__(self, text: str):
        self.s, self.i = text, 0
        self.names: list[str] = []
        self.args: list[tuple] = []

    def take(self) -> str:
        c = self.s[self.i]
        self.i += 1
        return c

    def number(self) -> int:
        neg = self.s[self.i] == "?"
        if neg:
            self.i += 1
        c = self.take()
        if c.isdigit():
            n = int(c) + 1
        else:
            n = 0
            while c != "@":
                n = n * 16 + ord(c) - ord("A")
                c = self.take()
        return -n if neg else n

    def fragment(self) -> str:
        c = self.s[self.i]
        if c.isdigit():
            self.i += 1
            return self.names[int(c)]
        if self.s.startswith("?$", self.i):
            self.i += 2
            end = self.s.index("@", self.i)
            tname = self.s[self.i:end]
            self.i = end + 1
            outer = self.names, self.args
            self.names, self.args = [tname], []
            targs = []
            while self.s[self.i] != "@":
                targs.append(self.arg())
            self.i += 1
            self.names, self.args = outer
            inside = ", ".join(declare(t) for t in targs)
            frag = f"{tname}<{inside}{' ' if inside.endswith('>') else ''}>"
            self.names.append(frag)
            return frag
        end = self.s.index("@", self.i)
        frag = self.s[self.i:end]
        self.i = end + 1
        self.names.append(frag)
        return frag

    def qualified(self) -> str:
        parts = [self.fragment()]
        while self.s[self.i] != "@":
            parts.append(self.fragment())
        self.i += 1
        return "::".join(reversed(parts))

    def arg(self) -> tuple:
        if self.s[self.i].isdigit():
            return self.args[int(self.take())]
        start = self.i
        t = self.type()
        if self.i - start > 1:
            self.args.append(t)
        return t

    def type(self) -> tuple:
        c = self.take()
        if c == "_":
            return ("prim", PRIMS["_" + self.take()])
        if c in PRIMS:
            return ("prim", PRIMS[c])
        if c in RECORDS:
            return ("rec", RECORDS[c], self.qualified())
        if c == "W":
            self.take()
            return ("rec", "enum", self.qualified())
        if c in "PQRS":
            own = {"P": "", "Q": "const", "R": "volatile", "S": "const volatile"}[c]
            if self.s[self.i] == "6":
                self.i += 1
                return ("ptr", "", self.function(), own)
            if self.s[self.i] == "8":
                raise ValueError("pointer to member")
            cv = CVS[self.take()]
            if self.s[self.i] == "Y":
                self.i += 1
                dims = [self.number() for _ in range(self.number())]
                return ("ptr", cv, ("arr", dims, self.type()), own)
            return ("ptr", cv, self.type(), own)
        if c == "A":
            return ("ref", CVS[self.take()], self.type())
        if c.isdigit():
            return self.args[int(c)]
        raise ValueError(f"type code {c!r}")

    def function(self) -> tuple:
        cc = CALLCONV.get(self.take(), "")
        ret = self.type()
        args = []
        while self.s[self.i] not in "@Z":
            if self.s[self.i] == "X" and not args:
                self.i += 1
                break
            args.append(self.arg())
        if self.s[self.i] == "@":
            self.i += 1
        if self.i < len(self.s) and self.s[self.i] == "Z":
            self.i += 1
        return ("fn", cc, ret, args)


MEMBER_STATIC = set("CDKLST")


def data_symbol(sym: str) -> tuple[str, tuple | None]:
    """(qualified name, type) of a data symbol; type is None for an
    extern "C" name, which carries none, or one this reader can't parse."""
    if not sym.startswith("?"):
        return sym[1:] if sym.startswith("_") else sym, None
    d = Demangle(sym[1:])
    try:
        name = d.qualified()
        storage = d.take()
        if storage not in "01234":
            return name, None
        t = d.type()
        cv = CVS.get(d.s[d.i], "") if d.i < len(d.s) else ""
        if cv and t[0] in ("prim", "rec"):
            t = ("cv", cv, t)
        return name, t
    except (ValueError, IndexError):
        return base_name(sym), None


def _join(tokens: list[str]) -> str:
    out = ""
    for tok in tokens:
        if not tok:
            continue
        prev = out[-1:] if out else ""
        word = tok[0].isalnum() or tok[0] == "_"
        if out and ((word and (prev.isalnum() or prev in "_*>")) or (tok == "(" and (prev.isalnum() or prev == ">"))
                    or (tok == "*" and prev == ",")):
            out += " "
        out += tok
    return out


def declare(t: tuple | None, name: str = "") -> str:
    """C declaration of `name` with type t (an abstract declarator without a name)."""
    return _join(_declare(t, [name] if name else []))


def _declare(t, inner: list[str], cv: str = "") -> list[str]:
    if t is None:
        return ["?"] + inner
    k = t[0]
    if k == "cv":
        return _declare(t[2], inner, " ".join(x for x in (t[1], cv) if x))
    if k == "prim":
        return ([cv] if cv else []) + [t[1]] + inner
    if k == "rec":
        return ([cv] if cv else []) + [t[2]] + inner
    if k == "raw":
        return [t[1]] + inner
    if k in ("ptr", "ref"):
        star = "*" if k == "ptr" else "&"
        own = " ".join(x for x in ((t[3] if k == "ptr" else ""), cv) if x)
        pointee = t[2]
        pcv = t[1]
        mid = [star] + ([own] if own else []) + inner
        if pointee[0] in ("fn", "arr"):
            return _declare(pointee, ["("] + mid + [")"], pcv)
        return _declare(pointee, mid, pcv)
    if k == "arr":
        dims = "".join(f"[{d}]" if d else "[]" for d in t[1])
        return _declare(t[2], inner + [dims], cv)
    if k == "fn":
        _, cc, ret, args = t
        if inner and inner[0] == "(":
            inner = ["(", cc] + inner[1:] if cc else inner
        arglist = ", ".join(declare(a) for a in args) or "void"
        return _declare(ret, inner + [f"({arglist})"])
    return ["?"] + inner


def type_size(t: tuple | None) -> int | None:
    if t is None:
        return None
    k = t[0]
    if k == "cv":
        return type_size(t[2])
    if k == "prim":
        return PRIM_SIZE.get(t[1])
    if k in ("ptr", "ref"):
        return 4
    if k == "arr":
        inner = type_size(t[2])
        if inner is None or not t[1] or not all(t[1]):
            return None
        n = inner
        for d in t[1]:
            n *= d
        return n
    return None


def load_known() -> tuple[dict[str, int], dict[int, tuple[str, int]], dict[int, tuple[str, str]]]:
    """data/symbols.csv, data/functions.csv (address -> (kind, size)), and
    data/progress.csv (address -> (file, mangled symbol) of every annotated
    definition)."""
    symbols = load_symbols()
    with FUNCTIONS.open() as fh:
        kinds = {int(r["address"], 16): (r["kind"], int(r["size"] or 0)) for r in csv.DictReader(fh)}
    annotated = {}
    if PROGRESS.exists():
        with PROGRESS.open() as fh:
            for r in csv.DictReader(fh):
                if r["symbol"] and r["status"] in ("matched", "partial"):
                    annotated[int(r["address"], 16)] = (r["file"], r["symbol"])
    return symbols, kinds, annotated


def address_of(sym: str, symbols: dict[str, int]) -> int | None:
    """The address a symbol names: its DAT_/FUN_ placeholder, or data/symbols.csv."""
    if sym.startswith("__imp_"):
        return None
    name = base_name(sym)
    if "::" in name and name in symbols:
        return symbols[name]
    m = PLACEHOLDER.search(name.split("::")[-1])
    if m:
        return int(m.group(1), 16)
    if name in symbols:
        return symbols[name]
    if "::" not in name:
        hits = {a for n, a in symbols.items() if "::" in n and n.split("::")[-1] == name}
        if len(hits) == 1:
            return next(iter(hits))
    if not sym.startswith("?"):
        return symbols.get(sym)
    return None


@dataclass
class Sym:
    index: int
    name: str
    value: int
    section: int
    sclass: int
    is_func: bool


@dataclass
class Sec:
    index: int
    name: str
    chars: int
    data: bytes
    relocs: list[tuple[int, int, int]]
    starts: list[int] = field(default_factory=list)

    @property
    def is_code(self) -> bool:
        return bool(self.chars & SCN_CODE)

    def slice_at(self, offset: int) -> tuple[int, int]:
        """The symbol-to-symbol slice holding offset."""
        i = bisect.bisect_right(self.starts, offset) - 1
        lo = self.starts[i] if i >= 0 else 0
        hi = self.starts[i + 1] if i + 1 < len(self.starts) else len(self.data)
        return lo, hi


@dataclass
class Obj:
    path: Path
    secs: list[Sec]
    syms: dict[int, Sym]
    raw: bytes
    library: bool = False
    gap: bool = False
    data: bool = False
    externals: dict[str, Sym] = field(default_factory=dict)
    absolutes: dict[str, int] = field(default_factory=dict)
    commons: dict[str, int] = field(default_factory=dict)
    section_starts: dict[str, int] = field(default_factory=dict)
    key: str = ""


def parse(path: Path, data: bytes | None = None, library: bool = False) -> Obj:
    data = path.read_bytes() if data is None else data
    _, nsects, _, symptr, nsyms, opthdr, _ = struct.unpack_from("<HHIIIHH", data, 0)
    strtab = data[symptr + nsyms * 18:]

    def name_of(raw: bytes) -> str:
        if raw[:4] == b"\0\0\0\0":
            (off,) = struct.unpack_from("<I", raw, 4)
            return strtab[off:strtab.index(b"\0", off)].decode("latin-1")
        return raw[:8].split(b"\0", 1)[0].decode("latin-1")

    secs = []
    for s in range(nsects):
        hdr = data[20 + opthdr + s * 40: 20 + opthdr + s * 40 + 40]
        sname = hdr[:8].split(b"\0", 1)[0].decode("latin-1")
        if sname.startswith("/"):
            sname = strtab[int(sname[1:]):].split(b"\0", 1)[0].decode("latin-1")
        size, rawptr, relptr, _, nrel, _, chars = struct.unpack_from("<IIIIHHI", hdr, 16)
        body = data[rawptr:rawptr + size] if rawptr else bytes(size)
        relocs = [struct.unpack_from("<IIH", data, relptr + r * 10) for r in range(nrel)]
        secs.append(Sec(s + 1, sname, chars, body, relocs))
    syms: dict[int, Sym] = {}
    i = 0
    while i < nsyms:
        raw = data[symptr + i * 18: symptr + i * 18 + 18]
        value, secnum, typ, sclass, naux = struct.unpack_from("<IhHBB", raw, 8)
        syms[i] = Sym(i, name_of(raw), value, secnum, sclass, (typ & 0x30) == 0x20)
        i += 1 + naux
    obj = Obj(path, secs, syms, data, library)
    for s in syms.values():
        if s.section > 0 and s.sclass in (2, 3) and not s.name.startswith("."):
            obj.secs[s.section - 1].starts.append(s.value)
            if s.sclass == 2:
                obj.externals[s.name] = s
        elif s.sclass == 2 and s.section == -1:
            obj.absolutes[s.name] = s.value
        elif s.sclass == 2 and s.section == 0 and s.value:
            obj.commons[s.name] = s.value
    for sec in secs:
        sec.starts = sorted(set(sec.starts))
    return obj

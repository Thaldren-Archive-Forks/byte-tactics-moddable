"""Find float values that game logic keeps on the x87 stack across several operations.

    uv run tools/x87scan.py             # writes data/x87_sites.csv, prints totals
    uv run tools/x87scan.py --quiet     # only write the file
    uv run tools/x87scan.py --formats   # also list the printf/scanf format strings of game logic

The game runs with the FPU at 53-bit precision (`_control87` 0x9001F), so an
x87 intermediate is a double with the extended exponent range. MSVC 5 keeps a
float expression on the stack and rounds it to 32 bits only where the source
stores it. A compiler that evaluates `float` in float (SSE, wasm f32) rounds
after every operation instead, and gets different bits wherever an expression
of two or more operations ends in a 32-bit store, a `double` store or a
conversion to int.

Method. The game-logic functions are those tools/crtcalls.py finds (its
`analyze`). Each source is compiled again with `/Zd` (line numbers only; the
function bytes are checked against the cached objects of tools/progress.py)
into build/x87scan/, and every game-logic function is decoded by recursive
descent as crtcalls does. The scan follows the x87 stack through the
instructions in address order, keeping for each slot the expression tree that
produced it, with a type for each node as C gives it: a `dword` load (float, F),
a `qword` load (double, D), an integer load (I), a constant such as fld1 (C),
or a value from a call or from an earlier block (U). A node is D if any operand
is D, else U if any is U, else F if any is F, else A (only integers and
constants, ambiguous). A site is recorded at each store (`fst`/`fstp` dword or
qword), `fist`/`fistp` and call to `_ftol`, when the expression has at least
two arithmetic operations (fadd, fsub, fmul, fdiv and variants, fsqrt, the
`_CI` math calls) and a float or integer load among its leaves.

Classes, from the node types:

  float-chain  an F-typed node (arithmetic on floats in C) is consumed by
               another operation, a conversion to int or a double store, so a
               float-evaluating compiler rounds where x87 does not
  safe         every consumed node is D: the C source uses double there, and a
               modern compiler computes the same bits
  unknown      a consumed node is A or U (a call result, a value from another
               block, integers only); read the source

A chain of integer loads alone cannot say whether the C was float or double
(`(float)a * b` and `(double)a * b` compile alike), and an inlined function's
operations are not those of the line it is reported on. REVIEWED holds the
class of each such site, from reading the C source; it is keyed by function
address and offset, so a change to the source needs it checked again.

Lines. The `/Zd` objects carry COFF line numbers, so each site gets the source
line of its store. A function from a header (an inline) is reported with the
line in that header and the file of the .cpp that holds it.

A chain left in st0 for the caller at a `ret` (a function returning float or
double) has no store in this function; the totals print those and leave them
out of the file.

Columns of data/x87_sites.csv: address (of the function), function, file,
line, offset (of the store or conversion, from the function start), chain (the
expression, f/d/i for float, double and integer loads), result (float-store,
double-store, ftol or fistp), class, disposition. Sorted by address and offset.

Disposition: `rewritten` for a float-chain site whose C source now states the
precision x87 used (REWRITTEN, the site's class then reads `safe`, and the
function still matches), `5` for a float-chain site nobody has handled yet (`--check` fails on it), `modern` for a float-chain site whose chain the source
states only under `#ifdef __clang__` (the MSVC 5 source is unchanged), `n/a`
for a safe site.
"""

import argparse
import csv
import io
import re
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from check import DEFAULT_FLAGS, ROOT, base_name, compile_source, select_function, source_key
from coff import parse_object
from crtcalls import analyze, decode_function
from sources import is_gap_source

OUT = ROOT / "data/x87_sites.csv"
BUILD = ROOT / "build/x87scan"
FLAGS = DEFAULT_FLAGS + " /Zd"
MAX_CHAIN = 240
FORMAT_CALLS = {"sprintf", "sscanf", "printf", "fprintf", "vsprintf", "_snprintf"}
REVIEWED = {
    (0x408100, 0x171): "float-chain",
    (0x408100, 0x292): "float-chain",
    (0x408100, 0x3a8): "float-chain",
    (0x40bfe0, 0xa6): "safe",
    (0x437840, 0xa6): "safe",
    (0x4394e0, 0x9c): "safe",
    (0x43d290, 0x3e7): "safe",
    (0x474760, 0x86): "safe",
    (0x48a1e0, 0x19e): "safe",
    (0x49a120, 0x26a): "safe",
    (0x49a120, 0x494): "safe",
    (0x49a850, 0x33): "safe",
    (0x49b3e0, 0x5d): "safe",
    (0x49be60, 0x691): "safe",
    (0x4a2580, 0x4fa): "float-chain",
    (0x4a2be0, 0xe3): "float-chain",
    (0x4a2be0, 0x211): "float-chain",
    (0x4a2e40, 0x254): "float-chain",
    (0x4a3ef0, 0x173): "float-chain",
    (0x4a4660, 0xf3): "float-chain",
}
REWRITTEN = {
    (0x401360, 0x78a),
    (0x401360, 0x7c7),
    (0x43d290, 0x15d),
    (0x43d290, 0x1af),
    (0x468cf0, 0x41e),
    (0x468cf0, 0x49e),
    (0x408100, 0x171),
    (0x408100, 0x292),
    (0x408100, 0x3a8),
    (0x468cf0, 0x702),
    (0x468cf0, 0x782),
}
MODERN = {
    (0x401360, 0x240),
    (0x401360, 0x685),
    (0x401360, 0x6e5),
    (0x401360, 0x80b),
    (0x401360, 0x839),
    (0x401360, 0x87d),
    (0x401360, 0x8b6),
    (0x409730, 0x1c9),
    (0x409730, 0x1e4),
    (0x409730, 0x5ab),
    (0x40aa40, 0x22d),
    (0x40aa40, 0x237),
    (0x40aa40, 0x23b),
    (0x41bd10, 0x53),
    (0x41bd10, 0x72),
    (0x43d290, 0x362),
    (0x43d290, 0x36a),
    (0x43d290, 0x36e),
    (0x43d290, 0x372),
    (0x459c70, 0x677),
    (0x49a120, 0x2c0),
    (0x4a2580, 0x4fa),
    (0x4a2be0, 0xe3),
    (0x4a2be0, 0x211),
    (0x4a2e40, 0x254),
    (0x4a3ef0, 0x173),
    (0x4a4660, 0xf3),
    (0x4b6f70, 0x47),
    (0x4b6f70, 0x4d),
    (0x4b6f70, 0x5e),
    (0x4b6ff0, 0x49),
    (0x4b6ff0, 0x53),
    (0x4b6ff0, 0x5d),
}
FLOAT_FORMAT = re.compile(r"%[-+ #0]*(?:\d+|\*)?(?:\.(?:\d+|\*))?[lL]?[feEgG]")

ARITH = {"fadd": "+", "fsub": "-", "fsubr": "-", "fmul": "*", "fdiv": "/", "fdivr": "/",
         "fiadd": "+", "fisub": "-", "fisubr": "-", "fimul": "*", "fidiv": "/", "fidivr": "/"}
REVERSED = {"fsubr", "fdivr", "fisubr", "fidivr"}
UNARY_OPS = {"fsqrt": "sqrt", "fsin": "sin", "fcos": "cos", "fptan": "tan", "frndint": "rint",
             "f2xm1": "f2xm1", "fyl2x": "yl2x", "fyl2xp1": "yl2xp1"}
TRANSPARENT = {"fchs": "-", "fabs": "abs"}
CONSTANTS = {"fld1": "1", "fldz": "0", "fldpi": "pi", "fldl2e": "l2e", "fldl2t": "l2t", "fldlg2": "lg2",
             "fldln2": "ln2"}
COMPARES = {"fcom": 0, "fcomp": 1, "fcompp": 2, "fucom": 0, "fucomp": 1, "fucompp": 2,
            "ficom": 0, "ficomp": 1}
WIDTH_TYPE = {"dword": "F", "qword": "D", "tbyte": "D"}
TYPE_LETTER = {"F": "f", "D": "d", "I": "i", "C": "", "U": "?", "A": ""}
FLOAT_CALLS_D = {"atof", "_hypot", "hypot", "floor", "ceil", "_floor", "_ceil", "sqrt", "_CIsqrt"}


class Node:
    """One value of the x87 stack: a load, or an operation on other nodes."""

    __slots__ = ("typ", "text", "children", "ops")

    def __init__(self, typ: str, text: str, children: tuple["Node", ...] = ()):
        self.typ = typ
        self.text = text
        self.children = children
        self.ops = 0

    @property
    def is_op(self) -> bool:
        return bool(self.children)


def combine(op: str, kids: tuple[Node, ...], unary_d: bool = False) -> Node:
    types = {k.typ for k in kids}
    if unary_d or "D" in types:
        typ = "D"
    elif "U" in types:
        typ = "U"
    elif "F" in types:
        typ = "F"
    else:
        typ = "A"
    if len(kids) == 1:
        text = f"{op}({kids[0].text})"
    else:
        text = f"({kids[0].text} {op} {kids[1].text})"
    return Node(typ, text, kids)


def unique_nodes(root: Node) -> list[Node]:
    seen: dict[int, Node] = {}
    todo = [root]
    while todo:
        n = todo.pop()
        if id(n) not in seen:
            seen[id(n)] = n
            todo.extend(n.children)
    return list(seen.values())


def classify(root: Node, result: str) -> tuple[str, int, bool]:
    """(class, number of operations, whether a float or integer load is a leaf)."""
    nodes = unique_nodes(root)
    ops = sum(1 for n in nodes if n.is_op)
    loads = any(not n.is_op and n.typ in ("F", "I") for n in nodes)
    consumed: list[Node] = [c for n in nodes for c in n.children if c.is_op]
    if result in ("ftol", "fistp", "double-store"):
        consumed.append(root) if root.is_op else None
    if any(c.typ == "F" for c in consumed):
        return "float-chain", ops, loads
    if any(c.typ in ("A", "U") for c in consumed):
        return "unknown", ops, loads
    return "safe", ops, loads


def compact(op_str: str) -> str:
    m = re.search(r"\[(.*)\]", op_str)
    return "[" + m.group(1).replace(" ", "") + "]" if m else op_str


def operand_width(op_str: str) -> str:
    return op_str.split(" ", 1)[0] if " ptr " in op_str else ""


def reloc_name(sec, ins) -> str:
    for r in sec.relocs:
        if ins.address <= r.offset < ins.address + ins.size:
            name = base_name(r.symbol)
            m = re.match(r"__real@\d+@([0-9a-f]+)", r.symbol)
            if m:
                return "const" + m.group(1)
            return name.lstrip("_") if not name.startswith("??") else "str"
    return ""


def call_symbol(sec, ins) -> str:
    for r in sec.relocs:
        if ins.address <= r.offset < ins.address + ins.size:
            return r.symbol
    return ""


class Stack:
    def __init__(self):
        self.slots: list[Node] = []
        self.call_result = ""

    def reset(self) -> None:
        self.slots = []
        self.call_result = ""

    def push(self, n: Node) -> None:
        self.slots.append(n)

    def pop(self) -> Node:
        if self.slots:
            return self.slots.pop()
        name, self.call_result = self.call_result, ""
        return Node("U", f"?{name}")

    def top(self, i: int = 0) -> Node:
        if i < len(self.slots):
            return self.slots[-1 - i]
        return Node("U", "?")

    def set(self, i: int, n: Node) -> None:
        if i < len(self.slots):
            self.slots[-1 - i] = n


def reg_index(text: str) -> int | None:
    m = re.fullmatch(r"st\((\d)\)|st", text.strip())
    if not m:
        return None
    return int(m.group(1)) if m.group(1) else 0


def scan_function(sec, insns) -> list[tuple[int, Node, str]]:
    """Walk a function's decoded instructions in address order and return
    (offset of the store or conversion, expression, result) for each."""
    stack = Stack()
    found: list[tuple[int, Node, str]] = []
    prev_falls = True
    for ins in sorted(insns, key=lambda i: i.address):
        if not prev_falls:
            stack.reset()
        mn = ins.mnemonic
        ops = [o.strip() for o in ins.op_str.split(",")] if ins.op_str else []
        prev_falls = mn not in ("ret", "jmp")
        if mn == "ret" and stack.slots:
            found.append((ins.address, stack.top(), "return"))
        if mn == "call":
            sym = call_symbol(sec, ins)
            plain = sym.lstrip("_")
            if plain == "ftol":
                found.append((ins.address, stack.pop(), "ftol"))
                stack.reset()
            elif plain.startswith("CI") and plain[2:] in ("acos", "asin", "atan", "sin", "cos", "tan", "exp",
                                                         "log", "log10", "sqrt", "fabs"):
                arg = stack.pop()
                stack.push(combine(plain[2:], (arg,), unary_d=True))
            elif plain.startswith("CI") and plain[2:] in ("atan2", "pow", "fmod"):
                b = stack.pop()
                a = stack.pop()
                stack.push(combine(plain[2:], (a, b), unary_d=True))
            elif plain in FLOAT_CALLS_D:
                stack.reset()
                stack.push(Node("D", f"{plain}()"))
            else:
                stack.reset()
                stack.call_result = plain or "indirect"
            continue
        if not mn.startswith("f"):
            continue
        if mn in ("fnstsw", "fstsw", "fldcw", "fnstcw", "fstcw", "fwait", "fnclex", "fnop", "ffree"):
            continue
        if mn == "fninit" or mn == "finit":
            stack.reset()
            continue
        mem = ops[0] if len(ops) == 1 and "[" in ops[0] else ""
        width = operand_width(mem)
        label = compact(mem) + (reloc_name(sec, ins) and "/" + reloc_name(sec, ins) or "") if mem else ""
        if mn in CONSTANTS:
            stack.push(Node("C", CONSTANTS[mn]))
        elif mn == "fld":
            if mem:
                typ = WIDTH_TYPE.get(width, "D")
                stack.push(Node(typ, TYPE_LETTER[typ] + label))
            else:
                stack.push(stack.top(reg_index(ops[0]) or 0))
        elif mn == "fild":
            stack.push(Node("I", {"word": "w", "dword": "i", "qword": "q"}.get(width, "i") + label))
        elif mn == "fxch":
            i = reg_index(ops[0]) if ops else 1
            a, b = stack.top(0), stack.top(i or 1)
            stack.set(0, b)
            stack.set(i or 1, a)
        elif mn in TRANSPARENT:
            top = stack.top()
            stack.set(0, Node(top.typ, f"{TRANSPARENT[mn]}{top.text}", top.children))
        elif mn in UNARY_OPS:
            if mn in ("fyl2x", "fyl2xp1"):
                x, y = stack.pop(), stack.pop()
                stack.push(combine(UNARY_OPS[mn], (y, x), unary_d=True))
            else:
                stack.set(0, combine(UNARY_OPS[mn], (stack.top(),), unary_d=True))
        elif mn == "fpatan":
            x, y = stack.pop(), stack.pop()
            stack.push(combine("patan", (y, x), unary_d=True))
        elif mn in ("fprem", "fprem1", "fscale", "fxtract", "fsincos", "fincstp", "fdecstp"):
            stack.reset()
        elif mn.rstrip("p") in ARITH or mn in ARITH:
            popping = mn.endswith("p") and mn not in ARITH
            base = mn[:-1] if popping else mn
            sym = ARITH[base]
            if mem:
                rhs_typ = "I" if base.startswith("fi") else WIDTH_TYPE.get(width, "D")
                rhs = Node(rhs_typ, TYPE_LETTER[rhs_typ] + label)
                lhs = stack.top()
                kids = (rhs, lhs) if base in REVERSED else (lhs, rhs)
                stack.set(0, combine(sym, kids))
            else:
                if len(ops) == 2:
                    dst, src = reg_index(ops[0]) or 0, reg_index(ops[1]) or 0
                elif len(ops) == 1 and popping:
                    dst, src = reg_index(ops[0]) or 0, 0
                elif len(ops) == 1:
                    dst, src = 0, reg_index(ops[0]) or 0
                else:
                    dst, src = 1, 0
                a, b = stack.top(dst), stack.top(src)
                kids = (b, a) if base in REVERSED else (a, b)
                stack.set(dst, combine(sym, kids))
                if popping:
                    stack.pop()
        elif mn in COMPARES:
            for _ in range(COMPARES[mn]):
                stack.pop()
        elif mn in ("fst", "fstp"):
            top = stack.top()
            if mem:
                if width == "dword":
                    found.append((ins.address, top, "float-store"))
                elif width == "qword":
                    found.append((ins.address, top, "double-store"))
            elif ops:
                stack.set(reg_index(ops[0]) or 0, top)
            if mn == "fstp":
                stack.pop()
        elif mn in ("fist", "fistp"):
            found.append((ins.address, stack.top(), "fistp"))
            if mn == "fistp":
                stack.pop()
        else:
            stack.reset()
    return found


def source_lines(sec, first_lines: dict[int, int]) -> list[tuple[int, int]]:
    """(section offset, absolute source line), sorted by offset."""
    out = []
    base = 0
    for key, line in sec.lines:
        if line == 0:
            base = first_lines.get(key, 0)
        else:
            out.append((key, base + line))
    return sorted(out)


def line_at(table: list[tuple[int, int]], offset: int) -> int:
    best = 0
    for off, line in table:
        if off > offset:
            break
        best = line
    return best


def fixed_bytes(sec, lo: int, hi: int) -> bytes:
    """A function's bytes with the fields the linker patches zeroed."""
    return bytes(b if m else 0 for b, m in zip(sec.data[lo:hi], sec.mask()[lo:hi]))


def compile_lines(src: Path) -> tuple[Path | None, str]:
    key = source_key(src, FLAGS)
    stamp = BUILD / src.relative_to(ROOT / "src").with_suffix(".key")
    obj = stamp.with_suffix(".obj")
    stamp.parent.mkdir(parents=True, exist_ok=True)
    if stamp.exists() and stamp.read_text() == key and obj.exists():
        return obj, ""
    out, log = compile_source(src, flags=FLAGS, out_dir="x87scan")
    if out:
        stamp.write_text(key)
    return out, log


def literal_string(obj, symbol: str) -> str | None:
    for s in obj.symbols:
        if s.name == symbol and s.section > 0:
            data = obj.sections[s.section - 1].data
            end = data.find(b"\0", s.value)
            return data[s.value:end if end >= 0 else None].decode("latin-1")
    return None


def format_strings(obj, sec, insns, lo: int) -> list[tuple[str, str]]:
    """(callee, format string) for each call to a printf or scanf family function:
    the string literals pushed since the previous call that contain a `%`."""
    out = []
    pending: list[str] = []
    for ins in sorted(insns, key=lambda i: i.address):
        for r in sec.relocs:
            if ins.address <= r.offset < ins.address + ins.size and ins.mnemonic != "call":
                if r.symbol.startswith(("??_C@", "$SG")):
                    text = literal_string(obj, r.symbol)
                    if text is not None:
                        pending.append(text)
        if ins.mnemonic == "call":
            callee = call_symbol(sec, ins).lstrip("_")
            if callee in FORMAT_CALLS:
                fmts = [t for t in pending if "%" in t]
                out.append((callee, " | ".join(fmts) if fmts else ""))
            pending = []
    return out


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true",
                    help="write nothing; exit 1 on a float chain that is neither rewritten nor modern, or a stale CSV")
    ap.add_argument("--quiet", action="store_true", help="only write the file")
    ap.add_argument("--formats", action="store_true",
                    help="print the printf and scanf format strings of game-logic functions, flagging %%f %%e %%g")
    args = ap.parse_args()

    g = analyze()
    by_file: dict[str, list[int]] = {}
    for a in sorted(g.game_reach):
        by_file.setdefault(g.infos[a]["file"], []).append(a)
    files = sorted(by_file)
    with ThreadPoolExecutor(max_workers=4) as pool:
        compiled = dict(zip(files, pool.map(lambda f: compile_lines(ROOT / f), files)))

    rows = []
    fmt_rows = []
    mismatched: list[str] = []
    functions_with_sites: set[int] = set()
    single_op_ftol = 0
    returned: list[tuple[int, str, str]] = []
    for f in files:
        obj_path, log = compiled[f]
        if not obj_path:
            sys.exit(f"{f}: does not compile with {FLAGS}\n{log}")
        obj = parse_object(obj_path.read_bytes(), obj_path.name)
        for address in by_file[f]:
            info = g.infos[address]
            picked, err = select_function(obj, None, info["qual"])
            if not picked:
                sys.exit(f"{f} {address:#x}: {err}")
            name, sec, lo, hi = picked
            if fixed_bytes(sec, lo, hi) != fixed_bytes(info["sec"], info["lo"], info["hi"]):
                mismatched.append(f"{address:#x}")
            labels = {s.name: s.value for s in obj.symbols if s.section == sec.index and s.name.startswith("$L")}
            insns, _, _ = decode_function(sec, lo, hi, labels, is_gap_source(ROOT / f))
            table = source_lines(sec, obj.first_lines)
            fname = g.by_addr.get(address) or base_name(name)
            for callee, fmt in format_strings(obj, sec, insns, lo):
                fmt_rows.append((address, fname, f, callee, fmt, g.infos[address]["qual"]))
            for off, root, result in scan_function(sec, insns):
                cls, ops, loads = classify(root, result)
                if result == "return":
                    if ops >= 2 and loads:
                        returned.append((address, fname, cls))
                    continue
                if ops == 1 and result == "ftol" and root.typ == "F":
                    single_op_ftol += 1
                if ops < 2 or not loads:
                    continue
                cls = REVIEWED.get((address, off - lo), cls)
                if (address, off - lo) in REWRITTEN:
                    cls = "safe"
                    disposition = "rewritten"
                elif cls == "float-chain":
                    disposition = "modern" if (address, off - lo) in MODERN else "5"
                else:
                    disposition = "n/a"
                functions_with_sites.add(address)
                chain = root.text if len(root.text) <= MAX_CHAIN else root.text[:MAX_CHAIN - 3] + "..."
                rows.append({"address": f"{address:#x}", "function": fname, "file": f,
                             "line": line_at(table, off), "offset": f"{off - lo:#x}", "chain": chain,
                             "result": result, "class": cls, "disposition": disposition, "_a": address, "_o": off - lo})
    rows.sort(key=lambda r: (r["_a"], r["_o"]))
    text = io.StringIO()
    w = csv.DictWriter(text, ["address", "function", "file", "line", "offset", "chain", "result", "class", "disposition"],
                       lineterminator="\n", extrasaction="ignore")
    w.writeheader()
    w.writerows(rows)
    todo = [r for r in rows if r["disposition"] == "5"]
    if args.check:
        for r in todo:
            print(f"x87scan: unhandled float chain in {r['function']} at {r['address']}+{r['offset']} ({r['file']}:{r['line']})")
        stale = OUT.read_text() != text.getvalue()
        if stale:
            print("x87scan: data/x87_sites.csv is out of date, run tools/x87scan.py")
        sys.exit(1 if todo or stale else 0)
    OUT.write_text(text.getvalue())

    if args.quiet:
        return
    print(f"{len(g.game_reach)} game-logic functions in {len(files)} files; "
          f"{len(functions_with_sites)} have sites; {len(rows)} sites")
    for cls in ("float-chain", "safe", "unknown"):
        in_cls = [r for r in rows if r["class"] == cls]
        per = {res: sum(1 for r in in_cls if r["result"] == res)
               for res in ("float-store", "double-store", "ftol", "fistp")}
        print(f"  {cls}: {len(in_cls)} sites in {len({r['address'] for r in in_cls})} functions; {per}")
    print("chains of two or more operations returned in st0 (not listed): "
          + ", ".join(sorted({f"{n} ({c})" for _, n, c in returned})))
    print(f"single-operation float expressions converted by _ftol (not listed): {single_op_ftol}")
    print(f"functions whose /Zd code differs from the cached object: {len(mismatched)} {mismatched[:8]}")
    no_line = sum(1 for r in rows if not r["line"])
    print(f"sites without a source line: {no_line}")
    if args.formats:
        for address, fname, f, callee, fmt, qual in sorted(fmt_rows):
            flag = "FLOAT" if FLOAT_FORMAT.search(fmt) else ""
            print(f"{address:#x} {fname} {f} {callee} {flag} {fmt!r}")


if __name__ == "__main__":
    main()

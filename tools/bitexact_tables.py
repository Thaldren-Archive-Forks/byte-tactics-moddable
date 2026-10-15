"""Generate lib/bitexact/bx_tables.c from orig/TotalA.exe.

    uv run tools/bitexact_tables.py

The tables the hand-written trig in src/util/math_4b70a0.cpp reads are data in
the exe: the sine table (0x509f00, with the cosine table a quarter turn on at
0x50a000, up to the rotation matrix at 0x50a400) and the two doubles at
0x509ef0 and 0x509ef8. They are copied byte for byte.

The exe's x87 code takes sin and cos from fsincos, whose 64-bit results the
port cannot compute with C doubles. Only 65536 angles reach it, so their
results are tabulated here: the correctly rounded 64-bit mantissa of sin and
cos of the double angle * g_angle16ToRad, computed in exact integer
arithmetic (tools/bitexact.py checks the table against the CPU's fsincos).
The same arithmetic gives atan(k/16) and pi as double-double constants for
the port's atan2.
"""

import math
import struct
from fractions import Fraction
from pathlib import Path

from check import ROOT, Original

OUT = ROOT / "lib/bitexact/bx_tables.c"
SIN_TABLE = 0x509F00
SIN_TABLE_BYTES = 0x500
RAD_TO_ANGLE16 = 0x509EF0
ANGLE16_TO_RAD = 0x509EF8

PREC = 256
ONE = 1 << PREC


def sincos_scaled(theta: float) -> tuple[int, int]:
    f = Fraction(theta)
    x = f.numerator * ONE // f.denominator
    x2 = x * x // ONE
    sin = term = x
    cos = tcos = ONE
    n = 1
    while term or tcos:
        tcos = -tcos * x2 // ONE // ((2 * n - 1) * (2 * n))
        term = -term * x2 // ONE // ((2 * n) * (2 * n + 1))
        cos += tcos
        sin += term
        n += 1
    return sin, cos


def to_x87(v: int) -> tuple[int, int, int]:
    """(mantissa high word, low word, sign/exponent word) of v / ONE as an 80-bit extended."""
    if v == 0:
        return 0, 0, 0
    sign = 0x8000 if v < 0 else 0
    a = abs(v)
    e = a.bit_length()
    shift = e - 64
    m, rem = divmod(a, 1 << shift)
    half = 1 << (shift - 1)
    if rem > half or (rem == half and m & 1):
        m += 1
    if m == 1 << 64:
        m >>= 1
        e += 1
    exp = e - PREC - 1 + 16383
    return m >> 32, m & 0xFFFFFFFF, sign | exp


def atan_scaled(x: int) -> int:
    for _ in range(6):
        x = x * ONE // (ONE + math.isqrt(ONE * ONE + x * x))
    x2 = x * x // ONE
    total = term = x
    n = 1
    while term:
        term = -term * x2 // ONE
        total += term // (2 * n + 1)
        n += 1
    return total * 64


def double_bits(d: float) -> tuple[int, int]:
    b = struct.unpack("<Q", struct.pack("<d", d))[0]
    return b >> 32, b & 0xFFFFFFFF


def double_double(v: int) -> tuple[tuple[int, int], tuple[int, int]]:
    f = Fraction(v, ONE)
    hi = float(f)
    lo = float(f - Fraction(hi))
    return double_bits(hi), double_bits(lo)


def rows(values: list[str], per_line: int) -> str:
    return "".join(
        "    " + ", ".join(values[i:i + per_line]) + (",\n" if i + per_line < len(values) else "\n")
        for i in range(0, len(values), per_line))


def main() -> None:
    orig = Original()
    sin = struct.unpack(f"<{SIN_TABLE_BYTES // 2}h", orig.read(SIN_TABLE, SIN_TABLE_BYTES))
    rad_to_angle16 = orig.read(RAD_TO_ANGLE16, 8)
    angle16_to_rad_bytes = orig.read(ANGLE16_TO_RAD, 8)
    angle16_to_rad = struct.unpack("<d", angle16_to_rad_bytes)[0]
    lo_hi = lambda b: struct.unpack("<II", b)[::-1]

    out = ['#include "bx_tables.h"\n\n']
    out.append(f"const bx_i16 bx_sin_table[{len(sin)}] = {{\n{rows([str(v) for v in sin], 16)}}};\n\n")
    for name, b in (("bx_rad_to_angle16_bits", rad_to_angle16), ("bx_angle16_to_rad_bits", angle16_to_rad_bytes)):
        hi, lo = lo_hi(b)
        out.append(f"const bx_u32 {name}[2] = {{0x{hi:08x}u, 0x{lo:08x}u}};\n")
    out.append("\n")

    atans = []
    for k in range(17):
        v = atan_scaled(k * ONE // 16)
        if k == 16:
            pi_half = Fraction(2 * v, ONE)
            assert abs(float(pi_half) * 2 - math.pi) < 1e-15
            pi_dd = double_double(4 * v)
            half_dd = double_double(2 * v)
        assert abs(float(Fraction(v, ONE)) - math.atan(k / 16)) < 1e-15
        atans.append(double_double(v))
    vals = []
    for hi, lo in atans:
        vals += [f"0x{hi[0]:08x}u", f"0x{hi[1]:08x}u", f"0x{lo[0]:08x}u", f"0x{lo[1]:08x}u"]
    out.append("const bx_u32 bx_atan_sixteenths[17 * 4] = {\n" + rows(vals, 4) + "};\n\n")
    vals = []
    for hi, lo in (pi_dd, half_dd):
        vals += [f"0x{hi[0]:08x}u", f"0x{hi[1]:08x}u", f"0x{lo[0]:08x}u", f"0x{lo[1]:08x}u"]
    out.append("const bx_u32 bx_pi_pi_half[2 * 4] = {\n" + rows(vals, 4) + "};\n\n")

    vals = []
    for idx in range(65536):
        a = idx - 65536 if idx >= 32768 else idx
        s, c = sincos_scaled(a * angle16_to_rad)
        sh, sl, se = to_x87(s)
        ch, cl, ce = to_x87(c)
        vals += [f"0x{sh:08x}u", f"0x{sl:08x}u", f"0x{ch:08x}u", f"0x{cl:08x}u", f"0x{se:04x}{ce:04x}u"]
    out.append("const bx_u32 bx_sincos_table[65536 * 5] = {\n" + rows(vals, 5) + "};\n")
    OUT.write_text("".join(out))
    print(f"wrote {OUT.relative_to(ROOT)} ({OUT.stat().st_size:,} bytes)")


if __name__ == "__main__":
    main()

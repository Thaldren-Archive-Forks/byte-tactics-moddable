# lib/bitexact

A portable C port of the hand-written fixed-point trigonometry in
`src/util/math_4b70a0.cpp`, giving the same result as the original for every
input. It lives outside `src/` so that `tools/link.py` never builds it into the
reference exe. It needs no libm, no `long double`, and no platform headers.

Plain C89 (MSVC 5 and clang), types from `bitexact.h` (`bx_i32` and so on), names
prefixed `bx_`. Little-endian only (x86, wasm).

## Entry points

| Port | Original |
| --- | --- |
| `bx_FixedSine` | `FixedSine` 0x4b70a0 |
| `bx_fixed_cos` | 0x4b70c0 |
| `bx_mul_hi32` | 0x4b70e0 |
| `bx_FUN_004b70ef`, `bx_FUN_004b7123` | `FUN_004b70ef`, `FUN_004b7123` |
| `bx_FUN_004b715a` | `FUN_004b715a` (atan2) |
| `bx_FUN_004b7173` | `FUN_004b7173` (rotate a point) |
| `bx_build_rotation_matrix` | 0x4b71a7, into a caller-owned `bx_i32[9]` |
| `bx_apply_rotation_matrix` | 0x4b72e2, matrix as a parameter |
| `bx_FUN_004b7381` | `FUN_004b7381` |

## Data

`bx_tables.c` is not committed. `uv run tools/bitexact_tables.py` computes it
(by default into `build/bitexact/bx_tables.c`), `tools/bitexact.py` runs it
before every build, and #5's build does the same. The generator does not read the exe, and its output is deterministic.
The tables, declared in `bx_tables.h`:

- `bx_sin_table`: 640 `bx_i16` entries, `round(sin(i * 2 * pi / 512) * 0x2000)`; the cosine table is the same array 128 entries on.
- `bx_rad_to_angle16_bits`, `bx_angle16_to_rad_bits`: the doubles `g_radToAngle16` and `g_angle16ToRad` as {high word, low word}. They are the decimal constants 10430.37835047 and 9.587379924285e-05, not exactly 0x8000 / pi and pi / 0x8000.
- `bx_atan_sixteenths`: atan(k / 16) for k = 0..16 as double-double, four words each: {hi high, hi low, lo high, lo low}.
- `bx_pi_pi_half`: pi, then pi / 2, as double-double in the same layout.
- `bx_sincos_table`: for each 16-bit angle (index: the angle as unsigned), five words: {sin mantissa high, sin mantissa low, cos mantissa high, cos mantissa low, sin sign/exponent in the high half and cos sign/exponent in the low half}. These are the 80-bit results the exe's `fsincos` returns, rounded exactly with integer arithmetic, so the rotate needs no `sin()`.

`tools/bitexact.py` also checks the generated sine table and doubles against the
bytes in `orig/TotalA.exe` and fails on any difference.

## How exactness is reached

The game runs with the FPU at 53-bit precision, round to nearest
(`_control87` 0x9001F). Under that mode `fmul`, `fadd` and `fistp` are plain
IEEE double operations, and the port does them in C doubles and 64-bit
integers. A build that evaluates doubles in x87 registers must set the same
mode; SSE2 and wasm need nothing.

- Rotate: the products with the 64-bit `fsincos` values are rounded to 53 bits in integer code, then summed and rounded to int like `fistp`, including its out-of-range result (`INT_MIN`).
- atan2: the exe's `fpatan` is not reproducible bit for bit in general. The port computes the angle in double-double (error far below 1e-15 of a unit) and rounds like the exe, so it differs only for inputs whose exact angle is within about 1e-13 of a half. No such input turned up in the tests.
- `fsincos` on a CPU is not correctly rounded near a quarter turn (this machine's differs from exact by up to thousands of ulps of a value near zero). The effect on a rotated int needs an input within about 1e-10 of a half, and none turned up either.

## Tests

`uv run tools/bitexact.py` builds `src/util/math_4b70a0.cpp` (as `tools/check.py`
compiles it) and the port into one console exe with the matching toolchain, runs
it under Wine and prints `name: N cases, M mismatches` per entry point. The
exe calls the original's entry points by their offsets from `FixedSine`, and
gets the original's tables (`g_sinTable`, `g_cosTable` and the two doubles) from the bytes of `orig/TotalA.exe`, while the port uses the generated ones. The exit status is
non-zero on any mismatch.

Inputs: every 16-bit angle, crossed with scales 0, +-1, powers of two,
INT_MIN and INT_MAX, and 10^6 or more random cases per function from a fixed
xorshift seed.

## Under clang (#5)

`test/bitexact_test.c` compares through a handful of `orig_*` function
pointers. For clang (i686 and wasm32) the same source is built either with
the vectors the MSVC 5 exe writes (inputs from the same generator, outputs of
the original), or, for i686, with the original linked in. Build with
`-msse2 -mfpmath=sse` (or the 53-bit mode set before the tests), `-std=c89`,
and the port must produce the same `0 mismatches` lines.

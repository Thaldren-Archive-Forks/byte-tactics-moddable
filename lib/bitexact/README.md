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
| `bx_sqrt` | x87 `fsqrt` at 53 bits (`sqrt`), in integer code |
| `bx_hypot` | LIBCMT `_hypot` 0x4fb440, its 64-bit-mantissa steps in integer code |
| `bx_acos` | LIBCMT `_CIacos` 0x4e67f0: `fpatan(sqrt((1+x)(1-x)), x)` from a double-double arctangent |
| `bx_strtof_msvc5` | the text to float of LIBCMT's `sscanf("%f")` (`_atoflt`, `___strgtold12`, `__ld12cvt`), digit for digit |
| `bx_rand_state`, `bx_srand`, `bx_rand` | LIBCMT `srand` 0x4e4860, `rand` 0x4e4870 |
| `bx_random_shuffle_int` | `std::random_shuffle`, MSVC 5's STL template (inlined, no address) |
| `bx_explosion_frame_draws`, `bx_init_explosions_draws`, `bx_init_explosions_rand` | the rand calls of `BuildExplosionFrame` 0x420d20 and `InitExplosions` 0x420620 |
| `bx_qsort` | LIBCMT `qsort` 0x4e5ed0 |

## rand, random_shuffle, qsort

`rand.c` and `qsort.c`, plain C89 like the trig port.

- **Explicit state.** LIBCMT keeps the `rand` seed per thread (`_getptd()+0x14`). The port takes a `bx_rand_state` so a caller names the stream it means. `seed = seed * 214013 + 2531011`, the value is `(seed >> 16) & 0x7fff`, and `bx_srand` stores the seed as it is. The game logic stream is the load thread's. The presentation stream (`HarnessViewRand`, `src/harness/harness.cpp`) uses the same algorithm with its own state, so it is a second `bx_rand_state`, never the logic one.
- **random_shuffle.** The STL header (`toolchain/msvc5-sp3/INCLUDE/ALGORITHM`, `_Random_shuffle`) draws once per element after the first: `rand() & 0x7fff`, and for a position `d` above 0x7fff widens that by or-ing in 0x7fff words (no extra draws), then swaps the element with `first[rn % d]`. A range of n elements draws n - 1 times; an empty range draws none. The two game calls (`LoadMatch`, 0x4568c0) shuffle int arrays, which is what the port covers.
- **qsort.** LIBCMT's qsort of this toolchain is the older recursive one with a cutoff: ranges of 8 or fewer elements go to a selection-style short sort (largest to the end, repeated); larger ones swap the middle element to the front as pivot, partition with `<= 0` from the left and `>= 0` from the right, swap the pivot into place, push the larger side on an explicit stack and loop on the smaller. It is not stable, and the port keeps the comparison and swap order, so a comparator sees the same calls and equal elements end in the same order. It was read from the disassembly of 0x4e5ed0, 0x4e6080 (short sort) and 0x4e60e0 (swap).

### The load thread's draws

`LoadMatch` seeds the load thread with `srand` (`game_load.cpp`, the host supplies the seed), and `LoadBattleAssets` then reaches `InitExplosions`, whose `BuildExplosionFrame` draws one `rand` per pixel, `size * size` per frame, with no test on the circle (the pixel is written transparent outside it, but the draw is made first). `InitExplosions` builds three explosions with fixed sizes, each frame stepping the radius by `(end - start) / count` (truncated toward zero):

| Explosion | Frames | Radii | Draws |
| --- | --- | --- | --- |
| 0 | 12 | 64 down to 20 in steps of 4 | sum of size squared |
| 1 | 15 | 128 down to 30 in steps of 7 | sum of size squared |
| 2 | 15 | 200 down to 46 in steps of 11 | sum of size squared |

Together 391606 draws, `bx_init_explosions_draws()`; `bx_init_explosions_rand` advances a state by that many. Nothing else draws between the seed and the start-position shuffle: `UpdateWind`, which `LoadBattleAssets` calls earlier and which draws from `rand`, does so only when `windCounter < gameTick`, and both are 0 there. So the shuffle of `LoadMatch` depends on this count: the sequence is `bx_srand`, `bx_init_explosions_rand`, then (only when n is 2 or less) the one draw of the `n > 2 || rand() ...` test, then `bx_random_shuffle_int`. The counts follow from the source, which the matching build confirms; none depends on a data file.

## Data

`bx_tables.c` is not committed. `uv run tools/bitexact_tables.py` computes it
(by default into `build/bitexact/bx_tables.c`), `tools/bitexact.py` runs it
before every build, and the modern build's CMake does the same. The generator does not read the exe, and its output is deterministic.
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

The math functions (`test/math_test.c`, see "x87 intermediates") add `name: N cases, M mismatches` lines for `bx_sqrt`, `bx_hypot`, `bx_strtof_msvc5` and `bx_sscanf` (the wrapper of `cmake/bitexact/sscanf.c` against the C runtime's `sscanf`, on the formats of `RunInitialMission` and some mixed ones, over generated and cut-off inputs, comparing the return count and every stored value), which must be 0, `bx_acos`, which may have a few (see `_CIacos` below), and `info:` lines for the candidates that do not match, which do not fail the run.

The second exe (`test/crt_test.c`, `test/stl_shuffle.cpp` for the STL) links LIBCMT's own `rand`, `srand` and `qsort` and MSVC 5's `std::random_shuffle` beside the ports: 1500 seeds (0, 1, 0xffffffff and random ones) of 10000 draws; shuffles of sizes 0 to 64 and 1000, 32767 to 32769, 40000 and 100000 over many seeds, comparing the order and the next four draws; the load thread sequence (seed, the explosion draws, a shuffle of 3 to 10 positions); and qsort over random, ascending and descending arrays of 0 to 2000 elements of 1 to 0x62 bytes with 1 to 256 distinct keys, comparing the full output (equal keys carry a payload the comparator ignores) and the comparator's call sequence as a hash of the offsets it was given.

Inputs of the trig exe: every 16-bit angle, crossed with scales 0, +-1, powers of two,
INT_MIN and INT_MAX, and 10^6 or more random cases per function from a fixed
xorshift seed.

## Under clang

`uv run tools/bitexact.py` runs the tests twice. The MSVC 5 exes also write what the originals returned (`build/bitexact/vectors/bitexact.bin` and `crt.bin`, about 900 MB together): every `ORACLE_*` call of the tests, in order, from `test/vectors.c`. Then the same test sources are built with `clang-cl` for i686 (`-DBX_REPLAY`: the original is not linked in, the oracle calls read the vectors) against MSVC 5's headers and `LIBCMT.LIB`, as the modern build is, and run under Wine. The library is compiled with SSE2 doubles (`-msse2 -mfpmath=sse`, no contraction), the test code with x87 (`-mno-sse`) like the game's files. Both halves must print the same `0 mismatches` lines; the clang half also fails if it runs out of vectors or leaves some unread. `--compiler msvc` or `--compiler clang` runs one half, the clang half on the vectors of the last MSVC run.

The generator inputs are deterministic in both builds, so the n-th oracle call of the clang run is the n-th of the MSVC run. The `info:` lines that need the CPU's own `fsincos` are the only live ones.

## In the modern build

`CMakeLists.txt` builds `lib/bitexact` as a static library (`bx_tables.c` first, by `uv run tools/bitexact_tables.py` into the build directory, as a custom command) and links it into the modern exe. `src/` is not edited; the routing is done by the build, in `cmake/bitexact/`:

- `src/util/math_4b70a0.cpp` is left out of the source list and `cmake/bitexact/trig.c` defines its symbols (`FixedSine`, `FUN_004b70c0` to `FUN_004b7381`) as calls into `bx_FixedSine` and the other trig ports. The matrix routines use the global `g_rotationMatrix`, as the original does. The angle arguments are `int` there, narrowed inside, because callers declare them as `short` or `int`.
- `route.h` is force-included (`/FI`) into each source that `data/crt_calls.csv` lists with a game-class `fsqrt`, `rand` or `srand` row. It turns `sqrt` into `bx_sqrt`, `rand` into `bx_game_rand` and `srand` into `bx_game_srand`. This also reaches the `rand` inside the `std::random_shuffle` that `LoadMatch` and `AssignStartPositions` instantiate, so the shuffle draws from the game stream with the order `bx_random_shuffle_int` is tested to have. `cmake/bitexact/game_rand.c` keeps the game stream in a `__declspec(thread)` `bx_rand_state` that starts at 1, as LIBCMT's per-thread data does, so the two streams of the original stay apart: `LoadMatch` runs on the load thread (`StartThread`, `game_load.cpp`), whose `srand`, explosion frames and shuffle draw from that thread's state, and every tick runs on the main thread, whose stream `GameMain` seeds with `srand` (`main.cpp`, listed by name in `CMakeLists.txt` because the inventory classes it presentation although it seeds all tick draws). No object calls the C runtime's `rand` or `srand` any more. `HarnessViewRand` keeps its own state. The trace field `rng.crtHoldrand` still reads the C runtime's word, which these calls no longer move, so traces differ from an unrouted build in that field alone until the harness reads the game stream.
- `_hypot` becomes `bx_hypot`, `acos` becomes `bx_acos` and `sscanf` becomes `bx_sscanf` (`cmake/bitexact/sscanf.c`). The wrapper has the C runtime's signature: a format with no plain `%f` is forwarded to the C runtime's `sscanf` as it stands (so the other files that read `data/crt_calls.csv` as routed keep their results), and one with a plain `%f` (no `*`, width or `l`) is walked directive by directive, a `%f` read by `bx_strtof_msvc5` and every other directive handed to the C runtime with a trailing `%n` to learn how much it used. Only `RunInitialMission` has such a format (`tools/x87scan.py --formats`), so a macro for the whole call, which would also catch the formats that need nothing, was avoided in favour of this check. `GameMain`'s `srand` is classed game by `MAINTAINER_SRAND` in `tools/crtcalls.py`, so `main.cpp` is routed from the CSV like any other file.
- Left on the C runtime, because the library has no port: `_ftol`, `atof`. `qsort` has no game-logic caller, and the presentation `sqrt` sites (files with no game-class row) keep the inline `fsqrt`.

## CRT and float operations in game logic

What the game's own functions call in the C runtime (LIBCMT) or compute inline
in the FPU, read from the matching build's objects by `tools/crtcalls.py`. The
full list, per function, is `data/crt_calls.csv` (`uv run tools/crtcalls.py`
rewrites it). A game-logic function is one reachable from the eight phase calls
of `RunGameSteps` (HandleNetPackets, UpdateAllUnits, UpdateProjectiles,
UpdateExplosions, UpdatePlayers, UpdateFeatures, UpdateWind, UpdateMeteors) or
from `LoadMatch`, or is `AssignStartPositions` (0x4568c0), which is a root by
explicit listing (`EXTRA_ROOTS` in the tool) because only frame handlers call
it. Everything else is presentation. Counts are game-logic
functions, then call or instruction sites. `sqrt` never appears as a call:
the compiler emits `fsqrt` inline.

- `fistp (FixedSine)`: 1 function, 3 sites: the merged port in lib/bitexact (trig.c)
- `fpatan (FixedSine)`: 1 function, 1 site: the merged port in lib/bitexact (trig.c)
- `fsincos (FixedSine)`: 1 function, 1 site: the merged port in lib/bitexact (trig.c)
- `_CIacos`: `fpatan(sqrt((1 + x) * (1 - x)), x)` on the x87, whose result is the 64-bit `fpatan` value (the same instruction as `bx_FUN_004b715a`'s), stored as a double. A plain-double series port differs in 40 to 46% of 500,000 arguments in [-1, 1] (`x 0.5`: want 1.0471975511965976, got 1.0471975511965974). `bx_acos` (`trig.c`) takes `s = sqrt((1 + x) * (1 - x))` in doubles, computes `atan2(s, x)` in double-double (the table reduction of `bx_FUN_004b715a` with double-double arithmetic and an 11-term series, about 2^-100 accurate), rounds it to a 64-bit mantissa and then to 53 bits. It differs in 7 of 500,000 arguments, all of the same kind: the exact angle lies just above a 53-bit midpoint (the low 11 bits of its 64-bit mantissa are 0x401) and the CPU's `fpatan` returned a value at or below it, that is it is not correctly rounded to 64 bits there. This is a property of the host CPU (the tests ran on the development machine), so `tools/bitexact.py` allows up to 50 such cases in 500,000 and fails above that.
- `_ftol`: 62 functions, 151 sites: #73
- `_hypot`: 14 functions, 18 sites: routed to `bx_hypot`
- `fsqrt`: 14 functions, 20 sites: #73
- `atof`: 4 functions, 4 sites: #73 (decimal text to double in the TDF and command readers, x87 intermediates)
- `FUN_004e81b0`: 4 functions, 6 sites: already exact: C++ library, integer and pointer logic
- `FUN_004e8320`: 4 functions, 6 sites: already exact: C++ library, integer and pointer logic
- `exception::exception`: 7 functions, 12 sites: already exact: C++ library, integer and pointer logic
- `exception::what`: 4 functions, 6 sites: already exact: C++ library, integer and pointer logic
- `exception::~exception`: 2 functions, 2 sites: already exact: C++ library, integer and pointer logic
- `std::_Lockit::_Lockit`: 46 functions, 104 sites: already exact: C++ library, integer and pointer logic
- `std::_Lockit::~_Lockit`: 46 functions, 108 sites: already exact: C++ library, integer and pointer logic
- `std::_Xran`: 6 functions, 6 sites: already exact: C++ library, integer and pointer logic
- `std::basic_string::_Eos`: 8 functions, 11 sites: already exact: C++ library, integer and pointer logic
- `std::basic_string::_Grow`: 8 functions, 14 sites: already exact: C++ library, integer and pointer logic
- `std::basic_string::_Split`: 6 functions, 11 sites: already exact: C++ library, integer and pointer logic
- `std::basic_string::_Tidy`: 7 functions, 8 sites: already exact: C++ library, integer and pointer logic
- `std::basic_string::erase`: 1 function, 1 site: already exact: C++ library, integer and pointer logic
- `std::length_error::length_error`: 1 function, 1 site: already exact: C++ library, integer and pointer logic
- `std::runtime_error::~runtime_error`: 4 functions, 5 sites: already exact: C++ library, integer and pointer logic
- `_msize`: 1 function, 1 site: already exact: allocation, deterministic if no logic reads an address
- `calloc`: 2 functions, 3 sites: already exact: allocation, deterministic if no logic reads an address
- `free`: 6 functions, 7 sites: already exact: allocation, deterministic if no logic reads an address
- `malloc`: 3 functions, 3 sites: already exact: allocation, deterministic if no logic reads an address
- `realloc`: 1 function, 1 site: already exact: allocation, deterministic if no logic reads an address
- `_chsize`: 1 function, 1 site: already exact: file system access, no arithmetic (host interface, #24)
- `_filelength`: 4 functions, 4 sites: already exact: file system access, no arithmetic (host interface, #24)
- `_findclose`: 5 functions, 5 sites: already exact: file system access, no arithmetic (host interface, #24)
- `_findfirst`: 1 function, 1 site: already exact: file system access, no arithmetic (host interface, #24)
- `_findnext`: 1 function, 1 site: already exact: file system access, no arithmetic (host interface, #24)
- `_mkdir`: 1 function, 2 sites: already exact: file system access, no arithmetic (host interface, #24)
- `fclose`: 8 functions, 14 sites: already exact: file system access, no arithmetic (host interface, #24)
- `fopen`: 5 functions, 6 sites: already exact: file system access, no arithmetic (host interface, #24)
- `fread`: 3 functions, 5 sites: already exact: file system access, no arithmetic (host interface, #24)
- `fseek`: 6 functions, 11 sites: already exact: file system access, no arithmetic (host interface, #24)
- `ftell`: 5 functions, 8 sites: already exact: file system access, no arithmetic (host interface, #24)
- `fwrite`: 4 functions, 15 sites: already exact: file system access, no arithmetic (host interface, #24)
- `_alldiv`: 29 functions, 52 sites: already exact: integer 64-bit helpers
- `_allmul`: 35 functions, 93 sites: already exact: integer 64-bit helpers
- `_allshl`: 10 functions, 15 sites: already exact: integer 64-bit helpers
- `_allshr`: 28 functions, 82 sites: already exact: integer 64-bit helpers
- `_itoa`: 3 functions, 3 sites: already exact: integer and byte string logic
- `_strcmpi`: 65 functions, 94 sites: already exact: integer and byte string logic
- `_strlwr`: 3 functions, 3 sites: already exact: integer and byte string logic
- `_strnicmp`: 6 functions, 10 sites: already exact: integer and byte string logic
- `atoi`: 8 functions, 17 sites: already exact: integer and byte string logic
- `isalnum`: 1 function, 1 site: already exact: integer and byte string logic
- `isdigit`: 1 function, 1 site: already exact: integer and byte string logic
- `isspace`: 3 functions, 5 sites: already exact: integer and byte string logic
- `memmove`: 7 functions, 7 sites: already exact: integer and byte string logic
- `strchr`: 4 functions, 7 sites: already exact: integer and byte string logic
- `strcspn`: 1 function, 1 site: already exact: integer and byte string logic
- `strncmp`: 20 functions, 36 sites: already exact: integer and byte string logic
- `strncpy`: 28 functions, 40 sites: already exact: integer and byte string logic
- `strrchr`: 3 functions, 5 sites: already exact: integer and byte string logic
- `strstr`: 4 functions, 5 sites: already exact: integer and byte string logic
- `strtok`: 2 functions, 11 sites: already exact: integer and byte string logic
- `tolower`: 3 functions, 6 sites: already exact: integer and byte string logic
- `toupper`: 4 functions, 6 sites: already exact: integer and byte string logic
- `_compress`: 1 function, 1 site: already exact: integer compression
- `_uncompress`: 1 function, 1 site: already exact: integer compression
- `deflate_fast`: 1 function, 3 sites: already exact: integer compression
- `deflate_slow`: 1 function, 6 sites: already exact: integer compression
- `deflate_stored`: 1 function, 1 site: already exact: integer compression
- `_controlfp`: 1 function, 3 sites: already exact: masks exceptions only, see FPU state
- `_exit`: 1 function, 1 site: already exact: no value flows into the simulation (process exit, crash report text, wall clock for the report only)
- `asctime`: 1 function, 2 sites: already exact: no value flows into the simulation (process exit, crash report text, wall clock for the report only)
- `atexit`: 8 functions, 8 sites: already exact: no value flows into the simulation (process exit, crash report text, wall clock for the report only)
- `exit`: 2 functions, 2 sites: already exact: no value flows into the simulation (process exit, crash report text, wall clock for the report only)
- `getenv`: 2 functions, 2 sites: already exact: no value flows into the simulation (process exit, crash report text, wall clock for the report only)
- `gmtime`: 1 function, 1 site: already exact: no value flows into the simulation (process exit, crash report text, wall clock for the report only)
- `localtime`: 1 function, 1 site: already exact: no value flows into the simulation (process exit, crash report text, wall clock for the report only)
- `raise`: 1 function, 1 site: already exact: no value flows into the simulation (process exit, crash report text, wall clock for the report only)
- `_chkstk`: 16 functions, 16 sites: already exact: stack probe
- `fprintf`: 2 functions, 21 sites: already exact: text formatting, integer and string formats (format strings not inspected, see limits)
- `printf`: 2 functions, 6 sites: already exact: text formatting, integer and string formats (format strings not inspected, see limits)
- `sprintf`: 68 functions, 190 sites: already exact: text formatting, integer and string formats (format strings not inspected, see limits)
- `sscanf`: 4 functions, 21 sites: already exact: text formatting, integer and string formats (format strings not inspected, see limits)
- `_purecall`: 18 functions, 56 sites: already exact: vtable filler, never called
- `time`: 2 functions, 2 sites: the #72 port (LoadMatch's seed input: the host supplies the seed; the other site is the crash report text)
- `rand`: 9 functions, 22 sites: the #72 port (the LoadMatch seeding and std::random_shuffle also draw through rand)
- `srand`: 1 function, 1 site: the #72 port (the LoadMatch seeding and std::random_shuffle also draw through rand)

`qsort` is not used by game logic (its one call is in the lobby UI). Float to
integer conversions in game logic are 62 functions through `_ftol` and one,
`FixedSine`, with an inline `fistp` (32-bit); no `fist` and no `fldcw` occur.

### FPU state

Game logic runs with the CRT's default control word: `_control87(0, 0)` is
0x9001F, or 0xD001F under Wine, which differs only in the infinity-control
bit. That is 53-bit precision (PC = double), round to nearest even, and all
exceptions masked. Nothing in game code changes precision or rounding: the only
control word writers are `_controlfp` in `Sound::CreateSampleFromFile`
(0x4cf370, masks the invalid and divide-by-zero exceptions around a call, then
restores the word) and in `SetFpuFussy` (0x4df160, the same mask for a command
line switch; presentation); both touch only the exception mask. So x87
intermediates round to 53-bit double precision, but keep the x87's extended
exponent range: a value that would overflow or underflow a double in memory
does not do so in a register, and rounding to double happens at each x87
operation, not at each C expression. Values spilled to memory as `float` or
`double` round again to that width. `_ftol` truncates toward zero; a bare
`fistp` rounds to nearest even under this control word.

### Method and limits

- Objects are the cached compiles of every game and gap source under `src/`
  (`build/progress/`). Calls are the relocations in each annotated function
  that resolve, through the objects' own definitions or `data/symbols.csv`, to
  a `library` function in `data/functions.csv` (or `_chkstk`). Win32 and
  DirectX stubs are left out. A callee whose name no object defines is read
  from the original exe at the same offset of the same function.
- Inline FPU instructions come from a recursive-descent decode of each
  function (linear for gap regions, which are hand-written assembly with
  several entry points).
- Reachability follows direct calls and jumps, every function whose address a
  function takes, and for each data symbol a reachable function references (a
  vtable, an order handler table such as `g_groundOrders`), every function
  pointer in the exe's data up to the next symbol.
- Indirect calls through registers or memory that none of that resolves are
  not followed: 229 through memory and 109 through registers in game-logic
  functions, mostly virtual calls and callbacks whose target table is filled at
  run time. A function reached only that way is classed presentation, so its
  rows may be missing from the game-logic list.
- Tables over-approximate: a function referencing a table reaches every
  entry. `LoadMatch` reaches `InitCommands`, which registers the console and
  cheat command tables, so 302 of the 1613 game-logic functions are reached
  only that way (marked `via console commands only` in the notes); 581 of the
  722 game-logic rows remain without them.
  Debug and crash-report code reached through the out-of-memory handler is
  likewise counted as game logic.
- Format strings of `sprintf`, `sscanf`, `printf` and `fprintf` are not
  inspected, so a `%f` or `%g` use (which would need #73) is not ruled out.
- `rand` against #52: the sites agree with its tables and the CSV has no
  `conflict:` notes left. Two calls were settled by decision: the maintainer
  ruled on #52 and #72 that `BuildExplosionFrame` (0x420d20), which #52 lists
  as presentation, is game logic (`LoadMatch` reaches it through
  `InitExplosions`), and `AssignStartPositions` (0x4568c0), which only
  `MainLoopTick` and `LoadingScreenFrame` call, is a game root by explicit listing.
  `DrawProjectiles` (0x49be60) and `StartMeteorShower` (0x438070) match #52 only
  through the console commands (`CmdMakePoster`, `CmdMeteor`). The `Rand` that
  #52 lists at 0x423c50 is inlined into `UpdateFeatures` (0x424050). Twelve
  `HarnessViewRand` callers (particles, speech, packet drop, console) are
  reachable from game logic: presentation code that game logic triggers, which
  draws from the separate stream, so they are not conflicts.

## x87 intermediates

`tools/x87scan.py` (#73, phase 1) finds the places where MSVC 5's x87 code keeps
a float value at double precision (53 bits, with the extended exponent range)
across several operations. The source says `float` there, MSVC rounds to 32
bits only at a store, and a compiler that does float arithmetic in float (SSE,
wasm f32) rounds after every operation, so the bits can differ. `uv run
tools/x87scan.py` writes `data/x87_sites.csv` (`address,function,file,line,offset,chain,result,class`)
and prints the totals; `--formats` also prints the printf and scanf family calls
of game logic with their format strings.

### Scan method

- Functions: the 1,613 game-logic functions of `tools/crtcalls.py` (its `analyze`), read from the matching build's objects. Each source is compiled again with `/Zd` (line numbers only) into `build/x87scan/`, and the function bytes are checked against the cached objects of `tools/progress.py`: all identical.
- Walk: each function is decoded by recursive descent as crtcalls does, and the x87 stack is followed in address order. Every slot holds the expression that produced it; `fld dword` is a float leaf, `fld qword` a double leaf, `fild` an integer leaf, and `fst`/`fstp` of a dword or qword, `fist`/`fistp` and `call _ftol` end a chain.
- Site: a chain of two or more arithmetic operations (fadd, fsub, fmul, fdiv and variants, fsqrt, the `_CI` calls) with a float or integer load among its leaves, ending in a float store (`float-store`), a double store (`double-store`), `_ftol` (`ftol`) or `fistp`. Each node gets the type C gives it: double if any operand is, else float if any is, else ambiguous (integers and constants only).
- Class `float-chain`: a float-typed operation feeds another operation, a conversion to int or a double store, so a float-evaluating compiler rounds where x87 does not. `safe`: every such operation is double in C. `unknown`: only integer loads, or a call result.
- Source line: the COFF line table of the `/Zd` object (`tools/coff.py` reads it). The line is the store's; for an inlined function it is the line of the call, not of the inline body (UpdatePlayerEconomy's `SettleResourceAccount` and `AddIncomeW`, DrawBattleFrame's `DrawResourcePanel`, RateUnitType in ComputeBaseWeights), and a scheduled store can be a line or two after its statement.
- Review: with 65 sites the classes were checked against the C source. 45 of the sites are classed as the scan gives them; for 20 the scan could not decide or was misled and `REVIEWED` in the tool holds the class (integer-only chains from `Length` helpers that use `float` locals are `float-chain`, those that use `double` locals are `safe`; the GUI `(int)((float)a * b / c)` forms are `float-chain`; UpdateMetalExtraction multiplies by a power of two, so reassociating is exact, and is `safe`).

### Scan counts

30 of the 1,613 game-logic functions have 65 sites.

| Class | Sites | Functions | float-store | double-store | ftol | fistp |
| --- | --- | --- | --- | --- | --- | --- |
| float-chain | 33 | 14 | 19 | 3 | 11 | 0 |
| safe | 32 | 19 | 2 | 5 | 24 | 1 |
| unknown | 0 | 0 | 0 | 0 | 0 | 0 |

The `disposition` column says what became of each site:

| Disposition | Sites | Meaning |
| --- | --- | --- |
| rewritten | 11 | the source now states the double precision x87 used, and the function still matches (`uv run tools/check.py`): 2 in UpdatePlayerEconomy (the wasted-resource sums), 2 in SteerAircraft (the two `* 65536.0` conversions), 4 in DrawBattleFrame (the energy bar, the share box and two more), 3 in ScoutTimer::OnTimer; these now read `safe` |
| modern | 33 | the chain is stated only under `#ifdef __clang__` (the MSVC 5 source is unchanged, because a rewrite changed the bytes or could not state the precision), so the modern build rounds as x87 did; the class stays `float-chain` |
| n/a | 21 | safe sites: the source is double there |

The 14 functions with `modern` sites: UpdatePlayerEconomy (7), ComputeBaseWeights (3), RefreshUnitLists (3), AddRepairProgress (2), SteerAircraft (4), DrawLitPieces (1), ApplyAreaDamage (1), DrawSliderBar (1), SyncAssocGadgets (2), SetListBoxScrollByName (1), DrawSlider (1), DrawProgressBarGadget (1), CrossProduct (3), NormalizeVector (3). The list with lines is in the CSV.

Not in the file: 19 single-operation float expressions converted by `_ftol` (`(int)(a * b)` with floats: the product is a double at the conversion, one rounding to float short), and no function returns a chain of two operations in st0. Chains over integer loads from 64-bit squares (`fild qword`) do not occur in game logic: the `Length` helpers load 32-bit ints.

### Math functions

| Function | Outcome | Left for |
| --- | --- | --- |
| `sqrt` | exact: `bx_sqrt` matches x87 `fsqrt` (double, float in and out, CRT `sqrt`) | none |
| `_hypot` | exact: `bx_hypot` emulates the 64-bit-precision steps; a plain-double port differs in 2.3% | none |
| `acos` | `bx_acos`: `fpatan` of `sqrt((1+x)(1-x))` and `x`; a plain-double port differs in 45.7%, `bx_acos` in 0.001% (the CPU's fpatan, see below) | none |
| `atof` | matches the correctly rounded double up to 15 digits | none for that form |
| `sscanf %f` | exact: `bx_strtof_msvc5` reproduces LIBCMT's digit handling and its off-by-one rounding, which differs from the correctly rounded value in 0.41% | none |

`tools/bitexact.py` compares, under the game's FPU mode, the C runtime's functions with portable candidates (`lib/bitexact/test/math_test.c`, 2,000,000 random cases each unless noted). Only a port that matches goes into the library.

- `fsqrt`: `bx_sqrt` (integer digit-by-digit square root, correctly rounded) matches x87 `fsqrt` on doubles (zeros, subnormals, integers and random bit patterns), matches float in and float out (`fld dword; fsqrt; fstp dword`, so the double rounding is harmless), and matches MSVC's `sqrt`: 0 mismatches each. It is in the library (`bx_sqrt`); a target with a correctly rounded `sqrt` instruction needs no port.
- `_hypot`: MSVC 5's `_hypot` (`__hypothlp`) sets the FPU to 64-bit precision (control word 0x133f) and computes `big = max(|x|, |y|)`, `ra = x / big`, `rb = y / big`, the sum `ra * ra + rb * rb`, its square root and the product with `big`, storing each of ra, rb, the sum, the root and the result as a double. A port in plain double steps differs in 45,901 of 2,000,000 cases (2.3%; 5.7% for integer arguments, 1.3% for random doubles, 2.0% for floats), for example `x -39 y -64972096`: want 64972096.000011712, simple 64972096.000011697. `bx_hypot` (`math.c`) does each step on a 64-bit mantissa in integer code (a 64 by 64 bit product, a long division, a digit-by-digit square root, an add with a sticky bit), rounds to nearest even to 64 bits and then, at each store, to 53 bits (with subnormals and overflow to infinity); 0 mismatches in 2,000,000 cases (integers, doubles over 1e-60 to 1e60, a far smaller argument, floats, integers from -64 to 64, exponents from -900 to 900). Infinite arguments give infinity and a NaN comes back as it is; results out of double range are not tested (the C runtime goes through `matherr` there).
- `_CIacos`: `fpatan(sqrt((1 + x) * (1 - x)), x)` on the x87, whose result is the 64-bit `fpatan` value (the same instruction as `bx_FUN_004b715a`'s). A plain-double series port differs in 45.7% of 500,000 arguments in [-1, 1] (`x 0.889465`: want 0.47462316414879513, got 0.47462316414879507). Left for phase 2 or #5: it needs the double-double arctangent of trig.c.
- `atof`: for decimal text of up to 15 digits with a decimal point (the form of TDF values), the result equals the correctly rounded double, computed as `(double)digits / 10^k` (the integer is exact in a double and 10^k is exact up to 10^15, one division): 0 mismatches in 1,000,000 cases of 1 to 9 digits and 1,000,000 of 10 to 15 digits. Longer text and exponents were not tested. No library function yet, since the port covers only that form.
- `sscanf("%f")` into a float: 4,089 of 1,000,000 cases (0.41%) differ from the correctly rounded double rounded to float, for example `"-93690462"`: sscanf gives -93690456, the candidate -93690464. The cause is in LIBCMT, not in the digits: `_atoflt` runs `___strgtold12` (up to 25 digits kept, a 96-bit integer of digits made by multiply by 10 and add, scaled by products from the table of powers of ten 10^(+-1..7 * 8^k) with `___ld12mul`'s 5 by 5 limb product), then `__ld12cvt` rounds to 24 bits with `_RoundMan`, whose test for the bits after the round bit (`_ZeroTail(man, 25)`) leaves out bit 25 itself, so when the round bit and the next bit are set and the rest is zero it rounds down. `bx_strtof_msvc5` (`strtof.c`) is a port of that chain, the tables generated by `tools/bitexact_tables.py` and checked against the exe (`bx_pow10_pos`, `bx_pow10_neg`); 0 mismatches against `sscanf` in 2,000,000 texts (integers, decimals with up to 40 digits, leading and trailing zeros, exponents out to e+-50, signs).
- `atan2` and `floor` are not called by game logic (data/crt_calls.csv), so they are not tested.

### Format strings

`uv run tools/x87scan.py --formats` finds the string literals pushed before each `sprintf`, `sscanf`, `printf` and `fprintf` call of the game-logic functions (190, 21, 6 and 21 calls; 36 have no literal in the argument window, in 13 functions, and the source of those has no float formats except the debug line `buildtimeleft: %1.3f` in the status panel and `HapiBank::WriteAuditFile`'s). `%f`, `%e` and `%g` appear in:

- `RunInitialMission` (0x487bf0), `sscanf` with `%f`, 6 calls: the mission script's unit orders (` %f %f` move and patrol positions, ` %f %f %f` with a wait time, ` %[...] %d %f %f`). The floats become `(int)(f * 65536.0)` order positions and `(int)(f * 30.0f)` times, so the result reaches game state: it needs the exact `sscanf` result above.
- `DrawNetworkStats` (`%1.1f`), `DrawBattleFrame` (`%.1f`, 2 calls) and `DrawUnitInfoPanel` (`+%.0f`, `+%.1f`, `-%.0f`, `-%.1f`): display text only.
- `HapiBank::WriteAuditFile` (`%f`): an audit file written from a console command, log only.

No game-logic function uses `%e` or `%g`. The `atof` calls are separate: `TdfRecord::GetFieldDouble` and `GetFieldFixed` parse TDF values into game state, and `CmdSearch` and `CommandArgs::GetFloatArg` are console-only.

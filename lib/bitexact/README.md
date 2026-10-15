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

## CRT and float operations in game logic

What the game's own functions call in the C runtime (LIBCMT) or compute inline
in the FPU, read from the matching build's objects by `tools/crtcalls.py`. The
full list, per function, is `data/crt_calls.csv` (`uv run tools/crtcalls.py`
rewrites it). A game-logic function is one reachable from the eight phase calls
of `RunGameSteps` (HandleNetPackets, UpdateAllUnits, UpdateProjectiles,
UpdateExplosions, UpdatePlayers, UpdateFeatures, UpdateWind, UpdateMeteors) or
from `LoadMatch`, or `AssignStartPositions` (0x4568c0), a root by explicit listing
(`EXTRA_ROOTS` in the tool) because only frame handlers call it; everything else is presentation. Counts are game-logic
functions, then call or instruction sites. `sqrt` never appears as a call:
the compiler emits `fsqrt` inline.

- `fistp (FixedSine)`: 1 function, 3 sites: the merged port in lib/bitexact (trig.c)
- `fpatan (FixedSine)`: 1 function, 1 site: the merged port in lib/bitexact (trig.c)
- `fsincos (FixedSine)`: 1 function, 1 site: the merged port in lib/bitexact (trig.c)
- `_CIacos`: 1 function, 2 sites: #73
- `_ftol`: 62 functions, 151 sites: #73
- `_hypot`: 14 functions, 18 sites: #73 (the sqrt family, not named in #73, to confirm there)
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
- `rand` against #52: the sites agree with its tables except one, marked
  `conflict:` in the CSV. `BuildExplosionFrame` (0x420d20) is presentation in
  #52, but `LoadMatch` reaches it through `InitExplosions`; the maintainer
  decided on #52 and #72 that it is game logic. `AssignStartPositions`
  (0x4568c0) is called only from `MainLoopTick` and `LoadingScreenFrame`, which
  nothing in game logic reaches, so it is a game root by explicit listing.
  `DrawProjectiles` (0x49be60) and `StartMeteorShower` (0x438070) match #52 only
  through the console commands (`CmdMakePoster`, `CmdMeteor`). The `Rand` that
  #52 lists at 0x423c50 is inlined into `UpdateFeatures` (0x424050). Twelve
  `HarnessViewRand` callers (particles, speech, packet drop, console) are
  reachable from game logic: presentation code that game logic triggers, which
  draws from the separate stream, so they are not conflicts.

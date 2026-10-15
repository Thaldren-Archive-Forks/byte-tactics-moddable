## CRT and float operations in game logic

What the game's own functions call in the C runtime (LIBCMT) or compute inline
in the FPU, read from the matching build's objects by `tools/crtcalls.py`. The
full list, per function, is `data/crt_calls.csv` (`uv run tools/crtcalls.py`
rewrites it). A game-logic function is one reachable from the eight phase calls
of `RunGameSteps` (HandleNetPackets, UpdateAllUnits, UpdateProjectiles,
UpdateExplosions, UpdatePlayers, UpdateFeatures, UpdateWind, UpdateMeteors) or
from `LoadMatch`; everything else is presentation. Counts are game-logic
functions, then call or instruction sites. `sqrt` never appears as a call:
the compiler emits `fsqrt` inline.

- `fistp (FixedSine)`: 1 function, 3 sites: #70 (math_4b70a0.cpp trig)
- `fpatan (FixedSine)`: 1 function, 1 site: #70 (math_4b70a0.cpp trig)
- `fsincos (FixedSine)`: 1 function, 1 site: #70 (math_4b70a0.cpp trig)
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
- `_alldiv`: 28 functions, 51 sites: already exact: integer 64-bit helpers
- `_allmul`: 34 functions, 92 sites: already exact: integer 64-bit helpers
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
- `rand`: 8 functions, 20 sites: the #72 port (the LoadMatch seeding and std::random_shuffle also draw through rand)
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
  cheat command tables, so 302 of the 1612 game-logic functions are reached
  only that way (marked `via console commands only` in the notes); 578 of the
  719 game-logic rows remain without them.
  Debug and crash-report code reached through the out-of-memory handler is
  likewise counted as game logic.
- Format strings of `sprintf`, `sscanf`, `printf` and `fprintf` are not
  inspected, so a `%f` or `%g` use (which would need #73) is not ruled out.
- `rand` against #52: the sites agree with its tables except two, marked
  `conflict:` in the CSV. `AssignStartPositions` (0x4568c0) is called only from
  `MainLoopTick` and `LoadingScreenFrame`, which nothing in game logic reaches,
  so it is presentation by the rule. `BuildExplosionFrame` (0x420d20) is
  presentation in #52, but `LoadMatch` reaches it through `InitExplosions`.
  `DrawProjectiles` (0x49be60) and `StartMeteorShower` (0x438070) match #52 only
  through the console commands (`CmdMakePoster`, `CmdMeteor`). The `Rand` that
  #52 lists at 0x423c50 is inlined into `UpdateFeatures` (0x424050). Twelve
  `HarnessViewRand` callers (particles, speech, packet drop, console) are
  reachable from game logic: presentation code that game logic triggers, which
  draws from the separate stream, so they are not conflicts.

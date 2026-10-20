# Contributing

Help is welcome, as code, as a coding agent (Claude Code, Codex, OpenCode)
working through issues, or as discussion on the plan. The plan is
[issue #2](https://github.com/ozgb/byte-tactics-moddable/issues/2): every
step of it is a sub-issue, and the issues record which steps wait on which.

## Setting up

You need Linux (or WSL) with clang and LLVM **22.1.8**, `cmake`, `ninja`,
`wine` (the gate plays the game under it), `7z`, `cabextract`, `curl`, `git`,
the GitHub CLI (`gh`, logged in) and [uv](https://docs.astral.sh/uv/). You also
need your own copy of Total Annihilation (the Steam version) and MSVC 5's
`INCLUDE` and `LIB` directories in `toolchain/msvc5-sp3/`. The MSVC 5 compiler
is not used to build `moddable`.

```sh
git clone https://github.com/ozgb/byte-tactics-moddable
cd byte-tactics-moddable
tools/setup_toolchain.sh        # MSVC 5 INCLUDE and LIB into toolchain/;
                                # copies TotalA.exe from Steam (set STEAM_TA if needed)
ls toolchain/msvc5-sp3/INCLUDE toolchain/msvc5-sp3/LIB orig/TotalA.exe
clang-cl --version              # must report 22.1.8
```

Clone this repository, not upstream Byte Tactics: the scripts branch from
`origin/moddable`.

## Modern build

The modern build compiles every file under `src/` with clang-cl for
`i686-pc-windows-msvc`, against MSVC 5's own headers and C runtime, and links
`build/modern/TotalA.exe` with `lld-link`. LLVM is pinned to **22.1.8**, and
`cmake/clang-cl-i686.cmake` fails on any other version.

```sh
sudo dnf install clang lld llvm cmake ninja-build      # Fedora
cmake -S . -B build/modern -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-i686.cmake
cmake --build build/modern
WINEARCH=win64 uv run tools/playtest.py --exe build/modern/TotalA.exe \
    --testmode reference/scenarios/skirmish2.txt --game-dir ~/.ta
```

Configuring runs `tools/modern_headers.py`, which writes patched copies of the
nine STL headers clang-cl cannot parse, and a case-insensitive overlay for
MSVC 5's upper-case `INCLUDE`, under `build/modern/`. The toolchain itself is
never edited. Building then runs `tools/aliases.py`, the generator that bridges
the many spellings of each function and global to the one definition at its
address, and links the result with the import
libraries made from `link/*.def`, MSVC 5's `LIBCMT.LIB` and its system
libraries, zlib 1.0.4 (the sources in `toolchain/thirdparty`, compiled with clang-cl as `/Gz /Zp1`) and the resources (`llvm-rc`). It
needs `uv`, `orig/TotalA.exe` (the icon, the cursor and the initial value of
some globals) and `llvm-dlltool`, `llvm-rc` and `lld-link`.

The objects are linked in `tools/sources.py`'s link order, the reference's, and
`tools/aliases.py` keeps one definition (the first that is not an inline copy) of a name defined in
several files (the copies kept in callers' files), as
LINK's `/FORCE:MULTIPLE` does, so the link prints no duplicate-symbol warning.

The link refuses unresolved names: there are no stubs. `tools/aliases.py` also
leaves out the code no live function reaches. With `-DBT_DATA_ORDER=ON` it lays
the game's data out in 3.1c's order (each global in a section named for its
address, with the original's bytes between them), for bisecting a difference
that a global's size or place might cause; by default the linker places each
global itself.

The build also compiles `lib/bitexact` and routes the game logic's `rand`,
`srand`, `sqrt`, `_hypot`, `acos`, the `%f` form of `sscanf` and the trig entry points of `src/util/math_4b70a0.cpp` to it
(see "In the modern build" in `lib/bitexact/README.md`); `src/` is untouched, so
the matching build is not affected. `uv run tools/bitexact.py` tests the library
with clang-cl against the vectors the reference branch's MSVC 5 build wrote.

The `modern-compile` CI job compiles the game and `lib/bitexact` with clang-cl
(`cmake --build build/modern --target game bitexact`, configured with
`-DBT_COMPILE_ONLY=ON`) on pull requests that touch `src/`, `include/`, `lib/`,
`cmake/`, `CMakeLists.txt`, `tools/setup_toolchain.sh`, `tools/sources.py`,
`pyproject.toml` or `uv.lock`. It checks that the
source still builds with LLVM 22.1.8 against MSVC 5's headers, which
`tools/setup_toolchain.sh` fetches into a cached `toolchain/`. The job also runs `uv run tools/rwsets.py --check`, which regenerates `data/rwsets.csv` (the globals, struct fields and functions each function reads, writes and calls, from clang's syntax tree) and fails when it differs from the committed file; run `uv run tools/rwsets.py` after a change to `src/` and commit the result. It also runs `uv run tools/hazards.py --check`, which lists the places in game logic that depend on heap addresses, uninitialised memory, time or other threads (`data/hazards.csv`, with a verdict and note per row that regeneration keeps); run `uv run tools/hazards.py` after a change to `src/` and commit the result. It also runs `uv run tools/stateclasses.py --check`, which proposes a class (sim, view, platform or mixed) for every global, static, region of `struct Game` and heap allocation of game logic, with the phases and presentation code that read and write it (`data/state_classes.csv`, with a hand-set `class` and a `note` that regeneration keeps); run `uv run tools/stateclasses.py` after a change to `src/` and commit the result. It also runs `uv run tools/coupling.py --check`, which assigns every function to a subsystem (`data/subsystems.csv`, with a hand-set `override` that regeneration keeps) and lists the calls and shared sim state between each pair of subsystems (`data/coupling.csv`); a new call or shared field between subsystems fails the check until `uv run tools/coupling.py` is run and the result committed, so new coupling is always a reviewed decision.
It does not
link: the link needs `orig/TotalA.exe`, which must never be committed, and
trace runs need the game's data, so both stay local.

## Running an agent

Start your agent in the repository folder. Claude Code reads `CLAUDE.md`,
which loads `AGENTS.md`; Codex and OpenCode read `AGENTS.md` directly. It tells
the agent how to pick and claim an issue, work in its own copy, pass the gate
and open a pull request. Give it this prompt:

> Follow AGENTS.md: pick up an open sub-issue of #2 with no open blockers, do
> it and open a pull request.

In short:

- Take only issues whose blockers are all closed.
- Every change keeps the game behaving exactly like 3.1c: identical traces
  for the scenario suite (`uv run tools/reference.py --check --exe build/modern/TotalA.exe`).
- Code comments are limited to three forms (see "Comments in code" in
  `AGENTS.md`); the reasons for a change go in its commit message and pull
  request.
- Have one or two issues claimed at a time rather than many.

## Running the lead and the maintainer

The plan is run by two Claude sessions with distinct roles: `/lead` implements issues and `/maintainer` reviews, merges and grooms. Each runs in its own clone, started with `BT_ROLE` set; `docs/team.md` has the setup and how the two work together.

## Changing the plan

If an issue looks wrong, or the plan misses something, comment on that issue
or on #2. A pull request that works around the plan is harder to review than
a comment that questions it.

## What happens to your pull request

A maintainer re-runs the checks the issue names and merges it. Please don't
commit any game files or Microsoft software. The reconstructed game code is
not licensed (see the README); the tools are MIT.

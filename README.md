<p align="center">
  <img src="docs/images/banner.jpg" alt="Total Annihilation: Byte Tactics" width="100%">
</p>

<p align="center">
  <b>Total Annihilation's own engine, rebuilt so every part of it can be swapped by a mod.</b>
</p>

---

## What is this?

This is the moddable fork of [TA: Byte Tactics](https://github.com/HectorBailey/byte-tactics),
a matching decompilation of Total Annihilation (Cavedog, 1997): C++ source
that compiles, with the original Visual C++ 5.0, to the shipped `TotalA.exe`
byte for byte.

Byte Tactics recovered the original engine. This fork turns it into a
moddable one:

- **Every game-logic subsystem is a WebAssembly module.** Economy, AI,
  pathfinding, weapons, targeting, line of sight, orders and the rest each sit
  behind an interface, and a mod replaces one by shipping its own module.
- **The default modules are the 3.1c code itself**, compiled to WebAssembly.
  With no mods loaded, the game plays exactly like 3.1c, bit for bit.
- **The whole game runs as WebAssembly on thin hosts:** a native host for
  Windows, Linux and macOS, and a browser host.
- **Two network modes:** deterministic lockstep for casual and modded games,
  and server-client for competitive games, where clients only receive what
  their player can see.
- **Existing TA mods keep working**, and mods can change engine behaviour
  through Lua hooks and quirk flags, following
  [Open Annihilation's mod profile format](https://github.com/open-annihilation/open-annihilation/blob/main/docs/mods/README.md)
  where possible.

> **No game files and no Microsoft software are included.** You bring your
> own copy of Total Annihilation.

## Status

The plan is set and the work has not started: the source in `src/` is still
the matching decompilation. The plan, its decisions and its order are in
[issue #2](https://github.com/ozgb/byte-tactics-moddable/issues/2), with one
sub-issue per step. The mod-support design it builds on is
[issue #1](https://github.com/ozgb/byte-tactics-moddable/issues/1).

## How it fits together

```
            engine.wasm   economy.wasm   ai.wasm   pathfind.wasm   orders.wasm   ...
            (core, view,  (3.1c default modules, or a mod's replacements)
             presentation)
                 |  one shared game-state memory, one shared function table
                 |  imports: display, audio, input, network, files, time
      +----------+-----------+
  native host              browser host
  wasmtime, SDL3,          Web Worker, WebGL2,
  OpenGL                   WebAudio, WebRTC
  (Windows, Linux, macOS)
```

- **Game logic** works directly on one shared block of game state, in the
  same order as 3.1c, which is what keeps the default modules bit-identical.
- **Presentation** (rendering, UI, sound and UI mods) reads a filtered view of
  what one player can see, and changes the game only by sending orders. That
  boundary is what lets the same game run in lockstep or server-client mode.
- **A native i686 debug build** compiles the same source into one native
  program, for debuggers, sanitizers and profilers.

## Staying faithful to 3.1c

The fork gives up matching bytes and network compatibility with 3.1c clients.
It keeps the game's behaviour:

- **The reference** is a build of a `moddable` commit
  (`uv run tools/reference.py HEAD`): 3.1c's game logic plus a test harness,
  which is part of `src/`.
- **A test mode** makes runs reproducible: fixed random seeds, one simulation
  step per frame, scripted AI-vs-AI games.
- **Traces** hash the game state after every phase of every tick. A change
  passes only if its traces are identical to the reference, and a divergence
  is reported at the first tick, phase and field that differ.

Every change to the engine is gated on that, on every platform.

## Building the reference

The reference builds from `src/` with the original compiler, under Wine. You
need `wine`, `7z`, `cabextract`, `curl` and [uv](https://docs.astral.sh/uv/),
and your own copy of Total Annihilation (the Steam version).

```sh
tools/setup_toolchain.sh        # VC++ 5.0 + SP3 into toolchain/, TotalA.exe from Steam (set STEAM_TA if needed)
uv run tools/check.py 0x401070  # one function: prints MATCH
uv run tools/place.py           # the whole exe, compared byte for byte with the original
```

The result is the shipped v3.1 exe, MD5 `8e74a1dffa1f5988624c52048f5b20cd`.
[docs/linking.md](docs/linking.md) explains how the build works.

## Repository layout

| Path | What is in it |
| --- | --- |
| `src/` | The engine source, in folders by subsystem |
| `include/` | Shared types and prototypes |
| `data/` | The function map, symbols and module ranges of the original exe |
| `tools/` | Build, comparison and play-test scripts |
| `docs/` | Guides from the decompilation, and bugs found in Cavedog's code (`docs/bugs.md`) |
| `orig/` | The original exe (ignored by git) and its expected hash |
| `toolchain/` | VC++ 5.0 and the Wine prefix (ignored by git) |

## Contributing

Work is handed out as the sub-issues of
[#2](https://github.com/ozgb/byte-tactics-moddable/issues/2). Coding agents
work through them following `AGENTS.md`. See `CONTRIBUTING.md` to help.

## Legal and licence

This is a preservation and research project. It is not affiliated with or
endorsed by Cavedog Entertainment or Wargaming, who own Total Annihilation.

- **Tooling.** The tooling in `tools/` is MIT licensed (see `LICENSE`).
- **Game code.** The reconstructed code in `src/` is derived from Cavedog's
  work; we claim no rights over it and it is not offered under any licence.
- **Game data.** No game files are included, and releases will never include
  them. The game runs from a copy of Total Annihilation you own.

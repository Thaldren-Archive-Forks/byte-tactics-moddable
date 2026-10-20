# State classes

`data/state_classes.csv` says, for every piece of the game's state, whether it belongs in the shared state block (#7) and whether presentation may touch it (#22). This file explains how the classes were chosen and what is left to split. It is the output of #125.

## The file

One row per item, sorted by kind, then address, then name. Kinds: `global` (a row of `data/globals.csv`), `static` (a variable the source defines that `data/globals.csv` does not list), `game-region` (a top-level field of `struct Game`) and `heap` (an allocation site in game logic).

Columns: `name, address, kind, proposed, class, rule, phases_write, phases_read, presentation, load_write, writers, readers, note`. The tool's docstring describes each one.

The classes:

- **sim**: simulation state. Goes in the shared state block.
- **view**: one player's view and the UI. Presentation owns it.
- **platform**: devices, files, windows, network slots and other host state.
- **mixed**: used both ways. Split it; the note says how.
- **const**: read-only input to the sim: nothing writes it, it stays in the image, and `--check` fails if a strong writer appears. A weak writer (a `?` in `writers`, the address taken or the table passed to a function) is allowed when the `note` says what it does with the table; `--check` fails on weak writers with an empty note. It also fails when load code reaches a strong writer.

`proposed` comes from the rules below and is rewritten on every run. `class` is the hand review, starts empty, and is kept by regeneration, as is `note`.

```sh
uv run tools/stateclasses.py                 # regenerate, keeping class and note
uv run tools/stateclasses.py --check         # fail if anything but class and note differs
uv run tools/stateclasses.py --unclassified  # fail if a row has an empty class
uv run tools/test_stateclasses.py            # tests
```

## The rules

The evidence is `data/rwsets.csv` joined to the call graph. Three walks give each function the set of places that reach it:

- **Phase roots** (the calls of `RunGameSteps`, `hazards.ROOTS`): phases 0 to 8, so a read or write gets the phases it happens in.
- **Load roots** (`LoadMatch`, `AssignStartPositions`): `load_write`.
- **Presentation roots and directories**: the non-sim calls of `RunGameSteps`, `MainLoopTick`, the frame handlers, and every function under `src/graphics/`, `src/sound/`, `src/gui/`, `src/frontend/` and `src/ingame/`. The phase and load walks stop at such a function; `StepGafSequence` is the one exception.

`proposed`, first match wins:

- **platform**: a device, file, window or DirectX type or name, or only `src/debug/` and `src/harness/` touch it.
- **mixed**: a phase writes it and presentation writes it, or presentation writes what a phase reads (load counts like a phase).
- **sim**: a phase writes it, or load writes it and a phase reads it, or a phase reads it and nothing writes it (tables).
- **view**: presentation writes it and no phase reads it, or only presentation reads it.
- empty: only code outside every walk touches it.

Known limits:

- **Type-wide evidence.** A struct-typed global, a region, a static or a heap block takes the accesses of every use of the type, not of the instance. Rect and Vec3 collect everything.
- **Weak writes.** `*_maybe_written` (a non-const pointer argument) and `*_escape` (address taken) count as writes for phases and load, but only as reads for presentation. A phase can be flagged sim on a weak write alone, and presentation can hide a real write behind one. The rule text says `(weak writes only)`.
- **Name-based walks.** Presentation is partly defined by directory, so a sim helper under those directories is called presentation. Indirect calls need the hand-resolved edges in `data/hazard_edges.csv`; a missing edge is a missing phase.

## Counts

As of the last read of `data/state_classes.csv` (2153 rows):

| kind | sim | view | platform | mixed | const | total |
|---|---|---|---|---|---|---|
| global | 76 | 379 | 296 | 0 | 23 | 774 |
| static | 5 | 14 | 223 | 0 | 6 | 248 |
| game-region | 120 | 507 | 128 | 39 | 0 | 794 |
| heap | 212 | 46 | 79 | 0 | 0 | 337 |

## Mixed cases and how to split them

The nine game-region rows with class `mixed`:

- `players` (+0x1b63): platform for the net slot, ping, `info` and `name`; sim for the rest (type, units, squads, explored map, resources, alliances).
- `localPlayer` (+0x2a42): sim reads it as the perspective player. Fix a sim perspective player at match start; keep a separate view local player.
- `playerIndex` (+0x2a43): the watched player in the views, but sim reads it (unit flag, visibility, line of sight). Same split as `localPlayer`.
- `weapons` (+0x2cf3): view for the text, animation, model, sound, shake and colour fields; sim for damage, range, reload, velocity and flags.
- `mapFlags` (+0x14281): sim bits for line of sight and mapping mode; the "view needs refresh" bit cleared by the camera becomes view state.
- `defs` (+0x1439b): view for `soundcategory` and `field_18e`; sim for the rest.
- `uiOptionFlags` (+0x37f2f): sim bits for treedeath and the cheat toggles (halfshot, shootall, drop, radar); view bits for clock, noshake, selboxes; bit 0 (skip net timeouts) is platform.
- `otherFlags` (+0x38a51): bit 0 is the pause request, written by the net and the UI. Give it to the driver; the other bits are UI.
- `net` (+0x391e9): the mission object. Sim for wind, tide, water damage, kill and time multipliers, start positions and the TDF lists; view for names, description, planet and the text fields.

## Presentation writes sim state (#22 crossings)

Found during review. Each needs an order, a message to the driver, or a split of the field.

- `HandleBuildPanelClick` (src/ingame/control_panel.cpp:1070-1077) calls `QueueBuildOrder`, then `AdjustBuildCount` (src/orders/order_list.cpp:1076). A click edits a unit's order list directly.
- `DrawWeaponCoverage` (src/ingame/markers.cpp:564) sets `order->flags |= 0x200000` and `order->cached` while drawing.
- Console commands and the debug hotkey:
  - `CmdATM`, `CmdNoMetal` and `CmdNoEnergy` write `Player.metal` and `Player.energy`.
  - `CmdSeaLevel` writes `seaLevel` and `CmdEdge` writes `mapWidth` and `mapHeight` (src/game/console_commands.cpp:843, :731-732).
  - `CmdControl` and `CmdView` write `localPlayer` and `playerIndex` (console_commands.cpp:854-868), which the tick reads.
  - The cheat toggles write `uiOptionFlags` bits.
  - `CmdILose` and `CmdIWin` write `endGameFlags`.
  - `CmdMeteor` writes the meteor globals.
  - `HandleDebugHotkey` (dialogs_493340.cpp:1364-1365) writes `Player.metal` and `Player.energy`.
- `HandleSurrenderChoice` writes `endGameFlags`.
- The options and keys pause bit is `otherFlags`.
- The camera clears a `mapFlags` bit that line of sight uses.
- The end-game screen frees `g_debrisMemCache` (src/frontend/endgame.cpp:1199, 1206).
- The lobby writes the packet manager and players before play. That is fine: nothing ticks yet.
- Not a crossing: `g_explodedPieces`. The draw only clears a slot when `DrawExplodedPiece` returns 0, which it never does (src/weapons/explosions.cpp:753).

## Determinism findings

- **`DrawProjectiles`** (src/weapons/weapons.cpp:3286-3320, the shotKind 7 lightning branch) draws its per-frame jitter from the sim CRT `rand()` stream.
  - #52 left it there because `data/crt_calls.csv` reaches it through a console-command edge.
  - No phase reaches it, so the number of sim `rand()` draws depends on frames drawn and visible lightning shots.
  - In test mode, frames and steps are 1:1, so it is reproducible there. It is still wrong by #52's rule, and is a lead on #122.
- **`CheckDefeat`** (src/game/victory.cpp:1523-1525): `g_cdMismatchDefeatTick` comes from CRT `rand()` when `g_cdPathMismatch` is set by the CD check (src/game/cd_check.cpp:126).
- **`g_timeoutTimerStart`** is set from wall-clock `GetTicks()` (src/network/net_game.cpp:2779). This is network only and already a #40 task.

## Limits and left out

- Load-time code is in the load walk, and a destructor's accesses are evidence (`delete` and temporaries are `call` rows in `data/rwsets.csv`). A destructor of a class that is neither user-provided nor virtual is not followed (see the docstring of `tools/rwsets.py`).
- The classes of clear-cut UI, debug and platform rows rest on the tool's evidence plus names. Rows that matter to the tick were checked against the code.
- Rows the review marked "unsure" in their notes (check the code before relying on them):
  - game-region: `unknown_2851`, `count_14277`, `eyes`, `count_148e7`, `data_148eb`, `interfaceType`, `gameSpeed`, `effectiveGameSpeed`, `net_flags`.
  - global: `DAT_004fc474`, `DAT_004fc978`, `DAT_004fcc48`, `DAT_004fd2e4`, `DAT_004fda70`, `DAT_004fdbe0`, `g_consoleCommands`, `g_cheatCommands`, `DAT_00501fcc`, `g_debugCommands`, `DAT_00502f98`, `DAT_00507b64`, `DAT_00507b68`, `g_rotationMatrix`, `g_emptyStringRefs`, `g_emptyString`, `g_sendCondenser`, `g_receiveCondenser`, `g_netFrameRateConfig`, `g_mapLoadFlag`.
  - heap: `LoadUnitFbi: GameAllocIgnoreTag BUILDING YARD`, the three `Mission::LoadMissionData` sites (MissionFeature, MissionRule, MissionUnit), `FrameQueue::ResetFrames`, `FrameQueue::QueueFrames`, `BuildDerivedLayers: operator new FogCell`, `LoadTntMap` (MAPPED MEMORY, TILE MAP), `CobScript::LoadScriptState` (SavedPiece), `AllocFrame` and the two `AllocDepthFrame` GafFrame sites.

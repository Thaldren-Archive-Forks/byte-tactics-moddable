# Reference build and scenarios

`uv run tools/reference.py HEAD` builds `build/reference/HEAD/TotalA.exe`: the
current commit, with the harness in `src/harness/harness.cpp`. The game functions
the harness changes are listed in `changed.txt`.

## Running a scenario

The exe takes a scenario file with `-testmode`. It starts a skirmish from the
file without menus or input, plays `ticks` game ticks, and exits with status 0.
A bad scenario or a failed check exits with status 2 and a message on stderr
that names the line.

```sh
build/reference/HEAD/TotalA.exe -testmode reference/scenarios/skirmish2.txt
WINEARCH=win64 uv run tools/playtest.py --exe build/reference/HEAD/TotalA.exe \
  --testmode reference/scenarios/skirmish2.txt     # under Wine, headless
```

The FPU check ignores the infinity-control bit, which no x87 since the 80387
uses, so both `0x9001F` and Wine's `0xD001F` pass.

## Scenario format

One `key value` per line. Blank lines and lines starting with `#` are ignored.
Numbers are decimal, `0x` hex or `0` octal. Unknown keys and values that do not
parse are errors. A later line for the same key replaces an earlier one.
Defaults are the ones in `LoadSettings`, never the registry.

| Key | Default | Meaning |
| --- | --- | --- |
| `game_seed` | 1 | Seed of the game-logic random generator |
| `crt_seed` | 1 | Seed of the C runtime `rand` |
| `view_seed` | 1 | Seed of the presentation random generator (screen shake, particles, menu effects) |
| `fpu_control_word` | `0x9001F` | Expected `_control87` value, ignoring the infinity-control bit; the run exits 2 if it differs |
| `ticks` | required | Game ticks to play (30 per second of game time) |
| `map` | required | Map name, the rest of the line (names have spaces) |
| `difficulty` | 1 | AI difficulty 0 to 2, one value for all computer players |
| `commander_death` | 1 | 1: the game ends when a commander dies |
| `mapping` | 1 | 1: the whole map starts mapped |
| `line_of_sight` | 1 | 1: line of sight is on |
| `los_type` | 1 | Line of sight type, 0 or 1 |
| `start_positions` | `fixed` | `fixed`: player N starts at position N; `random`: shuffled |
| `local_player` | the first human player, else 0 | Slot whose view the game shows; must be a slot some `player<N>.*` key names |
| `player<N>.controller` | `ai` | `ai` or `human` (a human does nothing) |
| `player<N>.side` | N mod 2 (0 is Arm, 1 is Core) | Side name from `sidedata.tdf`, any case |
| `player<N>.team` | 5 | Ally group 0 to 4; 5 is no team |
| `player<N>.color` | N | Colour index 0 to 9 |
| `player<N>.metal` | 1000 | Starting metal |
| `player<N>.energy` | 1000 | Starting energy |

`player<N>.*` keys take N from 0 to 9. A slot exists when any key names it,
slots no key names are open, and at least 2 slots must exist. The game's player
count is the highest named slot plus one.

## Scripted orders

A scenario can order the units of a `controller human` slot (a human otherwise
does nothing). One `order` line per order; lines may repeat and accumulate.

```
order <tick> <unit selector> <kind> <target>
order 5 0:ARMCOM:1 move 700 300
order 5 0:ARMCOM:1 build ARMSOLAR 640 260
order 150 0:ARMCOM:1 reclaim 0:ARMSOLAR:1
```

- `<tick>` is a game tick from 1 to `ticks`. The order is issued at the start of
  that tick, before the first phase, so the trace sees its effect in the same
  tick. Orders on one tick are issued in file order.
- A unit selector is `<slot>:<unit type>:<n>`, the n-th live unit of that type
  owned by that slot in pool-index order (the unit type name is the unit's name
  in the game data, any case), or `<slot>:all`, all of that slot's live units.
- A target is `x z` in world units, or a unit selector that names one unit
  (`<slot>:all` is not allowed there). With a unit selector the order is
  issued as if the cursor were over that unit.
- Kinds: `move`, `attack`, `patrol`, `guard` and `reclaim` take a target.
  `build <unit type> x z` takes the unit type before the position.

Each order is issued as a mouse click does: the harness selects the named
units, sets the order mode and cursor position, and calls the game's
`IssueOrderToSelection` (`build` calls `IssueMobileBuildOrders`, as a click on a
build site does). The selection is put back afterwards.

A bad selector, kind or position, a slot that is not `human`, a tick after the
last tick, an unknown unit type, a build site the game rejects, or a selector
that names no live unit at its tick exits with status 2 and a message that
names the line.

## Traces

In test mode the exe writes `trace.txt` in its working directory: a hash of the
game state after each phase of each tick, flushed every tick. LF line endings.

```
bt-trace 1
commit <40-hex sha from commit.txt beside the exe, or unknown>
scenario <path as given to -testmode>
game_seed <n>
crt_seed <n>
view_seed <n>
fpu_control_word 0x<hex>
<tick> <phase> <16 hex digits>
```

The phases run in this order each tick: `net` (only on ticks that handle
network packets), `units`, `projectiles`, `explosions`, `players`, `features`,
`wind`, `meteors`. Each covers the state its subsystem owns. The hash is 64-bit
FNV-1a over a canonical serialisation of that state. Two builds conform when
their traces are identical.

The stored form of a trace keeps one digest per tick (see The suite).

`tools/playtest.py --testmode` copies `trace.txt` (and `dump.txt`) into the
run's directory under `build/playtest/`, and copies `commit.txt` from the exe's
directory into the game directory for the header.

## Dumps

`-testdump <tick>` (`--testdump <tick>` for playtest.py, with `-testmode`) also
writes `dump.txt`: every hashed field after each phase of that tick. It is
ignored without `-testmode`; a value that does not parse exits with status 2.

```
bt-dump 1
<tick> <phase> <path> <value>
```

Paths are `gameTick`, `rng.parkMiller`, `rng.crtHoldrand`, `units[i].<field>`,
`projectiles[i].<field>`, `explodedPieces[slot].<field>` (and `.rec`),
`debris[i].<field>`, `pieces[i].<field>` (with `.vertices` and `.faces[k]`),
`players[i].<field>`, `features[i].<field>` (with `.list`), `wind.<field>` and
`meteors.<field>`. Every phase dumps all of them, not only its own subsystem's.

A field that differs between two builds is the same `<tick> <phase> <path>`
with another `<value>`. Scalars are `0x` and the field's bytes as one hex
number; byte arrays and unnamed gaps (`+0x<offset>`) are `hex:` and the bytes.

## Pointer IDs

A pointer is dumped as an ID, never an address: `null`, `<array>:<index>`
(`unit:12`, `player:3`, `unitdef:41`, `weapondef:7`, `projectile:5`,
`feature:9`, `piece:20`), or `heap` for a non-null pointer into no array that
has stable indexes. Indexes are the stable ones: unit pool index, projectile
array index, and so on.

## Comparing two builds

```sh
WINEARCH=win64 uv run tools/tracediff.py A/TotalA.exe B/TotalA.exe \
  --scenario reference/scenarios/skirmish2.txt
```

It runs both exes, reports the first tick and phase whose hashes differ, then
reruns both with `--testdump` at that tick and lists the fields that differ:

```
first divergence: tick 900, phase units
  units[12].health: a=0x0064 b=0x0063
```

Traces with different scenario or seeds are refused (exit 2).
Exit status is 0 for identical traces, 1 for a divergence and 2 for an error.
`--traces A B` compares existing `trace.txt` files without running, and
`--dumps A B --at <tick> <phase>` diffs two `dump.txt` files.

## The suite

`reference/scenarios/` holds the scenario suite: AI-versus-AI games (and the three `orders-` scenarios, which script a human slot) that vary the
map, the player count (2, 3, 4, 6, 8 and 10), the sides, ally teams, fixed and
random start positions, the options (difficulty, line of sight, mapping,
starting resources) and the length, up to 54000 ticks (30 minutes). Each has its
own game, C runtime and view seeds (the first three predate the suite and share
seeds) and runs deterministically: two runs give identical traces.

| Name | Map | Players | Ticks | Covers |
| --- | --- | ---: | ---: | --- |
| duel-coast-to-coast | Coast To Coast | 2 | 5400 | Two Core players on Coast To Coast, three minutes, difficulty 2 |
| duel-great-divide | Great Divide | 2 | 3600 | Two Arm players on Great Divide, random start positions, two minutes |
| duel-pass-dark | The Pass | 2 | 2700 | Two players on The Pass with the map unmapped and line of sight off, ninety seconds |
| duel-shore-to-shore | Shore to Shore | 2 | 2700 | Arm against Core on Shore to Shore with 5000 metal and energy, no commander death, difficulty 0 |
| eight-full-moon | Full Moon | 8 | 9000 | Eight players on Full Moon, alternating sides, five minutes |
| eight-greenhaven | Greenhaven | 8 | 5400 | Eight Arm players on Greenhaven, random start positions, three minutes |
| eight-pincushion-teams | Pincushion | 8 | 12600 | Eight players on Pincushion in four allied pairs, seven minutes |
| long-great-divide-2p | Great Divide | 2 | 54000 | Two players on Great Divide for 54000 ticks (thirty minutes), a long run that no commander death ends |
| long-greenhaven-8p | Greenhaven | 8 | 54000 | Eight players on Greenhaven in two allied teams of four for 54000 ticks (thirty minutes), a long run |
| orders-build-patrol | The Pass | 2 | 600 | Human Arm commander builds a solar collector, then patrols (`build`, `patrol`) |
| orders-guard-reclaim | The Pass | 2 | 600 | Human Arm commander builds a solar collector, guards it, then reclaims it (`build`, `guard`, `reclaim`) |
| orders-move-attack | The Pass | 2 | 600 | Human Arm commander moves, attacks the Core commander, then the whole slot moves (`move`, `attack`, `<slot>:all`) |
| quad-fox-holes | Fox Holes | 4 | 7200 | Four players on Fox Holes, alternating sides, four minutes |
| quad-king-of-the-hill | King of the Hill | 4 | 9000 | Four players on King of the Hill in two allied teams of two, five minutes |
| quad-lava-highground | Lava Highground | 4 | 5400 | Four Arm players on Lava Highground, random start positions, three minutes |
| quad-two-continents | Two Continents | 4 | 6000 | Four players on Two Continents in teams of players 0 and 3 against 1 and 2, random start positions, 100 metal and energy |
| six-hundred-isles | Hundred Isles | 6 | 9000 | Six players on Hundred Isles, alternating sides, five minutes |
| six-painted-desert | Painted Desert | 6 | 12600 | Six Core players on Painted Desert, random start positions, seven minutes |
| six-yerrot-mountains | Yerrot Mountains | 6 | 7200 | Six players on Yerrot Mountains in two allied teams of three, four minutes |
| skirmish2 | The Pass | 2 | 1800 | Two computer players on a small map, one minute of game time |
| skirmish4-long | Metal Heck | 4 | 27000 | Same as skirmish4 for 27000 ticks: reaches past tick 25786, where a hover unit's ground bob once read the wall clock |
| skirmish4 | Metal Heck | 4 | 9000 | Four computer players, one team each, five minutes of game time |
| ten-lava-run | Lava Run | 10 | 5400 | Ten players on Lava Run, alternating sides, three minutes |
| ten-ring-atoll | Ring Atoll | 10 | 9000 | Ten players on Ring Atoll, random start positions, five minutes, difficulty 2 |
| trio-anteer-strait | Anteer Strait | 3 | 5400 | Three players on Anteer Strait, players 0 and 1 allied, three minutes, line of sight type 0 |
| trio-dark-side | Dark Side | 3 | 9000 | Three players on Dark Side, fixed start positions, five minutes |
| trio-etorrep-glacier | Etorrep Glacier | 3 | 3600 | Three Core players on Etorrep Glacier, random start positions, two minutes |

## Stored traces and digests

`reference/traces/<scenario>.txt` holds one digest per tick, not the phase
hashes. The digest of a tick is the 64-bit FNV-1a hash of that tick's phase
hashes in trace order, each as 8 little-endian bytes. In test mode there are 7
phases per tick (`units` to `meteors`), since `net` does not run. The file:

```
bt-digest 1
trace_format bt-trace 1
commit <sha of the build that produced it>
scenario reference/scenarios/<name>.txt
game_seed <n>
crt_seed <n>
view_seed <n>
fpu_control_word 0x<hex>
<tick> <digest, 16 lowercase hex digits>
```

`tools/tracedigest.py trace.txt` converts a `trace.txt` to this form.

## Gate

```sh
export WINEARCH=win64
uv run tools/reference.py <ref> --traces     # build <ref>, play the suite, write reference/traces/
uv run tools/reference.py HEAD --check       # the gate: build HEAD, play the suite, compare
```

`uv run tools/reference.py HEAD --check` is the gate command later issues name.
Add `--game-dir <dir>` when the game is not in a Steam directory.

The gate applies when a pull request changes `src/`, `include/`,
`lib/bitexact/`, `reference/`, or the tools that build or run the exe
(`tools/link.py`, `tools/reference.py`, `tools/playtest.py` and the like). Other
changes do not need it.

`--traces` writes every trace with the built sha as `commit`. `--check` plays
every scenario (`--parallel N` at a time, default half the CPUs and at most 6), prints one line per scenario with its run time
and a total, and exits 0 when every digest equals the stored one. It exits 1
when a trace has no scenario or a scenario has no trace, and on a difference:
it builds the reference from the commit the trace records (cached in
`build/reference/<sha>`), prints the `tools/tracediff.py` report for the first
differing scenario and exits 1. `--check --exe <path>` checks a given exe instead
of building HEAD.

The full `--check` is what a pull request's gate needs.

## Quick check

```sh
uv run tools/reference.py HEAD --check --quick     # a fixed subset, for a fast look
```

`--quick` is the same `--check` restricted to the five scenarios in the
`QUICK` constant of `tools/reference.py`. Its scenarios take about 1 to 2
minutes at the default `--parallel`, after the build and checkall. A pass says so and reminds you that it is not the gate; a failure
prints the same `tools/tracediff.py` report as the full run. `--quick` goes with
`--check` and `--exe`, and `--traces` rejects it. It is no substitute for the
full `--check`, which is what a pull request's gate needs.

| Name | Players | Sides | Start positions |
| --- | ---: | --- | --- |
| duel-coast-to-coast | 2 | Core | fixed |
| duel-great-divide | 2 | Arm | random |
| eight-full-moon | 8 | Arm and Core | fixed |
| eight-greenhaven | 8 | Arm | random |
| orders-move-attack | 2 | Arm and Core, scripted orders | fixed |

`tools/tracediff.py --ignore-view-seed` accepts traces whose headers differ only
in `view_seed`; with `--scenario` given twice, run mode runs A on the first file
and B on the second.

Run time of the suite (27 scenarios), measured on a 20-core machine with Wine and
other jobs running, not counting the build. Each run hardlinks its Wine prefix from
the template and copies only the registry files, so runs share nothing else, and the
longest scenarios start first. `--check` with the default 6 scenarios at a time took
5:39 and 6:16 (load average 30 and 50), and 7:57 with `--parallel 2`. The code before
this change took 9:59 and 9:19 on the same machine. `long-greenhaven-8p` alone takes
about 4:30 and sets the floor. Digests are identical between `--parallel 1` and the
default.

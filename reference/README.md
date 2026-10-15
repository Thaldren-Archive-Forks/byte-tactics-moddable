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

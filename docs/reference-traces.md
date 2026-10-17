# Checking a new engine against the reference traces

The git tag `reference-traces-v1` is the fixed target for anyone writing a new
engine. Check your engine's game state against it, or take the deterministic
code as a base. This page names the current tag: **`reference-traces-v1`**.

## What the tag holds

- The source: 3.1c's game logic, and the conformity harness that writes traces
  (`src/harness/`).
- The trace format: `bt-trace 3` (the digest files are `bt-digest 3`). The
  format is in `reference/README.md`, the hashed fields in
  `src/harness/trace_schema.h`, and the coverage decisions in
  `data/trace_coverage.csv`.
- The scenario list: `reference/scenarios/`, 27 scenarios, each with its own
  seeds and tick count.
- The digests: `reference/traces/<scenario>.txt`, one 64-bit digest per tick
  for each scenario.

Check the tag out to get all four:

```sh
git checkout reference-traces-v1
```

A tag is never moved. Read this page at the tag you target.

## What a check needs besides the source

The game data is not in the repo. The digests were made with one install (a
GOG copy of Total Annihilation with the Byte Tactics archives), and a different
install can give different traces. Check yours with `sha256sum`. The hashes of
the game data files are below; the game also reads the other small files of the
install (`TotalA.exe` is replaced by the exe under test).

| File | sha256 |
| --- | --- |
| totala1.hpi | c3890a78e627e6e334f671f65ec418cd40d15ad2ad4110e931bb3dfced0814c3 |
| totala2.hpi | 043e321169f73157812ec8e65b5fae29898ae3f41e527bc419801d18cf0cbe63 |
| totala3.hpi | 98de3dd9307c9324b9afdcbadc90ed3247f31a1bf9e0e63a0366f1f2b2e31128 |
| totala4.hpi | 8c0ffa68ca8a0ff0a6e0eda98d232a9876cb0551f151a7c43415df0153e27810 |
| worlds.hpi | 71e7afc84239fb6753e02d3c79c6e9a8d4ad0071ae995756e84870934026962e |
| tactics1.hpi | 89db2522cfc50c34e1ceecd61e44dfd4e29655d38daca5c59f71168893427229 |
| tactics2.hpi | 9559aa40ee823b927fa8cba217426e10159d2f200e39558aa1a69824f5eb80dd |
| tactics3.hpi | 076fa8de92e5cabe4cb5905ed736591a703d2f2cda157c44abdbfcb6eec120b5 |
| tactics4.hpi | d850baeb2eb29dc2a3f736c780fe8726bf93fc1dd901c654dd769940199708f9 |
| tactics5.hpi | 0ca2653775308bc23058f09b7dccd6f057b630b1398c6f3ad6f96a78173c2bd0 |
| tactics6.hpi | 3328e6c13bd5e45005db7f418c9efa42fb95d566935f1ed72152f017a41b91a2 |
| tactics7.hpi | ed343078a7a294710f9dc4ccb6914c48ae40d430c48201270e809fe14e04287e |
| tactics8.hpi | 7420f0adf9a0f67c3851e0ba4a35a3334b313048f063ec67a2832779af75a35e |
| btdata.ccx | bc72cf10922f78965e9d95a01acbe4393ab3c7f22ebdbf119f356225ee05ef59 |
| btmaps.ccx | 896b2db7be849a9fdab9f87aac6bd89f67adce6083c0a13c8e0233a92faaecc9 |
| ccdata.ccx | 981cf35f05979e800468e9e7b480e474ee626e7b5d1c721103bc69a66ee0aed8 |
| ccmaps.ccx | 43b0029b3943edb290487d055e2b3c6df12f7d02ebf33ab5f2dda6fb33ccc70e |
| ccmiss.ccx | b51a881f9e333ea1a65a091d8a55eca7711755929e040e26a1c70a567c5407ec |
| rev31.gp3 | fa0da77b8fb4400ce606995703dec42eeab5b05d04ca8c0e43be324f04ce0441 |
| AFark.ufo | 8a713a677e02ec2850507e91d556b69d0159732281ba4afa381362e20845c3ee |
| AFlea.ufo | 3504018b73524c381b00fc46f0e3162130597f3baf07b2e7d584534f08021fec |
| AScarab.ufo | cd7a362f1a8705fbebb6ae609c377b1768b6fb09a22192f453348515c8931068 |
| Cometctr.ufo | b7055488519dc028acafa09d71cdf767c0bc1ba8faf21e0aa637408403619123 |
| Cormabm.ufo | 44dd8d03089fccbf1b5ca4004e3d4160e1f10c2dadc247ed01aeef1d2909b61a |
| CorNecro.ufo | 8492c9372f89243b48956bde26b69018edb47d00923bbf74f2c0c33d430229ed |
| corplas.ufo | fd470a472796c0f8f807d0bc5fb1b312c94751337f0af02d6880a0a1bffe7ba8 |
| Evadrivd.ufo | 4c825aff22668f7f7e55438be27b8fdff1d09ab6f53619974d82493bda5a274a |
| Example.ufo | 013d1309d9983da40480baccaa76574e423a9fa3d1b3c609dcf7369b9f71e0f3 |
| floggen.ufo | 0e88714622e3bd45f1d1747c81f84ba72f7e3a0c5d476a150f15535f9a3bc978 |
| Mndsmars.ufo | 69be3671a251f0b2c9940e502226c220c389cf49f7ae8bef788f31eae9e10704 |

## Test-mode rules a matching engine must follow

The digests were made in test mode, with these rules. An engine that breaks
one can diverge without a logic bug.

- New heap allocations are zero (the game's `-memset=0` switch). The reference
  never plays a scenario with uninitialised heap memory.
- Memory grown by `realloc` is zero from the old size up to the new size
  (the whole block when the old block is null). Shrinking fills nothing.
- Some fields that 3.1c leaves unset are skipped from the hash, because their
  value is whatever the memory held. Do not trace them,
  and do not rely on them. The list at this tag is:
  - `Order.pos.y` of `MobileBuild`, `VTOL_MobileBuild`, `Reclaim` and
    `VTOL_Reclaim` orders.
  - `PathOrder.pos.y` when the order that made it is `VTOL_MobileBuild` or
    `VTOL_Reclaim`.
  - `PathOrder.followOffset` when the order's flags have bit 0x80 set (the
    constructor for these orders never writes it, and nothing reads it then).
  - `AiSearchGoal` flags bit 2 (the constructor never sets it; only the net
    state serialiser writes it, and the reader runs after that, in net games
    only).
  - `Mission.startMetal` and `startEnergy` for players 3 to 10 (slots 2 to 9),
    unless the mission is a campaign mission (`Mission.type` is 1). Only
    `LoadMission` writes them, and only slots 0 and 1 (map_list.cpp:963-966);
    the only reader is `InitPlayerResources` (players_464290.cpp:1131-1134),
    which reads them only for game type 1.

  The code is `OrderPosYUnset`, `FxPosYUnset`, the `PathOrderFollow` visit and
  the `MissionStartRest` visit in `src/harness/harness.cpp`, and the evidence
  that nothing reads them is on the Order, PathOrder, AiSearchGoal and Mission
  rows of `data/trace_coverage.csv`.
- These stack locals are zero in test mode, because 3.1c reads them before it
  writes them and the digests keep the result:
  - `pos` in `ScoutTimer::OnTimer` (0x408100).
- `PlayerAI.hasSpecial` is zero until the first unit list refresh, because the
  constructor never sets it. `FindCellNearFeatures` can read it in the tick that
  first assigns the squads, before that tick's refresh, so it is hashed from
  tick 1.

## Comparing an engine against the digests

Your engine writes a `trace.txt` in the same format as the reference (see
`reference/README.md`: header, then one hash row per phase and a `world` row
each tick) for a scenario in `reference/scenarios/`. Reduce it to digests and
compare it with the stored file:

```sh
uv run tools/tracedigest.py trace.txt --commit <sha> --scenario reference/scenarios/skirmish2.txt > mine.txt
diff mine.txt reference/traces/skirmish2.txt
```

The digest file starts with a header (`bt-digest 3`, trace format, commit,
scenario, seeds, `fpu_control_word`), then one `<tick> <digest>` line per
tick. Your header must agree with the stored one except for `commit`. The
first differing tick is where your engine first diverges.

A digest names only the tick. To find the phase and field, build the
reference from the tag and diff full traces and dumps with `tools/tracediff.py`:

```sh
export WINEARCH=win64
uv run tools/reference.py reference-traces-v1                 # build build/reference/reference-traces-v1/TotalA.exe
uv run tools/tracediff.py --traces ref/trace.txt mine/trace.txt   # first tick and phase that differ
uv run tools/tracediff.py --dumps ref/dump.txt mine/dump.txt --at <tick> <phase>
```

`ref/trace.txt` and `ref/dump.txt` come from running the reference exe with
`tools/playtest.py --testmode <scenario> --testdump <tick>` (see
`reference/README.md`, "Running a scenario" and "Dumps"). The report names the
tick, the phase and the first differing field.

If your engine is a build of this source that runs under Wine, one command does
all of it:

```sh
uv run tools/reference.py --check --exe path/to/TotalA.exe --game-dir <install>
```

It plays every scenario, compares the digests with `reference/traces/`, and on
the first difference prints the `tracediff` report. Add `--quick` for a fixed
subset of five scenarios (not the gate). `--game-dir` names the install; the
default is Steam's.

## Later versions

A tag is never moved. A later change to the trace format, the scenario list or
the digests that should become a target gets a new tag (`reference-traces-v2`
and so on), and this page is updated to name it. The current tag is
`reference-traces-v1`.

# Subsystems

This file is the draft interface of each subsystem, from the coupling map of #128 (part of #6). For the nine subsystems that have a module issue (#9 and #11 to #18) it gives the entry points, the imports (calls out to other subsystems) and the sim read and write sets, so each issue starts from a draft rather than from the source. The issue's own text wins where the two differ, and each section ends with open questions where the data shows a coupling the issue did not expect.

Nothing here is typed by hand except the prose. Every list and count comes from the tool:

```sh
uv run tools/coupling.py                      # regenerate data/subsystems.csv (keeping override) and data/coupling.csv
uv run tools/coupling.py --check              # fail if either file differs from what the tool regenerates (CI runs this)
uv run tools/coupling.py --summary [NAME ...] # print entry points, imports and sim items per subsystem
```

The inputs are `data/rwsets.csv`, `data/hazard_edges.csv` and `data/state_classes.csv` (see `docs/state.md`); the tool's docstring has the rules that assign a function to a subsystem. The 4519 functions of `data/rwsets.csv` are in 17 subsystems, and the sim items are the rows of `data/state_classes.csv` of class `sim` or `mixed`, plus the struct fields classified the same way. A call or a shared item counts functions of the calling subsystem, not call sites.

The subsystems and the issue each maps to:

| subsystem | functions | issue |
|---|---|---|
| [wind_meteors](#wind_meteors) | 15 | #9 |
| [economy](#economy) | 38 | #11 |
| [features](#features) | 62 | #12 |
| [ai](#ai) | 160 | #13 |
| [pathfinding](#pathfinding) | 59 | #14 |
| [weapons](#weapons) | 118 | #15 |
| [targeting](#targeting) | 10 | #16 |
| [los_radar](#los_radar) | 118 | #17 |
| [orders](#orders) | 468 | #18 |
| [units](#the-remaining-subsystems) | 193 | none yet |
| [players](#the-remaining-subsystems) | 149 | none yet |
| [map](#the-remaining-subsystems) | 109 | none yet |
| [network](#the-remaining-subsystems) | 450 | none yet |
| [game](#the-remaining-subsystems) | 160 | none yet |
| [harness](#the-remaining-subsystems) | 58 | none yet |
| [presentation](#the-remaining-subsystems) | 1514 | none yet |
| [platform](#the-remaining-subsystems) | 838 | none yet |

The eight subsystems without a module issue are described briefly [below](#the-remaining-subsystems). #10 (per-module replay tests) uses all of them.

How to read a section. **Entry points** are the phase roots the subsystem holds and the functions other subsystems call, with the callers. The main loop and the frame handlers (`MainLoopTick`, `RunGameSteps`, `UpdateFramePacing`, `BattleFrame`, `EndGameFrame`, `MainFrameTick`) are in `game` by `override`, although most are in presentation folders (`src/ingame/`), so a phase root has callers in `game`, not in `presentation`. **Imports** are the functions it calls in other subsystems, by target. **Sim writes** and **sim reads** list items with their class from `data/state_classes.csv` and the number of its functions that access the item; "also written by" names the other writers. Lists are cut to the twelve most used named items. "Named" leaves out the shared value types (`Vec3`, `Point`, `Fixed`) and the per-file copies of a struct that the source declares several times (a name ending in an address, such as `Proj_0049c740`).


## wind_meteors

Issue #9. 15 functions. `UpdateWind`, `UpdateMeteors`, the meteor parameters and their save and load (`src/map/meteors.cpp`, plus `UpdateWind` in `src/game/game_state.cpp`).

**Entry points.** Phase roots: `UpdateWind` and `UpdateMeteors`, both called from `RunGameSteps`. `UpdateWind` is also called from `LoadBattleAssets` (game).

11 functions are called from other subsystems. The most called, with their callers:

- `UpdateWind` (2 from game)
- `CmdMeteor` (1 from platform)
- `DisableMeteors` (1 from map)
- `EmptyShutdownPreCleanup` (1 from game)
- `EnableMeteors` (1 from map)
- `InitMeteors` (1 from game)
- `LoadMeteors` (1 from game)
- By calling subsystem (functions called): game 6, map 4, platform 1

**Imports.** 14 functions of 2 subsystems, by target, each with the number of functions it calls there:

- platform (12): `HapiBank::OpenAccount`, `BuildDataPath`, `CommandArgs::GetIntArg`, `FatalError`, and 8 more
- weapons (2): `FindWeaponByName`, `SpawnProjectile`

**Sim writes.** 24 items in all, 19 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`g_meteorActive` (sim, 6 fn); `g_meteorNextStrikeTime` (sim, 6 fn); `g_meteorNextHitTime` (sim, 5 fn); `g_meteorOrigin` (sim, 5 fn); `g_meteorStrikeEndTime` (sim, 5 fn); `g_meteorTarget` (sim, 5 fn); `g_meteorsEnabled` (sim, 5 fn); `g_meteorScheduleGap` (sim, 3 fn); `g_meteorShowerDuration` (sim, 3 fn); `g_meteorSpawnInterval` (sim, 3 fn); `g_meteorSpawnMagnitude` (sim, 2 fn); `g_meteorWeapon` (sim, 2 fn)

**Sim reads.** 10 items only read, 9 named. The most used (`g_game`, the root pointer, left out):

`Game::gameTick` (mixed, 3 fn); `Game::mapHeightTiles` (sim, 2 fn); `Game::mapWidthTiles` (sim, 2 fn); `g_meteorWeaponName` (sim, 2 fn); `DAT_005119b8` (sim, 1 fn); `Game::field_38a47` (sim, 1 fn); `Game::windSpeedDenominator` (sim, 1 fn); `Game::windSpeedMax` (sim, 1 fn); `Game::windSpeedMin` (sim, 1 fn)

**Draft interface.**

- Exports: 11 functions (the two phase roots, `InitMeteors`, `LoadMeteors` and `SaveMeteors` for game, the map controls `EnableMeteors`, `DisableMeteors`, `SetMeteorParams` and `MeteorParams::LoadMeteorDefaults`, and the console command `CmdMeteor`).
- Needs from seams: 14 functions in platform, weapons.
- Owns: 18 named sim items that only this subsystem writes, for example `g_meteorActive`, `g_meteorNextStrikeTime`, `g_meteorNextHitTime`, `g_meteorOrigin`, `g_meteorStrikeEndTime`.
- Shares: 1 named sim item that other subsystems also write, for example `Game::windCounter`.
- Pilot fit (#9): 12 of its 14 imports are platform (TDF parsing, `RandomInt`, `FatalError`, the `HapiBank` account store). The only sim seam is weapons (`SpawnProjectile`, `FindWeaponByName`).
- The wind output is in `Game` fields (`windSpeed`, `windDirection`, `windFraction`, `windVelX`, `windZ`) that only this module writes but units, economy, features and presentation read. The meteor schedule is in `g_meteor*` statics, which must move into the state block (#7).

**Open questions.**

- `SpawnProjectile` is the one call into another sim subsystem. Does the pilot need a weapons seam (#8) before #15 exists, or a stub?
- `RandomInt` is counted as a platform import but draws from a game-logic stream (#3). Should the random streams be an engine service in the seam list instead?

## economy

Issue #11. 38 functions. The resource accounts (`UnitResources`), `UpdatePlayerEconomy`, resource sharing and the share console commands (`src/game/economy*.cpp`, `share_commands.cpp`, `UpdateResourceSharing` in `src/network/net_game.cpp`).

**Entry points.** Phase roots: `UpdateResourceSharing`, called from `RunGameSteps`. The per-tick income step `UpdatePlayerEconomy` is not a root: `UpdatePlayers` (players) calls it, so most of the economy runs inside the players phase.

20 functions are called from other subsystems. The most called, with their callers:

- `GetEnergyUse` (4 from ai)
- `ShareMapInfo` (1 from network, 1 from presentation)
- `UnitResources::RequestEnergyAndMetal` (1 from orders, 1 from units)
- `UnitResources::Reset` (1 from players, 1 from units)
- `UpdateMetalExtraction` (2 from units)
- `CmdNoEnergy` (1 from platform)
- `CmdNoMetal` (1 from platform)
- By calling subsystem (functions called): platform 7, units 6, players 4, ai 1, game 1, network 1

**Imports.** 18 functions of 6 subsystems, by target, each with the number of functions it calls there:

- platform (5): `CommandArgs::GetIntArg`, `HapiBank::OpenNamedBox`, `HapiBank::SeekBox`, `HapiBank::ReadBox`, and 1 more
- network (4): `AddMessage`, `BroadcastPlayerInfo`, `PlayerDpid_00457d30`, `SendPacketToPlayer`
- los_radar (3): `GetMapCell`, `ShareMapping`, `ShareRadar`
- game (2): `IsPlaying`, `SetSize_00496e90`
- players (2): `TransferEnergy`, `TransferMetal`
- units (2): `CobScript::StartScriptWithArgs`, `Unit::SetStateBits`

**Sim writes.** 41 items in all, 28 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Player::energy` (mixed, 7 fn, also written by game, players, presentation); `Player::metal` (mixed, 7 fn, also written by game, players, presentation); `UnitResources::energyUse` (mixed, 4 fn); `UnitResources::player` (mixed, 4 fn); `Player::energyCapacity` (sim, 3 fn, also written by players); `Player::metalCapacity` (sim, 3 fn, also written by players); `UnitResources::metalUse` (mixed, 3 fn); `Player::shareEnergy` (mixed, 2 fn, also written by players); `Player::shareMetal` (mixed, 2 fn, also written by players); `PlayerInfo::shareMetal` (mixed, 2 fn); `Unit::extraction` (mixed, 2 fn, also written by units); `UnitResources::energyUsePaid` (mixed, 2 fn)

**Sim reads.** 58 items only read, 52 named. The most used (`g_game`, the root pointer, left out):

`Game::localPlayer` (mixed, 8 fn); `Player::active` (mixed, 8 fn); `Player::type` (mixed, 8 fn); `Game::netGameFlags` (mixed, 7 fn); `Game::players` (sim, 7 fn); `Player::info` (mixed, 5 fn); `Game::difficulty` (mixed, 4 fn); `Player::index` (mixed, 3 fn); `Game::gameTick` (mixed, 2 fn); `Game::tidal` (sim, 2 fn); `Game::windFraction` (sim, 2 fn); `Unit::def` (mixed, 2 fn)

**Draft interface.**

- Exports: 20 functions (`UpdatePlayerEconomy`, `UpdateResourceSharing`, the spend and request calls (`UnitResources::SpendEnergy`, `SpendMetal`, `RequestEnergy`, `RequestEnergyAndMetal`), `GetEnergyUse` (ai) and `UpdateMetalExtraction` (units)).
- Needs from seams: 18 functions in platform, network, los_radar, game, players, units.
- Owns: 6 named sim items that only this subsystem writes, for example `UnitResources::energyUse`, `UnitResources::player`, `UnitResources::metalUse`, `PlayerInfo::shareMetal`, `UnitResources::energyUsePaid`.
- Shares: 22 named sim items that other subsystems also write, for example `Player::energy`, `Player::metal`, `Player::energyCapacity`, `Player::metalCapacity`, `Player::shareEnergy`.
- `Player::energy` and `Player::metal` are written by four subsystems (economy, game, players, presentation; class `mixed`), so the economy module cannot own them alone. The statistics (`energyWasted`, `totalMetalProduced` and the rest) are also written by network and players.
- Economy sends packets (`SendPacketToPlayer`, `BroadcastPlayerInfo`, `AddMessage`) and starts COB scripts (`CobScript::StartScriptWithArgs`); both are candidates for events instead of imports.

**Open questions.**

- Which subsystem owns the per-player account fields? A state-block struct for the accounts that economy writes and players and network only read would match the data better than the `Player` struct.
- `GetMapCell`, `ShareMapping` and `ShareRadar` are imports from los_radar: resource sharing also shares vision. Does that sharing belong to los_radar's interface or to economy's?

## features

Issue #12. 62 functions. `UpdateFeatures` and feature loading, spreading, burning, reclaim and wreckage (`src/map/features*.cpp`).

**Entry points.** Phase roots: `UpdateFeatures`, called from `RunGameSteps`.

23 functions are called from other subsystems. The most called, with their callers:

- `GetCellFeature` (1 from game, 1 from map, 1 from orders)
- `DamageFeature` (1 from network, 1 from weapons)
- `PlaceFeature` (1 from los_radar, 1 from units)
- `ReclaimFeature` (2 from orders)
- `CmdBurnAll` (1 from platform)
- `CmdBurnOne` (1 from platform)
- `CmdFeature` (1 from platform)
- By calling subsystem (functions called): game 8, los_radar 5, orders 3, network 3, platform 3, units 2

**Imports.** 46 functions of 8 subsystems, by target, each with the number of functions it calls there:

- platform (25): `TdfRecord::GetFieldString`, `RandomInt`, `TdfFile::ResetCurrentRecord`, `TdfFile::SelectRecord`, and 21 more
- presentation (9): `InitGafSequence`, `FindGafEntry`, `FreeObjectState`, `CreateObjectState`, and 5 more
- network (4): `BroadcastPacket`, `GetLocalDpid`, `GetHostDpid`, `SendPacketToPlayer`
- map (3): `Mission::GetGameType`, `RefreshAllPassMaps`, `GetCellMeanHeight`
- weapons (2): `ApplyAreaDamageAt`, `FindWeaponByName`
- game (1): `StepGafSequence`
- los_radar (1): `GetMapCell`
- 1 smaller targets: units (1)

**Sim writes.** 70 items in all, 44 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Game::features` (mixed, 24 fn); `Cell::feature` (mixed, 15 fn, also written by los_radar); `Game::featureCount` (mixed, 13 fn); `Cell::flags` (mixed, 10 fn, also written by map); `Pool::entries` (mixed, 8 fn); `Cell::spot` (mixed, 6 fn); `Spot::feature` (mixed, 6 fn); `Cell::offsetX` (mixed, 5 fn); `Cell::offsetY` (mixed, 5 fn); `Pool::usedHead` (mixed, 5 fn); `Pool::freeHead` (mixed, 4 fn); `Spot::used` (mixed, 4 fn)

**Sim reads.** 73 items only read, 67 named. The most used (`g_game`, the root pointer, left out):

`Game::mapWidthTiles` (sim, 8 fn); `Feature::name` (sim, 6 fn); `Game::heightMap` (mixed, 6 fn); `DAT_005119b8` (sim, 5 fn); `Feature::footprint` (sim, 5 fn); `Game::mapHeightTiles` (sim, 5 fn); `Game::spots` (sim, 5 fn); `Feature::flags` (sim, 4 fn); `Spot::anim` (sim, 4 fn); `Feature::flags16` (sim, 3 fn); `Feature::noobject` (sim, 3 fn); `Game::mapInfo` (mixed, 3 fn)

**Draft interface.**

- Exports: 23 functions (`UpdateFeatures`, the cell accessor `GetCellFeature`, `PlaceFeature` (los_radar, units), `DamageFeature` (weapons, network), `ReclaimFeature` (orders), and the load, save, pool and file-list calls from game and los_radar).
- Needs from seams: 46 functions in platform, presentation, network, map, weapons, game, los_radar, units.
- Owns: 38 named sim items that only this subsystem writes, for example `Game::features`, `Game::featureCount`, `Pool::entries`, `Cell::spot`, `Spot::feature`.
- Shares: 6 named sim items that other subsystems also write, for example `Cell::feature`, `Cell::flags`, `FeatureDamagePacket::sub`, `FeatureDamagePacket::type`, `FeatureDamagePacket::x`.
- It has 9 imports from presentation (`EmitWhiteSmoke`, `PlaySoundAtByName`, the GAF loaders, `CreateObjectState`). Sim code that draws or plays sound has to raise events instead (#22).
- Features and weapons call each other: features calls `ApplyAreaDamageAt`, weapons calls `DamageFeature`. `Cell::feature` is also written by los_radar (`ClearFeature` is in `line_of_sight.cpp`, so it is los_radar by file).

**Open questions.**

- `ClearFeature` is a feature operation in a LOS file. Should it move to features, which would remove the second writer of `Cell::feature`?
- `Cell::spot`, `Cell::offsetX` and `Cell::offsetY` are written here and read by los_radar, map, orders and weapons. Are they feature state or part of the cell record that map owns?

## ai

Issue #13. 160 functions. The computer player: `UpdatePlayerAI`, build ratings, weight scripts and squads (`src/ai/ai_player*.cpp`, `player_ai.cpp`, `ai_profile.cpp`, most of `src/units/squads.cpp`).

**Entry points.** Phase roots: none. `UpdatePlayerAI` and `SquadManager::TickIfActive` are called by `UpdatePlayers` (players), so the AI runs inside the players phase.

32 functions are called from other subsystems. The most called, with their callers:

- `GetFactoriesInRadius` (9 from orders)
- `operator-` (5 from orders)
- `SetUnitSquad` (3 from units, 1 from presentation)
- `GetVisibleEnemiesInRadius` (2 from orders, 1 from targeting)
- `IsPointVisible` (2 from los_radar, 1 from presentation)
- `SquadManager::MarkOwnerNetDirtyFromDamageSplit` (3 from weapons)
- `Bits::Test` (1 from targeting)
- By calling subsystem (functions called): players 10, orders 5, platform 5, presentation 3, game 2, los_radar 2

**Imports.** 48 functions of 11 subsystems, by target, each with the number of functions it calls there:

- platform (13): `RandomInt`, `operator new`, `CommandArgs::GetArg`, `CommandArgs::InitArgs`, and 9 more
- orders (6): `AddOrder`, `GetOrderType`, `operator+`, `DeleteOrders`, and 2 more
- players (5): `GetNetEnergy`, `GetEnergyIncome`, `GetMetalIncome`, `GetNetMetal`, and 1 more
- targeting (5): `GetWeaponTargetUnit`, `SetWeaponTargetUnit`, `ClearWeaponTarget`, `FindWeaponTarget`, and 1 more
- map (4): `Mission::GetNameSlot`, `CanBuildAt`, `CanPlaceUnitFootprint`, `Mission::GetGameType`
- units (4): `Unit::SetStateBits`, `UnitTypeSet::AddTypeOrCategory`, `NotifyUnitRefs`, `RemoveFast`
- presentation (3): `GetBuildSiteMetal`, `QueueBuildOrder`, `QueueUnitSpeechIfNotVisible`
- 4 smaller targets: weapons (3), los_radar (2), pathfinding (2), economy (1)

**Sim writes.** 25 items in all, 15 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`g_playerAI` (sim, 22 fn); `Unit::flags` (mixed, 10 fn, also written by game, los_radar, map, orders, players, units, weapons); `SquadManager::timers` (sim, 8 fn); `SquadTimer::next` (sim, 7 fn); `SquadManager::player` (sim, 5 fn); `Unit::group` (mixed, 4 fn, also written by orders, units); `g_aiCommandsEnabled` (sim, 4 fn); `PlayerAI::lastTick` (sim, 3 fn); `PlayerView::squads` (mixed, 3 fn); `SquadManager::countdown` (sim, 3 fn); `PlayerAI::centre` (sim, 2 fn); `PlayerAI::hasSpecial` (sim, 2 fn)

**Sim reads.** 124 items only read, 101 named. The most used (`g_game`, the root pointer, left out):

`Game::gameTick` (mixed, 11 fn); `Game::unitDefs` (mixed, 11 fn); `Game::unitDefCount` (mixed, 9 fn); `Unit::def` (mixed, 8 fn); `Game::mapInfo` (mixed, 6 fn); `Player::index` (mixed, 6 fn); `Game::mapWidthWorld` (sim, 5 fn); `Player::active` (mixed, 5 fn); `Player::type` (mixed, 5 fn); `Unit::unitDefIndex` (mixed, 5 fn); `UnitDef::extractsMetal` (sim, 5 fn); `UnitDef::flags1` (mixed, 5 fn)

**Draft interface.**

- Exports: 32 functions (`UpdatePlayerAI`, `SquadManager::TickIfActive`, the create, reset and destroy calls from game, `SetUnitSquad` (units), `SquadManager::MarkOwnerNetDirtyFromDamageSplit` (weapons) and helpers other subsystems borrow: `GetFactoriesInRadius` (orders), `GetVisibleEnemiesInRadius` (orders, targeting), `IsPointVisible` (los_radar)).
- Needs from seams: 48 functions in platform, orders, players, targeting, map, units, presentation, weapons, los_radar, pathfinding, economy.
- Owns: 13 named sim items that only this subsystem writes, for example `g_playerAI`, `SquadManager::timers`, `SquadTimer::next`, `SquadManager::player`, `g_aiCommandsEnabled`.
- Shares: 2 named sim items that other subsystems also write, for example `Unit::flags`, `Unit::group`.
- It acts through other modules: orders (`AddOrder`, `IssueAttackOrder`, `DeleteOrders`), targeting (5 calls) and `Unit::SetStateBits`. Its own sim writes are few.
- Its read set is the widest of the nine (124 items: `Unit`, `UnitDef`, `Player`, `Game`). A read-only view of the state block fits it better than a getter per field.

**Open questions.**

- The AI calls the pathfinder's open-list heap (`MakeHeap`, `PopHeapFirst`). Is that heap a pathfinding export or shared library code?
- `GetFactoriesInRadius` (9 calls) and `operator-` (5) are used by orders. Do these helpers belong in a shared maths or query seam instead of ai?

## pathfinding

Issue #14. 59 functions. The search in `src/ai/pathfind*.cpp`: per-player cost tables, the node budget, the open heap and the grid.

**Entry points.** Phase roots: none. `Pathfinder::RunSearches` is called by `UpdatePlayers` (players).

4 functions are called from other subsystems. The most called, with their callers:

- `Pathfinder::AbortIfGoalMatch` (2 from orders)
- `PopHeapFirst` (2 from ai)
- `MakeHeap` (1 from ai)
- `Pathfinder::RunSearches` (1 from players)
- By calling subsystem (functions called): ai 2, orders 1, players 1

**Imports.** 24 functions of 3 subsystems, by target, each with the number of functions it calls there:

- orders (16): `AiSearchGoal::SetWaypoints`, `ApproachRadius::ApproxDistExcess`, `OrderFx::ApproxDist`, `PointMarker::ApproxDist`, and 12 more
- platform (4): `operator delete`, `operator new`, `GameAllocIgnoreTag`, `GameFreeThunk`
- units (4): `MovementClass::RefreshUnitIfStale`, `MovementClass::Get`, `MovementClass::InBounds`, `MovementClass::RefreshMovedUnits`

**Sim writes.** 46 items in all, 42 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`OpenHeap::items` (sim, 13 fn); `OpenHeap::pool` (sim, 12 fn); `Grid::cells` (mixed, 11 fn, also written by los_radar, map); `Node::index` (sim, 11 fn); `OpenHeap::count` (sim, 11 fn); `Grid::width` (mixed, 9 fn, also written by los_radar, map); `Grid::dirty` (sim, 8 fn); `PathCell::flags` (sim, 8 fn); `OpenHeap::topPopped` (sim, 7 fn); `OpenHeap::freeHead` (sim, 6 fn); `Pathfinder::costScale` (sim, 6 fn); `Pathfinder::object` (mixed, 6 fn)

**Sim reads.** 28 items only read, 25 named. The most used (`g_game`, the root pointer, left out):

`Pathfinder::grid` (sim, 6 fn); `g_dirDeltaX` (sim, 3 fn); `g_dirDeltaZ` (sim, 3 fn); `Game::mapHeightTiles` (sim, 2 fn); `Game::mapWidthTiles` (sim, 2 fn); `Game::players` (sim, 2 fn); `MovementClass::footprintX` (mixed, 2 fn); `MovementClass::footprintZ` (mixed, 2 fn); `NodeData::pos` (sim, 2 fn); `Unit::motion` (mixed, 2 fn); `g_turnCosts` (sim, 2 fn); `AiSearchGoal::target` (sim, 1 fn)

**Draft interface.**

- Exports: 4 functions (`Pathfinder::RunSearches` (players), `Pathfinder::AbortIfGoalMatch` (orders), `MakeHeap` and `PopHeapFirst` (ai)).
- Needs from seams: 24 functions in orders, platform, units.
- Owns: 38 named sim items that only this subsystem writes, for example `OpenHeap::items`, `OpenHeap::pool`, `Node::index`, `OpenHeap::count`, `Grid::dirty`.
- Shares: 4 named sim items that other subsystems also write, for example `Grid::cells`, `Grid::width`, `Grid::count`, `Grid::height`.
- Only 4 functions are called from outside, so the entry side is small. The import side is not: 16 of its 24 imports are goal-object methods (`PathGoal`, `PointMarker`, `RingApproach`, `OrderFx`, `ApproachRadius`, `AiSearchGoal`) that orders implements. The goals are a callback interface that orders passes in.
- The budget (`Pathfinder::budget`, `g_playerBudgetCap`, `g_budgetRefreshCounter`, `g_playerTickLoad`) is state this module writes alone: the limit #14 wants the module to read from outside.

**Open questions.**

- `Grid::cells`, `width` and `height` show as written by pathfinding, los_radar and map. Either `Grid` is one name for unrelated structs (type-wide evidence) or there is a real shared grid; a clang layout run would tell.
- The goals live in `order_targets.cpp` (orders) but only pathfinding calls `ApproxDist` and `ContainsCell`. Should the goal interface be declared by pathfinding?

## weapons

Issue #15. 118 functions. `UpdateProjectiles`, `UpdateExplosions`, `UpdateUnitWeapons`, ballistics, damage and the weapon definitions (`src/weapons/*.cpp`).

**Entry points.** Phase roots: `UpdateProjectiles` and `UpdateExplosions`, called from `RunGameSteps`. `UpdateUnitWeapons` is not a root: `UpdateAllUnits` (units) calls it.

33 functions are called from other subsystems. The most called, with their callers:

- `Unit::ClaimWeapons` (32 from orders)
- `Unit::ReleaseWeapons` (17 from orders, 1 from units)
- `DamageUnit` (4 from orders, 4 from units, 2 from players)
- `WeaponCanReachUnit` (7 from orders, 1 from ai, 1 from targeting)
- `FindFeatureAtPos` (3 from orders, 1 from players)
- `FindWeaponByName` (1 from features, 1 from units, 1 from wind_meteors)
- `GetWeaponRange` (2 from orders, 1 from targeting)
- By calling subsystem (functions called): units 12, game 8, orders 7, ai 3, network 3, players 3

**Imports.** 69 functions of 13 subsystems, by target, each with the number of functions it calls there:

- platform (25): `RandomInt`, `operator new`, `BuildDataPath`, `GameAllocIgnoreTag`, and 21 more
- presentation (16): `EmitWeaponSmoke`, `PlaySoundAt`, `EmitWhiteSmoke`, `AccumulateScreenShake`, and 12 more
- units (10): `CobScript::StartScriptWithArgs`, `GetAimFromPosition`, `CobScript::StartScript`, `CobScript::FindScript`, and 6 more
- network (3): `BroadcastPacket`, `GetLocalDpid`, `SendScriptCallByName`
- ai (2): `SquadManager::MarkOwnerNetDirtyFromDamageSplit`, `ReactToAttack`
- features (2): `DamageFeature`, `GetFootprintCentre`
- harness (2): `HarnessPoseObject`, `HarnessTestMode`
- 6 smaller targets: los_radar (2), orders (2), targeting (2), economy (1), game (1), map (1)

**Sim writes.** 297 items in all, 141 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Game::projectiles` (mixed, 18 fn); `WeaponDef::index` (sim, 11 fn); `Game::projectileCount` (mixed, 9 fn); `WeaponDef::speed` (sim, 9 fn); `WeaponFirePacket::aimOrVel` (sim, 9 fn); `WeaponFirePacket::weaponDefIndex` (sim, 8 fn); `Game::projCount` (sim, 7 fn); `WeaponFirePacket::origin` (sim, 7 fn); `WeaponFirePacket::targetUnitId` (sim, 7 fn); `WeaponFirePacket::type` (sim, 7 fn); `WeaponDef::holdTime` (sim, 6 fn); `WeaponFirePacket::fireHeading` (sim, 6 fn)

**Sim reads.** 243 items only read, 118 named. The most used (`g_game`, the root pointer, left out):

`Game::seaLevel` (mixed, 9 fn); `Unit::player` (sim, 9 fn); `Unit::heading` (sim, 8 fn); `Unit::utype` (sim, 8 fn); `Game::gameTick` (mixed, 7 fn); `Game::gravity` (sim, 7 fn); `Unit::playerIndex` (sim, 6 fn); `Game::mapInfo` (mixed, 5 fn); `Unit::script` (mixed, 5 fn); `Game::netGameFlagsWord` (mixed, 4 fn); `Game::units` (mixed, 4 fn); `Game::weaponDefs` (sim, 4 fn)

**Draft interface.**

- Exports: 33 functions (the two roots, `UpdateUnitWeapons`, `Unit::ClaimWeapons` and `Unit::ReleaseWeapons` (orders), `DamageUnit` (orders, players, units), `WeaponCanReachUnit`, `WeaponCanReachPos`, `FindWeaponByName`, `SpawnProjectile` and the net packet handlers `ApplyWeaponFirePacket` and `ApplyProjectileHitPacket`).
- Needs from seams: 69 functions in platform, presentation, units, network, ai, features, harness, los_radar, orders, targeting, economy, game, map.
- Owns: 125 named sim items that only this subsystem writes, for example `Game::projectiles`, `WeaponDef::index`, `Game::projectileCount`, `WeaponDef::speed`, `WeaponFirePacket::aimOrVel`.
- Shares: 16 named sim items that other subsystems also write, for example `Game::cameraSnapTicks`, `Unit::flags`, `g_explodedPieces`, `Game::count`, `Game::cameraFollowTrackObj`.
- The largest import list of the nine: 69 functions in 13 subsystems, 16 of them presentation (smoke, sound, screen shake, GAF and model loading) and 10 units (COB scripts, piece positions).
- Most of the 297 writes are not shared state: dozens of `WeaponDef::*` fields written when the TDFs load, packet buffers (`WeaponFirePacket`, `UnitDamagePacketOut`) and per-file struct copies. The shared state is `Game::projectiles`, `Game::projectileCount`, `Unit::health`, `Unit::flags` and the weapon slot fields.
- It also writes view state: `Game::cameraSnapPos` and `cameraSnapTicks` (from `DetonateProjectile` and `CheckProjectileCollision`).

**Open questions.**

- `DamageUnit` writes `Unit::health`, which game, orders, players and units also write. Is damage a weapons export or a units operation that weapons requests? #15 already asks which machine applies damage in multiplayer.
- `Unit::ClaimWeapons` has 32 callers, all in orders. It is a lifecycle call, not a per-tick one; does it belong to a unit lifecycle seam?

## targeting

Issue #16. 10 functions. Target choice and the weapon target setters and getters (`FindWeaponTarget`, `FindBestTargetIfFireAtWill`, `SetWeaponTargetUnit`). It is small because the file rules put the order goal objects in orders; see the notes.

**Entry points.** Phase roots: none. Order handlers (orders) and the AI call it.

9 functions are called from other subsystems. The most called, with their callers:

- `SetWeaponTargetUnit` (8 from orders, 2 from ai)
- `ClearWeaponTarget` (5 from orders, 1 from ai)
- `FindBestTargetIfFireAtWill` (6 from orders)
- `GetWeaponTargetUnit` (3 from orders, 2 from ai, 1 from weapons)
- `SetWeaponTargetPos` (3 from orders, 1 from ai)
- `FindWeaponTarget` (1 from ai)
- `GetWeaponTargetPos` (1 from weapons)
- By calling subsystem (functions called): orders 6, ai 5, weapons 2, units 1

**Imports.** 11 functions of 5 subsystems, by target, each with the number of functions it calls there:

- units (5): `CobScript::FindScript`, `CobScript::StartScriptWithArgs`, `GetGroundHeight`, `GetSweetSpot`, and 1 more
- ai (2): `Bits::Test`, `GetVisibleEnemiesInRadius`
- weapons (2): `GetWeaponRange`, `WeaponCanReachUnit`
- platform (1): `RandomInt`
- presentation (1): `Class_004800c0::EraseSwapBack`

**Sim writes.** 5 items in all, 0 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

none

**Sim reads.** 33 items only read, 25 named. The most used (`g_game`, the root pointer, left out):

`Game::units` (mixed, 2 fn); `Unit::id` (sim, 2 fn); `Unit::script` (mixed, 2 fn); `Game::matchFlags` (sim, 1 fn); `Game::seaLevel` (mixed, 1 fn); `Player::active` (mixed, 1 fn); `Player::type` (mixed, 1 fn); `Unit::def` (mixed, 1 fn); `Unit::flags` (mixed, 1 fn); `Unit::killCount` (mixed, 1 fn); `Unit::mode20` (sim, 1 fn); `Unit::motion` (mixed, 1 fn)

**Draft interface.**

- Exports: 9 functions (all of them are target setters, getters and searches; `SetWeaponTargetUnit` (10 callers), `GetWeaponTargetUnit` (6), `ClearWeaponTarget` (6) and `FindBestTargetIfFireAtWill` (6) are the busiest).
- Needs from seams: 11 functions in units, ai, weapons, platform, presentation.
- Owns: no named sim item that only this subsystem writes.
- Shares: no named sim item that another subsystem also writes.
- It has no strong write to a named sim item. Its 5 written items are `Vec3` and `Point` value types. The target itself is held in the weapon slot (`UnitWeaponSlot`), and the setters probably write it through a pointer, which counts as a read.
- It imports the enemy search from ai (`GetVisibleEnemiesInRadius`, `Bits::Test`) and the range checks from weapons (`GetWeaponRange`, `WeaponCanReachUnit`). Its 5 units imports are COB script calls, `GetGroundHeight` and `GetSweetSpot`.

**Open questions.**

- With no state of its own, is targeting a set of functions over the weapon slot, and so part of weapons, as #16 allows?
- `unit_targets.cpp` is one file in 3.1c, split here by name into units 11, targeting 8, orders 3 and weapons 3. The split is the rule's guess and needs a decision in #16.

## los_radar

Issue #17. 118 functions. Line of sight, radar and sonar, the LOS tables, the map cell accessor and the TNT loader (`src/map/line_of_sight*.cpp`, `los_tables*.cpp`, `radar.cpp`, and two `src/game/players_46*.cpp` files).

**Entry points.** Phase roots: none. `UpdatePlayers` (players) calls `RecalculateLineOfSight`, `UpdateUnitLineOfSight` and `UpdateSensorRadarAndCloak`, so LOS runs inside the players phase.

42 functions are called from other subsystems. The most called, with their callers:

- `GetMapCell` (13 from features, 2 from weapons, 1 from ai, 1 from economy, 1 from game, 1 from map, 1 from network, 1 from presentation, 1 from units)
- `GetMapCellAtPosition` (4 from orders, 3 from weapons, 1 from map, 1 from presentation)
- `IsUnitVisibleToPlayer` (4 from presentation, 1 from ai, 1 from units)
- `RecalculateLineOfSight` (3 from game, 1 from players, 1 from presentation)
- `UpdateUnitLineOfSight` (2 from players, 2 from units, 1 from orders)
- `StringRef::Assign` (2 from platform, 1 from map, 1 from units)
- `ClampWorldPosToTerrain` (2 from game, 1 from players)
- By calling subsystem (functions called): units 12, platform 9, game 8, presentation 8, map 7, players 6

**Imports.** 47 functions of 8 subsystems, by target, each with the number of functions it calls there:

- presentation (18): `GetGafFrame`, `DrawFrame`, `StringRef::ReleaseRef`, `AllocFrame`, and 14 more
- platform (16): `operator delete`, `GameFreeThunk`, `GameAllocIgnoreTag`, `TdfFile::SelectRecord`, and 12 more
- features (5): `FreeFeaturePool`, `InitFeatureAnimPool`, `PlaceFeature`, `PlaceMissionFeatures`, and 1 more
- ai (2): `IsPointVisible`, `HasReadyUnitInRange`
- map (2): `Mission::GetNameSlot`, `VisitObjectsInRange`
- network (2): `AddMessage`, `BroadcastPlayerInfo`
- game (1): `SaveSettings`
- 1 smaller targets: units (1)

**Sim writes.** 118 items in all, 103 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`SightQuery::sightDistance` (mixed, 11 fn); `Game::heightMap` (mixed, 9 fn); `SightQuery::cacheCell` (mixed, 9 fn); `SightQuery::frameIdx` (mixed, 9 fn); `Game::visibilityMask` (mixed, 8 fn); `Grid::height` (mixed, 7 fn, also written by map, pathfinding); `Grid::width` (mixed, 7 fn, also written by map, pathfinding); `SightQuery::player` (mixed, 7 fn); `GafFrame::xOffset` (mixed, 6 fn, also written by presentation, weapons); `GafFrame::yOffset` (mixed, 6 fn, also written by presentation, weapons); `Game::eyes` (sim, 6 fn); `Grid::cells` (mixed, 6 fn, also written by map, pathfinding)

**Sim reads.** 119 items only read, 78 named. The most used (`g_game`, the root pointer, left out):

`Game::mapWidthTiles` (sim, 16 fn); `Game::mapHeightTiles` (sim, 15 fn); `Game::playerIndex` (mixed, 14 fn); `Game::seaLevel` (mixed, 10 fn); `Game::losTable` (sim, 8 fn); `Unit::def` (mixed, 8 fn); `FrameTable::count` (sim, 6 fn); `Game::players` (sim, 6 fn); `Player::info` (mixed, 6 fn); `Unit::player` (sim, 6 fn); `Game::gameTick` (mixed, 5 fn); `Game::localPlayer` (mixed, 5 fn)

**Draft interface.**

- Exports: 42 functions (the three players-phase calls, `GetMapCell` and `GetMapCellAtPosition`, `IsUnitVisibleToPlayer`, `RevealNewUnit`, `ShareMapping` and `ShareRadar` (economy), and the load, free and console calls).
- Needs from seams: 47 functions in presentation, platform, features, ai, map, network, game, units.
- Owns: 85 named sim items that only this subsystem writes, for example `SightQuery::sightDistance`, `Game::heightMap`, `SightQuery::cacheCell`, `SightQuery::frameIdx`, `Game::visibilityMask`.
- Shares: 18 named sim items that other subsystems also write, for example `Grid::height`, `Grid::width`, `GafFrame::xOffset`, `GafFrame::yOffset`, `Grid::cells`.
- 18 imports are presentation (`AllocSurface`, `DrawCircle`, `DrawFrame`, `DownsampleFrame`) and 16 platform. The file rules put the radar picture and minimap code here (`DrawRadarUnits`, `ResizeRadarPicture`, the `Game::minimapGadget*` writes), so most of the 118 functions are likely view code. This is the boundary #17 asks to keep clear.
- The result is `Game::visibilityMask`, which ai, economy, map, orders, pathfinding and presentation read. `Player::explored` is read here and written elsewhere.

**Open questions.**

- `GetMapCell` has 22 callers in 9 subsystems. It is the map cell accessor, not a LOS function. Should map own it, with los_radar importing it?
- `Game::visibilityMask` is read by pathfinding and ai as well as presentation. Which part is sim state and which is the player view (#22)?

## orders

Issue #18. 468 functions. The order state machines, VTOL orders, order lists, unit motion (`UnitMotion`) and the order goal objects (`src/orders/*.cpp`, 14,000 lines). The largest subsystem.

**Entry points.** Phase roots: none. `UpdateAllUnits` (units) calls `RunOrders`; units also call `UnitMotion::UpdateMotion`.

62 functions are called from other subsystems. The most called, with their callers:

- `CopyDwordIfNonNull` (5 from platform)
- `GetOrderType` (3 from ai, 1 from game, 1 from presentation)
- `operator+` (2 from ai, 2 from weapons, 1 from presentation)
- `AddOrder` (3 from ai, 1 from game)
- `Order::OrStatusFlags` (4 from units)
- `AiSearchGoal::SetWaypoints` (3 from pathfinding)
- `ApproachRadius::ApproxDistExcess` (3 from pathfinding)
- By calling subsystem (functions called): units 21, pathfinding 16, presentation 13, ai 6, game 6, map 3

**Imports.** 86 functions of 13 subsystems, by target, each with the number of functions it calls there:

- units (26): `GetHeadingBetween`, `Unit::SetStateBits`, `AttachUnitToPiece`, `GetNanoPiecePosition`, and 22 more
- platform (11): `operator new`, `RandomInt`, `operator delete`, `HapiBank::OpenNamedBox`, and 7 more
- map (9): `CanPlaceUnitFootprint`, `VisitObjectsInRange`, `IsPadSlotFree`, `PickRandomReclaimableResourcesInRadius`, and 5 more
- network (7): `BitReader::ReadBits`, `BitWriter::WriteBits`, `SendScriptCallNoArgs`, `BitWriter::GrowBuffer`, and 3 more
- weapons (7): `Unit::ClaimWeapons`, `Unit::ReleaseWeapons`, `WeaponCanReachUnit`, `DamageUnit`, and 3 more
- presentation (6): `QueueUnitSpeech`, `EmitNanoParticles`, `MarkSelectionOrdersDirty`, `EmitReverseNanoParticles`, and 2 more
- targeting (6): `SetWeaponTargetUnit`, `FindBestTargetIfFireAtWill`, `ClearWeaponTarget`, `GetWeaponTargetUnit`, and 2 more
- 6 smaller targets: ai (5), features (3), los_radar (3), economy (1), pathfinding (1), players (1)

**Sim writes.** 149 items in all, 114 after leaving out the shared value types (Vec3, Point, Fixed) and per-file copies of a struct (names ending in an address). The most used:

`Order::flags` (mixed, 95 fn, also written by presentation); `Unit::pos` (mixed, 66 fn, also written by presentation, units); `Order::state` (sim, 52 fn); `Order::pos` (sim, 39 fn); `Order::next` (mixed, 37 fn); `PathOrderAttach::owner` (mixed, 30 fn); `Unit::flags` (mixed, 25 fn, also written by ai, game, los_radar, map, players, units, weapons); `Unit::health` (mixed, 20 fn, also written by game, players, units, weapons); `Unit::buildLeft` (mixed, 15 fn, also written by players, units); `Angles16::heading` (mixed, 12 fn, also written by game, presentation, units); `Order::unit` (mixed, 12 fn); `PathGoal::owner` (sim, 11 fn)

**Sim reads.** 300 items only read, 193 named. The most used (`g_game`, the root pointer, left out):

`Unit::def` (mixed, 84 fn); `Unit::motion` (mixed, 49 fn); `Unit::list` (sim, 31 fn); `Unit::player` (sim, 23 fn); `UnitDef::altitude` (sim, 23 fn); `Order::target` (sim, 22 fn); `Unit::carrier` (mixed, 22 fn); `Unit::list2` (sim, 22 fn); `UnitDef::maxHealth` (sim, 20 fn); `UnitDef::flags` (sim, 18 fn); `g_missionOrderTableBegin` (sim, 16 fn); `UnitMotion::flags` (sim, 14 fn)

**Draft interface.**

- Exports: 62 functions (`RunOrders`, `RunSecondaryOrders`, `AddOrder`, `DeleteOrders`, `GetOrderType`, `IssueAttackOrder`, `OrderType::GetTableEntry` (presentation), the `UnitMotion::` functions (13 in orders, 4 called from outside: `UpdateMotion`, `LoadMotion`, `SaveMotion`, `DestroyObject`), and the goal methods pathfinding calls).
- Needs from seams: 86 functions in units, platform, map, network, weapons, presentation, targeting, ai, features, los_radar, economy, pathfinding, players.
- Owns: 87 named sim items that only this subsystem writes, for example `Order::state`, `Order::pos`, `Order::next`, `PathOrderAttach::owner`, `Order::unit`.
- Shares: 27 named sim items that other subsystems also write, for example `Order::flags`, `Unit::pos`, `Unit::flags`, `Unit::health`, `Unit::buildLeft`.
- The units imports (26) are the unit lifecycle and geometry: `CreateUnit`, `SetUnitPosition`, `Unit::SetStateBits`, COB script calls and piece positions. Weapons (7) is mostly `Unit::ClaimWeapons` and `Unit::ReleaseWeapons`.
- It is the heaviest writer of core unit fields: `Unit::pos` from 66 functions and `Unit::flags` from 25. The fields are also written by units and presentation (`pos`), and by seven other subsystems (`flags`).

**Open questions.**

- Which side owns `Unit::pos`, `flags`, `health` and `buildLeft` in the state block (#7): units, with orders going through its seam, or orders? This decides whether #18 can split.
- The split #18 allows would follow the `UnitMotion::` functions (13 in orders). Is motion a module of its own?

## The remaining subsystems

Each has no module issue yet. Entry points and couplings come from the same tool (`uv run tools/coupling.py --summary NAME`); the numbers count functions.

**units** (193 functions). Phase root `UpdateAllUnits`, which calls `RunOrders` (orders) and `UpdateUnitWeapons` (weapons). 68 functions are called from outside; the busiest are `GetHeadingBetween` (28), `Unit::SetStateBits` (24), `GetGroundHeight` (20). It is called most by orders (26 distinct functions) and game (24). It imports 179 functions, mostly platform (56), presentation (32), orders (21) and network (17). It holds the unit table and lifecycle code (`CreateUnit`, `Unit::SetStateBits`, COB scripts), and it is the other side of most of orders' writes.

**players** (149 functions). Phase root `UpdatePlayers`, which calls into economy (`UpdatePlayerEconomy`), ai (`UpdatePlayerAI`, squads), pathfinding (`RunSearches`) and los_radar (three LOS calls) in turn. 41 functions are called from outside, mostly by game (17) and presentation (14); the busiest are `KillPlayerUnits` (5), `ShutdownScoreTables` (4), `TransferEnergy` (4). It imports 77 functions: platform 23, network 12, ai 10, presentation 9, los_radar 6. It holds the per-player state and victory conditions.

**map** (109 functions). No phase root. 51 functions are called from outside, by presentation (24), game (22), units (12) and orders (9); the busiest are `Mission::GetGameType` (45), `Mission::LoadMissionByName` (15), `Mission::GetMissionName` (12) (mission accessors) and `CanPlaceUnitFootprint` (8). It imports 62 functions: platform 36, los_radar 7, wind_meteors 4. It holds the terrain, the pass maps and footprints.

**network** (450 functions). Phase root `HandleNetPackets`, which dispatches into every subsystem, so what it reaches says nothing about the owner. 103 functions are called from outside, mostly by presentation (65) and game (32); the busiest are `BroadcastPlayerInfo` (26), `BroadcastPacket` (23), `AddMessage` (18). It imports 86 functions: presentation 26, platform 21, players 11, units 10. Its packet handlers write many sim items (250 in all).

**game** (160 functions). Phase root `StepAllGafSequences`. Start-up, load, save and shutdown code, and the main loop: `MainLoopTick`, `RunGameSteps` (which calls every phase root), `UpdateFramePacing`, `BattleFrame`, `EndGameFrame`, `MainFrameTick`, `RunEndGameState` and `UpdateBattleRoom` (the last two pump `HandleNetPackets`) are set to game by `override`, because what drives the step is not presentation. 92 functions are called from outside, by platform (67 distinct functions) and presentation (24); the busiest are `SetCursorMode` (19), `SaveSettings` (9), `SavedGameSelectHandler` (7). It imports 444 functions, the most of any subsystem, 215 of them from presentation, then platform 88, network 32, units 24 and map 22.

**harness** (58 functions). The test harness of #3 (`src/harness`). No phase root. 10 functions are called from outside; the busiest are `HarnessViewRand` (20), `HarnessTestMode` (4), `HarnessCrtSeed` (2). It imports 11 functions (presentation 5, map 3). The sim calls it too: weapons imports `HarnessPoseObject` and `HarnessTestMode`.

**presentation** (1514 functions). Rendering, UI and sound (`src/graphics`, `src/gui`, `src/frontend`, `src/ingame`, `src/sound`), except what a phase walk reaches, the main loop and frame handlers (game), and the timer, window and environment code of `src/graphics/display.cpp` (platform). No phase root, and none of its functions calls one (a test checks this). 310 functions are called from outside, mostly by game (215 distinct functions); the busiest are `QueueUnitSpeech` (27), `PlaySoundByName` (18), `GetDisplay` (17). It imports 247 functions: platform 85, network 65, game 24, map 24, players 14. The reverse direction is the one to remove: 53 imports from presentation in the nine module subsystems, the largest in los_radar (18), weapons (16) and features (9).

**platform** (838 functions). Devices, files, memory, the TDF and `HapiBank` readers, the console and the C library, and the timers (`GetTicks`, `GetMilliseconds`), window and environment code of `src/graphics/display.cpp` (24 functions, set by `override`). No phase root. 178 functions are called from outside, by game (88 distinct functions), presentation (85), units (56) and map (36); the busiest are `operator new` (138), `GameFreeThunk` (134), `operator delete` (104), then `Translate` (84) and `RandomInt` (60). It imports 138 functions: game 67, presentation 44.

## Notes from the data

- **`order_targets.cpp` is in orders.** It holds the order goal objects (`PathGoal`, `PointMarker`, `RingApproach` and the rest: 100 functions of the file are in orders), so the file rule puts it in `orders`, while #16 lists it for targeting. Targeting is then 10 functions, 8 of them from `unit_targets.cpp`. The pathfinding section shows the same line from the other side: the goals are what pathfinding calls into.
- **`movement_class.cpp` is in units and map.** Its functions are split between `units` (14) and `map` (7, the pass maps) and one in `features`, while #14 lists the file for pathfinding. Pathfinding imports four `MovementClass` functions from units.
- **`unit_targets.cpp` is split four ways** by the name rules: units 11, targeting 8, orders 3, weapons 3.
- **Phases cut across subsystems.** The phase roots are in wind_meteors, economy, features, weapons, units, players and game, and none is in ai, pathfinding, los_radar or orders. `UpdatePlayers` runs economy, ai, pathfinding and los_radar work, and `UpdateAllUnits` runs orders and weapons work. A module's entry point is often called from another subsystem's phase, not from the loop.
- **`g_game` is left out of the read lists.** Every subsystem reads it as the root pointer; the fields behind it are the items listed.

Limits of the data:

- `Game::` fields are keyed by name, not by offset. Each source file declares its own partial view of `struct Game`, so two names for one offset are two items, and two subsystems can share bytes the tables show as separate. A clang layout run would fix this.
- Only strong writes count as writes. An address taken or a pointer passed to a non-const parameter counts as a read, so a setter that writes through a pointer (targeting's) shows as a reader, and the callee's own rows show the write.
- Shared value types (`Vec3`, `Point`, `Fixed`) and unrelated structs with one name (`Grid`, `Pos`) give every subsystem that touches them a row. They are left out of the named lists but still counted in the totals.
- Calls through a function pointer or a virtual method appear only where `data/hazard_edges.csv` resolves them by hand.
- Destructors are missing from the call data until #133, so the cleanup calls of a subsystem are undercounted.
- A function in a mixed folder (`src/game`, `src/map`) is assigned by file, then by phase walk, then by name. The overrides in `data/subsystems.csv` (72) fix the known misses; a wrong assignment shows as a surprising coupling above.

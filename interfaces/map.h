#ifndef INTERFACES_MAP_H
#define INTERFACES_MAP_H

struct Vec3;
struct Point16;
struct Unit;
struct UnitDef;
struct Los_0047d2e0;
class Mission;
class HapiBank;
class MovementClass;
class SpatialBucket;

// Major version of the map interface, changed when a slot is removed or its signature changes.
#define MAP_VERSION_MAJOR 2
// Minor version of the map interface, changed when a slot is added.
#define MAP_VERSION_MINOR 1

// Visits each unit VisitObjectsInRange finds. Implementations keep the vtable pointer first and this method in the first slot.
class UnitVisitor {
public:
    // Called once for each unit within range. Hooks: none.
    virtual void Visit(Unit* unit) = 0;
};

// Terrain and map data.
struct MapSeam {
    // Returns the mean ground height of the cell under a position. Hooks: none.
    int (__stdcall* GetCellMeanHeight)(Vec3* pos);
    // Rebuilds the pass maps of every movement class after the terrain changes between two cells. Hooks: none.
    void (__stdcall* RefreshAllPassMaps)(Point16 a, Point16 b);
    // Returns the game type of a mission (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* GetGameType)(Mission* mission);
    // Returns one of the file names of a mission, by slot number (thiscall in 3.1c, this first). Hooks: none.
    char* (__stdcall* GetNameSlot)(Mission* mission, int index);
    // Calls a visitor on each unit within a range of a position. Hooks: none.
    void (__stdcall* VisitObjectsInRange)(Vec3* pos, int range, UnitVisitor* visitor);
    // Returns 1 when a unit type can be built at a cell, with a visibility check when los is given. Hooks: none.
    int (__stdcall* CanBuildAt)(UnitDef* unit, Point16 cell, short type, Los_0047d2e0* los);
    // Returns 1 when the footprint of a unit type fits at a cell (flags 2 counts a cell off the map as a fit). Hooks: none.
    int (__stdcall* CanPlaceUnitFootprint)(UnitDef* unit, unsigned short otherTypeId, Point16 cell, int flags);
    // Builds the pass map of every movement class for the loaded terrain. Hooks: none.
    void (__stdcall* BuildAllPassMaps)(void);
    // Frees the cache of mission names and the mission object. Hooks: none.
    void (__stdcall* FreeOtaEnumCacheAndMission)(void);
    // Reads the explored-map state of a saved game from a bank. Hooks: none.
    void (__stdcall* LoadMappingData)(HapiBank* file);
    // Reads the metal plot map of a saved game from a bank. Hooks: none.
    void (__stdcall* LoadMetalPlotmap)(HapiBank* file);
    // Reads the player feature plot map of a saved game from a bank. Hooks: none.
    void (__stdcall* LoadPlayerFeaturesPlotmap)(HapiBank* file);
    // Writes the explored-map state of a saved game to a bank. Hooks: none.
    void (__stdcall* SaveMappingData)(HapiBank* file);
    // Writes the metal plot map of a saved game to a bank. Hooks: none.
    void (__stdcall* SaveMetalPlotmap)(HapiBank* file);
    // Writes the player feature plot map of a saved game to a bank. Hooks: none.
    void (__stdcall* SavePlayerFeaturesPlotmap)(HapiBank* file);
    // Sets the kind of mission the game runs (campaign, skirmish or multiplayer). Hooks: none.
    void (__stdcall* SetMissionType)(int type);
    // Moves a position to where the footprint of a unit type sits on the grid. Hooks: none.
    void (__stdcall* SnapWorldPosToFootprint)(UnitDef* unit, Vec3* pos);
    // Puts a unit on the map cells and in the spatial grid at its position. Hooks: none.
    void (__stdcall* AddUnitToMap)(Unit* unit);
    // Takes a unit off the map cells and out of the spatial grid. Hooks: none.
    void (__stdcall* RemoveUnitFromMap)(Unit* unit);
    // Returns 1 when the footprint of a unit fits at a position. Hooks: none.
    int (__stdcall* CanPlaceFootprintAt)(Unit* unit, Vec3* pos);
    // Returns 1 when a slot of a landing pad is free. Hooks: none.
    int (__stdcall* IsPadSlotFree)(Unit* pad, int id);
    // Picks a random energy and a random metal feature to reclaim within a radius, and returns 1 when it found either. Hooks: none.
    int (__stdcall* PickRandomReclaimableResourcesInRadius)(Vec3* pos, int range, Vec3** energy, float* energyAmount, Vec3** metal, float* metalAmount);
    // Clears the cells a unit's footprint covers and unlinks the unit from them. Hooks: none.
    void (__stdcall* ClearFootprintAndUnlink)(Unit* unit);
    // Makes the neighbours of a unit's footprint give up the cells the unit now covers. Hooks: none.
    void (__stdcall* ForceNeighborFootprintReclaim)(Unit* unit);
    // Opens or closes the yard of a unit. Hooks: none.
    void (__stdcall* SetYardOpen)(Unit* unit, int open);
    // Links a unit at the front of a spatial grid bucket (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* SpatialBucketPrependUnit)(SpatialBucket* bucket, int unusedEdx, Unit* unit);
    // Unlinks a unit from a spatial grid bucket (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* SpatialBucketUnlinkUnit)(SpatialBucket* bucket, int unusedEdx, Unit* unit);
    // Rebuilds the part of a movement class's pass map between two cells (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* RefreshPassMap)(MovementClass* movementClass, int unusedEdx, Point16 a, Point16 b);
    // Moves a campaign on to its next mission and returns 1 when there is one (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* AdvanceMission)(Mission* mission);
    // Returns the name of the campaign file of a mission (thiscall in 3.1c: this in ecx). Hooks: none.
    char* (__fastcall* GetCampaignName)(Mission* mission);
    // Returns the index of the current mission of a campaign (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* GetMissionIndex)(Mission* mission);
    // Returns the name of the current mission (thiscall in 3.1c: this in ecx). Hooks: none.
    char* (__fastcall* GetMissionName)(Mission* mission);
    // Writes the start position of a player and returns 1 when the mission has one (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* GetStartPosition)(Mission* mission, int unusedEdx, Vec3* out, int id);
    // Loads a campaign file (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* LoadCampaign)(Mission* mission, int unusedEdx, char* file);
    // Loads a mission by its name and returns 1 when it exists (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* LoadMissionByName)(Mission* mission, int unusedEdx, char* name);
    // Returns 1 when a campaign has a mission of that index (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* MissionExists)(Mission* mission, int unusedEdx, int index);
    // Rebuilds the list of maps, for multiplayer when set (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* RefreshMapList)(Mission* mission, int unusedEdx, int multiplayer);
    // Makes the mission of that index of a campaign the current one and returns 1 when it exists (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* SelectMission)(Mission* mission, int unusedEdx, int index);
    // Returns the checksum of the terrain file of a mission (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* ComputeMapChecksum)(Mission* mission);
    // Returns the size of the terrain file of a mission, or 0 when it has none (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* GetTerrainLength)(Mission* mission);
};

#endif

#ifndef INTERFACES_MAP_H
#define INTERFACES_MAP_H

struct Vec3;
struct Point16;
struct UnitDef;
struct Los_0047d2e0;
class Mission;

// Major version of the map interface, changed when a slot is removed or its signature changes.
#define MAP_VERSION_MAJOR 1
// Minor version of the map interface, changed when a slot is added.
#define MAP_VERSION_MINOR 3

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
    void (__stdcall* VisitObjectsInRange)(Vec3* pos, int range, void* visitor);
    // Returns 1 when a unit type can be built at a cell, with a visibility check when los is given. Hooks: none.
    int (__stdcall* CanBuildAt)(UnitDef* unit, Point16 cell, short type, Los_0047d2e0* los);
    // Returns 1 when the footprint of a unit type fits at a cell (flags 2 counts a cell off the map as a fit). Hooks: none.
    int (__stdcall* CanPlaceUnitFootprint)(UnitDef* unit, UnitDef* other, Point16 cell, int flags);
};

#endif

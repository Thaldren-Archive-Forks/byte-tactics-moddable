#ifndef INTERFACES_MAP_H
#define INTERFACES_MAP_H

struct Vec3;
struct Point16;
class Mission;

// Major version of the map interface, changed when a slot is removed or its signature changes.
#define MAP_VERSION_MAJOR 1
// Minor version of the map interface, changed when a slot is added.
#define MAP_VERSION_MINOR 2

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
};

#endif

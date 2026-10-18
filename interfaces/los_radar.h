#ifndef INTERFACES_LOS_RADAR_H
#define INTERFACES_LOS_RADAR_H

struct Cell;
struct Player;
struct Unit;
struct Vec3;
struct Point16;

// Major version of the los_radar interface, changed when a slot is removed or its signature changes.
#define LOS_RADAR_VERSION_MAJOR 1
// Minor version of the los_radar interface, changed when a slot is added.
#define LOS_RADAR_VERSION_MINOR 2

// Line of sight and radar.
struct LosRadarSeam {
    // Returns the map cell at a position, or null outside the map. Hooks: none.
    Cell* (__stdcall* GetMapCell)(int x, int y);
    // Console command ShareMapping: toggles the local player's map sharing and announces it. Hooks: none.
    void (__stdcall* CmdShareMapping)(int unused);
    // Console command ShareRadar: toggles the local player's radar sharing and announces it. Hooks: none.
    void (__stdcall* CmdShareRadar)(int unused);
    // Writes the map position of a world position, kept inside the terrain, to out. Hooks: none.
    void (__stdcall* ClampWorldPosToTerrain)(int x, int y, Vec3* out);
    // Removes the line of sight of each eyeball (a temporary view) whose time is up. Hooks: none.
    void (__stdcall* ExpireEyeballs)(void);
    // Releases the map's line of sight, sorting and icon data when a match shuts down. Hooks: none.
    void (__stdcall* FreeMapResources)(void);
    // Releases the radar picture surfaces. Hooks: none.
    void (__stdcall* FreeRadar)(void);
    // Builds the radar picture and its surfaces for a match. Hooks: none.
    void (__stdcall* InitRadar)(void);
    // Loads the terrain map file of the mission. Hooks: none.
    void (__stdcall* LoadTntMap)(void);
    // Rebuilds the line of sight of every unit; the flag is 1 to also redraw the radar. Hooks: none.
    void (__stdcall* RecalculateLineOfSight)(int flag);
    // Returns 1 when a unit is visible to a player's side, by sight, radar or sonar. Hooks: none.
    int (__stdcall* IsUnitVisibleToPlayer)(Player* player, Unit* unit);
    // Returns the map cell under a world position, or null outside the map. Hooks: none.
    Cell* (__stdcall* GetMapCellAtPosition)(Vec3* pos);
    // Returns the cell a footprint starting under a world position begins on. Hooks: none.
    Cell* (__stdcall* GetOriginCellAtPosition)(Vec3* pos);
    // Recomputes the terrain height range of the cells in a rectangle (a position and a size, in cells). Hooks: none.
    void (__stdcall* UpdateCellHeightRange)(Point16 pos, Point16 size);
    // Moves the line of sight of a unit to where it now is. Hooks: none.
    void (__stdcall* UpdateUnitLineOfSight)(Unit* unit);
    // Takes the line of sight of a unit off the map. Hooks: none.
    void (__stdcall* RemoveUnitLineOfSight)(Unit* unit);
    // Gives a new unit its first line of sight and tells the radar about it. Hooks: none.
    void (__stdcall* RevealNewUnit)(Unit* unit);
    // Adds an eyeball, a temporary view at a position (a is its sight distance, b a flag, c its lifetime in ticks). Hooks: none.
    void (__stdcall* AddEyeball)(Vec3* src, int a, int b, int c);
    // Draws the units the radar shows on the radar picture. Hooks: none.
    void (__stdcall* DrawRadarUnits)(void);
    // Marks on the radar picture the cells the players have mapped. Hooks: none.
    void (__stdcall* UpdateRadarMapped)(void);
    // Updates which units radar, sonar and cloaking show and hide for each player. Hooks: none.
    void (__stdcall* UpdateSensorRadarAndCloak)(void);
};

#endif

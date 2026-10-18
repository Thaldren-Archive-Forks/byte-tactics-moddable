#ifndef INTERFACES_LOS_RADAR_H
#define INTERFACES_LOS_RADAR_H

struct Cell;

// Major version of the los_radar interface, changed when a slot is removed or its signature changes.
#define LOS_RADAR_VERSION_MAJOR 1
// Minor version of the los_radar interface, changed when a slot is added.
#define LOS_RADAR_VERSION_MINOR 1

// Line of sight and radar.
struct LosRadarSeam {
    // Returns the map cell at a position, or null outside the map. Hooks: none.
    Cell* (__stdcall* GetMapCell)(int x, int y);
    // Console command ShareMapping: toggles the local player's map sharing and announces it. Hooks: none.
    void (__stdcall* CmdShareMapping)(int unused);
    // Console command ShareRadar: toggles the local player's radar sharing and announces it. Hooks: none.
    void (__stdcall* CmdShareRadar)(int unused);
};

#endif

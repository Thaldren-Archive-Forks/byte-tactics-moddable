#ifndef INTERFACES_UNITS_H
#define INTERFACES_UNITS_H

struct Unit;
struct Vec3;
struct Callback;
class CobScript;

// Major version of the units interface, changed when a slot is removed or its signature changes.
#define UNITS_VERSION_MAJOR 1
// Minor version of the units interface, changed when a slot is added.
#define UNITS_VERSION_MINOR 1

// Unit simulation.
struct UnitsSeam {
    // Advances every unit for one tick.
    void (__stdcall* UpdateAllUnits)(void);
    // Returns the height of the ground under a position. Hooks: none.
    int (__stdcall* GetGroundHeight)(Vec3* pos);
    // Writes the position of a unit's sweet spot, the point weapons aim at. Hooks: none.
    void (__stdcall* GetSweetSpot)(Unit* unit, Vec3* out);
    // Returns the index of a named script of a unit, or a negative number when it has none (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* FindScript)(CobScript* script, const char* name);
    // Starts a named script with up to four arguments and returns its thread, or a negative number (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* StartScriptWithArgs)(CobScript* script, char* name, Callback* callback, int update, int count, int a, int b, int c, int d);
    // Sets or clears the bits of a unit's state given by a mask (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* SetStateBits)(Unit* unit, int mask, int set);
};

#endif

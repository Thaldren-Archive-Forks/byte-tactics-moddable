#ifndef INTERFACES_UNITS_H
#define INTERFACES_UNITS_H

struct Unit;
struct Vec3;
struct Callback;
class CobScript;
class MovementClass;
class UnitTypeSet;

// Major version of the units interface, changed when a slot is removed or its signature changes.
#define UNITS_VERSION_MAJOR 1
// Minor version of the units interface, changed when a slot is added.
#define UNITS_VERSION_MINOR 4

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
    // Returns the 2-bit passability of a cell in a movement class's pass map (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* MovementClassGet)(MovementClass* self, int x, int y);
    // Returns 1 when a cell is inside a movement class's pass map (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* MovementClassInBounds)(MovementClass* self, unsigned int x, unsigned int y);
    // Brings a movement class's pass map up to date for the units that moved lately, as seen from a unit (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* RefreshMovedUnits)(MovementClass* self, Unit* unit);
    // Refreshes a movement class's pass map for a unit when the map changed since the unit was last updated (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* RefreshUnitIfStale)(MovementClass* self, Unit* unit);
    // Starts a named script with a callback and returns its thread, or a negative number (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* StartScript)(CobScript* script, const char* name, Callback* callback, int update);
    // Runs a named script that only answers a query and returns its result (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* QueryScript)(CobScript* script, char* name, int* p2, int* p3, int* p4, int* p5);
    // Writes the world position a unit's weapon aims from. Hooks: none.
    void (__stdcall* GetAimFromPosition)(Unit* unit, Vec3* out, unsigned char weapon);
    // Returns the offset of a piece of a unit from its position. Hooks: none.
    Vec3 (__stdcall* GetPieceOffset)(Unit* unit, int piece);
    // Returns the world position of a piece of a unit. Hooks: none.
    Vec3 (__stdcall* GetPiecePosition)(Unit* unit, int piece);
    // Marks a unit as recently damaged, for its health bar. Hooks: none.
    void (__stdcall* MarkRecentlyDamaged)(Unit* unit);
    // Notifies everything that holds a reference to a unit of an event. Hooks: none.
    void (__stdcall* NotifyUnitRefs)(Unit* unit, int event);
    // Adds a unit type, or the unit types of a category, to a set by name, and writes 1 to out for a type and 0 for a category (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* UnitTypeSetAddTypeOrCategory)(UnitTypeSet* self, char* text, int* out);
};

#endif

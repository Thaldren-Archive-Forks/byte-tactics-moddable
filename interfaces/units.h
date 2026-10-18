#ifndef INTERFACES_UNITS_H
#define INTERFACES_UNITS_H

namespace std {
template<class T> class allocator;
template<class T, class A> class vector;
}

struct Unit;
struct UnitDef;
struct Vec3;
struct Angles16;
struct Callback;
class CobScript;
class HapiBank;
class MovementClass;
class UnitMotion;
class UnitRef;
class UnitTypeSet;

// Major version of the units interface, changed when a slot is removed or its signature changes.
#define UNITS_VERSION_MAJOR 1
// Minor version of the units interface, changed when a slot is added.
#define UNITS_VERSION_MINOR 5

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
    // Allocates the memory of the unit table. Hooks: none.
    void (__stdcall* AllocateUnitMemory)(void);
    // Frees the memory of the unit table. Hooks: none.
    void (__stdcall* FreeUnitMemory)(void);
    // Frees the unit type information read from the unit files. Hooks: none.
    void (__stdcall* FreeUnitInfo)(void);
    // Reads the unit type information again from the unit files. Hooks: none.
    void (__stdcall* RefreshUnitInfo)(void);
    // Loads every unit type. Hooks: none.
    void (__stdcall* LoadUnitTypes)(void);
    // Frees every unit type. Hooks: none.
    void (__stdcall* FreeUnitTypes)(void);
    // Loads the build menus of the unit types. Hooks: none.
    void (__stdcall* LoadDownloadMenus)(void);
    // Frees the build menus of the unit types. Hooks: none.
    void (__stdcall* FreeDownloadMenus)(void);
    // Frees the movement classes. Hooks: none.
    void (__stdcall* FreeMovementClasses)(void);
    // Builds the table of unit categories. Hooks: none.
    void (__stdcall* InitUnitCategories)(void);
    // Frees the table of unit categories. Hooks: none.
    void (__stdcall* FreeUnitCategories)(void);
    // Kills every unit. Hooks: none.
    void (__stdcall* KillAllUnits)(void);
    // Kills every unit of a type. Hooks: none.
    void (__stdcall* KillUnitsOfType)(short id);
    // Loads a unit type again from its file. Hooks: none.
    void (__stdcall* ReloadUnitType)(unsigned short id);
    // Kills a unit, with a cause. Hooks: none.
    void (__stdcall* KillUnit)(Unit* unit, int cause);
    // Reads the units of a saved game from a bank. Hooks: none.
    void (__stdcall* LoadUnits)(HapiBank* file);
    // Writes the units of a saved game to a bank. Hooks: none.
    void (__stdcall* SaveUnits)(HapiBank* file);
    // Reads one unit of a saved game from a bank and returns it. Hooks: none.
    Unit* (__stdcall* LoadUnit)(unsigned short id, HapiBank* file);
    // Creates a unit of a type for a player at a position and returns it, or 0. Hooks: none.
    Unit* (__stdcall* CreateUnit)(unsigned char player, unsigned short typeId, Vec3 pos, int param_4, int mode, int param_6);
    // Returns the unit type of a name, or 0 when there is none. Hooks: none.
    UnitDef* (__stdcall* FindUnitType)(const char* name);
    // Returns the index of the unit type of a name, or 0 when there is none. Hooks: none.
    unsigned short (__stdcall* FindUnitTypeId)(const char* name);
    // Looks a mission unit up by its name among the units made so far. Hooks: none.
    int (__stdcall* FindMissionUnit)(char* name, std::vector<Unit*, std::allocator<Unit*> >* units, int value);
    // Attaches a unit to a piece of another unit, or detaches it when the target is 0. Hooks: none.
    void (__stdcall* AttachUnitToPiece)(Unit* unit, Unit* target, char piece, char mode);
    // Writes the world position of the nano piece of a unit. Hooks: none.
    void (__stdcall* GetNanoPiecePosition)(Unit* unit, Vec3* out);
    // Returns the angles of a piece of a unit. Hooks: none.
    Angles16 (__stdcall* GetPieceAngles)(Unit* unit, int piece);
    // Adds build progress to a unit under construction and returns 1 when it is done. Hooks: none.
    int (__stdcall* AddBuildProgress)(Unit* builder, Unit* unit, float amount);
    // Adds repair progress to a unit and returns 1 when it is done. Hooks: none.
    int (__stdcall* AddRepairProgress)(Unit* builder, Unit* unit, float amount);
    // Lets a unit that stopped being built lose some of its progress. Hooks: none.
    void (__stdcall* ApplyUnfinishedBuildDecay)(Unit* unit, int ticks);
    // Completes the unit a builder was building. Hooks: none.
    void (__stdcall* FinishConstruction)(Unit* builder, Unit* unit);
    // Moves a unit to a position and returns its new flags. Hooks: none.
    int (__stdcall* SetUnitPosition)(Unit* unit, Vec3 pos, int param);
    // Returns 1 when a unit can load a target (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* CanLoad)(Unit* unit, int unusedEdx, Unit* target);
    // Returns 1 when a unit can reclaim a target (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* CanReclaim)(Unit* unit, int unusedEdx, Unit* target);
    // Returns 1 when a unit can repair a target (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* CanRepair)(Unit* unit, int unusedEdx, Unit* target);
    // Unlinks a reference from the list of the unit it points at (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* UnitRefUnlink)(UnitRef* ref);
    // Starts a script by the index of its name with up to four arguments and returns its thread (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* StartScriptWithArgsByIndex)(CobScript* script, int index, Callback* callback, int update, int count, int a, int b, int c, int d);
    // Starts the script that tells a unit's script whether the unit is in the water, on the ground or in the air, when that changed (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* UpdateSfxOccupy)(UnitMotion* motion, int unusedEdx, Unit* unit);
};

#endif

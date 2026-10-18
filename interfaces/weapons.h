#ifndef INTERFACES_WEAPONS_H
#define INTERFACES_WEAPONS_H

struct Unit;
struct Vec3;
struct Point16;
struct WeaponDef;

// Major version of the weapons interface, changed when a slot is removed or its signature changes.
#define WEAPONS_VERSION_MAJOR 1
// Minor version of the weapons interface, changed when a slot is added.
#define WEAPONS_VERSION_MINOR 3

// Projectiles and explosions.
struct WeaponsSeam {
    // Moves every projectile and resolves its hits for one tick.
    void (__stdcall* UpdateProjectiles)(void);
    // Advances every explosion for one tick.
    void (__stdcall* UpdateExplosions)(void);
    // Adds a projectile of the given weapon at a position with a velocity, returning 0 when the table is full. Hooks: none.
    int (__stdcall* SpawnProjectile)(WeaponDef* weapon, Vec3* pos, Vec3* vel, int flag);
    // Finds a weapon definition by its TDF name, or null. Hooks: none.
    WeaponDef* (__stdcall* FindWeaponByName)(char* name);
    // Deals a weapon's area damage at a position, on behalf of an owner. Hooks: none.
    void (__stdcall* ApplyAreaDamageAt)(void* owner, Vec3* pos);
    // Returns the range of one of a unit's weapons. Hooks: none.
    int (__stdcall* GetWeaponRange)(Unit* unit, unsigned int weapon);
    // Returns whether one of a unit's weapons can reach another unit. Hooks: none.
    int (__stdcall* WeaponCanReachUnit)(Unit* shooter, Unit* target, unsigned char weapon);
    // Returns the projectile of a unit's weapon that can be targeted, or null. Hooks: none.
    int* (__stdcall* FindTargetableProjectile)(Unit* unit, unsigned int weapon);
    // Returns whether a unit's weapon can reach a position from a position. Hooks: none.
    int (__stdcall* WeaponCanReachPos)(Unit* unit, Vec3* from, Vec3* to, int flags);
    // Allocates the table of weapon slots. Hooks: none.
    void (__stdcall* AllocWeaponArray)(void);
    // Frees the table of weapon slots. Hooks: none.
    void (__stdcall* FreeWeaponArray)(void);
    // Loads the weapon definitions from the weapon TDF files. Hooks: none.
    void (__stdcall* LoadWeaponTypes)(void);
    // Frees the weapon definitions. Hooks: none.
    void (__stdcall* FreeWeaponTypes)(void);
    // Prepares the explosion tables. Hooks: none.
    void (__stdcall* InitExplosions)(void);
    // Frees the explosion tables. Hooks: none.
    void (__stdcall* FreeExplosions)(void);
    // Deals damage to a unit on behalf of a source unit. Hooks: none.
    void (__stdcall* DamageUnit)(Unit* source, Unit* target, int amount, int type, unsigned short extra);
    // Returns the feature index at a position, filling in its cell and size, or 0xffff. Hooks: none.
    unsigned short (__stdcall* FindFeatureAtPos)(Vec3* pos, Point16* cell, Point16* size);
    // Marks a unit's weapon as released and tells its script (thiscall in 3.1c: this in ecx, edx unused). Hooks: none.
    void (__fastcall* ReleaseWeapons)(Unit* unit, int unusedEdx, unsigned char index);
    // Marks a unit's weapon as claimed and tells its script (thiscall in 3.1c: this in ecx, edx unused). Hooks: none.
    void (__fastcall* ClaimWeapons)(Unit* unit, int unusedEdx, unsigned char index);
    // Detonates a unit's weapon, as when the unit dies. Hooks: none.
    void (__stdcall* DetonateUnitWeapon)(Unit* unit, int second);
    // Sets up the weapon slots of a new unit. Hooks: none.
    void (__stdcall* InitUnitWeaponSlots)(Unit* unit);
    // Removes the projectiles a unit fired. Hooks: none.
    void (__stdcall* RemoveUnitProjectiles)(Unit* unit);
    // Aims and fires a unit's weapons for one tick. Hooks: none.
    void (__stdcall* UpdateUnitWeapons)(Unit* unit);
};

#endif

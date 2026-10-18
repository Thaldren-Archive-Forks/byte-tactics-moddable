#ifndef INTERFACES_WEAPONS_H
#define INTERFACES_WEAPONS_H

struct Unit;
struct Vec3;
struct WeaponDef;

// Major version of the weapons interface, changed when a slot is removed or its signature changes.
#define WEAPONS_VERSION_MAJOR 1
// Minor version of the weapons interface, changed when a slot is added.
#define WEAPONS_VERSION_MINOR 2

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
};

#endif

#ifndef INTERFACES_WEAPONS_H
#define INTERFACES_WEAPONS_H

struct WeaponDef;
struct Vec3;

// Major version of the weapons interface, changed when a slot is removed or its signature changes.
#define WEAPONS_VERSION_MAJOR 1
// Minor version of the weapons interface, changed when a slot is added.
#define WEAPONS_VERSION_MINOR 1

// Projectiles and explosions.
struct WeaponsSeam {
    // Moves every projectile and resolves its hits for one tick.
    void (__stdcall* UpdateProjectiles)(void);
    // Advances every explosion for one tick.
    void (__stdcall* UpdateExplosions)(void);
    // Adds a projectile of the given weapon at a position with a velocity, returning 0 when the table is full. Hooks: none.
    int (__stdcall* SpawnProjectile)(WeaponDef* weapon, Vec3* pos, Vec3* vel, int flag);
    // Finds a weapon definition by its TDF name, or null. Hooks: none.
    char* (__stdcall* FindWeaponByName)(char* name);
};

#endif

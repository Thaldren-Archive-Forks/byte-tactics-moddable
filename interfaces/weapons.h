#ifndef INTERFACES_WEAPONS_H
#define INTERFACES_WEAPONS_H

// Major version of the weapons interface, changed when a slot is removed or its signature changes.
#define WEAPONS_VERSION_MAJOR 1
// Minor version of the weapons interface, changed when a slot is added.
#define WEAPONS_VERSION_MINOR 0

// Projectiles and explosions.
struct WeaponsSeam {
    // Moves every projectile and resolves its hits for one tick.
    void (__stdcall* UpdateProjectiles)(void);
    // Advances every explosion for one tick.
    void (__stdcall* UpdateExplosions)(void);
};

#endif

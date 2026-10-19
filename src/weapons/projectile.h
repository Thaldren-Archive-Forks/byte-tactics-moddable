#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "../util/vec3.h"

struct Unit;
struct WeaponDef;

#pragma pack(push, 1)

struct Projectile {
    WeaponDef* type;
    Vec3 position;
    char unknown_10[0x52 - 0x10];
    Unit* attacker;
    char unknown_56[0x66 - 0x56];
    unsigned char owner;
    char unknown_67[2];
    unsigned short flags;
};

#pragma pack(pop)

#endif

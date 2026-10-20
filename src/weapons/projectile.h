#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "../util/angles.h"
#include "../util/vec3.h"

struct Unit;
struct WeaponDef;

#pragma pack(push, 1)

struct Projectile {
    WeaponDef* type;
    union {
        Vec3 position;
        Vec3Words positionWords;
    };
    Vec3 start;
    Vec3 direction;
    Vec3 target;
    Angles16 angles;
    int speed;
    unsigned int range;
    int spawnTick;
    int time;
    int nextSmokeTick;
    union {
        int targetUnit;
        Unit* targetUnitPtr;
    };
    Unit* attacker;
    union {
        int field_56;
        Projectile* interceptedProjectile;
    };
    short cellX;
    short cellZ;
    union {
        short radius;
        short groundHeightAvg;
    };
    short active;
    unsigned short piece;
    short propellerSpin;
    unsigned char owner;
    short compactIndex;
    union {
        unsigned short flags;
        struct {
            unsigned short b0 : 1;
            unsigned short dead : 1;
            unsigned short b2 : 2;
            unsigned short state : 2;
            unsigned short rest : 10;
        } flagBits;
    };
};

#pragma pack(pop)

static_assert(sizeof(Projectile) == 0x6b, "Projectile is 0x6b bytes");

#endif

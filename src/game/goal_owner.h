#ifndef GOAL_OWNER_H
#define GOAL_OWNER_H

#include "../orders/order_target_types.h"
#include "../util/angles.h"
#include "../util/vec3.h"

struct UnitDef;
class UnitMotion;

struct Target_00490880 {
    char unknown_0[0x2e];
    union {
        unsigned char flags;
        unsigned char occupyFlags;
        struct {
            unsigned char mode : 2;
        };
    };
};

struct Vec3_004907e0 {
    Coord_0044f2a0 x;
    Coord_0044f2a0 y;
    Coord_0044f2a0 z;
};

#pragma pack(push, 2)
struct Struct_004907e0 {
    union {
        Target_00490880* target;
        UnitMotion* obj;
    };
    char pad4[0x5c - 0x4];
    Target_0044f2a0* list;
    char pad60[0x64 - 0x60];
    Angles16 angles;
    union {
        Vec3 pos;
        Vec3_004907e0 fixedPos;
    };
    char pad76[0x82 - 0x76];
    unsigned char* spatialBucket;
    char pad86[0x92 - 0x86];
    UnitDef* def;
};
#pragma pack(pop)

#endif

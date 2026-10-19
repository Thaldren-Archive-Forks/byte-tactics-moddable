#ifndef DEBRIS_00420F30_H
#define DEBRIS_00420F30_H

#ifndef TA_TYPES_H
#include "../graphics/handle.h"
#include "../util/vec3.h"
#endif

#pragma pack(push, 1)
struct Debris_00420f30 {
    int field_0;
    Handle ref1;
    Handle ref2;
    Vec3 pos;
    Vec3 size;
    Vec3 vel;
    Vec3 spin;
    short angle_x;
    short angle_y;
    short angle_z;
    unsigned short flag : 1;
    unsigned short : 15;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Debris_00420f30) == 84, "Debris_00420f30");
#endif

#endif

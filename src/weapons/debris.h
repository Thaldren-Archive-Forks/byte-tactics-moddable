#ifndef DEBRIS_H
#define DEBRIS_H

#ifndef TA_TYPES_H
#include "../graphics/handle.h"
#endif
#include "../util/vec3.h"

#pragma pack(push, 1)
struct Debris {
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
static_assert(sizeof(Debris) == 84, "Debris");
#endif

#endif

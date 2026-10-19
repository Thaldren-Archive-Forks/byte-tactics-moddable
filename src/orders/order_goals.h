#ifndef ORDER_GOALS_H
#define ORDER_GOALS_H

#include "order_target_types.h"

struct Unit;

#pragma pack(push, 1)
class PackedPosGoal {
public:
    char unknown[0x27];
    PackedPosGoal(Unit* unit);
};

class LiteGoal {
public:
    char unknown[0x28];
    LiteGoal(Unit* unit);
};

class PatrolGoal {
public:
    void* vtable;
    int field_4;
    Owner_0044f5c0* owner;
    Point16 points[3];
    int count;
    PatrolGoal(Unit* unit);
};
#pragma pack(pop)

#endif

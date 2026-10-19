#ifndef ORDER_GOALS_H
#define ORDER_GOALS_H

#include "order_target_types.h"
#include "../game/packed_goal.h"

struct Unit;

#pragma pack(push, 1)
class PackedPosGoal : public PackedGoal {
public:
    PackedPosGoal(Unit* unit);
    virtual ~PackedPosGoal();
    virtual void DeserializeNetUnitState(BitReader*);
};

class LiteGoal : public PackedGoal {
public:
    union {
        struct {
            unsigned char dirty : 1;
            unsigned char mode : 2;
        };
        struct {
            unsigned char state : 3;
        };
        unsigned char netDirtyFlags;
    };

    LiteGoal(Unit* unit);
    virtual void SetPathOrder(void* param);
    virtual void TickTowardGoal();
    virtual int HasNetUnitState();
    virtual void SerializeNetUnitState(BitWriter*);
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

static_assert(sizeof(PackedPosGoal) == 0x27, "PackedPosGoal size");
static_assert(sizeof(LiteGoal) == 0x28, "LiteGoal size");

#endif

#ifndef PACKED_GOAL_H
#define PACKED_GOAL_H

#include "goal_owner.h"
#include "../ai/ai_search_goal.h"
#include "../orders/order_fx.h"

class PackedGoal : public PathGoal {
public:
    Vec3 pos;
    Vec3 vel;
    short heading;
    char field_26;
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

    PackedGoal(Struct_004907e0* p);
    virtual void TickTowardGoal();
    virtual void ExportGoalPose(Vec3*, Vec3*, short*);
};

#pragma pack(push, 2)
class Class_0044e080 : public OrderFx {
public:
    char unknown_8[0x36 - 0x8];
    Class_0044e080(Struct_004907e0* owner, BitReader* reader);
};
#pragma pack(pop)

#endif

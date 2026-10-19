#ifndef ORDER_GOALS_H
#define ORDER_GOALS_H

#include "../ai/ai_search_goal.h"

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

class PatrolGoal : public PathGoal {
public:
    Point16 points[3];
    int count;

    PatrolGoal(Unit* unit);
    virtual void FillWaypointWorldPos(Vec3*, int, int);
    virtual int HasReadyWaypoints();
    virtual void DeserializeNetUnitState(BitReader*);
};
#pragma pack(pop)

#endif

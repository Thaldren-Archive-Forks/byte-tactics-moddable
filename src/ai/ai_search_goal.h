#ifndef AI_SEARCH_GOAL_H
#define AI_SEARCH_GOAL_H

#include "../util/vec3.h"

struct Vec3;
struct Point16;

struct Unit;
class OrderFx;
class BitWriter;
class BitReader;
class AiSearchGoal;

#pragma pack(push, 1)
class PathGoal {
public:
    OrderFx* target;
    Unit* owner;

    PathGoal(Unit* p);
    virtual ~PathGoal() {}
    virtual void SetPathOrder(void* param);
    virtual void TickTowardGoal();
    virtual void FillWaypointWorldPos(Vec3*, int, int);
    virtual void ExportGoalPose(Vec3*, Vec3*, short*);
    virtual int HasReadyWaypoints();
    virtual AiSearchGoal* TryClaimRepath();
    virtual int HasNetUnitState();
    virtual void SerializeNetUnitState(BitWriter*);
    virtual void DeserializeNetUnitState(BitReader*);
    virtual void DrawOnSurface(void*);
};

class AiSearchGoal : public PathGoal {
public:
    Point16 points[20];
    int count;
    unsigned int repathClaimTick;
    union {
        struct {
            unsigned char active : 1;
            unsigned char flag_1 : 1;
            unsigned char flag_2 : 1;
            unsigned char flag_3 : 1;
        };
        unsigned char flags;
    };

    AiSearchGoal(Unit* p);
    virtual ~AiSearchGoal();
    virtual void SetPathOrder(void* param);
    virtual void TickTowardGoal();
    virtual void FillWaypointWorldPos(Vec3*, int, int);
    virtual int HasReadyWaypoints();
    virtual AiSearchGoal* TryClaimRepath();
    virtual int HasNetUnitState();
    virtual void SerializeNetUnitState(BitWriter*);
    virtual void DrawOnSurface(void*);
    void SetWaypoints(Point16* src, int n);
    void TruncateWaypointsFrom(int n);
};
#pragma pack(pop)

static_assert(sizeof(AiSearchGoal) == 0x65, "AiSearchGoal");

#endif

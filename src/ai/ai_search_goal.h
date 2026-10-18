#ifndef AI_SEARCH_GOAL_H
#define AI_SEARCH_GOAL_H

#ifndef TA_TYPES_H
#include "../util/vec3.h"
#endif

struct Vec3;
struct Point16;

struct Unit;
class Base_00490a10;
class BitWriter;
class BitReader;
class AiSearchGoal;

class PathGoal {
public:
    Base_00490a10* target;             // +0x4
    Unit* owner;                       // +0x8

    PathGoal(Unit* p);
    virtual ~PathGoal();                                          // slot 0
    virtual void SetPathOrder(void* param);                       // slot 1
    virtual void TickTowardGoal();                                // slot 2
    virtual void FillWaypointWorldPos(Vec3*, int, int);           // slot 3
    virtual void ExportGoalPose(Vec3*, Vec3*, short*);            // slot 4
    virtual int HasReadyWaypoints();                              // slot 5
    virtual AiSearchGoal* TryClaimRepath();                       // slot 6
    virtual int HasNetUnitState();                                // slot 7
    virtual void SerializeNetUnitState(BitWriter*);               // slot 8
    virtual void DeserializeNetUnitState(BitReader*);             // slot 9
    virtual void DrawOnSurface(void*);                            // slot 10
};

class AiSearchGoal : public PathGoal {
public:
    Point16 points[20];                // +0xc
    int count;                         // +0x5c
    unsigned int repathClaimTick;      // +0x60
    union {
        struct {
            unsigned char active : 1;  // +0x64 bit 0
            unsigned char flag_1 : 1;
            unsigned char flag_2 : 1;
            unsigned char flag_3 : 1;
        };
        unsigned char flags;
    };
    char unknown_65[3];

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

#endif

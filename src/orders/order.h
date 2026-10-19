#ifndef ORDER_H
#define ORDER_H

#include "../util/vec3.h"
#include "mission_type.h"
#include "path_order_attach.h"

struct Unit;
class OrderFx;
class HapiBank;
// Unused here: the symbol ids these declarations take keep the allocation of
// the files that share this header (docs/c2-regalloc.md). They are the types
// behind the pointers the order views carry.
struct Feature;
struct Point;
struct Link_004895c0;
struct SaveDesc_0043a1f0;
struct Head_0043a1f0;
struct Entry_0043a1f0;
struct Vec3_0043a1f0;

#pragma pack(push, 1)
struct Order {
    char unknown_0[4];             // +0x00, the vpointer the polymorphic views model
    MissionType kind;              // +0x04, index into the order-type table
    unsigned char state;           // +0x05
    union {
        unsigned int flags;        // +0x06
        struct {
            unsigned int low : 15;
            unsigned int waiting : 1;
            unsigned int high : 16;
        };
        struct {
            unsigned int flag_0 : 1;
            unsigned int flag_1 : 1;
            unsigned int flag_2 : 1;
            unsigned int started : 1;
            unsigned int flag_rest : 28;
        } groups;
    };
    unsigned int wakeFrame;        // +0x0a
    union {
        Unit* unit;
        Unit* source;
    };
    union {
        struct {
            char unknown_12[4];
            Unit* target;
            char unknown_1a[8];
        };
        PathOrderAttach link;
    };
    Vec3 pos;                      // +0x22
    // One-arm unions: each costs symbol ids the including files match at.
    union {
        Point16 start;             // +0x2e
        struct {
            short x;
            short z;
        };
    };
    union {
        Point16 cached;            // +0x32
    };
    union {
        int type;                  // +0x36
        int id;
        int wait;
        int weapon;
        int elapsed;
        int unitType;
        int ticks;
        int mode;
        int done;
        int attempts;
        int time;
        int guardRange;
        int approachRadius;
        int angle;
        int side;
        int piece;
    };
    union {
        int radius;                // +0x3a
        int step;
        int waitLimit;
        int count;
        int unused;
        int duration;
        int countdown;
        int parity;
        int misses;
    };
    union {
        int progress;              // +0x3e
        int range;
        int retries;
        int capabilities;
    };
    unsigned int flags_42;         // +0x42
    unsigned int created;          // +0x46
    Order* next;                   // +0x4a
    unsigned int subFlags;         // +0x4e
    void* attached;                // +0x52

    Order(unsigned char kind, Unit* unit, Vec3* pos, int a, int b, int c);
    Order(MissionType kind, Unit* unit, void* pos, int a, int b, int c);
    Order(unsigned char kind, int a, int b, int c, int d, int e);
    Order(Unit* unit, HapiBank* file, char* name);
    ~Order();
    void SetDeadlineTicks(int param);
    void ReattachFxToUnit();
    void MergeFlagsFromTable(int k);
    void MergeFlagsFromTable(MissionType k);
    void OrStatusFlags(unsigned int flags);
    void SetAttachedFx(OrderFx* obj);
    void AttachApproachRadiusGoal(Vec3* pos, int n);
    void AttachRingApproachGoal(Vec3* pos, int radius1, int radius2);
    void AttachBuildFootprintMarker(Point16 cell, Point16 size);
    void AnnounceStatusIfFlagged(const char* text);
    Unit* Target() { return target; }
    void Wait() { waiting = 1; }
    Vec3* Position() { return &pos; }
    int Advance(int distance);
    int SerializeToSave(Unit* unit, HapiBank* file, char* name);
};
#pragma pack(pop)

#endif

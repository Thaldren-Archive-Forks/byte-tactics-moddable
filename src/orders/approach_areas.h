#ifndef APPROACH_AREAS_H
#define APPROACH_AREAS_H

#include "order_fx.h"
#include "../util/vec3.h"

#pragma pack(push, 1)
class __declspec(novtable) ApproachRadius : public OrderFx {
public:
    union {
        struct {
            union {
                Point16 pos;
                struct {
                    short x;
                    short y;
                };
            };
            int radius;
            int radiusSq;
        };
        Vec3 v;
    };

    ApproachRadius(int owner, HapiBank* file, char* name);
    ApproachRadius(Order* source, int x, int y, int r);
    void* Destroy(unsigned char flag);
    int ContainsCell(int px, int py);
    int ContainsUnit(Unit* unit);
    int FillWorldPos(int* out);
    int ApproxDistExcess(int px, int py);
    int Serialize(int unused, HapiBank* file, char* name);
    int GetType();
    void AppendGoalCell(void* cells);
};

class __declspec(novtable) RingApproach : public OrderFx {
public:
    union {
        struct {
            union {
                Point16 pos;
                struct {
                    short x;
                    short y;
                };
            };
            int inner;
            int outer;
            int innerSq;
            int outerSq;
        };
        struct {
            int a;
            int b;
            int c;
            int d;
            int e;
        } data;
    };

    RingApproach(int owner, HapiBank* file, char* name);
    RingApproach(Order* source, int x, int y, int r1, int r2);
    void* Destroy(unsigned char flag);
    int ContainsCell(int param_1, int param_2);
    int ContainsUnit(Unit* unit);
    int GetType();
    int ApproxDistExcess(int px, int py);
    int Serialize(int unused, HapiBank* file, char* name);
    int FillWorldPos(Vec3* out);
    void AppendGoalCell(void* cells);
};

class __declspec(novtable) PointMarker : public OrderFx {
public:
    int x1;
    int x2;
    int y1;
    int y2;

    PointMarker(int owner, HapiBank* file, char* name);
    PointMarker(Order* owner, Point16 pos, Point16 size);
    void* Destroy(unsigned char flag);
    int ContainsCell(int x, int y);
    int ApproxDist(int x, int y);
    int GetType();
    int FillWorldPos(int* out);
    int Serialize(int unused, HapiBank* file, char* name);
    void AppendGoalCell(void* cells);
};

struct ApproachRadiusRecord {
    int reserved;
    Vec3 v;
};

struct RingApproachRecord {
    int reserved;
    int pos;
    int inner;
    int outer;
    int innerSq;
    int outerSq;
};

struct PointMarkerRecord {
    int reserved;
    int x1;
    int x2;
    int y1;
    int y2;
};

static_assert(sizeof(ApproachRadiusRecord) == 16, "ApproachRadiusRecord size");
static_assert(sizeof(RingApproachRecord) == 24, "RingApproachRecord size");
static_assert(sizeof(PointMarkerRecord) == 20, "PointMarkerRecord size");
#pragma pack(pop)

#endif

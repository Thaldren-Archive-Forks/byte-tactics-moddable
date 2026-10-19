#ifndef APPROACH_AREAS_H
#define APPROACH_AREAS_H

#include "order_fx.h"

struct Point16;

#pragma pack(push, 1)
class ApproachRadius : public OrderFx {
public:
    short x;
    short y;
    int radius;
    int radiusSq;

    ApproachRadius(int owner, HapiBank* file, char* name);
    ApproachRadius(Order* source, int x, int y, int r);
    void* Destroy(unsigned char flag);
    int ContainsCell(int px, int py);
    int FillWorldPos(int* out);
    int ApproxDistExcess(int px, int py);
    int Serialize(int unused, HapiBank* file, char* name);
    int GetType();
};

class RingApproach : public OrderFx {
public:
    short x;
    short y;
    int inner;
    int outer;
    int innerSq;
    int outerSq;

    RingApproach(int owner, HapiBank* file, char* name);
    RingApproach(Order* source, int x, int y, int r1, int r2);
    void* Destroy(unsigned char flag);
    int ContainsCell(int param_1, int param_2);
    int ContainsUnit(Unit* unit);
    int GetType();
};

class PointMarker : public OrderFx {
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
};
#pragma pack(pop)

#endif

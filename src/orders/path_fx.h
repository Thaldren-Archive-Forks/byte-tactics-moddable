#ifndef PATH_FX_H
#define PATH_FX_H

#include "order_fx.h"
#include "path_order_attach.h"

#pragma pack(push, 1)
class Class_0044e2d0 : public OrderFx {
public:
    short field_8;
    short field_a;
    short field_c;
    short field_e;
    short field_10;
    int field_12;
    PathOrderAttach ref;
    Vec3 pos;
    char unknown_32[4];

    Class_0044e2d0(Order* order, const Vec3& pos);
    void SetAltitude(int altitude);
    void SetApproachRadius(short radius);
};

class Class_0044e190 : public OrderFx {
public:
    short field_8;
    short field_a;
    short field_c;
    short field_e;
    short field_10;
    Unit* field_12;
    PathOrderAttach ref;
    Vec3 pos;
    int field_32;

    Class_0044e190(Order* order, Unit* unit);
};

class Class_0044e250 : public OrderFx {
public:
    short field_8;
    short field_a;
    short field_c;
    short field_e;
    short field_10;
    Unit* field_12;
    PathOrderAttach ref;
    Vec3 pos;
    char unknown_32[4];

    Class_0044e250(Order* order, Unit* unit, int value);
};

class Class_0044e330 : public OrderFx {
public:
    short field_8;
    short field_a;
    short field_c;
    short field_e;
    short field_10;
    int field_12;
    PathOrderAttach ref;
    Vec3 pos;
    char unknown_32[4];

    Class_0044e330(Order* order, Unit* unit, const Vec3& p);
};

class HapiBank;

class Class_0044de80 : public OrderFx {
public:
    short field_8;
    short field_a;
    short field_c;
    short field_e;
    short field_10;
    Unit* field_12;
    PathOrderAttach ref;
    Vec3 pos;
    int field_32;

    Class_0044de80(int owner, HapiBank* file, char* name);
};
#pragma pack(pop)

#endif

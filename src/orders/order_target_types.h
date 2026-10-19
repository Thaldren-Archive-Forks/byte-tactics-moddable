#ifndef ORDER_TARGET_TYPES_H
#define ORDER_TARGET_TYPES_H

struct UnitDef;

#pragma pack(push, 1)
struct Object_0044ea60 {
    char unknown_0[0x92];
    UnitDef* type;
};

struct Target_0044f2a0 {
    char unknown_0[0x42];
    unsigned int flags;
};

union Coord_0044f2a0 {
    int fixed;
    short half[2];
};

struct Target_0044f5c0 {
    char unknown_0[0x2e];
    unsigned char flag_0 : 1;
    unsigned char flag_1 : 1;
    unsigned char flag_2 : 1;
    unsigned char flag_rest : 5;
};

struct Owner_0044f5c0 {
    Target_0044f5c0* target;
};
#pragma pack(pop)

#endif

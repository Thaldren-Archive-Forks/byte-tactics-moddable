#ifndef UNIT_H
#define UNIT_H

#ifndef TA_TYPES_H
#include "../util/vec3.h"
#endif

class TdfRecord;
struct UnitMotion;
struct Mover;
struct Limits_0043e490;
struct Order;
struct UnitDef;
struct Player;
class Class_004b0a70;
struct SpotState;
class PathOrderAttach;

#pragma pack(push, 1)
struct Unit {
    int field_0;
    TdfRecord* current;
    int field_8;
    UnitMotion* type;
    Mover* f10;
    char unknown_14[2];
    short field_16;
    short field_18;
    char unknown_1a;
    unsigned char f_1b;
    char unknown_1c[2];
    unsigned char field_1e;
    unsigned char f_1f;
    int field_20;
    char unknown_24[2];
    int field_26;
    char unknown_2a[2];
    Mover* f2c;
    char unknown_30[10];
    unsigned char b3a;
    unsigned char f_3b;
    char unknown_3c[12];
    Limits_0043e490* f48;
    char unknown_4c[10];
    unsigned char b56;
    unsigned char f_57;
    int field_58;
    Order* list;
    Order* list2;
    short field_64;
    short heading;
    short field_68;
    Vec3 pos;
    Point16 pos_76;
    int field_7a;
    Point16 footprint;
    int field_82;
    int field_86;
    Unit* cargo;
    Unit* next;
    UnitDef* def;
    Player* player;
    Class_004b0a70* script;
    SpotState* state;
    PathOrderAttach* head;
    unsigned short field_a6;
    unsigned short id;
    unsigned short fix_lo;
    int field_ac;
    int timeout;
    char unknown_b4[4];
    unsigned short f_b8;
    unsigned short unknown_ba : 2;
    unsigned short dirty : 1;
    unsigned short : 13;
    float field_bc;
    char unknown_c0[20];
    float metal;
    char unknown_d8[4];
    int f_dc;
    char unknown_e0[4];
    unsigned short field_e4;
    unsigned short f_e6;
    char unknown_e8[4];
    Player* player_ec;
    Unit* attacker;
    unsigned char field_f4;
    unsigned char orderKind;
    unsigned char field_f6;
    unsigned char field_f7;
    unsigned char field_f8;
    unsigned char field_f9;
    unsigned char field_fa;
    int field_fb;
    unsigned char owner;
    int field_100;
    union { float speed; int speed_bits; };
    short health;
    char unknown_10a[4];
    unsigned char field_10e;
    unsigned char bit0 : 1;
    unsigned char bit1 : 1;
    unsigned char bit2 : 1;
    unsigned char bit3 : 1;
    unsigned char unknown_10f : 4;
    unsigned int flags;
    int pad114;
    Unit(void);
    ~Unit(void);
    int SelectRecord(char*);
    int Ready(void);
    unsigned char GetState(void);
    unsigned char PlayerIndex(void);
    void SetStateBits(int, int);
    unsigned char ChooseWeapon();
    void ClaimWeapons(unsigned char index);
    void ReleaseWeapons(unsigned char index);
};
#pragma pack(pop)

#endif

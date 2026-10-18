#ifndef UNIT_H
#define UNIT_H

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
struct Vec3;
struct Point16;

#pragma pack(push, 1)
struct Unit {
    int field_0;  // +0x0
    TdfRecord* current;       // +0x4
    int field_8;  // +0x8
    UnitMotion* type;      // +0xc
    Mover* f10;  // +0x10
    char unknown_14[2];
    short field_16;  // +0x16
    short field_18;  // +0x18
    char unknown_1a;  // +0x1a
    unsigned char f_1b;  // +0x1b
    char unknown_1c[2];
    unsigned char field_1e;  // +0x1e
    unsigned char f_1f;  // +0x1f
    int field_20;  // +0x20
    char unknown_24[2];
    int field_26;  // +0x26
    char unknown_2a[2];
    Mover* f2c;  // +0x2c
    char unknown_30[10];
    unsigned char b3a;  // +0x3a
    unsigned char f_3b;  // +0x3b
    char unknown_3c[12];
    Limits_0043e490* f48;  // +0x48
    char unknown_4c[10];
    unsigned char b56;  // +0x56
    unsigned char f_57;  // +0x57
    int field_58;  // +0x58
    Order* list;  // +0x5c
    Order* list2;  // +0x60
    short field_64;  // +0x64
    short heading;  // +0x66
    short field_68;  // +0x68
    Vec3 pos;  // +0x6a
    Point16 pos_76;  // +0x76
    int field_7a;  // +0x7a
    Point16 footprint;  // +0x7e
    int field_82;  // +0x82
    int field_86;  // +0x86
    Unit* cargo;  // +0x8a
    Unit* next;  // +0x8e
    UnitDef* def;  // +0x92
    Player* player;  // +0x96
    Class_004b0a70* script;  // +0x9a
    SpotState* state;  // +0x9e
    PathOrderAttach* head;  // +0xa2
    unsigned short field_a6;  // +0xa6
    unsigned short id;  // +0xa8
    unsigned short fix_lo;  // +0xaa
    int field_ac;  // +0xac
    int timeout;  // +0xb0
    char unknown_b4[4];
    unsigned short f_b8;  // +0xb8
    unsigned short unknown_ba : 2;  // +0xba
    unsigned short dirty : 1;
    unsigned short : 13;
    float field_bc;  // +0xbc
    char unknown_c0[20];
    float metal;  // +0xd4
    char unknown_d8[4];
    int f_dc;  // +0xdc
    char unknown_e0[4];
    unsigned short field_e4;  // +0xe4
    unsigned short f_e6;  // +0xe6
    char unknown_e8[4];
    Player* player_ec;  // +0xec
    Unit* attacker;  // +0xf0
    unsigned char field_f4;  // +0xf4
    unsigned char orderKind;  // +0xf5
    unsigned char field_f6;  // +0xf6
    unsigned char field_f7;  // +0xf7
    unsigned char field_f8;  // +0xf8
    unsigned char field_f9;  // +0xf9
    unsigned char field_fa;  // +0xfa
    int field_fb;  // +0xfb
    unsigned char owner;  // +0xff
    int field_100;  // +0x100
    union { float speed; int speed_bits; };  // +0x104
    short health;  // +0x108
    char unknown_10a[4];
    unsigned char field_10e;  // +0x10e
    unsigned char bit0 : 1;  // +0x10f
    unsigned char bit1 : 1;
    unsigned char bit2 : 1;
    unsigned char bit3 : 1;
    unsigned char unknown_10f : 4;
    unsigned int flags;  // +0x110
    int pad114;  // +0x114
    Unit(void);
    ~Unit(void);
    int SelectRecord(char*);
    int Ready(void);
    unsigned char GetState(void);
    unsigned char PlayerIndex(void);
    void SetStateBits(int, int);
    unsigned char ChooseWeapon();
};
#pragma pack(pop)

#endif

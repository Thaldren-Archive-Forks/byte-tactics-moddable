#ifndef UNIT_H
#define UNIT_H

#ifndef TA_TYPES_H
#include "../util/vec3.h"
#endif
#include "../util/angles.h"
#include "../game/unit_resources.h"
#include "../weapons/unit_weapon_slot.h"

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
    int motion;
    union {
        struct {
            TdfRecord* current;
            int field_8;
            UnitMotion* type;
            Mover* weapon;
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
            int lastTick;
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
        };
        UnitWeaponSlot weapons[3];
    };
    int extraction;
    Order* list;
    Order* list2;
    union {
        struct {
            short field_64;
            short heading;
            short pitch;
        };
        Angles16 angles;
    };
    union {
        Vec3 pos;
        struct {
            unsigned short xFraction;
            short xWhole;
            unsigned short yFraction;
            short yWhole;
            unsigned short zFraction;
            short zWhole;
        };
    };
    Point16 cell;
    union {
        int losCacheCell;
        struct {
            short losCacheCellX;
            short losCacheCellZ;
        };
    };
    Point16 footprint;
    int spatialBucket;
    int carrier;
    Unit* cargo;
    Unit* cargoNext;
    UnitDef* def;
    Player* player;
    Class_004b0a70* script;
    SpotState* state;
    PathOrderAttach* head;
    unsigned short unitDefIndex;
    unsigned short id;
    unsigned short hoverBobPhase;
    int group;
    int workTime;
    char unknown_b4[4];
    unsigned short killCount;
    union {
        struct {
            unsigned short unknown_ba : 2;
            unsigned short dirty : 1;
            unsigned short : 13;
        };
        short netDirtyFlags;
    };
    union {
        struct {
            float energy;
            char unknown_c0[20];
            float metal;
            char unknown_d8[4];
            int f_dc;
            char unknown_e0[4];
            unsigned short field_e4;
            unsigned short f_e6;
            char unknown_e8[4];
            Player* owner;
        };
        UnitResources resourceSlot;
    };
    Unit* attacker;
    unsigned char lastAttackerSlot;
    unsigned char lastDamageType;
    unsigned char healthPercent;
    unsigned char prevHealthPercent;
    unsigned char losSightFrameIdx;
    unsigned char transportPiece;
    unsigned char recentlyDamagedTimer;
    int postTransferHoldoff;
    unsigned char playerIndex;
    int unfinishedInitZero;
    union {
        float speed;
        int speed_bits;
        float buildLeft;
    };
    short health;
    union {
        char unknown_10a[4];
        struct {
            unsigned char team;
            char unknown_10b[3];
        };
    };
    union {
        unsigned char activateFlags;
        struct {
            unsigned char activate_bit0 : 1;
            unsigned char activate_bit1 : 1;
            unsigned char activate_bit2 : 1;
            unsigned char activate_bit3 : 1;
            unsigned char activate_bit4 : 1;
            unsigned char unknown_10e : 3;
        };
    };
    union {
        struct {
            unsigned char bit0 : 1;
            unsigned char bit1 : 1;
            unsigned char bit2 : 1;
            unsigned char bit3 : 1;
            unsigned char unknown_10f : 4;
        };
        char cobStateFlags;
    };
    union {
        unsigned int flags;
        struct {
            unsigned int mode : 2;
            unsigned int unknown_110_2 : 14;
            unsigned int moved : 1;
            unsigned int unknown_110_17 : 1;
            unsigned int fire : 2;
            unsigned int move : 2;
            unsigned int f22_23 : 2;
            unsigned int f24_25 : 2;
            unsigned int f26_27 : 2;
            unsigned int unknown_110_28 : 4;
        };
        struct {
            unsigned int bits_110 : 14;
            unsigned int flag_110 : 1;
            unsigned int unknown_110_15 : 17;
        } panel;
        struct {
            unsigned int b0 : 1;
            unsigned int b1 : 1;
            unsigned int b2 : 1;
            unsigned int b3 : 1;
            unsigned int b4 : 1;
            unsigned int b5 : 1;
            unsigned int b6 : 1;
            unsigned int b7 : 1;
            unsigned int b8 : 1;
            unsigned int b9 : 1;
            unsigned int b10 : 1;
            unsigned int b11 : 1;
            unsigned int b12 : 1;
            unsigned int b13 : 1;
            unsigned int b14 : 1;
            unsigned int b15 : 1;
            unsigned int b16 : 1;
            unsigned int b17 : 1;
            unsigned int b18 : 1;
            unsigned int b19 : 1;
            unsigned int b20 : 1;
            unsigned int b21 : 1;
            unsigned int b22 : 1;
            unsigned int b23 : 1;
            unsigned int b24 : 1;
            unsigned int b25 : 1;
            unsigned int b26 : 1;
            unsigned int b27 : 1;
            unsigned int b28 : 1;
            unsigned int b29 : 1;
            unsigned int b30 : 1;
            unsigned int b31 : 1;
        } f110;
    };
    int zBufferFlag;
    Unit(void);
    ~Unit(void);
    int SelectRecord(char*);
    int Ready(void);
    unsigned char GetState(void);
    unsigned char PlayerIndex(void);
    void SetStateBits(int, int);
    void SetStateBits(unsigned char, int);
    unsigned char ChooseWeapon();
    void ClaimWeapons(unsigned char index);
    void ReleaseWeapons(unsigned char index);
    int CanLoad(Unit* other);
    int CountCargo(void);
    int CanReclaim(void* param_1);
    int CanRepair(Unit* other);
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
#include <stddef.h>
static_assert(sizeof(Unit) == 0x118, "Unit is 0x118 bytes");
static_assert(offsetof(Unit, weapons) == 0x4, "weapons");
static_assert(offsetof(Unit, angles) == 0x64, "angles");
static_assert(offsetof(Unit, pos) == 0x6a, "pos");
static_assert(offsetof(Unit, netDirtyFlags) == 0xba, "netDirtyFlags");
static_assert(offsetof(Unit, losCacheCell) == 0x7a, "losCacheCell");
static_assert(offsetof(Unit, resourceSlot) == 0xbc, "resourceSlot");
static_assert(offsetof(Unit, owner) == 0xec, "owner");
static_assert(offsetof(Unit, buildLeft) == 0x104, "buildLeft");
static_assert(offsetof(Unit, cobStateFlags) == 0x10f, "cobStateFlags");
#endif

#endif

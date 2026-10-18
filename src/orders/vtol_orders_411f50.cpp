#include "../../interfaces/seams.h"
// Decompiled by Claude Opus 5.5, finished by deepseek-v4.1-flash and GPT-6, edited by deepseek-v4.1, finished by GPT-6.1-sol, edited by Claude Opus 5.5, finished by Claude Opus 5.5. re-verified by GPT-6. Names are provisional.
// "Attacking" order handler of aircraft (VTOL). Interrupts hand over to a
// "VTOL_SEEKATTACK" order; the order follows its target unit and gives up
// outside its range. State 0 prepares the order (PrepVtolClimb), states 1 and
// 2 make attack runs past the target, state 4 turns around after a pause,
// state 5 aims at the target again and state 6 flies on and, when the unit is
// below three quarters of its health, sends it to a random repair pad
// ("VTOL_LANDING").
#include <list>
#include <windows.h>
#include <math.h>
#include <vector>

struct Point { short x, y; };
#include "../util/vec3.h"
static inline Vec3 PosPlus(const Vec3& a, const Vec3& other)
{
    Vec3 r;
    r.x = a.x + other.x;
    r.y = a.y + other.y;
    r.z = a.z + other.z;
    return r;
}

struct Unit;
class UnitMotion {
public:
    char unknown_0[0x22];
    short field_22;                    // +0x22
    char unknown_24[0x2e - 0x24];
    unsigned char flags;               // +0x2e
    void SetFlightMode(Unit* unit, int state);
};
#include "../state/block.h"
#include "path_order_attach.h"
class MissionType { public: unsigned char index; MissionType(const char*); };

// Unused here: forward declarations of real functions; their symbol ids keep the
// allocation of the functions after the merged views (docs/c2-regalloc.md).
void UpdateMouseScroll();
void UpdateEdgeScroll();
void CenterCameraOnRadarClick();
void CenterCameraOnStartPosition();
#pragma pack(push, 1)
struct UnitDef {
    char pad0[0x1fa]; unsigned int maxHealth;
    char pad1fe[0x216 - 0x1fe]; unsigned short attackrunlength;
    char pad218[0x21c - 0x218]; short altitude;
    char pad21e[0x241 - 0x21e]; unsigned int flags;
};
struct Struct_Unit96 {
    char pad0[0x146]; unsigned char slotIndex;
};
#include "../units/unit.h"
struct Order {
    char pad0[5]; unsigned char state; unsigned int flags;
    char padA[0x12 - 0xa]; PathOrderAttach target;
    Vec3 pos;
    Point start;                       // +0x2e
    char pad32[0x3e - 0x32]; int range;
    unsigned int flags_42;
    char pad46[0x4a - 0x46]; int next;
    void AnnounceStatusIfFlagged(const char*);
    void SetAttachedFx(int);
    void SetDeadlineTicks(int);
    Order(MissionType type, Unit* target, Vec3* pos, int c, int d, int e);
    char unknown_4e[0x8];
    // Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
    void ReattachFxToUnit();
    void MergeFlagsFromTable(int k);
    void AttachRingApproachGoal(Vec3* pos, int radius1, int radius2);
    Order(Unit* unit, void* file, char* name);
    void OrStatusFlags(unsigned int flags);
    void AttachApproachRadiusGoal(Vec3* pos, int radius);
    void AttachBuildFootprintMarker(Point cell, Point size);
    Unit* Target();
    void Wait();
    Vec3* Position();
};
struct Struct_Game391e9 {
    char pad0[0xd3c]; int gravity;
};
struct Game {
    char pad0[0x391e9]; Struct_Game391e9* mapInfo;
};
class Class_0044e2d0 {
public:
    char unknown_0[0x36];
    Class_0044e2d0(Order* order, const Vec3& pos);
    void SetAltitude(int);
    void SetApproachRadius(short);
};
class Class_0044e190 {
public:
    char unknown_0[0x36];
    Class_0044e190(Order* order, Unit* unit);
};
#pragma pack(pop)

extern Game* g_game;

int __stdcall RandomInt(int);
int __cdecl FUN_004b70ef(short, int);
int __cdecl FUN_004b7123(short, int);
int __stdcall GetHeadingBetween(Vec3*, Vec3*);
void __stdcall AppendOrder(Unit*, Order*);
void __stdcall AppendOrderToTail(Unit*, Order*);

static inline Vec3 Offset(short angle, int distance)
{
    Vec3 v;
    v.x = -FUN_004b70ef(angle, distance);
    v.y = 0;
    v.z = -FUN_004b7123(angle, distance);
    return v;
}

// Declared here, defined after AirStrikeOrder: the definition's local object
// is numbered before the function otherwise, which moves its register
// allocation (docs/c2-regalloc.md).
static inline void AttachMove(Order* order, const Vec3& dest, short radius);

// 0x40f200, matched in 0x40f200.cpp; inlined into the state 0 case below.
void __stdcall PrepVtolClimb(Unit* unit, Order* order, unsigned int flags)
{
    g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
    if (unit->carrier)
        g_seams.units->AttachUnitToPiece(unit, 0, -1, 2);
    g_seams.units->SetStateBits(unit, 1, 1);
    if ((((UnitMotion*)unit->motion)->flags & 3) == 1) {
        ((UnitMotion*)unit->motion)->SetFlightMode(unit, 2);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, unit->pos);
        obj->SetAltitude(unit->def->altitude / 2);
        order->SetAttachedFx((int)obj);
        order->flags |= flags | 0xe0;
    }
}


// Stays in a file of its own: it matches only in this file's symbol context.
// FUNCTION: 0x411f50
int __stdcall AirStrikeOrder(Unit* unit, Order* order, unsigned int flags)
{
    if (flags & 0x1000a) {
        if (!order->next && (unit->flags & 0x300000))
            AppendOrderToTail(unit, new (kSim) Order("VTOL_SEEKATTACK", order->target.owner, &order->pos, 0, 0, 0));
        return 5;
    }
    Unit* target = order->target.owner;
    if (!target && (order->flags_42 & 0x200)) {
        if (!order->next)
            AppendOrderToTail(unit, new (kSim) Order("VTOL_SEEKATTACK", 0, &unit->pos, 0, 0, 0));
        return 5;
    }
    if (target)
        order->pos = target->pos;
    if (order->range && (int)_hypot(unit->xWhole - order->start.x, unit->zWhole - order->start.y) >= order->range)
        return 5;
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags & 0x800)) {
            order->AnnounceStatusIfFlagged("Attacking");
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        break;
    case 1: {
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        // Pointer locals declared bare, then &order->pos assigned first: orders the two lea.
        Vec3* op;
        Vec3* up;
        op = &order->pos;
        up = &unit->pos;
        if ((int)_hypot(order->pos.x - unit->pos.x, order->pos.z - unit->pos.z) < 0x1e00000) {
            int angle = GetHeadingBetween(up, op);
            Vec3 dest = PosPlus(unit->pos, Offset(angle, 0x8c00000));
            AttachMove(order, dest, 0x3c0);
            order->flags |= 0xe2;
            return 1;
        }
        return 1;
    }
    case 2: {
        // Declared bare and assigned in this order, as in case 1.
        Vec3* op2;
        Vec3* up2;
        op2 = &order->pos;
        up2 = &unit->pos;
        int dist = (int)_hypot(order->pos.x - unit->pos.x, order->pos.z - unit->pos.z);
        int angle = GetHeadingBetween(up2, op2);
        int radius = dist / 2;
        Vec3 dest = PosPlus(unit->pos, Offset(RandomInt(0x4000) + angle - 0x2000, radius));
        AttachMove(order, dest, 0x1e0);
        order->flags = 0x100e8;
        return 1;
    }
    case 3:
        return 1;
    case 4: {
        if (flags & 0xe0)
            return 1;
        UnitDef* def = unit->def;
        // size and rate are loaded before the !rate test.
        int size = def->altitude;
        int rate = g_game->mapInfo->gravity;
        if (!rate)
            break;
        // Float local x: keeps the constant multiply from moving outermost.
        float x = (float)sqrt(size * 2.0 / rate) * 30.0f;
        // attackrunlength is added in a second statement: one sum changes the x87 order.
        int time = (int)(x * ((UnitMotion*)unit->motion)->field_22) + 1;
        time += def->attackrunlength;
        Class_0044e2d0* obj;
        if (order->target.owner)
            obj = (Class_0044e2d0*)new (kSim) Class_0044e190(order, order->target.owner);
        else
            obj = new (kSim) Class_0044e2d0(order, order->pos);
        obj->SetApproachRadius(time);
        order->SetAttachedFx((int)obj);
        order->SetDeadlineTicks(1);
        order->flags |= 0x100e8;
        return 2;
    }
    case 5: {
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        g_seams.targeting->SetWeaponTargetPos(unit, &order->pos, 0);
        int angle = GetHeadingBetween(&unit->pos, &order->pos);
        Vec3 dest = PosPlus(unit->pos, Offset(angle, (unit->def->attackrunlength + 0x3c0) << 16));
        AttachMove(order, dest, 0x3c0);
        order->flags = 0xe2;
        return 1;
    }
    case 6: {
        g_seams.targeting->ClearWeaponTarget(unit, 0);
        Vec3 dest = PosPlus(unit->pos, Offset(unit->heading, 0x5a00000));
        AttachMove(order, dest, 0x80);
        order->flags = 0xe2;
        if (unit->health < unit->def->maxHealth / 4 * 3) {
            std::vector<Unit*> pads;
            g_seams.ai->GetFactoriesInRadius(((Struct_Unit96*)unit->player)->slotIndex, &unit->pos, 0xf00, &pads);
            if (!pads.empty()) {
                order->SetAttachedFx(0);
                Unit* pad = pads[RandomInt(pads.size())];
                AppendOrder(unit, new (kSim) Order("VTOL_LANDING", pad, 0, 0, 0, 0));
                order->flags = 0;
                return 0;
            }
            return 0;
        }
        order->state = 3;
        return 2;
    }
    }
    return 7;
}

static inline void AttachMove(Order* order, const Vec3& dest, short radius)
{
    Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, dest);
    obj->SetApproachRadius(radius);
    order->SetAttachedFx((int)obj);
}

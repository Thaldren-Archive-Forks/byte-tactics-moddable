// Decompiled by Claude Opus 5.5. Names are provisional.
// <stdio.h> (or another header, see tools/headers.py) is needed only for
// compiler state: without it the energy + metal sum loads metal first.
#include <stdio.h>

#include "../state/block.h"
#include "box.h"

struct Unit;

#include "unit_motion.h"

// Unused here: forward declarations of real functions; their symbol ids keep the
// allocation of the functions after the merged views (docs/c2-regalloc.md).
void UpdateMouseScroll();
void UpdateEdgeScroll();

#pragma pack(push, 1)
#include "../map/feature.h"

#include "../units/unit_def.h"

#include "../units/unit.h"
// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
int __stdcall FindLandingPad(Unit*, int);
unsigned short __stdcall ChooseBuildOption(unsigned int, Unit*);
void __stdcall ClearWeaponTarget(Unit*, int);
void __stdcall DetonateUnitWeapon(Unit*, int);
void __stdcall DrawUnit(void*, Unit*);

struct Order {
    char unknown_0[5];
    unsigned char state;               // +0x5
    unsigned int flags;                // +0x6
    char unknown_a[0x22 - 0xa];
    Vec3 pos;                          // +0x22
    char unknown_2e[0x36 - 0x2e];
    int time;                          // +0x36
    void AnnounceStatusIfFlagged(const char*);
    void SetAttachedFx(int);
    void SetDeadlineTicks(int ticks);
    // Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
    void ReattachFxToUnit();
    void MergeFlagsFromTable(int k);
    void AttachRingApproachGoal(Vec3* pos, int radius1, int radius2);
    Order(Unit* unit, void* file, char* name);
    void OrStatusFlags(unsigned int flags);
    void AttachApproachRadiusGoal(Vec3* pos, int radius);
    void AttachBuildFootprintMarker(Point16 cell, Point16 size);
    Unit* Target();
    void Wait();
    Vec3* Position();
};

class Class_0044e2d0 {
public:
    char unknown_0[0x36];
    Class_0044e2d0(Order* order, const Vec3& pos);
    void SetAltitude(int);
};

#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

#include "../../interfaces/seams.h"
void __stdcall QueueUnitSpeech(Unit* unit, int kind, char* text);
void __stdcall EmitReverseNanoParticles(Box* from, Vec3* to, int count);

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


// Order handler "Reclaiming" (second variant, driven by a Class_0044e2d0
// move object rather than the turn/approach states of 0x404ad0).
// Stays in a file of its own: it matches only in this file's symbol context.
// FUNCTION: 0x414770
int __stdcall VtolReclaimOrder(Unit* unit, Order* order, int flags)
{
    Point16 cell;
    Point16 size;
    unsigned short index = g_seams.weapons->FindFeatureAtPos(&order->pos, &cell, &size);
    if (index == 0xffff) {
        QueueUnitSpeech(unit, 7, "Reclamation failed");
        return 8;
    }
    Feature* f = &g_game->features[index];
    if (!(f->flags & 0x80))
        return 8;
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800) && (unit->def->flags2 & 0x400)) {
            order->AnnounceStatusIfFlagged("Reclaiming");
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        break;
    case 1: {
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, order->pos);
        order->SetAttachedFx((int)obj);
        order->flags = 0xe0;
        return 1;
    }
    case 2:
        if (flags & 0x40)
            return 8;
        order->time = (int)(30.0f - (f->value + f->metal) * -0.5f);
        QueueUnitSpeech(unit, 11, 0);
        return 1;
    case 3:
        order->SetDeadlineTicks(2);
        order->time -= 2;
        if (order->time <= 0)
            return 1;
        unit->workTime = g_game->ticks + 300;
        if (order->time > 30) {
            Vec3 nano;
            g_seams.units->GetNanoPiecePosition(unit, &nano);
            Box box;
            box.lo.x = cell.x << 20;
            box.lo.z = cell.y << 20;
            box.lo.y = g_seams.units->GetGroundHeight(&box.lo) << 16;
            box.hi = box.lo;
            box.hi.x += f->footprintX << 20;
            box.hi.z += f->footprintZ << 20;
            box.hi.y += f->height << 16;
            EmitReverseNanoParticles(&box, &nano, 6);
            EmitReverseNanoParticles(&box, &nano, 6);
        }
        return 2;
    case 4:
        g_seams.features->ReclaimFeature(unit, &order->pos);
        return 5;
    }
    return 7;
}

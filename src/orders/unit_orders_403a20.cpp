#include "../../interfaces/seams.h"
// Decompiled by GPT-6 Astra. Names are provisional.
// Stays in its own file: in unit_orders.cpp the first state's cell lookup loads g_game before the index, and the nano box corners need a member operator+ view of Vec3.
#include "win_types.h"
#include <math.h>
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
#include "mission_type.h"

#include "path_order_attach.h"
#pragma pack(push, 1)
#include "../units/unit_def.h"
#include "../units/unit.h"
#include "order.h"
#include "../game/game.h"
#pragma pack(pop)
extern Game* g_game;
static inline UnitDef* Definitions() { return g_game->defs; }
union Fixed { int value; struct { unsigned short fraction; short whole; }; };
void __stdcall QueueUnitSpeech(Unit*, int, const char*);
void __stdcall MarkSelectionOrdersDirty(Unit*);
void __stdcall AddOrder(MissionType, int, Unit*, Unit*, Vec3*, int, int);
short __stdcall GetHeadingBetween(Vec3*, Vec3*);
void __stdcall StartBuildingScript(Unit*, Order*, short);
int __stdcall WaitIfNotInBuildStance(Unit*, Order*, int);
void __stdcall EmitNanoParticles(Vec3*, Vec3*, int);
static inline Point16 WorldToCell(Vec3 v, Point16 origin)
{
    Point16 c;
    c.x = (v.x - (origin.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (origin.y << 19) + 0x80000) >> 20;
    return c;
}
static inline Point16 WorldToCell16(Vec3 v, Point16 origin)
{
    Point16 c;
    c.x = (v.x - (origin.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (origin.y << 19) + 0x80000) >> 20;
    return c;
}
static inline void CellToWorld(Point16 origin, Point16 c, Vec3* v)
{
    v->x = (origin.x + c.x * 2) << 19;
    v->z = (origin.y + c.y * 2) << 19;
}
// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
struct Feature;
// FUNCTION: 0x403a20
int __stdcall MobileBuildOrder(Unit* unit, Order* order, int flags)
{
    if (flags & 8) {
        QueueUnitSpeech(unit, 7, "Construction terminated");
        MarkSelectionOrdersDirty(unit);
        return 8;
    }
    if (flags & 2) {
        MarkSelectionOrdersDirty(unit);
        return 5;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0: {
        UnitDef* def = &Definitions()[order->type];
        Point16 cell = WorldToCell(order->pos, *(Point16*)&def->footprintx);
        CellToWorld(*(Point16*)&def->footprintx, cell, &order->pos);
        order->retries = 0;
        order->AttachBuildFootprintMarker((*(Point16*)&cell), (*(Point16*)&def->footprintx));
        order->flags = 0xe0;
        return 1;
    }
    case 1: {
        UnitDef* def = &Definitions()[order->type];
        if (flags & 0x40) {
            Fixed distance;
            distance.value = (int)_hypot(unit->pos.x - order->pos.x, unit->pos.z - order->pos.z);
            int gap = distance.whole - (int)(_hypot(unit->footprint.x, unit->footprint.y) * 8.0);
            gap += (int)(_hypot(def->footprintx, def->footprintz) * -8.0);
            unsigned int range = 0;
            range = unit->def->buildRange;
            if (gap > (int)range) {
                QueueUnitSpeech(unit, 7, "I can't reach the construction site");
                return 8;
            }
        }
        if (!g_seams.map->CanPlaceUnitFootprint(def, 0, WorldToCell16(order->pos, *(Point16*)&Definitions()[order->type].footprintx), 1)) {
            if (!order->retries)
                QueueUnitSpeech(unit, 7, "Waiting for target area to clear");
            else if (order->retries > 10) {
                QueueUnitSpeech(unit, 7, "Target area was blocked");
                return 8;
            }
            order->retries++;
            order->SetDeadlineTicks(30);
            return 2;
        }
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
        g_seams.map->SnapWorldPosToFootprint(def, &order->pos);
        ((PathOrderAttach*)((char*)order + 0x12))->SetUnit(
            g_seams.units->CreateUnit(unit->playerIndex, (short)order->type, order->pos, 0, 1, 0));
        if (!order->target) {
            QueueUnitSpeech(unit, 7, "Unable to create any more units");
            order->SetDeadlineTicks(300);
            return 2;
        }
        QueueUnitSpeech(unit, 9, "Starting construction");
        MarkSelectionOrdersDirty(unit);
        AddOrder("getbuilt", 1, order->target, unit, 0, 0, 0);
        StartBuildingScript(unit, order, GetHeadingBetween(&unit->pos, &order->target->pos) - unit->heading);
        return 1;
    }
    case 2:
        return WaitIfNotInBuildStance(unit, order, 10);
    case 3: {
        int rate = 0;
        rate = unit->def->buildRate;
        if (g_seams.units->AddBuildProgress(unit, order->target, (float)(rate / 30))) {
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(unit, &start);
            Vec3 bounds[2];
            bounds[0] = PosPlus(order->target->pos, *(Vec3*)&order->target->def->modelMinX);
            bounds[1] = PosPlus(order->target->pos, *(Vec3*)&order->target->def->modelMaxX);
            EmitNanoParticles(&start, bounds, 6);
        }
        unit->workTime = g_game->ticks + 300;
        if (order->target->buildLeft != 0.0f) {
            order->SetDeadlineTicks(1);
            order->flags |= 0xa;
            return 2;
        }
        return 1;
    }
    case 4:
        QueueUnitSpeech(unit, 8, "Building complete");
        return 5;
    }
    return 7;
}

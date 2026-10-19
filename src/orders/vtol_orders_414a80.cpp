// Decompiled by Claude Opus 5.5. Names are provisional.
#include "../state/block.h"
#include "../util/vec3.h"

static inline Vec3 Sub(const Vec3& a, const Vec3& b)
{
    Vec3 r; r.z = a.z - b.z; r.y = a.y - b.y; r.x = a.x - b.x; return r;
}

static inline int Square(const Vec3& v)
{
    __int64 a = v.x, b = v.z;
    return (int)((a*a) >> 32) + (int)((b*b) >> 32);
}

struct Point {
    short x, y;
};

struct Unit;

#include "unit_motion.h"


#pragma pack(push, 1)
#include "../units/unit_def.h"

#include "../units/unit.h"

#include "../units/unit_ref.h"

#include "order.h"

#include "path_fx.h"
#pragma pack(pop)

void __stdcall QueueUnitSpeech(Unit* unit, int kind, const char* text);
void __stdcall EmitReverseNanoParticles(Vec3* box, Vec3* from, int count);
int __stdcall ComputeReclaimDamagePulse(Unit* unit, Unit* target, int n);
#include "../../interfaces/seams.h"
void __stdcall CellToWorldPos(Point cell, Vec3* out, Point origin);

static inline Point WorldToCell(Vec3 v, Point origin)
{
    Point c;
    c.x = (v.x - (origin.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (origin.y << 19) + 0x80000) >> 20;
    return c;
}

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
        order->SetAttachedFx(obj);
        order->flags |= flags | 0xe0;
    }
}

// Order handler "Reclaiming" for a unit target (the hovering variant of
// 0x404730): state 0 checks the builder can reclaim, state 1 snaps the order
// position to the target's cell and moves over it, state 2 drains the target
// while it stays in build range.
// Stays in a file of its own: it matches only in this file's symbol context.
// FUNCTION: 0x414a80
int __stdcall VtolReclaimUnitOrder(Unit* unit, Order* order, unsigned int flags)
{
    Unit* target = order->target;
    if (!target || (flags & 0x10008))
        return 5;
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags1 & 0x800)) {
            if (!(unit->def->flags2 & 0x400)) {
                QueueUnitSpeech(unit, 7, "Reclamation failed");
                return 7;
            }
            if (!g_seams.units->CanReclaim(unit, 0, target)) {
                QueueUnitSpeech(unit, 7, "That unit cannot be reclaimed");
                return 8;
            }
            order->AnnounceStatusIfFlagged("Reclaiming");
            PrepVtolClimb(unit, order, 0);
            return 1;
        }
        return 7;
    case 1: {
        order->elapsed = ComputeReclaimDamagePulse(unit, target, 15);
        order->duration = 0;
        Point origin = *(Point*)&unit->footprint;
        Point cell = WorldToCell(order->pos, origin);
        CellToWorldPos(cell, &order->pos, origin);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, order->target->pos);
        order->SetAttachedFx(obj);
        order->flags |= 0x100e8;
        QueueUnitSpeech(unit, 11, 0);
        return 1;
    }
    case 2: {
        if (flags & 0x40)
            return 9;
        order->flags |= 0x10008;
        Vec3 delta = Sub(unit->pos, order->target->pos);
        int range = 0;
        range = unit->def->buildRange;
        int square = Square(delta);
        if (square <= range * range && g_seams.units->CanReclaim(unit, 0, order->target)) {
            if (order->duration >= 15) {
                g_seams.weapons->DamageUnit(unit, order->target, order->elapsed, 5, 0);
                order->duration = 0;
            }
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(order->source, &start);
            Vec3 bounds[2];
            bounds[1] = order->target->pos;
            bounds[0] = order->target->pos;
            bounds[0].x += order->target->def->modelMinX;
            bounds[0].z += order->target->def->modelMinZ;
            bounds[1].x += order->target->def->modelMaxX;
            bounds[1].z += order->target->def->modelMaxZ;
            bounds[1].y += order->target->def->modelMaxY;
            EmitReverseNanoParticles(bounds, &start, 6);
            order->SetDeadlineTicks(2);
            order->duration += 2;
            return 2;
        }
        order->SetDeadlineTicks(30);
        return 0;
    }
    }
    return 7;
}

#include "../../interfaces/seams.h"
// Decompiled by Claude Opus 5.5, finished by deepseek-v4.1-flash and GPT-6, verified by GPT-6.1-Sol, finished by space-bunny-free, edited by deepseek-v4.1-flash, edited by Claude Opus 5.5, matched by Claude Opus 5.5. Names are provisional.
// VTOL patrol order handler for construction aircraft. State 0 starts
// patrolling ("Patrolling"; PrepVtolClimb). State 1 sets the next waypoint,
// then lands on a free pad when damaged (VTOL_LANDING), helps build or guards
// a unit it can see (VTOL_HELPBUILD), or reclaims metal or energy
// (VTOL_RECLAIM).
#include <vector>

#include "../state/block.h"
#include "../util/vec3.h"

#include "mission_type.h"

struct Unit;
class LandingPadList : public std::vector<Unit*> {};

#include "unit_motion.h"

#pragma pack(push, 1)
#include "../units/unit_def.h"
struct PlayerView {
    char pad0[0x8c]; float energy;
    float GetEnergy() { return energy; }
    char pad90[8]; float metal;
    float GetMetal() { return metal; }
    char pad9c[8]; float energyCapacity, metalCapacity;
    char padac[0x146 - 0xac]; unsigned char index;
};
#include "../units/unit.h"
#include "order.h"
#include "path_fx.h"
#pragma pack(pop)

class RepairableUnitVisitor {
public:
    PlayerView* owner;
    std::vector<Unit*>* units;
    Unit* self;
    RepairableUnitVisitor(PlayerView* o, std::vector<Unit*>* v, Unit* s) : owner(o), units(v), self(s) {}
    virtual void CollectRepairableUnit(Unit*);
};

int __stdcall RandomInt(int);
void __stdcall EnsurePatrolReturnOrder(Unit*, Order*);
void __stdcall AppendOrder(Unit*, Order*);
int __stdcall IssueRepairOrder(Unit*, Unit*, int);
union Fixed { int v; struct { unsigned short frac; short whole; } p; };

// inline: no function may be compiled before VtolRepairPatrolOrder, or the landed test returns.
inline void __stdcall PrepVtolClimb(Unit* unit, Order* order, unsigned int flags)
{
    g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
    if (unit->carrier)
        g_seams.units->AttachUnitToPiece(unit, 0, -1, 2);
    g_seams.units->SetStateBits(unit, 1, 1);
    if ((((UnitMotion*)unit->motion)->flags & 3) == 1) {
        ((UnitMotion*)unit->motion)->SetFlightMode(unit, 2);
        Class_0044e2d0* obj = new (kSim) Class_0044e2d0((Order*)order, unit->pos);
        obj->SetAltitude(unit->def->altitude / 2);
        order->SetAttachedFx(obj);
        order->flags |= flags | 0xe0;
    }
}

static inline int Land(Unit* unit, Order* order)
{
    if ((unsigned int)unit->health < (unit->def->maxHealth >> 2) * 3) {
        LandingPadList pads;
        g_seams.ai->GetFactoriesInRadius(((PlayerView*)unit->player)->index, &unit->pos, 0xf00, &pads);
        if (!pads.empty()) {
            order->SetAttachedFx(0);
            Unit* pad = pads[RandomInt(pads.size())];
            AppendOrder(unit, new (kSim) Order("VTOL_LANDING", pad, 0, 0, 0, 0));
            order->flags = 0;
            return 1;
        }
    }
    // Empty statement after the health test folds the landed test away.
    do {} while (0);
    return 0;
}

static inline float Total(float base, float amount)
{
    float value = base;
    value += amount;
    return value;
}

// Stays in a file of its own: it matches only in this file's symbol context.
// FUNCTION: 0x4152f0
int __stdcall VtolRepairPatrolOrder(Unit* unit, Order* order, unsigned int flags)
{
    // flags is unsigned int: the repeated 0xe0 then keeps order in esi.
    if ((flags & 0x48) != 0) {
        order->SetDeadlineTicks(0x1e);
        return 0;
    }
    // The loop lets the four reclaim arms share the last arm's constructor tail.
    for (;;) {
        switch (order->state) {
        case 0:
            if (unit->motion && (unit->def->flags1 & 0x800) && (unit->def->flags2 & 0x200)) {
                if (order->target != 0)
                    order->pos = order->target->pos;
                EnsurePatrolReturnOrder(unit, order);
                order->AnnounceStatusIfFlagged("Patrolling");
                PrepVtolClimb(unit, order, 0);
                return 1;
            }
            break;
        case 1: {
            if (flags & 0xe0)
                return 6;
            Class_0044e2d0* obj = new (kSim) Class_0044e2d0(order, order->pos);
            obj->SetAltitude(unit->def->altitude);
            order->SetAttachedFx(obj);
            order->SetDeadlineTicks(0x2d);
            order->flags |= 0xe0;
            if (Land(unit, order))
                return 0;
            if (((PlayerView*)unit->player)->energy >= ((PlayerView*)unit->player)->energyCapacity * 0.2) {
                std::vector<Unit*> units;
                int range = unit->def->range << 16;
                RepairableUnitVisitor collector((PlayerView*)unit->player, &units, unit);
                g_seams.map->VisitObjectsInRange(&unit->pos, range, (UnitVisitor*)&collector);
                if (!units.empty()) {
                    Unit* target = units[RandomInt(units.size())];
                    if (g_seams.units->CanRepair(unit, 0, target) && target->buildLeft == 0.0f) {
                        if (IssueRepairOrder(unit, target, 0))
                            return 6;
                        return 3;
                    }
                    if (g_seams.units->CanRepair(unit, 0, target) && target->buildLeft != 0.0f) {
                        order->SetAttachedFx(0);
                        AppendOrder(unit, new (kSim) Order("VTOL_HELPBUILD", target, 0, 0, 0, 0));
                        order->flags = 0;
                        return 3;
                    }
                }
            }
            Vec3 energyPos, metalPos;
            Vec3* energy = &energyPos;
            Vec3* metal = &metalPos;
            float energyAmount, metalAmount;
            Fixed range;
            range.v = 0xf00000;
            if (g_seams.map->PickRandomReclaimableResourcesInRadius(&unit->pos, range.v, &energy, &energyAmount, &metal, &metalAmount)) {
                // Amounts read through GetEnergy/GetMetal and Total: gives the x87 load order.
                if (((PlayerView*)unit->player)->GetMetal() < ((PlayerView*)unit->player)->metalCapacity * 0.2 && metal) {
                    order->SetAttachedFx(0);
                    AppendOrder(unit, new (kSim) Order("VTOL_RECLAIM", 0, metal, 0, 0, 0));
                    order->flags = 0;
                    return 3;
                }
                if (((PlayerView*)unit->player)->GetEnergy() < ((PlayerView*)unit->player)->energyCapacity * 0.2 && energy) {
                    order->SetAttachedFx(0);
                    AppendOrder(unit, new (kSim) Order("VTOL_RECLAIM", 0, energy, 0, 0, 0));
                    order->flags = 0;
                    return 3;
                }
                if (metal && Total(((PlayerView*)unit->player)->GetMetal(), metalAmount) <= ((PlayerView*)unit->player)->metalCapacity) {
                    order->SetAttachedFx(0);
                    AppendOrder(unit, new (kSim) Order("VTOL_RECLAIM", 0, metal, 0, 0, 0));
                    order->flags = 0;
                    return 3;
                }
                if (energy && Total(((PlayerView*)unit->player)->GetEnergy(), energyAmount) <= ((PlayerView*)unit->player)->energyCapacity) {
                    order->SetAttachedFx(0);
                    AppendOrder(unit, new (kSim) Order("VTOL_RECLAIM", 0, energy, 0, 0, 0));
                    order->flags = 0;
                    return 3;
                }
            }
            return 2;
        }
        }
        return 7;
    }
}

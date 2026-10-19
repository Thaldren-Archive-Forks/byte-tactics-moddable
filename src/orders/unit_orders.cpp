// Decompiled by Claude Opus 5.5, GPT-6, GPT-6 Astra, GPT-6.1-sol, Opus, Sonnet,
// Claude Sonnet 5.5, claude-sonnet-5-5, space-bunny-free, DeepSeek V4.1 Flash,
// deepseek-v4.1-flash, deepseek-v4.1 and Haiku. Names are provisional.
// The ground order handlers the table at 0x4fc490 names, from Stop (0x401c20)
// to the std::vector<Unit*> destroy member at 0x406c00: the unit's order state
// machine, the mobile builders, capture, reclaim, resurrect, repair, standby,
// park, follow, transport and teleport handlers, with the order tables'
// registrations at the end. The unit's order object (Order) carries the state
// machine, the target and the position; the unit (Unit) and its type (UnitDef)
// carry the fields the handlers read and write.
// The handlers 0x403a20, 0x404ad0, 0x404db0, 0x405980, 0x406300 and 0x406aa0
// stay in files of their own: their register allocation follows symbol ids and
// views this file's context cannot give them. 0x401e00 and 0x403f70 are
// defined after the other handlers, ahead of the order tables. The
// vector members 0x406c10, 0x406c40 and 0x406c70 stay in theirs: the inline
// _Construct below makes the repair patrol call 0x406c70, which would then
// also change their own bytes, and 0x406c70 itself would be inlined into the
// patrol.

struct Unit;

void __stdcall CopyDwordIfNonNull(Unit** dest, Unit* const* src);
// The repair patrol's push_back inlines vector::insert, whose _Construct this
// overload replaces; it must be declared before <vector>.
namespace std {
inline void _Construct(Unit** dest, Unit* const& src) { CopyDwordIfNonNull(dest, &src); }
}

#include <vector>

#include "../state/block.h"
#include "../util/vec3.h"

// The whole part (high 16 bits) of a 16.16 coordinate: the fractional half the
// file's own Vec3 carried before the shared one.
static inline short Whole(const int& v) { return ((short*)&v)[1]; }

static inline Vec3 Vec3Add(const Vec3& a, const Vec3& b) {
    Vec3 r; r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; return r;
}

static inline Vec3 Vec3Sub(const Vec3& a, const Vec3& b) {
    Vec3 r; r.z = a.z - b.z; r.y = a.y - b.y; r.x = a.x - b.x; return r;
}

#include "box.h"

struct UnitDef;

#include "../units/cob_script.h"

// Unused here: this header and these forward declarations take the symbol ids that keep
// BuildWeaponOrder (0x402b70) and the order handlers after it matching (docs/c2-regalloc.md).
#include "../sound/sound.h"
struct FrameTable;
struct Chunk;
class SquadManager;
class AssaultTimer;
class EscortTimer;
class SquadScoutTimer;


#include "../game/unit_resources.h"

#include "mission_type.h"

#include "path_order_attach.h"

#pragma pack(push, 1)
#include "../units/unit_def.h"

// Keeps its own view: the declarations of player.h move the symbol ids that 0x402b70, 0x404730 and 0x405d90 match at.
#include "../network/player.h"

#include "../weapons/weapon_def.h"

#include "../game/game.h"

struct CloakDefView {
    char unknown_0[0x245];
    unsigned int lowbits : 13;
    unsigned int flag : 1;
};

struct Sub_403010 {
    char unknown_0[0x245];
    unsigned int unused_bits : 2;
    unsigned int flag : 1;             // +0x245, bit 2
    unsigned int unused_bits2 : 29;
};

#include "../util/angles.h"
// This header's symbol ids replace those of the Weapon view and of the
// declarations dropped above and below (docs/c2-regalloc.md).
#include "../weapons/unit_weapon_slot.h"

#include "order.h"
// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
int __stdcall FindWeaponTarget(Unit*, unsigned char, int);
void __stdcall ReactToAttack(Unit*, Unit*, int);
void __stdcall ScaleUnitWeights(int, unsigned int*, float, int);
void __stdcall SetUnitLimits(int, unsigned int*, int, int);
void __stdcall BuildGameInfo(char*, int*, int*, int*, int*);
void __stdcall BuildEntryGuiName(char*, unsigned short, int);
char* __stdcall BuildScrollItems1(char*, char*, int);
char* __stdcall BuildScrollItems2(char*, int, int);
int __stdcall GetBuilderCount(int);
int __stdcall GetUnitCount(int, unsigned int);
int __stdcall DirectionFromStep(int, int);

#include "../units/unit.h"
inline int Unit::Ready() { return (flags & 0x10000000) && !(flags & 0x4000); }
#pragma pack(pop)

inline int Order::Advance(int distance) {
    // The cast stays: without it BuildWeaponOrder (0x402b70) compiles to different code.
    ((Order*)this)->AttachApproachRadiusGoal(&target->pos, distance);
    step++;
    return 1;
}

#include "../../interfaces/seams.h"

extern Game* g_game;

void __stdcall RegisterOrderTypes(void* table, int id);
void __stdcall AppendOrder(Unit* owner, Order* node);
int __stdcall RandomInt(int range);
MissionType __stdcall GetOrderType(unsigned char mode, Unit* unit, Unit* target, void* flags);
void __stdcall QueueUnitSpeech(Unit* unit, int kind, const char* text);
void __stdcall UpdateBuildMenuIfFocusUnit(Unit* unit);
int __stdcall WaitIfNotInBuildStance(Unit* unit, Order* order, int flags);
Unit* __stdcall CreateUnit(unsigned char player, unsigned short type, Vec3 pos,
                           int a, int b, int c);
void __stdcall AddOrder(MissionType kind, int remove, Unit* owner, Unit* id, void* pos,
                        int typeId, int amount);
void __stdcall EmitNanoParticles(Vec3* from, Box* to, int count);
void __stdcall MarkSelectionOrdersDirty(Unit* unit);
void __stdcall EnsurePatrolReturnOrder(Unit* unit, Order* order);
int __stdcall IssueAttackOrder(Unit* unit, Order* order, int param);

// Unused here: the symbol ids these declarations take keep the allocation of
// BuildWeaponOrder (0x402b70) and the handlers after it (docs/c2-regalloc.md).
void EnableAICommands();
void RegisterAICommands();
void FUN_00406f40();
void ResetAIPlayers();
void RegisterVtolOrders();
void StepAllGafSequences();
void ResetNetStats();
void FUN_004161f0();
void InitCommands();
int UpdatePlacementGhostValidity();
void RefreshSelectionOrders();
void DispatchOrdersPanelPageFlags();
void ResetCameraState();
void FindLocalCommander();
void __cdecl ClearCameraFollowState();
void ClampCameraPosition();
void ClampCameraTarget();
void UpdateScreenShake();
void UpdateCameraFollow();
void BeginMouseScroll();
void EndMouseScroll();
void UpdateMouseScroll();
void UpdateEdgeScroll();
void CenterCameraOnRadarClick();
void CenterCameraOnStartPosition();
void RegisterDataArchives();
int FUN_0041d8a0();
int GetCdPathMismatch();
void CreateGameObject();
void InitMissionStatus();
void SetUpEndMissionScreen();

// Handler of the "Stopping" entry in the order table at 0x4fc490: stops the
// unit and, for a VTOL that is flying, queues a VTOL_LANDIFCAN order.
// FUNCTION: 0x401c20
int __stdcall StopOrder(Unit* unit, Order* order, int unused)
{
    order->AnnounceStatusIfFlagged(0);
    g_seams.targeting->ClearWeaponTarget(unit, 0);
    g_seams.targeting->ClearWeaponTarget(unit, 1);
    g_seams.targeting->ClearWeaponTarget(unit, 2);
    if ((unit->flags & 3) == 2 && (unit->def->flags1 & 0x800)) {
        AppendOrder(unit, new (kSim) Order("VTOL_LANDIFCAN", 0, &unit->pos, 0, 0, 0));
    }
    return 5;
}

// An order handler from the same table as 0x401c20: updates two unit flag
// bits and returns 5, like its neighbour.
// FUNCTION: 0x401cc0
int __stdcall MakeSelectableOrder(Unit* unit, Order* order, int unused)
{
    unit->flags = (unit->flags & ~0x8000) | 0x20;
    return 5;
}

// FUNCTION: 0x401ce0
int __stdcall WaitOrder(Unit* unit, Order* order, int flags)
{
    if(order->radius) {
        std::vector<Unit*> units;
        g_seams.ai->GetVisibleEnemiesInRadius(unit->playerIndex,&unit->pos,order->radius,0,&units);
        if(!units.empty()) return 5;
        if(order->wait<=0) return 5;
        int delay=RandomInt(30)+150;
        order->wait-=delay;
        order->SetDeadlineTicks(delay);
        return 2;
    }
    unsigned state=0;
    state=order->state;
    switch(state) {
    case 0: order->SetDeadlineTicks(order->wait); return 1;
    case 1: return 5;
    default: return 7;
    }
}

// 0x401e00 is defined after the other handlers (its place would move the
// symbol ids of the ones after it); this declaration keeps their ids.
struct PadE_00401e00_0 { int field; };

// FUNCTION: 0x401fd0
int __stdcall WaitForAttackOrder(int unused1, Order* obj, int unused3)
{
    if (obj->target == 0) {
        return 5;
    }
    unsigned int s = 0;
    s = obj->state;
    switch (s) {
    case 0:
        obj->flags = 0x18;
        return 1;
    case 1:
        return 5;
    default:
        return 7;
    }
}

// FUNCTION: 0x402010
int __stdcall SelfDestructOrder(Unit* unit, Order* order, int flags)
{
    if(!(order->count&0xf0000000)) order->count=unit->def->selfdestructcountdown|0xf0000000;
    if(!order->done && unit->def->selfdestructcountdown>0) {
        int sounds[6]={22,21,20,19,18,17};
        if(flags&2) {
            if(!(unit->flags&0x4000)) { QueueUnitSpeech(unit,23,0); return 5; }
        } else {
            int count=order->count&0x0fffffff;
            if(!count) order->done=1;
            else order->count=(count-1)|0xf0000000;
            if(count>=0) {
                QueueUnitSpeech(unit,sounds[count],0);
                if(count>0) {
                    order->SetDeadlineTicks(30);
                    order->flags|=2; return 1;
                }
                if(count==0) {
                    order->SetDeadlineTicks(RandomInt(15));
                    order->flags|=2; return 1;
                }
            }
            g_seams.weapons->DamageUnit(unit,unit,30000,3,0);
        }
    } else g_seams.weapons->DamageUnit(unit,unit,30000,3,0);
    return 5;
}

// FUNCTION: 0x402160
int __stdcall AttackNoMoveOrder(Unit* unit, Order* order, int flags)
{
    if (order->target == 0 || (flags & 0x10808) != 0)
        return 5;
    switch (order->state) {
    case 0:
        order->AnnounceStatusIfFlagged(0);
        return 1;
    case 1:
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        g_seams.targeting->SetWeaponTargetUnit(unit, order->target, 0);
        order->flags = 0x11808;
        return 1;
    case 2:
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        return 9;
    default:
        return 7;
    }
}

// Order handler: follows the order's target unit, waits a random number of
// ticks near it and, when interrupted, picks a random unit around the
// remembered position (GetVisibleEnemiesInRadius fills a vector) as the new target.
// FUNCTION: 0x4021f0
int __stdcall GuardNoMoveOrder(Unit* unit, Order* order, int flags)
{
    if (flags & 0x10008) {
        order->state = 3;
        return 2;
    }
    switch (order->state) {
    case 0:
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        order->SetDeadlineTicks(0x1e);
        return 1;
    case 1: {
        order->link.SetUnit(g_seams.targeting->GetWeaponTargetUnit(unit, 0));
        Unit* t = order->target;
        if (t != 0 && (t->flags & 0x10000000)) {
            order->pos = t->pos;
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
            g_seams.targeting->SetWeaponTargetUnit(unit, order->target, 0);
            order->wait = 0;
            order->waitLimit = RandomInt(3) + 3;
            return 1;
        }
        order->SetDeadlineTicks(0x1e);
        return 2;
    }
    case 2:
        if (flags & 0x4000)
            order->wait = 0;
        else
            order->wait++;
        if (order->wait <= order->waitLimit && g_seams.weapons->WeaponCanReachUnit(unit, order->target, 0)) {
            order->flags |= 0x7008;
            return 2;
        }
        if (RandomInt(100) < 0x50) {
            order->wait = 0;
            return 1;
        }
        break;
    case 3: {
        std::vector<Unit*> units;
        g_seams.ai->GetVisibleEnemiesInRadius(unit->playerIndex, &order->pos, 0x280, 0, &units);
        if (!units.empty()) {
            order->link.SetUnit(units[RandomInt(units.size())]);
            g_seams.targeting->SetWeaponTargetUnit(unit, order->target, 0);
            order->state = 1;
            return 2;
        }
        break;
    }
    default:
        return 7;
    }
    return 0;
}

// 0x402430 stays defined before 0x402640: the constant pool order follows it.
// Keep a header include: without one the register choices change.
#include <stdio.h>

// Order handler "Repairing": the order's target unit (the builder) repairs
// `unit`, spending its worker time and drawing nano particles from its nano
// piece to the unit's bounding box.
// FUNCTION: 0x402430
int __stdcall SelfRepairOrder(Unit* unit, Order* order, int unused)
{
    if (order->target == 0) {
        QueueUnitSpeech(unit, 7, "Repair aborted.");
        return 8;
    }
    switch (order->state) {
    case 0:
        if (!(order->target->def->flags1Byte & 0x40))
            return 7;
        if (order->target->buildLeft == 0.0f && (unit->activateFlags & 1)) {
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
            return 1;
        }
        return 8;
    case 1:
        if ((unsigned int)unit->health >= unit->def->maxHealth)
            return 1;
        unit->workTime = g_game->ticks + 0x96;
        if (g_seams.units->AddRepairProgress(order->target, unit, (float)(order->target->def->buildRate / 30))) {
            Vec3 nano;
            g_seams.units->GetNanoPiecePosition(order->target, &nano);
            Box box;
            box.hi = unit->pos;
            box.lo = unit->pos;
            box.lo.x += unit->def->modelMinX;
            box.lo.z += unit->def->modelMinZ;
            box.hi.x += unit->def->modelMaxX;
            box.hi.z += unit->def->modelMaxZ;
            box.hi.y += unit->def->modelMaxY;
            EmitNanoParticles(&nano, &box, 6);
        }
        order->SetDeadlineTicks(1);
        order->flags |= 8;
        return 2;
    case 2:
        QueueUnitSpeech(unit, 10, "Unit repaired");
        return 5;
    default:
        return 7;
    }
}

// World position (16.16 fixed point) to the map cell of the top-left corner
// of a footprint centred there.
static inline Point16 GridCell(Vec3 pos, Point16 size)
{
    Point16 cell;
    cell.x = (pos.x - (size.x << 19) + 0x80000) >> 20;
    cell.y = (pos.z - (size.y << 19) + 0x80000) >> 20;
    return cell;
}

// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
void StartScreenFade();
void StepScreenFade();

// Order handler "Nanolathing" of a mobile builder: places the unit to build
// at the script's build piece, then spends worker time on it. When the order
// is cancelled, the metal already spent is refunded (only half or 70% of it
// for AI players on some difficulty settings).
// FUNCTION: 0x402640
int __stdcall BuildingBuildOrder(Unit* unit, Order* order, int flags)
{
    if (flags & 2) {
        if (order->target != 0) {
            float refund = (unsigned int)((1.0f - order->target->buildLeft) * order->target->def->metalCost);
            // The (double) casts on the full refund stay: they keep refund off the FP stack.
            if (unit->resourceSlot.player->active && unit->resourceSlot.player->type == 2) {
                switch (g_game->difficulty) {
                case 1:
                    unit->resourceSlot.metalMake += refund * 0.7;
                    break;
                case 0:
                    unit->resourceSlot.metalMake += refund * 0.5;
                    break;
                default:
                    unit->resourceSlot.metalMake += (double)refund;
                    break;
                }
            } else {
                unit->resourceSlot.metalMake += (double)refund;
            }
            g_seams.units->FinishConstruction(unit, order->target);
            g_seams.weapons->DamageUnit(unit, order->target, 30000, 9, 0);
        }
        g_seams.units->SetStateBits(unit, 9, 0);
        UpdateBuildMenuIfFocusUnit(unit);
        return 5;
    }
    if (flags & 8) {
        QueueUnitSpeech(unit, 7, "Construction stopped");
        order->count--;
        UpdateBuildMenuIfFocusUnit(unit);
        return 0;
    }
    switch (order->state) {
    case 0:
        order->link.SetUnit(0);
        if (unit->flags & 0x20000000) {
            if (order->count <= 0) {
                g_seams.units->SetStateBits(unit, 1, 0);
                return 5;
            }
            g_seams.units->SetStateBits(unit, 1, 1);
            return 1;
        }
        break;
    case 1:
        return WaitIfNotInBuildStance(unit, order, 2);
    case 2: {
        int piece = -1;
        g_seams.units->QueryScript((CobScript*)unit->script, "QueryBuildInfo", &piece, 0, 0, 0);
        order->pos = g_seams.units->GetPiecePosition(unit, piece);
        UnitDef* ut = &g_game->defs[order->unitType];
        Point16 cell = GridCell(order->pos, *(Point16*)&ut->footprintx);
        if (!g_seams.map->CanPlaceUnitFootprint(ut, 0, cell, unit->flags & 3)) {
            order->SetDeadlineTicks(15);
            order->flags |= 2;
            return 2;
        }
        order->link.SetUnit(g_seams.units->CreateUnit(unit->playerIndex, order->unitType, order->pos, 0, 1, 0));
        if (order->target == 0) {
            QueueUnitSpeech(unit, 7, "Unable to create any more units");
            order->SetDeadlineTicks(300);
            order->flags |= 2;
            return 2;
        }
        QueueUnitSpeech(unit, 9, "Starting construction");
        g_seams.units->AttachUnitToPiece(order->target, unit, piece, 1);
        order->target->moveOrder = unit->moveOrder;
        order->target->fireOrder = unit->fireOrder;
        AddOrder("getbuilt", 1, order->target, unit, 0, 0, 0);
        g_seams.units->SetStateBits(unit, 8, 1);
        UpdateBuildMenuIfFocusUnit(unit);
        return 1;
    }
    case 3:
        if (order->target != 0) {
            if (g_seams.units->AddBuildProgress(unit, order->target, (float)(unit->def->buildRate / 30))) {
                Vec3 nano;
                g_seams.units->GetNanoPiecePosition(unit, &nano);
                Box box;
                box.hi = order->target->pos;
                box.lo = order->target->pos;
                box.lo.x += order->target->def->modelMinX;
                box.lo.z += order->target->def->modelMinZ;
                box.hi.x += order->target->def->modelMaxX;
                box.hi.z += order->target->def->modelMaxZ;
                box.hi.y += order->target->def->modelMaxY;
                EmitNanoParticles(&nano, &box, 6);
            }
            if (order->target->buildLeft != 0.0f) {
                order->SetDeadlineTicks(1);
                order->flags |= 0xa;
                return 2;
            }
            return 1;
        }
        break;
    case 4:
        QueueUnitSpeech(unit, 8, 0);
        g_seams.units->SetStateBits(unit, 8, 0);
        g_seams.units->FinishConstruction(unit, order->target);
        order->link.SetUnit(0);
        order->count--;
        UpdateBuildMenuIfFocusUnit(unit);
        return 0;
    }
    return 7;
}

// Order handler "Nanolathing" of a building that stockpiles weapons (the
// "BuildingBuild" entry of the order table at 0x4fc490): builds `count`
// rounds for weapon `weapon`, 5 ticks of build time per step, paying the
// energy and metal share of each step (energy first, as RequestEnergyAndMetal takes
// them), up to 200 stockpiled rounds.
// FUNCTION: 0x402b70
int __stdcall BuildWeaponOrder(Unit* unit, Order* order, int unused)
{
    // energyCharge's declaration stays at function scope: the frame slots of
    // the locals below follow it.
    int energyCharge;
    WeaponDef* t = unit->weapons[order->weapon].weapon;
    switch (order->state) {
    case 0:
        if (order->count <= 0)
            return 5;
        if (unit->weapons[order->weapon].stockpile >= 200) {
            order->SetDeadlineTicks(300);
            return 2;
        }
        order->progress = 0;
        return 1;
    case 1: {
        // The float locals declared next, total, prev and the ints after them:
        // fixes the fild order; total stays after the ternary.
        float fnext, ftotal, fprev;
        int prev = order->progress;
        int next = prev + 5 < t->reloadTime ? prev + 5 : t->reloadTime;
        int total = t->reloadTime;
        fnext = next;
        ftotal = total;
        fprev = prev;
        int metalCharge = (int)(fnext * t->metalPerShot / ftotal) - (int)(fprev * t->metalPerShot / ftotal);
        energyCharge = (int)(fnext * t->energyPerShot / ftotal) - (int)(fprev * t->energyPerShot / ftotal);
        if (g_seams.economy->RequestEnergyAndMetal(&unit->resourceSlot, energyCharge, metalCharge)) {
            order->progress = next;
            if (next >= t->reloadTime)
                return 1;
            order->SetDeadlineTicks(5);
            return 2;
        }
        order->SetDeadlineTicks(10);
        return 2;
    }
    case 2:
        unit->weapons[order->weapon].stockpile++;
        order->count--;
        UpdateBuildMenuIfFocusUnit(unit);
        return 0;
    default:
        return 7;
    }
}

// Order handler: waits for the order's time (at most 1800 ticks).
// A char loop counter gives the separate countdown register (ebx = 3).
// FUNCTION: 0x402d10
int __stdcall ParalyzeOrder(Unit* unit, Order* order, int unused)
{
    if (order->ticks == 0) {
        g_seams.units->SetStateBits(unit, 0x10, 0);
        return 5;
    }
    if (order->ticks > 0x708)
        order->ticks = 0x708;
    g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
    for (char i = 0; i < 3; i++)
        g_seams.targeting->ClearWeaponTarget(unit, i);
    order->SetAttachedFx(0);
    order->SetDeadlineTicks(order->ticks);
    order->ticks = 0;
    g_seams.units->SetStateBits(unit, 0x10, 1);
    return 1;
}

// MissionType's operator== compares the index bytes, so the loop head is
// `cmp cl, dl` and `order` lands in ebp, `queued` in ebx. Order::Target() is
// a trivial inline accessor: every `order->Target()->X` in the flag-merge
// tail goes through it, and the temporary it adds shifts the rotation of the
// scratch registers back into phase.
// FUNCTION: 0x402da0
int __stdcall GetBuiltOrder(Unit* unit, Order* order, unsigned int flags)
{
    if (unit->buildLeft == 0.0f) {
        MarkSelectionOrdersDirty(unit);
        if (unit->motion) {
            int queued = 0;
            if (order->target) {
                MissionType move("QMove");
                MissionType patrol("QPatrol");
                for (Order* node = order->target->list; node; node = node->next) {
                    MissionType kind;
                    if (node->kind == move)
                        kind = GetOrderType(2, unit, 0, node->Position());
                    else if (node->kind.index == patrol.index)
                        kind = GetOrderType(9, unit, 0, node->Position());
                    if (kind.index) {
                        AddOrder(kind, 1, unit, 0, node->Position(), 0, 0);
                        queued = 1;
                    }
                }
                if (unit->Ready() && order->Target()->Ready()) {
                    unit->moveOrder = order->Target()->moveOrder;
                    unit->fireOrder = order->Target()->fireOrder;
                    if (unit->player->active && unit->player->type == 1)
                        unit->group = order->Target()->group;
                }
            }
            if (!queued)
                AddOrder("PARK", 1, unit, 0, 0, 0, 0);
        }
        return 5;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
        order->SetDeadlineTicks(300);
        order->Wait();
        return 1;
    case 1:
        order->SetDeadlineTicks(30);
        order->Wait();
        return 1;
    case 2:
        if (flags & 0x8000) {
            order->SetDeadlineTicks(30);
        } else if (flags & 1) {
            order->SetDeadlineTicks(11);
            g_seams.units->ApplyUnfinishedBuildDecay(unit, 11);
        }
        order->Wait();
        return 2;
    default:
        return 7;
    }
}

// Order handler: state 0 calls ClaimWeapons(3) on the unit, state 1 waits
// ten ticks.
// FUNCTION: 0x402fc0
int __stdcall BeCarriedOrder(Unit* unit, Order* order, int unused)
{
    if (unit->carrier == 0) {
        return 5;
    }
    switch (order->state) {
    case 0:
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
        return 1;
    case 1:
        order->SetDeadlineTicks(10);
        return 2;
    default:
        return 7;
    }
}

// FUNCTION: 0x403010
int __stdcall ActivateOrder(Unit* unit, int unused1, int unused2)
{
    if (((Sub_403010*)unit->def)->flag) {
        g_seams.units->SetStateBits(unit, 1, 1);
    }
    return 5;
}

// FUNCTION: 0x403040
int __stdcall DeactivateOrder(Unit* unit, int unused1, int unused2)
{
    if (((Sub_403010*)unit->def)->flag) {
        g_seams.units->SetStateBits(unit, 1, 0);
    }
    return 5;
}

// FUNCTION: 0x403070
int __stdcall CloakOnOrder(char* param1, int unused1, int unused2)
{
    CloakDefView* p = *(CloakDefView**)(param1 + 0x92);
    if (p->flag) {
        *(unsigned int*)(param1 + 0x110) |= 0x800;
    }
    return 5;
}

// FUNCTION: 0x4030a0
int __stdcall CloakOffOrder(char* unit, int unused1, int unused2)
{
    CloakDefView* p = *(CloakDefView**)(unit + 0x92);
    if (p->flag) {
        *(unsigned int*)(unit + 0x110) &= ~0x800;
    }
    return 5;
}

// FUNCTION: 0x4030d0
int __stdcall StandingMoveOrder(Unit* unit, Order* order, int unused)
{
    unit->moveOrder = order->mode;
    return 5;
}

// A char loop counter gives the separate countdown register (ebx = 3).
// FUNCTION: 0x403100
int __stdcall StandingFireOrder(Unit* unit, Order* order, int unused)
{
    unit->fireOrder = order->mode;
    if (order->mode == 0 || order->mode == 1) {
        for (char i = 0; i < 3; i++) {
            if (unit->weapons[i].flags & 0x10) {
                g_seams.targeting->ClearWeaponTarget(unit, i);
            }
        }
    }
    return 5;
}

// FUNCTION: 0x403160
int __stdcall QMoveQPatrolOrder(int unused1, void* order, int unused3)
{
    ((Order*)order)->SetDeadlineTicks(0x3c);
    return 6;
}

struct Unit;
struct Order;
struct View;
struct Pos;

#pragma pack(push, 1)
struct UnitOrderType {
    const char* status;
    int (__stdcall* run)(Unit* unit, Order* order, int flags);
    void (__stdcall* draw)(void* surface, View* view, Order* order, Pos* out, int flag);
    int target;
    unsigned int flags;
    unsigned char internal;
    const char* name;
};
#pragma pack(pop)

// Registers a table with RegisterOrderTypes under a numeric id; one of several
// small functions doing the same for different tables.
// FUNCTION: 0x403180
void RegisterUnitOrders()
{
    // The name (the definition below spells it too) is what tools/progress.py
    // learns the table's address from.
#ifdef __clang__
    extern const UnitOrderType g_unitOrders[23];
    RegisterOrderTypes((void*)g_unitOrders, 0x17);
#else
    extern char g_unitOrders[];
    RegisterOrderTypes(g_unitOrders, 0x17);
#endif
}

// Order handler: asks GetOrderType for the next order kind (returned as a
// MissionType by value), passes it on by value (the 0x406240 call site
// builds the same argument in place) and returns state 2.
// FUNCTION: 0x403190
int __stdcall AttackSpecialOrder(Unit* unit, Order* order, int unused)
{
    order->MergeFlagsFromTable(GetOrderType(3, unit, order->target, 0).index);
    order->weapon = 2;
    return 2;
}

// Order handler: state 0 (only when the unit's +0x86 is clear) resets the
// order and calls AttachApproachRadiusGoal with its position; state 1 posts message 6
// when bit 0x20 of the third argument is set.
// FUNCTION: 0x4031d0
int __stdcall MoveGroundOrder(Unit* unit, Order* order, int flags)
{
    switch (order->state) {
    case 0:
        if (unit->carrier != 0)
            return 7;
        order->AnnounceStatusIfFlagged(0);
        order->AttachApproachRadiusGoal(&order->pos, order->approachRadius + 4);
        order->flags = 0xe0;
        return 1;
    case 1:
        if (flags & 0x20) {
            QueueUnitSpeech(unit, 6, 0);
            return 5;
        }
        return 9;
    default:
        return 7;
    }
}

// FUNCTION: 0x403260
int __stdcall AttackKamikazeOrder(Unit* unit, Order* order, unsigned flags)
{
    if(flags&0x10008) return 5;
    if(order->target) order->pos=order->target->pos;
    unsigned state=0; state=order->state;
    switch(state) {
    case 0:
        if(unit->carrier) return 7;
        order->AnnounceStatusIfFlagged(0);
        order->AttachApproachRadiusGoal(&order->pos,(unsigned short)unit->def->kamikazedistance<16?16:(unsigned short)unit->def->kamikazedistance);
        order->SetDeadlineTicks(60);
        order->flags|=0xe0; return 1;
    case 1:
        if(flags&0x20) {
            QueueUnitSpeech(unit,6,0);
            AppendOrder(unit,new (kSim) Order("SELFDESTRUCT",0,0,1,0,0));
            return 5;
        }
        if(flags&0x40) return 8;
        order->state=0; return 2;
    default: return 7;
    }
}

// FUNCTION: 0x4033a0
int __stdcall PatrolOrder(Unit* unit, Order* order, int flags)
{
    unsigned state=0; state=order->state;
    switch(state) {
    case 0:
        if(!unit->motion) return 7;
        order->AnnounceStatusIfFlagged(0);
        EnsurePatrolReturnOrder(unit,order);
        order->SetDeadlineTicks(1); return 1;
    case 1:
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        order->AttachApproachRadiusGoal(&order->pos,0);
        order->SetDeadlineTicks(15);
        order->flags|=0xe0; return 1;
    case 2:
        if(flags&0xe0) { order->state=1; return 6; }
        {
            Order* next=(Order*)g_seams.targeting->FindBestTargetIfFireAtWill(unit);
            if(next && IssueAttackOrder(unit,next,0)) { order->flags=0; order->state=1; return 3; }
        }
        order->SetDeadlineTicks(RandomInt(30)+30);
        order->state=1; return 4;
    default: return 7;
    }
}

// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
struct Pad_00400000_0 { int field; };
struct Pad_00400000_1 { int field; };
struct Pad_00400000_2 { int field; };
struct Pad_00400000_3 { int field; };
struct Pad_00400000_4 { int field; };
struct Pad_00400000_5 { int field; };
struct Pad_00400000_6 { int field; };
struct Pad_00400000_7 { int field; };

char* __stdcall lstrcpynA(char* dest, const char* src, int count);
#include <math.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

union Fixed {
    int value;
    struct {
        unsigned short fraction;
        short whole;
    };
};

// Unused here: the symbol ids this declaration takes keep the allocation (docs/c2-regalloc.md).
int DrawWrappedText(char*, char*, int, int, int, int, int);

#include "../map/feature.h"

struct FeatureSpot {
    char unknown_0[0x20];
    Angles16 rot;                      // +0x20
    char unknown_26[0x30 - 0x26];
};

#include "../map/cell.h"

#include "../map/mission.h"

struct FeatureDamagePacket {
    unsigned char type;
    unsigned char sub;
    short x;
    short z;
};

class DamagedAllyCollector {
public:
    Player* owner;
    std::vector<Unit*>* units;
    Unit* self;
    DamagedAllyCollector(Player* o, std::vector<Unit*>* v, Unit* s) : owner(o), units(v), self(s) {}
    virtual void CollectDamagedAlly(Unit*);
};

// The original keeps part 2's 0.0f at 0x4fc920, after part 1's at 0x4fc6d0;
// the named constant keeps the two apart (a shared literal would sit at one).
static const float Zero_004fc920 = 0.0f;

// Unused here: the symbol ids this declaration takes keeps the allocation (docs/c2-regalloc.md).
extern char DAT_004fc6e8[];
extern const float DAT_004fc930, DAT_004fc934, DAT_004fc938, DAT_004fc93c;

unsigned short __stdcall FindFeatureAtPos(Vec3* pos, Point16* cell, Point16* size);
int __stdcall GetHeadingBetween(Vec3* from, Vec3* to);
void __stdcall StartBuildingScript(Unit* unit, Order* order, short turn);
void __stdcall StopBuildingScript(Unit* unit, Order* order);
int __stdcall WaitIfCobBusy(Unit* unit, Order* order, int flags);
int __stdcall ComputeReclaimDamagePulse(Unit* unit, Unit* target, int flags);
int __stdcall IssueRepairOrder(Unit* unit, Unit* target, int param);
void __stdcall EmitNanoParticles(Vec3* from, Vec3* to, int count);
void __stdcall EmitReverseNanoParticles(Box* from, Vec3* to, int count);
void __stdcall EmitReverseNanoParticles(Vec3* from, Vec3* to, int count);
void __stdcall ReclaimFeature(Unit* unit, Vec3* pos);
Cell* __stdcall GetOriginCellAtPosition(Vec3* pos);
unsigned short __stdcall GetCellFeature(Cell* cell);
Cell* __stdcall GetMapCellAtPosition(Vec3* pos);
void __stdcall RemoveFeature(void* target, int flag);
int __stdcall BroadcastPacket(int player, void* data, int size);
int __stdcall PickRandomReclaimableResourcesInRadius(Vec3* pos, int range, Vec3** energy, float* energyAmount,
                           Vec3** metal, float* metalAmount);
int __cdecl FUN_004b70ef(int angle, int distance);
int __cdecl FUN_004b7123(int angle, int distance);
void __stdcall EmitTeleportParticles(Vec3* from, Vec3* to, int count, int param);
// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
int __stdcall IsUnitCommander(Unit*);
void __stdcall AlignUnitToGround(Unit*);
int __stdcall FindLandingPad(Unit*, int);
unsigned short __stdcall ChooseBuildOption(unsigned int, Unit*);
void __stdcall DetonateUnitWeapon(Unit*, int);
// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
void WriteScreenshot(char*, char*, int, int, int, int);
void DrawTextClipped(void*, char*, int, int, int, int);
int ScanDirectory(char*, char*, char*, int, int, int);
int CheckDirectXVersion(int, int, int, int, int);

// Order handler: attack-move style state machine. The inner step switch
// advances order->step after cases 6 and 7 in one `order->step++` after the
// switch; cases 0 and 5 go through the inline Advance(), so MSVC merges only
// those two (at the pushed distance) and keeps case 6 separate.
// FUNCTION: 0x4034a0
int __stdcall AttackChaseOrder(Unit* unit, Order* order, unsigned int flags)
{
    int weapon = order->weapon;
    if ((flags & 0x800) || !order->target || (flags & 0x10008)) return 5;
    if (order->range && (int)_hypot(Whole(unit->pos.x) - order->start.x,
                                    Whole(unit->pos.z) - order->start.y) >= order->range)
        return 5;
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (!unit->motion || unit->def->flying || !(unit->flags & 0x80000000)) break;
        order->AnnounceStatusIfFlagged(0);
        order->pos = unit->pos;
        order->step = 0;
        if (!weapon) order->weapon = g_seams.targeting->ChooseWeapon(unit);
        return 1;
    case 1:
        order->SetAttachedFx(0);
        if (flags & 0x3000) return 1;
        if (!g_seams.weapons->WeaponCanReachUnit(unit, order->target, weapon)) return 1;
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 2);
        g_seams.targeting->SetWeaponTargetUnit(unit, order->target, weapon);
        order->flags = 0x13808;
        return 2;
    case 2:
        weapon = g_seams.weapons->GetWeaponRange(unit, weapon);
        switch (order->step) {
        case 0:
            return order->Advance(weapon);
        case 1: case 2: case 3: case 4:
            if (abs(unit->pos.y - order->target->pos.y) > 0x80000) {
                order->AttachApproachRadiusGoal(&order->target->pos, weapon / 2);
                order->step = 6;
                return 1;
            } else {
                int angle = GetHeadingBetween(&order->target->pos, &unit->pos);
                angle += RandomInt(0x8000) - 0x4000;
                int distance = weapon << 16;
                int dx = -FUN_004b70ef(angle, distance);
                int dz = -FUN_004b7123(angle, distance);
                Vec3* target = &order->target->pos;
                Vec3 offset = { dx, 0, dz };
                Vec3 pos = Vec3Add(*target, offset);
                order->AttachApproachRadiusGoal(&pos, weapon / 4);
                return 1;
            }
        case 5:
            return order->Advance(weapon / 2);
        case 6:
            order->AttachApproachRadiusGoal(&order->target->pos, 0);
            break;
        case 7:
            order->AttachRingApproachGoal(&order->target->pos, weapon, weapon / 2);
            break;
        case 8:
            order->AttachRingApproachGoal(&order->target->pos, weapon * 2, weapon);
            order->step = 0;
            return 1;
        default: return 7;
        }
        order->step++;
        return 1;
    case 3:
        if (flags & 0x40e0) { order->state = 1; return 4; }
        if (g_seams.weapons->WeaponCanReachUnit(unit, order->target, weapon)) {
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 2);
            g_seams.targeting->SetWeaponTargetUnit(unit, order->target, weapon);
            order->flags = 0x148e8;
            order->SetDeadlineTicks(30);
            return 2;
        }
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        order->flags = 0x100e8;
        order->SetDeadlineTicks(30);
        return 2;
    }
    return 7;
}

// FUNCTION: 0x4038a0
int __stdcall SuppressOrder(Unit* unit,Order* order,unsigned flags)
{
    if(flags&0x800) return 5;
    unsigned state=0; state=order->state;
    switch(state) {
    case 0:
        if(unit->def->flying) return 8;
        order->AnnounceStatusIfFlagged(0);
        order->radius=g_seams.weapons->GetWeaponRange(unit,(unsigned char)order->weapon); return 1;
    case 1:
        if(order->weapon==2) {
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
            g_seams.targeting->SetWeaponTargetPos(unit,&order->pos,2);
            order->flags=0x1c00; return 1;
        }
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 0);
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 1);
        g_seams.targeting->SetWeaponTargetPos(unit,&order->pos,0);
        g_seams.targeting->SetWeaponTargetPos(unit,&order->pos,1);
        order->flags=0x1c00; return 1;
    case 2:
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        if(flags&0x400) { order->state=1; return 6; }
        if(unit->motion) {
            if(order->radius<=0) return 9;
            order->AttachApproachRadiusGoal(&order->pos,order->radius);
            order->flags=0xe0;
            order->radius-=RandomInt(g_seams.weapons->GetWeaponRange(unit,(unsigned char)order->weapon)/3);
            order->state=1; return 4;
        }
        return 9;
    default: return 7;
    }
}

// Unused here (0x403a20 stays in its own file): the symbol ids these helpers
// take keep the allocation (docs/c2-regalloc.md).
static inline UnitDef* Definitions() { return g_game->defs; }

static inline Point16 WorldToCell(Vec3 v, Point16 footprint)
{
    Point16 c;
    c.x = (v.x - (footprint.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (footprint.y << 19) + 0x80000) >> 20;
    return c;
}

static inline void CellToWorld(Point16 footprint, Point16 c, Vec3* v)
{
    v->x = (footprint.x + c.x * 2) << 19;
    v->z = (footprint.y + c.y * 2) << 19;
}

// The float constants of the handlers that stay in their own files must come
// first in this object's constant pool, as in the original translation unit
// (the copies of 0x403a20 and 0x403f70 in 0x404270's original file).
int __stdcall PadConstants_403a20(double x)
{
    if (x == 8.0) return 1;
    if (x == -8.0) return 2;
    if (x == 16.0) return 3;
    if (x == 0.0) return 4;
    return 5;
}

// Capture order handler, a state machine on order->state.
// FUNCTION: 0x404270
int __stdcall CaptureOrder(Unit* unit, Order* order, unsigned int flags)
{
    Unit* target = order->link.Get();
    if (!target || (flags & 0x10008)) {
        QueueUnitSpeech(unit, 7, "Capture failed");
        return 8;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0: {
        if (!unit->motion) return 7;
        if (!(unit->def->flags2 & 0x1000)) return 7;
        if (target->def->capture) {
            QueueUnitSpeech(unit, 7, "That unit cannot be captured");
            return 8;
        }
        if (target->buildLeft != Zero_004fc920) {
            QueueUnitSpeech(unit, 7, "That unit is a cloud of vapor and cannot be captured");
            return 8;
        }
        order->AnnounceStatusIfFlagged("Capturing");
        order->duration = (int)(order->link.Get()->def->EnergyCost() / 2000 + order->link.Get()->def->MetalCost() / 140 + 150);
        order->duration = order->duration < 1800 ? order->duration : 1800;
        order->duration = (order->link.Get()->health + order->link.Get()->def->MaxHealth()) * order->duration / (order->link.Get()->def->maxHealth * 2);
        int experience = 0;
        experience = order->link.Get()->killCount;
        order->duration = ((experience / 5 + 10) * order->duration * 10) / 100;
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
        order->AttachBuildFootprintMarker(order->link.Get()->cell, order->link.Get()->footprint);
        order->flags = 0x100e8;
        return 1;
    }
    case 1: {
        if (flags & 0x40) return 8;
        Vec3* position = &unit->pos;
        Fixed distance;
        distance.value = (int)_hypot(unit->pos.x - target->pos.x, unit->pos.z - target->pos.z);
        int gap = distance.whole - (int)(_hypot(unit->footprint.x, unit->footprint.y) * 8.0);
        gap += (int)(_hypot(order->link.Get()->footprint.x, order->link.Get()->footprint.y) * -8.0);
        unsigned int range = 0;
        range = unit->def->buildRange;
        if (gap > (int)range) return 0;
        StartBuildingScript(unit, order, GetHeadingBetween(position, &order->link.Get()->pos) - unit->angles.heading);
        return 1;
    }
    case 2:
        return WaitIfNotInBuildStance(unit, order, 0x10008);
    case 3:
        QueueUnitSpeech(unit, 11, 0);
        return 1;
    case 4: {
        if (target->motion && (target->flags & 0xc)) {
            StopBuildingScript(unit, order);
            order->SetDeadlineTicks(30);
            return 0;
        }
        if (order->elapsed >= order->duration) return 1;
        Vec3 start;
        g_seams.units->GetNanoPiecePosition(order->source, &start);
        Vec3 bounds[2];
        bounds[1] = order->link.Get()->pos;
        bounds[0] = order->link.Get()->pos;
        bounds[0].x += order->link.Get()->def->modelMinX;
        bounds[0].z += order->link.Get()->def->modelMinZ;
        bounds[1].x += order->link.Get()->def->modelMaxX;
        bounds[1].z += order->link.Get()->def->modelMaxZ;
        bounds[1].y += order->link.Get()->def->modelMaxY;
        EmitReverseNanoParticles(bounds, &start, 6);
        unit->workTime = g_game->ticks + 900;
        order->elapsed += 2;
        order->SetDeadlineTicks(2);
        return 2;
    }
    case 5:
        g_seams.players->GiveUnitToPlayer(target, unit->player, 0);
        QueueUnitSpeech(unit, 16, 0);
        return 5;
    }
    return 7;
}

static inline int SquaredDistance(int dx, int dz)
{
    __int64 x = dx;
    __int64 z = dz;
    return (int)((x * x) >> 32) + (int)((z * z) >> 32);
}

// The same for 0x404ad0's -0.5f, which follows 0x404270's constants.
float PadConstant_404ad0(float x)
{
    return x * -0.5f;
}

// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
void ScheduleFadeTick();
int IsFadeDone();
void StepPaletteFade();
void FillEndGameStatistics();
int AreStatBarsComplete();
int ShouldShowNextMission();
void OpenEndMissionScreen();
void EnableEndMissionButtons();
void ShowEndMissionScreen();
void OpenCdCheckDialog();
int DrawEndGameFrame();

// FUNCTION: 0x404730
int __stdcall ReclaimUnitOrder(Unit* unit, Order* order, unsigned int flags)
{
    Unit* target = order->link.Get();
    if (!target || (flags & 0x10008)) return 5;
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (unit->motion && (unit->def->flags2 & 0x400)) {
            if (!g_seams.units->CanReclaim(unit, 0, target)) {
                QueueUnitSpeech(unit, 7, "That unit cannot be reclaimed");
                QueueUnitSpeech(unit, 7, "Reclamation failed");
                return 8;
            }
            order->AnnounceStatusIfFlagged("Reclaiming");
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
            return 1;
        }
        QueueUnitSpeech(unit, 7, "Reclamation failed");
        return 7;
    case 1:
        if (flags & 0x20) return 1;
        order->AttachBuildFootprintMarker(target->cell, target->footprint);
        order->flags |= 0x100e8;
        order->SetDeadlineTicks(15);
        order->elapsed = ComputeReclaimDamagePulse(unit, order->link.Get(), 15);
        order->duration = 0;
        return 2;
    case 2:
        if (flags & 0x40) return 9;
        StartBuildingScript(unit, order, GetHeadingBetween(&unit->pos, &target->pos) - unit->angles.heading);
        return 1;
    case 3:
        return WaitIfNotInBuildStance(unit, order, 0x10008);
    case 4:
        QueueUnitSpeech(unit, 11, 0);
        return 1;
    case 5: {
        Vec3 delta = Vec3Sub(unit->pos, target->pos);
        int range = 0;
        range = unit->def->buildRange;
        range += target->def->radius184;
        int square = SquaredDistance(delta.x, delta.z);
        if (square <= range * range && g_seams.units->CanReclaim(unit, 0, order->link.Get())) {
            if (order->duration >= 15) {
                g_seams.weapons->DamageUnit(unit, order->link.Get(), order->elapsed, 5, 0);
                order->duration = 0;
            }
            unit->workTime = g_game->ticks + 900;
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(order->source, &start);
            Vec3 bounds[2];
            bounds[1] = order->link.Get()->pos;
            bounds[0] = order->link.Get()->pos;
            bounds[0].x += order->link.Get()->def->modelMinX;
            bounds[0].z += order->link.Get()->def->modelMinZ;
            bounds[1].x += order->link.Get()->def->modelMaxX;
            bounds[1].z += order->link.Get()->def->modelMaxZ;
            bounds[1].y += order->link.Get()->def->modelMaxY;
            EmitReverseNanoParticles(bounds, &start, 6);
            order->SetDeadlineTicks(2);
            order->duration += 2;
            return 2;
        }
        order->SetDeadlineTicks(15);
        StopBuildingScript(unit, order);
        return 0;
    }
    }
    return 7;
}

// Unused here: the symbol ids this declaration takes keeps the allocation (docs/c2-regalloc.md).
void RunEndGameState();

// Order handler "Repair" of a builder: walks up to the target unit, then
// spends worker time on it until its health is full.
// FUNCTION: 0x405300
int __stdcall RepairUnitOrder(Unit* unit, Order* order, int flags)
{
    if (!order->target) {
        QueueUnitSpeech(unit, 7, "Repairs unsuccessful.");
        return 5;
    }
    if (order->range && (int)_hypot(Whole(unit->pos.x) - order->start.x, Whole(unit->pos.z) - order->start.y) >= order->range)
        return 5;
    if (order->target->mode != 1) {
        QueueUnitSpeech(unit, 7, "Repairs unsuccessful.");
        return 5;
    }
    switch (order->state) {
    case 0:
        if (unit->motion && (unit->def->flags1Byte & 0x40) && order->target->buildLeft == Zero_004fc920) {
            order->AnnounceStatusIfFlagged("Repairing");
            return 1;
        }
        break;
    case 1: {
        if (flags & 0x40)
            return 8;
        Fixed distance;
        distance.value = (int)_hypot(unit->pos.x - order->target->pos.x, unit->pos.z - order->target->pos.z);
        int gap = distance.whole - (int)(_hypot(unit->footprint.x, unit->footprint.y) * 8.0);
        gap += (int)(_hypot(order->target->footprint.x, order->target->footprint.y) * -8.0);
        unsigned int range = 0;
        range = unit->def->buildRange;
        if (gap > (int)range) {
            order->AttachBuildFootprintMarker(order->target->cell, order->target->footprint);
            order->SetDeadlineTicks(RandomInt(30) + 30);
            order->flags |= 0xe8;
            return 2;
        }
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
        StartBuildingScript(unit, order, GetHeadingBetween(&unit->pos, &order->target->pos) - unit->angles.heading);
        return 1;
    }
    case 2:
        return WaitIfNotInBuildStance(unit, order, 8);
    case 3:
        if ((unsigned int)order->target->health >= order->target->def->maxHealth)
            return 1;
        if (order->target->flags & 0xc) {
            StopBuildingScript(unit, order);
            order->SetDeadlineTicks(15);
            return 0;
        }
        unit->workTime = g_game->ticks + 150;
        if (g_seams.units->AddRepairProgress(unit, order->target, (float)(unit->def->buildRate / 30))) {
            Vec3 nano;
            g_seams.units->GetNanoPiecePosition(order->source, &nano);
            Box box;
            box.hi = order->target->pos;
            box.lo = order->target->pos;
            box.lo.x += order->target->def->modelMinX;
            box.lo.z += order->target->def->modelMinZ;
            box.hi.x += order->target->def->modelMaxX;
            box.hi.z += order->target->def->modelMaxZ;
            box.hi.y += order->target->def->modelMaxY;
            EmitNanoParticles(&nano, &box, 6);
        }
        order->SetDeadlineTicks(1);
        order->flags |= 8;
        return 2;
    case 4:
        QueueUnitSpeech(unit, 10, "Unit repaired");
        return 5;
    }
    return 7;
}

// FUNCTION: 0x405740
int __stdcall RepairUnitNoMoveOrder(Unit* unit, Order* order, int unused)
{
    Unit* target = order->link.Get();
    if (!target) {
        QueueUnitSpeech(unit, 7, "Repairs unsuccessful.");
        return 5;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0:
        if (!(unit->def->flags1Byte & 0x40)) return 7;
        if (target->buildLeft == Zero_004fc920 && (unit->activateFlags & 1)) {
            g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
            return 1;
        }
        return 8;
    case 1: {
        if ((unsigned int)target->health >= target->def->maxHealth) return 1;
        if (target->flags & 0xc) return 1;
        unit->workTime = g_game->ticks + 150;
        int rate = 0;
        rate = unit->def->buildRate;
        if (g_seams.units->AddRepairProgress(unit, order->link.Get(), (float)(rate / 30))) {
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(order->source, &start);
            Vec3 bounds[2];
            bounds[1] = order->link.Get()->pos;
            bounds[0] = order->link.Get()->pos;
            bounds[0].x += order->link.Get()->def->modelMinX;
            bounds[0].z += order->link.Get()->def->modelMinZ;
            bounds[1].x += order->link.Get()->def->modelMaxX;
            bounds[1].z += order->link.Get()->def->modelMaxZ;
            bounds[1].y += order->link.Get()->def->modelMaxY;
            EmitNanoParticles(&start, bounds, 6);
        }
        order->SetDeadlineTicks(1);
        order->flags |= 8;
        return 2;
    }
    case 2:
        QueueUnitSpeech(unit, 10, "Unit repaired");
        return 5;
    }
    return 7;
}

// FUNCTION: 0x405d90
void DamagedAllyCollector::CollectDamagedAlly(Unit* unit)
{
    if (unit == self) return;
    unsigned int index = 0;
    index = unit->player->index;
    if (!owner->allied[index]) return;
    unsigned int kind = unit->flags & 3;
    if ((unsigned char)kind != 1) return;
    if ((unsigned int)unit->health >= unit->def->maxHealth && unit->buildLeft == Zero_004fc920) return;
    if (unit->lastAttackerSlot == owner->index && unit->lastDamageType == 5) return;
    units->push_back(unit);
}

// FUNCTION: 0x405fe0
int __stdcall StandbyOrder(Unit* unit,Order* order,int flags)
{
    unsigned state=0; state=order->state;
    switch(state) {
    case 0:
        if(!unit->motion) return 7;
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        order->flags|=0x10000;
        order->SetDeadlineTicks(1); return 1;
    case 1:
        {
            Unit* next=g_seams.targeting->FindBestTargetIfFireAtWill(unit);
            if(next && IssueAttackOrder(unit,(Order*)next,0)) return 5;
        }
        order->flags|=0x10000;
        order->SetDeadlineTicks(RandomInt(30)+30); return 2;
    default: return 7;
    }
}

// FUNCTION: 0x406090
int __stdcall StandbyMineOrder(Unit* unit, Order* order, int unused)
{
    switch (order->state) {
    case 0:
        if (!(unit->flags & 0x20000000)) return 7;
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        order->flags |= 0x10000;
        order->SetDeadlineTicks(1);
        return 1;
    case 1:
        {
            Unit* other = g_seams.targeting->FindBestTargetIfFireAtWill(unit);
            if (other && other->mode == 1 && (unit->flags & 0x300000)) {
                AppendOrder(unit, new (kSim) Order("SELFDESTRUCT", 0, 0, 1, 0, 0));
                return 5;
            }
            order->flags |= 0x10000;
            order->SetDeadlineTicks(RandomInt(30) + 30);
            return 2;
        }
    default: return 7;
    }
}

static inline Point16 MakePoint(int x,int y) { Point16 p; p.x=x; p.y=y; return p; }
static inline Point16 SubPoint(Point16 a,Point16 b) { Point16 p; p.x=a.x-b.x; p.y=a.y-b.y; return p; }
static inline Point16 Convert(Vec3 v) { Point16 p; p.x=(unsigned)v.x>>20; p.y=(unsigned)v.z>>20; return p; }

// FUNCTION: 0x4061a0
int __stdcall ParkOrder(Unit* unit,Order* order,int flags)
{
    switch(order->state) {
    case 0:
        if(!unit->motion) return 7;
        if(unit->def->flying) {
            order->pos=unit->pos;
            order->MergeFlagsFromTable(MissionType("VTOL_MOVE").index);
            return 0;
        }
        {
            int size=unit->footprint.x;
            if(unit->def->minwaterdepth>=0) size+=3;
            Point16 start=SubPoint(Convert(unit->pos),MakePoint(size*4,size*3));
            Point16 extent=MakePoint(size*8,size*6);
            order->AttachBuildFootprintMarker(start,extent);
            order->flags=0xe0;
            return 1;
        }
    case 1:
        if(flags&0x20) return 5;
        if(order->next) return 5;
        order->SetDeadlineTicks(30); return 0;
    default: return 7;
    }
}

// Unused here (0x406300 stays in its own file): the symbol ids these helpers
// take keep the allocation (docs/c2-regalloc.md).
static inline int Contains(unsigned int* bits, unsigned short index) { return bits[index >> 5] & (1 << (index & 31)); }

static inline Vec3 Offset(short angle, int distance) { Vec3 v; v.x=-FUN_004b70ef(angle,distance); v.y=0; v.z=-FUN_004b7123(angle,distance); return v; }

// Unused here: the symbol ids this declaration takes keeps the allocation (docs/c2-regalloc.md).
void FreeMemoryCache();

// Keep cases 1 and 3 separate: MSVC merges their identical bodies.
// FUNCTION: 0x406780
int __stdcall GroundPickupOrder(Unit* unit, Order* order, unsigned char flags)
{
    Unit* target = order->target;
    if (target && !(flags & 8)) {
        switch(order->state) {
        case 0:
            if (!unit->motion) break;
            if (!(unit->def->flags2 & 0x100)) break;
            if (target->footprint.x > (short)unit->def->capacity) {
                QueueUnitSpeech(unit, 7, "Unit is too large to transport"); return 8;
            }
            order->AnnounceStatusIfFlagged("Loading unit"); return 1;
        case 1: return WaitIfCobBusy(unit, order, 8);
        case 2:
            {
            int id = target->id;
            g_seams.units->StartScriptWithArgs((CobScript*)unit->script, "TransportPickup", 0, 1, 1, id, 0, 0, 0);
            QueueUnitSpeech(unit, 12, 0);
            ++order->attempts;
            order->SetDeadlineTicks(15); return 1;
            }
        case 3: return WaitIfCobBusy(unit, order, 8);
        case 4:
            if (target->carrier) return 5;
            if (order->attempts >= 3) return 9;
            order->AttachApproachRadiusGoal(&target->pos, 0);
            order->flags = 0xe8; return 1;
        case 5: order->SetAttachedFx(0); return 0;
        default: break;
        }
        return 7;
    }
    QueueUnitSpeech(unit, 7, "Transport mission failed"); return 8;
}

// FUNCTION: 0x406900
int __stdcall GroundUnloadOrder(Unit* unit, Order* order, int flags)
{
    if (flags&8) {
        QueueUnitSpeech(unit,7,"Unloading process is proceeding non-optimally");
        return 8;
    }
    switch(order->state) {
    case 0:
        if (!unit->motion) break;
        if (!(unit->def->flags2&0x100)) break;
        order->link.SetUnit(unit->cargo);
        if (!order->target) return 5;
        order->AnnounceStatusIfFlagged("Unloading");
        g_seams.units->StartScriptWithArgs((CobScript*)unit->script, "TransportDrop",0,1,1,order->target->id,
            (order->pos.x&0xffff0000)+(order->pos.z>>16),0,0);
        ++order->attempts;
        order->SetDeadlineTicks(15);
        return 1;
    case 1: return WaitIfCobBusy(unit,order,8);
    case 2:
        if (*(Unit**)&order->target->carrier!=unit) { QueueUnitSpeech(unit,13,0); return 5; }
        if (order->attempts>=3) return 9;
        if ((unsigned char)(unit->def->flags1>>12)&1)
            order->AttachApproachRadiusGoal(&order->pos,(int)(unit->def->height180*1.5));
        else order->AttachApproachRadiusGoal(&order->pos,0);
        order->flags=0xe8;
        return 1;
    case 3: return 0;
    }
    return 7;
}

// Unused here (0x406aa0 stays in its own file): the symbol ids these helpers
// take keep the allocation (docs/c2-regalloc.md).
static inline Vec3 Add(const Vec3& a,const Vec3& b) { Vec3 r; r.x=a.x+b.x; r.y=a.y+b.y; r.z=a.z+b.z; return r; }
static inline Vec3 Sub(const Vec3& a,const Vec3& b) { Vec3 r; r.x=a.x-b.x; r.y=a.y-b.y; r.z=a.z-b.z; return r; }

// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
void InitExplosions();
void FreeExplosions();

// Registers a table with RegisterOrderTypes under a numeric id; one of several
// small functions doing the same for different tables.
// FUNCTION: 0x406bf0
void RegisterGroundOrders()
{
#ifdef __clang__
    extern const UnitOrderType g_groundOrders[22];
    RegisterOrderTypes((void*)g_groundOrders, 0x16);
#else
    extern char g_groundOrders[];
    RegisterOrderTypes(g_groundOrders, 0x16);
#endif
}

// std::vector<Unit*>::_Destroy(first, last) from MSVC 5's <vector>: empty,
// since the element type is trivial.
// Its callers (0x405d90, 0x40b530, 0x480250 and others) inline
// vector::insert and call 0x406c40 (_Ufill), 0x406c10 (_Ucopy) and 0x406c00
// (_Destroy) with ecx set to the vector. 0x40ad80 calls _Ucopy, _Ufill,
// size (0x40c560) and insert (0x408f30) on one vector of units, and 0x48d220
// calls _Destroy and erase (0x40c9f0), so all of these are members of the
// same std::vector<Unit*> (see 0x40c510.cpp).
typedef std::vector<Unit*> Vec_00406c00;
typedef void (Vec_00406c00::*DestroyFn_00406c00)(Vec_00406c00::iterator, Vec_00406c00::iterator);

// _Destroy is protected: the derived class takes its address to emit it out of line.
struct Access_00406c00 : Vec_00406c00 {
    static DestroyFn_00406c00 fn;
};

// FUNCTION: 0x406c00 ?_Destroy@?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@IAEXPAPAUUnit@@0@Z
DestroyFn_00406c00 Access_00406c00::fn = &Access_00406c00::_Destroy;

// 0x406c10 (_Ucopy), 0x406c40 (_Ufill) and 0x406c70 (_Construct) stay in their
// own files: the inline _Construct above is what makes the repair patrol's
// vector insert (0x405d90) call 0x406c70, and with it in this file _Ucopy and
// _Ufill call it too instead of inlining the placement new, and 0x406c70
// itself is inlined into insert, so all three change bytes here.

// FUNCTION: 0x401e00
int __stdcall AttackUTypeOrder(Unit* unit, Order* order, int unused)
{
    unsigned int s = 0;
    s = order->state;
    switch (s) {
    case 0:
        if (!(unit->def->flags2Byte & 0x10))
            return 7;
        order->SetDeadlineTicks(RandomInt(0x5a) + 1);
        return 1;
    case 1: {
        Unit* best = 0;
        int bestDist = 0x7fffffff;
        for (Unit* u = g_game->units + 1; u <= g_game->unitsEnd; u++) {
            if (u->unitDefIndex == order->id && unit->player->allied[u->player->index] == 0) {
                int dz = u->pos.z - unit->pos.z;
                int dx = u->pos.x - unit->pos.x;
                int d = (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);
                d -= RandomInt(d / 2);
                if (d <= bestDist) {
                    best = u;
                    bestDist = d;
                }
            }
        }
        if (best) {
            MissionType kind = GetOrderType(3, unit, best, 0);
            AppendOrder(unit, new (kSim) Order(kind, best, 0, 0, 0, 0));
            return 0;
        }
        return 5;
    }
    default:
        return 7;
    }
}

// Unused here: the symbol ids this declaration takes keep the allocation (docs/c2-regalloc.md).
struct Projectile;

// A member operator+ on the unit's position: the nano box corners in 0x403f70
// only compile this way, where the free Vec3Add would load the operands in
// another order.
struct Vec3Sum : Vec3 {
    Vec3 operator+(const Vec3& other) const {
        Vec3 r;
        r.x = x + other.x;
        r.y = y + other.y;
        r.z = z + other.z;
        return r;
    }
};

// The original radius expression adds the second dimension twice rather than
// squaring it: fld x; fld y; fld st(1); fmul st(2); fadd st(1); fadd st(1).
// FUNCTION: 0x403f70
int __stdcall HelpBuildOrder(Unit* unit, Order* order, int flags)
{
    if (flags & 2) { MarkSelectionOrdersDirty(unit); return 5; }
    Unit* target = order->target;
    if (!target) {
        QueueUnitSpeech(unit, 7, "Construction terminated");
        return 8;
    }
    unsigned int state = 0;
    state = order->state;
    switch (state) {
    case 0: {
        if (!unit->motion || !(unit->def->flags1Byte & 0x40)) break;
        double x = target->def->footprintx;
        double y = target->def->footprintz;
        int radius = (int)(sqrt(x * x + y + y) * 16.0) / 2;
        unsigned int range = 0;
        range = unit->def->buildRange;
        order->AttachRingApproachGoal(&target->pos, range + radius, radius);
        order->flags = 0xe8;
        return 1;
    }
    case 1:
        if (flags & 0x40) {
            QueueUnitSpeech(unit, 7, "I can't get there");
            return 8;
        }
        if (target->buildLeft == 0.0f) return 5;
        g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
        StartBuildingScript(unit, order, GetHeadingBetween(&unit->pos, &order->target->pos) - unit->angles.heading);
        MarkSelectionOrdersDirty(unit);
        return 1;
    case 2:
        return WaitIfNotInBuildStance(unit, order, 10);
    case 3: {
        int rate = 0;
        rate = unit->def->buildRate;
        if (g_seams.units->AddBuildProgress(unit, target, (float)(rate / 30))) {
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(unit, &start);
            Vec3 bounds[2];
            bounds[0] = (Vec3Sum&)order->target->pos + *(Vec3*)&order->target->def->modelMinX;
            bounds[1] = (Vec3Sum&)order->target->pos + *(Vec3*)&order->target->def->modelMaxX;
            EmitNanoParticles(&start, (Box*)bounds, 6);
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
        order->flags |= 2;
        return 5;
    }
    return 7;
}

int __stdcall StopOrder(Unit* unit, Order* order, int flags);
int __stdcall MakeSelectableOrder(Unit* unit, Order* order, int flags);
int __stdcall WaitOrder(Unit* unit, Order* order, int flags);
int __stdcall AttackUTypeOrder(Unit* unit, Order* order, int flags);
int __stdcall WaitForAttackOrder(Unit* unit, Order* order, int flags);
int __stdcall SelfDestructOrder(Unit* unit, Order* order, int flags);
int __stdcall AttackNoMoveOrder(Unit* unit, Order* order, int flags);
int __stdcall GuardNoMoveOrder(Unit* unit, Order* order, int flags);
int __stdcall SelfRepairOrder(Unit* unit, Order* order, int flags);
int __stdcall BuildingBuildOrder(Unit* unit, Order* order, int flags);
int __stdcall BuildWeaponOrder(Unit* unit, Order* order, int flags);
int __stdcall ParalyzeOrder(Unit* unit, Order* order, int flags);
int __stdcall GetBuiltOrder(Unit* unit, Order* order, int flags);
int __stdcall BeCarriedOrder(Unit* unit, Order* order, int flags);
int __stdcall ActivateOrder(Unit* unit, Order* order, int flags);
int __stdcall DeactivateOrder(Unit* unit, Order* order, int flags);
int __stdcall CloakOnOrder(Unit* unit, Order* order, int flags);
int __stdcall CloakOffOrder(Unit* unit, Order* order, int flags);
int __stdcall StandingMoveOrder(Unit* unit, Order* order, int flags);
int __stdcall StandingFireOrder(Unit* unit, Order* order, int flags);
int __stdcall QMoveQPatrolOrder(Unit* unit, Order* order, int flags);
int __stdcall AttackSpecialOrder(Unit* unit, Order* order, int flags);
int __stdcall MoveGroundOrder(Unit* unit, Order* order, int flags);
int __stdcall AttackKamikazeOrder(Unit* unit, Order* order, int flags);
int __stdcall PatrolOrder(Unit* unit, Order* order, int flags);
int __stdcall AttackChaseOrder(Unit* unit, Order* order, int flags);
int __stdcall SuppressOrder(Unit* unit, Order* order, int flags);
int __stdcall MobileBuildOrder(Unit* unit, Order* order, int flags);
int __stdcall HelpBuildOrder(Unit* unit, Order* order, int flags);
int __stdcall CaptureOrder(Unit* unit, Order* order, int flags);
int __stdcall ReclaimUnitOrder(Unit* unit, Order* order, int flags);
int __stdcall ReclaimOrder(Unit* unit, Order* order, int flags);
int __stdcall ResurrectOrder(Unit* unit, Order* order, int flags);
int __stdcall RepairUnitOrder(Unit* unit, Order* order, int flags);
int __stdcall RepairUnitNoMoveOrder(Unit* unit, Order* order, int flags);
int __stdcall RepairPatrolOrder(Unit* unit, Order* order, int flags);
int __stdcall StandbyOrder(Unit* unit, Order* order, int flags);
int __stdcall StandbyMineOrder(Unit* unit, Order* order, int flags);
int __stdcall ParkOrder(Unit* unit, Order* order, int flags);
int __stdcall FollowGroundOrder(Unit* unit, Order* order, int flags);
int __stdcall GroundPickupOrder(Unit* unit, Order* order, int flags);
int __stdcall GroundUnloadOrder(Unit* unit, Order* order, int flags);
int __stdcall TeleportOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolLandIfCanOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolStandbyOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolMoveOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolFollowOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolSeekAttackOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolSeekGuardOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolPatrolOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolPickupOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolUnloadOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolLandingOrder(Unit* unit, Order* order, int flags);
int __stdcall AirStrikeOrder(Unit* unit, Order* order, int flags);
int __stdcall AirToGroundOrder(Unit* unit, Order* order, int flags);
int __stdcall AirToAirOrder(Unit* unit, Order* order, int flags);
int __stdcall AirToGroundHoverOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolEvadeOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolMobileBuildOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolHelpBuildOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolReclaimOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolReclaimUnitOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolRepairUnitOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolGetRepairedOrder(Unit* unit, Order* order, int flags);
int __stdcall VtolRepairPatrolOrder(Unit* unit, Order* order, int flags);
int __stdcall ReadyOrder(Unit* unit, Order* order, int flags);
void __stdcall DrawBuildFootprint(void* surface, View* view, Order* order, Pos* out, int flag);
void __stdcall DrawPathAnim(void* surface, View* view, Order* order, Pos* out, int flag);
void __stdcall DrawWeaponCoverage(void* surface, View* view, Order* order, Pos* out, int flag);

// every unit's orders, registered by 0x403180.
// GLOBAL: 0x4fc490
extern const UnitOrderType g_unitOrders[23] = {
    {"Stopping", StopOrder, 0, 0, 0x13, 0, "Stop"},
    {"Attacking", AttackNoMoveOrder, DrawWeaponCoverage, 8, 0x28001, 0, "Attack_NoMove"},
    {"Activate", ActivateOrder, 0, 0, 0x1006013, 0, "Activate"},
    {"Deactivate", DeactivateOrder, 0, 0, 0x1006013, 0, "Deactivate"},
    {"Cloaking", CloakOnOrder, 0, 0, 0x1006013, 0, "Cloak_On"},
    {"Decloaking", CloakOffOrder, 0, 0, 0x1006013, 0, "Cloak_Off"},
    {"Acknowledged", StandingMoveOrder, 0, 0, 0x1006013, 0, "Standing_MoveOrder"},
    {"Acknowledged", StandingFireOrder, 0, 0, 0x1006013, 0, "Standing_FireOrder"},
    {"Nanolathing", BuildingBuildOrder, 0, 0, 0x10010c13, 0, "BuildingBuild"},
    {"Nanolathing", BuildWeaponOrder, 0, 0, 0xc014013, 0, "BuildWeapon"},
    {"SELF DESTRUCT ENGAGED", SelfDestructOrder, 0, 0, 0x4004013, 0, "SelfDestruct"},
    {"SELF DESTRUCT ENGAGED", SelfDestructOrder, 0, 0, 0x13, 0, "SelfDestructFG"},
    {"Paralyzed", ParalyzeOrder, 0, 0, 0x2413, 0, "Paralyze"},
    {"Under construction", GetBuiltOrder, 0, 0, 0x22413, 0, "GetBuilt"},
    {"Being transported", BeCarriedOrder, 0, 0, 0x2413, 0, "BeCarried"},
    {"Unit is available", MakeSelectableOrder, 0, 0, 0x413, 0, "MakeSelectable"},
    {"Waiting", WaitOrder, 0, 0, 0x413, 0, "Wait"},
    {"Waiting for attack", WaitForAttackOrder, 0, 0, 0x20413, 0, "WaitForAttack"},
    {"Attacking", AttackUTypeOrder, 0, 0, 0x413, 0, "AttackUType"},
    {"Ready", GuardNoMoveOrder, 0, 0, 0x2013, 0, "Guard_NoMove"},
    {"Repairing", SelfRepairOrder, 0, 0, 0x20413, 1, "SelfRepair"},
    {"Ready with orders", QMoveQPatrolOrder, DrawPathAnim, 2, 0x4000e, 0, "QMove"},
    {"Ready with orders", QMoveQPatrolOrder, DrawPathAnim, 2, 0x40007, 0, "QPatrol"},
};

// the orders of units that move on the ground, registered by 0x406bf0.
// GLOBAL: 0x4fc6e8
extern const UnitOrderType g_groundOrders[22] = {
    {"Standby", StandbyOrder, 0, 0x10, 0x200000f, 0, "Standby"},
    {"Standby", StandbyMineOrder, 0, 0x10, 0x200000f, 1, "Standby_Mine"},
    {"Moving", MoveGroundOrder, DrawPathAnim, 0x12, 0x4020e, 0, "Move_Ground"},
    {"Guarding", FollowGroundOrder, DrawPathAnim, 0x12, 0x20005, 0, "Follow_Ground"},
    {"Suppressing fire", SuppressOrder, DrawWeaponCoverage, 8, 0x41001, 0, "Suppress"},
    {"Attacking", AttackChaseOrder, DrawWeaponCoverage, 8, 0x28001, 0, "Attack_Chase"},
    {"Attacking", AttackKamikazeOrder, DrawWeaponCoverage, 8, 0x60001, 0, "Attack_Kamikaze"},
    {"Annihilating", AttackSpecialOrder, DrawWeaponCoverage, 8, 0x68001, 0, "AttackSpecial"},
    {"Parking", ParkOrder, 0, 0, 0xe, 0, "Park"},
    {"Patrolling", PatrolOrder, DrawPathAnim, 0x12, 0x41207, 0, "Patrol"},
    {"Loading", GroundPickupOrder, DrawWeaponCoverage, 8, 0x2000c, 0, "Ground_Pickup"},
    {"Unloading", GroundUnloadOrder, DrawWeaponCoverage, 8, 0x4000d, 0, "Ground_Unload"},
    {"Teleporting", TeleportOrder, DrawWeaponCoverage, 8, 0x60009, 0, "Teleport"},
    {"Nanolathing", MobileBuildOrder, DrawBuildFootprint, 0x13, 0x10050800, 0, "MobileBuild"},
    {"Nanolathing", HelpBuildOrder, DrawWeaponCoverage, 0x18, 0x10020806, 0, "HelpBuild"},
    {"Repair patrol", RepairPatrolOrder, DrawPathAnim, 0x12, 0x41207, 0, "RepairPatrol"},
    {"Repairing", RepairUnitOrder, DrawPathAnim, 0x12, 0x10020006, 0, "RepairUnit"},
    {"Capturing", CaptureOrder, DrawWeaponCoverage, 8, 0x20004, 0, "Capture"},
    {"Resurrecting", ResurrectOrder, DrawPathAnim, 0x12, 0x2000b, 0, "Resurrect"},
    {"Reclaiming", ReclaimOrder, DrawPathAnim, 0x12, 0x1008000b, 0, "Reclaim"},
    {"Reclaiming", ReclaimUnitOrder, DrawPathAnim, 0x12, 0x1002000b, 0, "ReclaimUnit"},
    {"Repairing", RepairUnitNoMoveOrder, DrawWeaponCoverage, 0x18, 0x20006, 0, "RepairUnitNoMove"},
};

// the orders of aircraft, registered by 0x415b20.
// GLOBAL: 0x4fca18
extern const UnitOrderType g_vtolOrders[22] = {
    {"Standby", VtolStandbyOrder, 0, 0, 0x200000f, 0, "VTOL_Standby"},
    {"Moving", VtolMoveOrder, DrawPathAnim, 2, 0x4020e, 0, "VTOL_Move"},
    {"Landing", VtolLandingOrder, DrawWeaponCoverage, 8, 0x6000e, 0, "VTOL_Landing"},
    {"Loading", VtolPickupOrder, DrawWeaponCoverage, 8, 0x20008, 0, "VTOL_Pickup"},
    {"Unloading", VtolUnloadOrder, DrawWeaponCoverage, 8, 0x40009, 0, "VTOL_Unload"},
    {"Guarding", VtolFollowOrder, DrawPathAnim, 2, 0x20005, 0, "VTOL_Follow"},
    {"Patrolling", VtolPatrolOrder, DrawPathAnim, 2, 0x41207, 0, "VTOL_Patrol"},
    {"Airstrike", AirStrikeOrder, DrawWeaponCoverage, 8, 0x60002, 0, "AirStrike"},
    {"Engaging target", AirToAirOrder, DrawWeaponCoverage, 8, 0x20001, 0, "AirToAir"},
    {"Engaging target", AirToGroundOrder, DrawWeaponCoverage, 8, 0x20001, 0, "AirToGround"},
    {"Engaging target", AirToGroundHoverOrder, DrawWeaponCoverage, 8, 0x20001, 0, "AirToGroundHover"},
    {"Nanolathing", VtolMobileBuildOrder, DrawBuildFootprint, 3, 0x10050800, 0, "VTOL_MobileBuild"},
    {"Nanolathing", VtolHelpBuildOrder, DrawWeaponCoverage, 8, 0x10020806, 0, "VTOL_HelpBuild"},
    {"Repair patrol", VtolRepairPatrolOrder, DrawPathAnim, 2, 0x41207, 0, "VTOL_RepairPatrol"},
    {"Repairing", VtolRepairUnitOrder, DrawPathAnim, 2, 0x10020006, 0, "VTOL_RepairUnit"},
    {"Reclaiming", VtolReclaimOrder, DrawPathAnim, 2, 0x1008000b, 0, "VTOL_Reclaim"},
    {"Reclaiming", VtolReclaimUnitOrder, DrawPathAnim, 2, 0x1002000b, 0, "VTOL_ReclaimUnit"},
    {"Evading", VtolEvadeOrder, 0, 0, 0x13, 0, "VTOL_Evade"},
    {"Seeking to attack", VtolSeekAttackOrder, 0, 0, 0x60013, 0, "VTOL_SeekAttack"},
    {"Seeking to guard", VtolSeekGuardOrder, 0, 0, 0x60013, 0, "VTOL_SeekGuard"},
    {"Under repair", VtolGetRepairedOrder, 0, 0, 0x20013, 0, "VTOL_GetRepaired"},
    {"Seeking to land", VtolLandIfCanOrder, 0, 0, 0x40013, 0, "VTOL_LandIfCan"},
};

// One order, registered on its own by 0x43bc90.
// GLOBAL: 0x4fd288
extern const UnitOrderType g_readyOrder[1] = {
    {"Ready", ReadyOrder, 0, 0, 0xf, 0, ""},
};

extern const int PadAlign_004fc6d0 = 0;

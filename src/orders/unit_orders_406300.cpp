// Decompiled by Claude Opus 5.5, finished by GPT-6, verified by GPT-6.1-sol, edited by deepseek-v4.1, finished by deepseek-v4.1-flash, finished by space-bunny-free, finished by deepseek-v4.1-flash. Names are provisional.
// Stays in its own file: in unit_orders.cpp the weapon slot loop keeps its counter in a register, where the original keeps it in memory.

#include <stdio.h>
#include "../state/block.h"
#include "../util/vec3.h"
static inline Vec3 operator+(const Vec3& a, const Vec3& b) { Vec3 r; r.x=a.x+b.x; r.y=a.y+b.y; r.z=a.z+b.z; return r; }
// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
class UnitMotion;
struct Player;
struct Unit;
struct Order;
#include "mission_type.h"

#pragma pack(push, 1)
struct WeaponDef { char pad0[0x111]; unsigned int flags; };
#include "../units/unit_def.h"
struct Owner { char pad0[0x108]; unsigned char allied[0x3e]; unsigned char index; };
#include "../units/unit.h"
#include "order.h"

#include "../../interfaces/seams.h"

#pragma pack(pop)
int __stdcall IssueAttackOrder(Unit*, Unit*, int);
MissionType __stdcall GetOrderType(unsigned char, Unit*, Unit*, int);
void __stdcall AppendOrder(Unit*, Order*);
int __stdcall RandomInt(int);
int __cdecl FUN_004b70ef(short, int);
int __cdecl FUN_004b7123(short, int);
static inline int Contains(unsigned int* bits, unsigned short index) { return bits[index >> 5] & (1 << (index & 31)); }
static inline Vec3 Offset(short angle, int distance) { Vec3 v; v.x=-FUN_004b70ef(angle,distance); v.y=0; v.z=-FUN_004b7123(angle,distance); return v; }
// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
int __stdcall GetOrderTarget(int param_1);
// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
int CheckDirectXVersion(int, int, int, int, int);
struct Feature;
// FUNCTION: 0x406300
int __stdcall FollowGroundOrder(Unit* unit, Order* order, int flags)
{
    if (!order->target) return 5;
    if (unit->carrier) return 7;
    if ((unsigned char)(order->target->def->flags1 >> 11) & 1) return 8;
    unsigned int state=0; state=order->state;
    switch(state) {
    case 0: {
        order->AnnounceStatusIfFlagged("Guarding");
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
        order->approachRadius=(unit->footprint.x + order->target->footprint.x + 2) << 4;
        int distance=order->approachRadius << 16;
        short angle=RandomInt(0x10000);
        order->pos=Offset(angle,distance);
        return 1;
    }
    case 1: {
        Unit* attacker=order->target->attacker;
        if (attacker && !((Owner*)attacker->player)->allied[((Owner*)unit->player)->index] && (flags & 0x10) &&
            !Contains(unit->def->categories,attacker->unitDefIndex)) {
            if (IssueAttackOrder(unit,attacker,1)) { order->flags=0; return 3; }
            if (unit->flags & 0x300000) {
                for (unsigned char i=0;i<3;++i) {
                    UnitWeaponSlot* weapon=&unit->weapons[i];
                    if ((weapon->flags&2) && (weapon->flags&0x10) && !((unsigned char)(((WeaponDef*)weapon->weapon)->flags >> 26)&1)) {
                        Unit* target=g_seams.targeting->GetWeaponTargetUnit(unit,i);
                        if (!target || !g_seams.weapons->WeaponCanReachUnit(unit,target,i) || Contains(unit->def->weaponCategories[i],target->unitDefIndex))
                            g_seams.targeting->SetWeaponTargetUnit(unit,attacker,i);
                    }
                }
            }
        }
        if ((unsigned int)order->target->health < order->target->def->maxHealth && (unit->def->flags1&0x40)) {
            MissionType kind=GetOrderType(8,unit,order->target,0);
            if(kind.index) {
                order->SetAttachedFx(0);
                AppendOrder(unit,new (kSim) Order(kind,order->target,0,0,0,0));
                order->flags=0; return 3;
            }
        }
        if (order->target->list && order->target->list->kind.index &&
            (unit->def->flags1&0x40) && (order->target->def->flags1&0x40) &&
            (order->target->list->flags_42&0x100000) && unit!=order->target->list->Target()) {
            int building=order->target->list->kind=="MobileBuild" ||
                         order->target->list->kind=="BuildingBuild";
            Order* other=order->target->list;
            int actionable=((other->flags_42&0x200) && other->target) || (other->flags_42&0x400);
            MissionType kind;
            if (!building && actionable) {
                order->SetAttachedFx(0);
                kind=order->target->list->kind;
                // Target() and Position() on the base: keeps the two order->order loads from merging.
                AppendOrder(unit,new (kSim) Order(kind,order->Target()->list->target,order->Target()->list->Position(),0,0,0));
                order->flags=0; return 3;
            }
            if (building && other->target) {
                order->SetAttachedFx(0);
                kind=MissionType("HelpBuild");
                AppendOrder(unit,new (kSim) Order(kind,order->Target()->list->target,order->Target()->list->Position(),0,0,0));
                order->flags=0; return 3;
            }
        }
follow:
        Vec3 pos=order->target->pos+order->pos;
        order->AttachApproachRadiusGoal(&pos,order->approachRadius/2);
        order->SetDeadlineTicks(30);
        order->flags|=0x18;
        return 2;
    }
    }
    return 7;
}

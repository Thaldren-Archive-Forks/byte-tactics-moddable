// Decompiled by GPT-5.6 Astra, finished by deepseek-v4.1-flash and GPT-6, edited by deepseek-v4.1, finished by GPT-6.1-sol, finished by deepseek-v4.1-flash, finished by xiaomi/mimo-v2.6-pro, finished by Space Bunny Free, finished by Claude Opus 5.5. Names are provisional.
// VTOL guard order handler ("Guarding"): the aircraft circles the guarded
// unit, fights back against its attacker, repairs it, and takes over the
// guarded unit's own build or reclaim order (as the VTOL_ variant of that
// order). The ground twin is 0x406300.
#include <stdio.h>
#include "../state/block.h"
#include "../util/vec3.h"
static inline Vec3 operator+(const Vec3& a, const Vec3& b) { Vec3 r; r.x=a.x+b.x; r.y=a.y+b.y; r.z=a.z+b.z; return r; }
struct Unit;
struct Order;
#include "mission_type.h"

#pragma pack(push, 1)
struct WeaponDef { char pad0[0xdc]; int range; char pade0[0x111-0xe0]; unsigned int flags; };
#include "../units/unit_def.h"
struct Owner { char pad0[0x108]; unsigned char allied[0x3e]; unsigned char index; };
#include "unit_motion.h"
#include "../units/unit.h"
#include "order.h"

class Class_0044e2d0 {
public:
    char data[0x36];
    Class_0044e2d0(Order*, const Vec3&);
    void SetAltitude(int);
    void SetApproachRadius(int);
};

#include "../../interfaces/seams.h"
#include "../game/game.h"
#pragma pack(pop)
extern Game* g_game;
short __stdcall GetHeadingBetween(Vec3*, Vec3*);
union Fixed { int value; struct { unsigned short frac; short whole; } parts; };
Vec3 __stdcall DirectionFromAngle(short, Fixed);
static inline Vec3 Direction(short angle, int range) { Fixed distance; distance.value=range; return DirectionFromAngle(angle,distance); }
Vec3 __stdcall AddVec3(const Vec3&, const Vec3&);
void __stdcall AppendOrderToTail(Unit*, Order*);

int __stdcall IssueAttackOrder(Unit*, Unit*, int);
MissionType __stdcall GetOrderType(unsigned char, Unit*, Unit*, int);
void __stdcall AppendOrder(Unit*, Order*);
int __stdcall RandomInt(int);
int __cdecl FUN_004b70ef(short, int);
int __cdecl FUN_004b7123(short, int);
static inline int Contains(unsigned int* bits, unsigned short index) { return bits[index >> 5] & (1 << (index & 31)); }
static inline Vec3 Offset(short angle, int distance) { Vec3 v; v.x=-FUN_004b70ef(angle,distance); v.y=0; v.z=-FUN_004b7123(angle,(int)distance); return v; }

// Used on the target argument of the order-steal calls: the plain chain changes codegen.
static inline Unit* OrderTarget(Order*order) { return order->target; }

// Stays in a file of its own: it matches only in this file's symbol context.
// FUNCTION: 0x40fbe0
int __stdcall VtolFollowOrder(Unit* unit, Order* order, int flags)
{
    if (order->target && !(flags&0x48)) {
        if (unit->spatialBucket==g_game->overflowBucket) {
            Vec3 center;
            center.x=(g_game->mapWidth/2)<<16;
            center.z=(g_game->mapHeight/2)<<16;
            short angle=GetHeadingBetween(&unit->pos,&center);
            Vec3 pos=AddVec3(unit->pos,Direction(angle,0x3200000));
            Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
            move->SetApproachRadius(128);
            order->flags|=0xe0;
            order->SetAttachedFx((OrderFx*)move);
            return 2;
        }
        // Copied through a pointer: assigning order->pos directly changes codegen.
        Vec3* dst=&order->pos;
        *dst=order->target->pos;
        switch(order->state) {
        case 0:
            if (unit->motion && (unit->def->flags1&0x800)) {
                order->AnnounceStatusIfFlagged("Guarding");
                g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
                if (unit->carrier) g_seams.units->AttachUnitToPiece(unit,0,-1,2);
                g_seams.units->SetStateBits(unit, 1,1);
                if ((((UnitMotion*)unit->motion)->flags&3)==1) {
                    ((UnitMotion*)unit->motion)->SetFlightMode(unit,2);
                    Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,unit->pos);
                    move->SetAltitude(unit->def->altitude/2);
                    order->SetAttachedFx((OrderFx*)move);
                    order->flags|=0xe0;
                }
                order->angle=RandomInt(0x10000);
                order->parity=order->angle&1;
                return 1;
            }
            return 7;
        case 1:
            g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
            return 1;
        case 2: {
            Unit* attacker=order->target->attacker;
            if (attacker && !((Owner*)attacker->player)->allied[((Owner*)unit->player)->index]) {
                if (flags & 0x10) {
                    if (!Contains(unit->def->categories,attacker->unitDefIndex)) {
                        if (IssueAttackOrder(unit,attacker,1)) { order->flags=0; return 3; }
                        if (unit->flags & 0x300000) {
                            unsigned char i=0;
                            UnitWeaponSlot* weapon=unit->weapons;
                            do {
                                // the test is in this sense on purpose: it is what
                                // gives the loop the original's jump polarity, its
                                // register for the pointer and its stack slots
                                if (!((weapon->flags&2) && weapon->flags&0x10 && ((unsigned char)(((WeaponDef*)weapon->weapon)->flags >> 26)&1) == 0)) {
                                } else {
                                    Unit* target=g_seams.targeting->GetWeaponTargetUnit(unit,i);
                                    if (!target || !g_seams.weapons->WeaponCanReachUnit(unit,target,i) || Contains(unit->def->weaponCategories[i],target->unitDefIndex))
                                        g_seams.targeting->SetWeaponTargetUnit(unit,attacker,i);
                                }
                                ++i;
                                weapon++;
                            } while (i<3);
                        }
                    }
                }
            }
            if (g_seams.units->CanRepair(unit, 0, order->target)) {
                MissionType kind=GetOrderType(8,unit,order->target,0);
                if (kind.index) {
                    order->SetAttachedFx(0);
                    AppendOrder(unit,new (kSim) Order(kind,order->target,0,0,0,0));
                    order->flags=0; return 3;
                }
            }
            // `order->target->list` is spelled out at every use, with no
            // local for it: that is what makes MSVC reload it after each
            // MissionType constructor call, and what frees the callee-saved
            // register the block's zero constant ends up in.
            if (order->target->list && order->target->list->kind.index && (unit->def->flags1&0x40) &&
                g_seams.units->CanRepair(unit, 0, order->target->list->target) &&
                (order->target->def->flags1&0x40) && order->target->list &&
                (order->target->list->flags_42&0x100000) && unit!=order->target->list->target) {
                int building = order->target->list->kind=="MobileBuild" || order->target->list->kind=="BuildingBuild" || order->target->list->kind=="VTOL_MobileBuild";
                int actionable = ((order->target->list->flags_42&0x200) && order->target->list->target) || (order->target->list->flags_42&0x400);
                MissionType kind;
                if (!building && actionable) {
                    kind=order->target->list->kind;
                    if (kind=="REPAIRUNIT") kind=MissionType("VTOL_REPAIRUNIT");
                    if (kind=="RECLAIM") kind=MissionType("VTOL_RECLAIM");
                    if (kind=="RECLAIMUNIT") kind=MissionType("VTOL_RECLAIMUNIT");
                    if (kind=="HELPBUILD") kind=MissionType("VTOL_HELPBUILD");
                    order->SetAttachedFx(0);
                    AppendOrder(unit,new (kSim) Order(kind,OrderTarget(order)->list->target,&order->target->list->pos,0,0,0));
                    order->flags=0; return 3;
                }
                if (building && order->target->list->target) {
                    order->SetAttachedFx(0);
                    kind=MissionType("VTOL_HelpBuild");
                    AppendOrder(unit,new (kSim) Order(kind,OrderTarget(order)->list->target,&order->target->list->pos,0,0,0));
                    order->flags=0; return 3;
                }
            }
            if (flags&0xe0) order->angle += -RandomInt(0x2000)-0x4000;
            short angle=order->angle;
            Vec3 pos;
            if ((unsigned char)(unit->flags>>31)&1)
                pos=order->target->pos+Offset(angle,(((WeaponDef*)unit->weapons[0].weapon)->range+160)<<16);
            else pos=order->target->pos+Offset(angle,0x1400000);
            Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
            move->SetApproachRadius(128);
            order->SetAttachedFx((OrderFx*)move);
            order->SetDeadlineTicks(30);
            order->flags|=0xf8;
            return 2;
        }
        }
        return 7;
    }
    if (!order->next) AppendOrderToTail(unit,new (kSim) Order("VTOL_SEEKGUARD",order->target,&order->pos,0,0,0));
    return 5;
}

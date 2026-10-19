#include "../../interfaces/seams.h"
// Decompiled by GPT-6 Astra, finished by space-bunny-free, finished by deepseek-v4.1-flash,
// finished by GPT-6.1-sol, finished by Claude Opus 5.5. Names are provisional.
// Kept: the operand order of the six bounds adds follows symbol ids, so this
// header set matters.
#include <memory.h>
#include "../state/block.h"
#include "../state/block.h"
#include <windows.h>
struct PointS16 { short x, y; };
#include "../util/vec3.h"
static inline Vec3 PosMinus(const Vec3& a, const Vec3& v) { Vec3 r; r.x=a.x-v.x; r.y=a.y-v.y; r.z=a.z-v.z; return r; }
static inline Vec3 PosPlus(const Vec3& a, const Vec3& other)
{
    Vec3 r;
    r.x = a.x + other.x;
    r.y = a.y + other.y;
    r.z = a.z + other.z;
    return r;
}

struct Unit;
#include "unit_motion.h"

class Class_0044e730 { public: void SetApproachRadius(int); };
class Class_0044e720 { public: void SetHeading(int); };
class Class_0044e6c0 { public: void SetAltitude(int); };
#include "mission_type.h"

#include "path_order_attach.h"
#pragma pack(push, 1)
struct UnitDef {
    char pad0[0x14a]; PointS16 origin;
    char pad14e[8]; int canBuild; char pad15a[4]; Vec3 min, max;
    char pad176[0x1fe-0x176]; unsigned short buildRate;
    char pad200[0x212-0x200]; unsigned short buildRange;
    char pad214[8]; short altitude; char pad21e[0x241-0x21e]; unsigned int flags,flags2;
};
#include "../units/unit.h"
// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
int __stdcall IsUnitCommander(Unit*);
int __stdcall FindLandingPad(Unit*, int);
unsigned short __stdcall ChooseBuildOption(unsigned int, Unit*);
void __stdcall ClearWeaponTarget(Unit*, int);
void __stdcall DetonateUnitWeapon(Unit*, int);

// Keeps its own view: the shared header's extra declarations move this
// function's bounds-add register allocation.
struct UnitRef {
    void* table;
    Unit* ptr;
    // The empty do-while is a debug check that compiles to nothing.
    Unit* Get() { do {} while (0); return ptr; }
};
#include "order.h"
#include "../game/game.h"
class Class_0044e2d0 { public: char data[0x36]; Class_0044e2d0(Order*,const Vec3&); };
#pragma pack(pop)
extern Game* g_game;
static inline UnitDef* Definitions() { return g_game->defs; }
union Fixed { int value; struct { unsigned short fraction; short whole; }; };
void __stdcall QueueUnitSpeech(Unit*, int, const char*);
void __stdcall MarkSelectionOrdersDirty(Unit*);
void __stdcall AddOrder(MissionType, int, Unit*, Unit*, Vec3*, int, int);
int __stdcall GetHeadingBetween(Vec3*, Vec3*);
void __stdcall StartBuildingScript(Unit*, Order*, short);
int __stdcall WaitIfNotInBuildStance(Unit*, Order*, int);
void __stdcall EmitNanoParticles(Vec3*, Vec3*, int);
static inline PointS16 WorldToCell(Vec3 v, PointS16 origin)
{
    PointS16 c;
    c.x = (v.x - (origin.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (origin.y << 19) + 0x80000) >> 20;
    return c;
}
static inline void CellToWorld(PointS16 origin, PointS16 c, Vec3* v)
{
    v->x = (origin.x + c.x * 2) << 19;
    v->z = (origin.y + c.y * 2) << 19;
}
void __stdcall CellToWorldPos(PointS16,Vec3*,PointS16);
int __cdecl FUN_004b70ef(short,int);
int __cdecl FUN_004b7123(short,int);
short __cdecl FUN_004b715a(int,int);
static inline Vec3 Offset(short angle,int distance) { Vec3 v; v.x=-FUN_004b70ef(angle,distance); v.y=0; v.z=-FUN_004b7123(angle,distance); return v; }
static inline short Angle(Vec3* a,Vec3* b) { return FUN_004b715a(a->x-b->x,a->z-b->z); }
// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
int CheckDirectXVersion(int, int, int, int, int);
struct Feature;
// Stays in a file of its own: it matches only in this file's symbol context.
// FUNCTION: 0x414380
int __stdcall VtolHelpBuildOrder(Unit* unit,Order* order,int flags)
{
    if (order->target && !(flags&8)) {
        if (flags&2) { MarkSelectionOrdersDirty(unit); return 5; }
        unsigned int state=0; state=order->state;
        switch(state) {
        case 0:
        // Two nested ifs, not one &&: part of what places the bounds adds.
        if (unit->motion) {
            if (unit->def->flags&0x800) {
                if (!unit->def->canBuild) return 7;
                order->AnnounceStatusIfFlagged("Building");
                g_seams.weapons->ClaimWeapons((Unit*)unit, 0, 3);
                if (unit->carrier) g_seams.units->AttachUnitToPiece(unit,0,-1,2);
                g_seams.units->SetStateBits(unit, 1,1);
                if ((((UnitMotion*)unit->motion)->flags&3)==1) {
                    ((UnitMotion*)unit->motion)->SetFlightMode(unit,2);
                    Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,unit->pos);
                    ((Class_0044e6c0*)move)->SetAltitude(unit->def->altitude/2);
                    order->SetAttachedFx((OrderFx*)move);
                    order->flags|=0xe0;
                }
                return 1;
            }
        }
        break;
    case 1: {
        order->retries=0;
        Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,order->target->pos);
        ((Class_0044e730*)move)->SetApproachRadius(unit->def->buildRange);
        order->SetAttachedFx((OrderFx*)move);
        order->flags=0xe0;
        return 1;
    }
    case 2:
        if (flags&0x40) return 8;
        if (order->target->buildLeft==0.0f) return 5;
        StartBuildingScript(unit,order,(short)GetHeadingBetween(&unit->pos,&order->target->pos));
        MarkSelectionOrdersDirty(unit);
        return 1;
    case 3: {
        if ((unsigned int)g_game->ticks%150==0) {
            int angle=GetHeadingBetween(&unit->pos,&order->target->pos);
            int range=unit->def->buildRange<<16;
            angle+=0xdb6e;
            Vec3* base=&order->target->pos;
            Vec3 pos=PosMinus(*base,Offset(angle,range));
            Class_0044e2d0* move=new (kSim) Class_0044e2d0(order,pos);
            ((Class_0044e720*)move)->SetHeading((unsigned short)angle);
            order->SetAttachedFx((OrderFx*)move);
        }
        int rate=0; rate=unit->def->buildRate;
        // The ok local is needed: it adds a register candidate that keeps order in esi.
        int ok=g_seams.units->AddBuildProgress(unit,order->target,(float)(rate/30));
        if (ok) {
            Vec3 start;
            g_seams.units->GetNanoPiecePosition(unit,&start);
            Vec3 bounds[2];
            bounds[0]=PosPlus(order->target->pos,order->target->def->min);
            bounds[1]=PosPlus(order->target->pos,order->target->def->max);
            EmitNanoParticles(&start,bounds,6);
        }
        if (order->target->buildLeft!=0.0f) {
            order->SetDeadlineTicks(1);
            order->flags|=0xa;
            return 2;
        }
        return 5;
    }
    }
        return 7;
    }
    QueueUnitSpeech(unit,7,"Construction terminated by hostile action");
    MarkSelectionOrdersDirty(unit);
    return 8;
}

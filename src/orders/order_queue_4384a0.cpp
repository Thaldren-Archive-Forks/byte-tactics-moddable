#include "../../interfaces/seams.h"
// Decompiled by Opus, Haiku, deepseek-v4.1, DeepSeek V4.1 Flash, deepseek-v4.1-flash, space-bunny-free, Space Bunny Free and Claude Opus 5.5. Names are provisional.
// The build-queue object Order and the code around it: the unit's
// list of orders at +0x5c, the order-type table at g_missionOrderTableBegin, the
// StartBuilding and StopBuilding script calls and the objects attached to an
// order. A build-queue object holds a kind from the order-type table, the unit
// it belongs to, a link into its owner's list and an attached object at +0x52.
// Its base (vtable 0x4fd2cc, one slot, the empty 0x43a1e0) has no destructor
// of its own, so the destructor stores only the derived vtable 0x4fd2c8.
#include <stdio.h>
#include <string.h>

class Order;
class HapiBank;

#pragma pack(push, 1)
struct Vec3_0043a1f0 {
    int x, y, z;
};

class CobScript;
class Slot_0043a1f0;

struct Owner_0043a1f0 {
    Slot_0043a1f0* slot;               // +0x0
};

struct UnitType {                      // 0x249 bytes
    char unknown_0[0x20];
    char name[0x20];                   // +0x20
    char unknown_40[0x18a - 0x40];
    float metalCost;                   // +0x18a
    char unknown_18e[0x1fa - 0x18e];
    unsigned int maxHealth;            // +0x1fa
    unsigned short field_1fe;          // +0x1fe
    char unknown_200[0x241 - 0x200];
    union {
        unsigned char flags;           // +0x241
        unsigned int flags32;          // +0x241
    };
    char unknown_245[0x249 - 0x245];
};

#include "../units/unit.h"

struct Entry_0043a1f0 {                // 0x19-byte entries at g_missionOrderTableBegin
    char unknown_0[4];
    void (__stdcall* notify)(Unit* unit, Order* obj, int code);            // +0x4
    char unknown_8[0x11 - 0x8];
    union {
        unsigned int flags;            // +0x11, default flags of the kind
        struct {
            char unknown_11[3];
            unsigned char flag14;      // +0x14
        };
    };
    char* name;                        // +0x15
};

#include "../game/game.h"

struct SaveDesc_0043a1f0 {             // the 0x3a-byte snapshot, read and written raw
    unsigned short unitType;           // +0x00
    unsigned short ownerType;          // +0x02
    int field_4;                       // +0x04
    unsigned char kind;                // +0x08
    unsigned char flag5;               // +0x09
    unsigned int flags6;               // +0x0a
    int last_id;                       // +0x0e
    Vec3_0043a1f0 pos;                 // +0x12
    int start;                         // +0x1e
    int cached;                        // +0x22
    int field_26;                      // +0x26, the order's unitType (+0x36); +0x00 has the name
    int radius;                        // +0x2a
    int progress;                      // +0x2e
    unsigned int flags;                // +0x32
    int subFlags;                      // +0x36
};
#pragma pack(pop)

#include "../state/block.h"
#include "../units/cob_script.h"

#include "../units/unit_ref.h"

#include "../util/vec3.h"
#include "air_maneuver_order.h"

// Unused here: forward declarations of real functions; their symbol ids keep
// ComputeReclaimDamagePulse (0x438650) matching (docs/c2-regalloc.md).
void UpdateMouseScroll();
void UpdateEdgeScroll();
void CenterCameraOnRadarClick();
void CenterCameraOnStartPosition();
void RegisterDataArchives();
int GetCdPathMismatch();
void CreateGameObject();
void InitMissionStatus();
void SetUpEndMissionScreen();
void StartScreenFade();
void StepScreenFade();
void ScheduleFadeTick();
int IsFadeDone();
void StepPaletteFade();

class Slot_0043a1f0 {
public:
    virtual void Slot0();
    virtual void Attach(void* obj);
    OrderFx* current;                  // +0x4
};

int __stdcall OrderTypeNameLess(int param_1, char* param_2);
Entry_0043a1f0* __stdcall LowerBoundOrderTypes(Entry_0043a1f0* first, Entry_0043a1f0* last,
                                       char* const& value, int(__stdcall* pred)(int, char*),
                                       int* unused);
int __stdcall SendScriptCallNoArgs(Unit* obj, short index);
void __stdcall SendScriptCall(Unit* obj, int index, int param_3, int param_4, int param_5, int param_6, int param_7);
void __stdcall QueueUnitSpeech(Unit* unit, int kind, char* text);

extern Game* g_game;
extern Entry_0043a1f0* g_missionOrderTableBegin;
extern Entry_0043a1f0* g_missionOrderTableEnd;

#include "path_order_attach.h"

// An order type held as its index in the sorted order-type table. Callers
// build it from a name as a by-value temporary (0x403260, 0x4118e0, ...), so
// this is its constructor.
#include "mission_type.h"

#pragma pack(push, 1)
#include "path_fx.h"

// Unused here: real functions declared for their symbol ids, which take the
// place of the Point view this file used to define (docs/c2-regalloc.md).
int CloakOnOrder(char*, int, int);
int GetUnitCount(int, unsigned int);

#include "approach_areas.h"

#pragma pack(pop)

#include "order.h"

// Links `child` into the owner's list in front of `before`.
static inline void InsertBefore(Unit* p, Order* child, Order* before)
{
    Order** link = (child->flags_42 & 0x40000) ? &p->list2 : &p->list;
    while (*link != before) {
        link = &(*link)->next;
    }
    *link = child;
    child->unit = p;
    child->next = before;
    if (before != 0) {
        child->flags_42 |= before->flags_42 & 0x4000;
    }
}

// If the owner has a count at +0x86, drop every object without flag 4 from its
// list at +0x5c (marking all but the head 0x10000 first, as DeleteOrders does),
// then build a "BECARRIED" object and insert it at the head of the list its
// own flag 0x40000 selects.
// FUNCTION: 0x4384a0
void __stdcall AddBeCarriedOrder(Unit* p)
{
    if (p->carrier) {
        Order* first = p->list;
        Order** pp = &p->list;
        Order* node;
        while ((node = *pp) != 0) {
            if (node->flags_42 & 4) {
                pp = &node->next;
            } else {
                *pp = node->next;
                if (node != first) {
                    node->flags_42 |= 0x10000;
                }
                delete node;
            }
        }
        Order* child =
            new (kSim) Order("BECARRIED", (Unit*)p->carrier, 0, 0, 0, 0);
        InsertBefore(p, child, (child->flags_42 & 0x40000) ? p->list2 : p->list);
    }
}

// FUNCTION: 0x438590
void __stdcall StartBuildingScript(Unit* obj, Order* target, unsigned short param_3)
{
    int index = g_seams.units->FindScript((CobScript*)obj->script, "StartBuilding");
    g_seams.units->StartScriptWithArgsByIndex((CobScript*)obj->script, index, 0, 0, 1, param_3, 0, 0, 0);
    SendScriptCall(obj, index, 1, param_3, 0, 0, 0);
    target->flags_42 |= 0x400000;
}

// FUNCTION: 0x4385f0
void __stdcall StopBuildingScript(Unit* obj, Order* target)
{
    if (target->flags_42 & 0x400000) {
        int index = g_seams.units->FindScript((CobScript*)obj->script, "StopBuilding");
        g_seams.units->StartScriptWithArgsByIndex((CobScript*)obj->script, index, 0, 0, 0, 0, 0, 0, 0);
        SendScriptCallNoArgs(obj, index);
        target->flags_42 &= ~0x400000;
    }
}

// FUNCTION: 0x438650
int __stdcall ComputeReclaimDamagePulse(Unit* a, Unit* b, int n)
{
    UnitType* bt = ((UnitType *)b->def);
    float v = bt->metalCost > 10.0f ? bt->metalCost : 10.0f;
    // The 64-bit numerator is built by hand (signed 32-bit chain in lo, zero hi):
    // keeps the multiply order and the unsigned fild qword.
    union {
        __int64 q;
        struct {
            int lo;
            int hi;
        } w;
    } p;
    p.w.lo = ((UnitType *)a->def)->field_1fe * ((a->killCount + 5) / 5) * (int)bt->maxHealth * n;
    p.w.hi = 0;
    int r = (int)((double)p.q / (v * 300.0f));
    if (r <= 1) {
        r = 1;
    }
    return r;
}

// FUNCTION: 0x438700
unsigned int __stdcall WaitIfNotInBuildStance(void* param_1, void* param_2, unsigned int param_3)
{
    if ((*(unsigned char*)((char*)param_1 + 0x10f) & 1) == 0) {
        *(unsigned int*)((char*)param_2 + 6) = param_3 | 4;
        return 2;
    }
    return 1;
}

// FUNCTION: 0x438730
int __stdcall WaitIfCobBusy(char* param_1, char* param_2, unsigned int param_3)
{
    if (*(unsigned char*)(param_1 + 0x10f) & 2) {
        *(unsigned int*)(param_2 + 6) = param_3 | 4;
        return 2;
    }
    return 1;
}

extern int __cdecl _strcmpi(const char*, const char*);

// FUNCTION: 0x438760
MissionType::MissionType(const char* name)
{
    Entry_0043a1f0* first = g_missionOrderTableBegin;
    int n = g_missionOrderTableEnd - g_missionOrderTableBegin;
    for (; 0 < n; ) {
        int n2 = n / 2;
        Entry_0043a1f0* m = first;
        m += n2;
        int less = _strcmpi(m->name, name) < 0;
        if (less)
            first = ++m, n -= n2 + 1;
        else
            n = n2;
    }
    if (first != g_missionOrderTableEnd && _strcmpi(first->name, name) == 0) {
        index = (unsigned char)(first - g_missionOrderTableBegin);
        return;
    }
    index = 0;
}

struct OrderType;

struct OrderType
{
public:
    int GetTableEntry();
};

// FUNCTION: 0x438830
int OrderType::GetTableEntry()
{
    unsigned int result = 0;
    result = *(unsigned char*)this;
    int* base = (int*)&g_missionOrderTableBegin;
    int fives = result * 5;
    return *base + fives * 4 + fives;
}

// FUNCTION: 0x438850
int __fastcall GetTableEntryByTypeByte(unsigned char* param_1)
{
    unsigned int eax = 0;
    eax = *param_1;
    eax = eax + eax * 4;
    int edx = (int)g_missionOrderTableBegin + eax * 4;
    eax = eax + edx;
    return eax;
}

// FUNCTION: 0x438870
void Order::OrStatusFlags(unsigned int param_1)
{
    subFlags |= param_1;
}

// Clears flag 0x2000 of an order and, if it was set, posts message kind 5
// with the given text for the order's unit.
// FUNCTION: 0x438880
void Order::AnnounceStatusIfFlagged(const char* text)
{
    if (flags_42 & 0x2000) {
        flags_42 &= ~0x2000;
        QueueUnitSpeech(unit, 5, (char*)text);
    }
}

// FUNCTION: 0x4388b0
void Order::ReattachFxToUnit()
{
    if (attached != 0) {
        void* p1 = *(void**)unit;
        void* p2 = *(void**)p1;
        void* p3 = *(void**)p2;
        void* fn_ptr = *(void**)((char*)p3 + 4);
#ifdef __clang__
        ((void (__thiscall*)(void*, int))fn_ptr)(p2, (int)(OrderFx*)attached);
#else
        ((void (__stdcall*)(int))fn_ptr)((int)(OrderFx*)attached);
#endif
    }
}

// FUNCTION: 0x4388d0
void Order::SetAttachedFx(OrderFx* obj)
{
    if (((Owner_0043a1f0 *)unit->motion)) {
        if (attached) {
            ((Owner_0043a1f0 *)unit->motion)->slot->Attach(0);
            delete (OrderFx*)attached;
            attached = 0;
        }
        if (obj) {
            subFlags &= ~0x3e0;
            ((Owner_0043a1f0 *)unit->motion)->slot->Attach(obj);
            attached = obj;
        }
    }
}

// Creates an ApproachRadius from an order position when the unit's definition
// does not have flag 0x800 set, detaches the current attachment and attaches
// the new one. Same class as 0x4388d0; the flag test comes first here.
// FUNCTION: 0x438930
void Order::AttachApproachRadiusGoal(Vec3* p, int n)
{
    if ((((UnitType *)unit->def)->flags32 & 0x800) == 0) {
        ApproachRadius* obj = new (kSim) ApproachRadius(this, p->x, p->z, n);
        if (((Owner_0043a1f0 *)unit->motion)) {
            if (attached) {
                ((Owner_0043a1f0 *)unit->motion)->slot->Attach(0);
                delete (OrderFx*)attached;
                attached = 0;
            }
            if (obj) {
                subFlags &= ~0x3e0;
                ((Owner_0043a1f0 *)unit->motion)->slot->Attach(obj);
                attached = obj;
            }
        }
    } else {
        if (((Owner_0043a1f0 *)unit->motion) && attached) {
            ((Owner_0043a1f0 *)unit->motion)->slot->Attach(0);
            delete (OrderFx*)attached;
            attached = 0;
        }
    }
}

// FUNCTION: 0x438a00
void Order::AttachRingApproachGoal(Vec3* pos, int radius1, int radius2)
{
    if (!(((UnitType *)unit->def)->flags32 & 0x800)) {
        RingApproach* obj = new (kSim) RingApproach(this, pos->x, pos->z, radius1, radius2);
        if (((Owner_0043a1f0 *)unit->motion)) {
            if (attached) {
                ((Owner_0043a1f0 *)unit->motion)->slot->Attach(0);
                delete (OrderFx*)attached;
                attached = 0;
            }
            if (obj) {
                subFlags &= ~0x3e0;
                ((Owner_0043a1f0 *)unit->motion)->slot->Attach(obj);
                attached = obj;
            }
        }
    } else {
        if (((Owner_0043a1f0 *)unit->motion)) {
            if (attached) {
                ((Owner_0043a1f0 *)unit->motion)->slot->Attach(0);
                delete (OrderFx*)attached;
                attached = 0;
            }
        }
    }
}

// FUNCTION: 0x438ad0
void Order::AttachBuildFootprintMarker(Point16 cell, Point16 size)
{
    if (!(((UnitType *)unit->def)->flags32 & 0x800)) {
        PointMarker* obj = new (kSim) PointMarker(this, cell, size);
        if (((Owner_0043a1f0 *)unit->motion)) {
            if (attached) {
                ((Owner_0043a1f0 *)unit->motion)->slot->Attach(0);
                delete (OrderFx*)attached;
                attached = 0;
            }
            if (obj) {
                subFlags &= ~0x3e0;
                ((Owner_0043a1f0 *)unit->motion)->slot->Attach(obj);
                attached = obj;
            }
        }
    } else {
        if (((Owner_0043a1f0 *)unit->motion)) {
            if (attached) {
                ((Owner_0043a1f0 *)unit->motion)->slot->Attach(0);
                delete (OrderFx*)attached;
                attached = 0;
            }
        }
    }
}

// Changes the object's kind: stores the new kind and reloads its flags from
// the kind table (g_missionOrderTableBegin, 0x19-byte entries, default flags at +0x11),
// keeping the object's own bits 9 and 10 (0x600; the constructor 0x43a0c0
// clears them individually). The layout (kind at +0x4, flags at +0x42) is
// Order's.
// FUNCTION: 0x438b90
void Order::MergeFlagsFromTable(int k)
{
    kind.index = k;
    // The two table index expressions must differ, or the entry load is shared.
    flags_42 = ((g_missionOrderTableBegin[k & 0xff].flags ^ flags_42) & 0x600) ^ g_missionOrderTableBegin[kind.index].flags;
}

// FUNCTION: 0x438be0
int __stdcall GetOrderFlags(void* param)
{
    if (!param) return 0;
    void* ptr1 = *(void**)((char*)param + 0x5c);
    if (!ptr1) return 0;
    return *(int*)((char*)ptr1 + 0x42);
}

#include "../util/hapi_bank.h"

// Destructor (callers do `if (p) { p->~X(); operator delete(p); }`): notifies
// the owner through a callback table, stops the unit's build animation (the
// same code as StopBuildingScript), releases the attached object at +0x52 and
// unlinks the list node at +0x12 (0x489650 is the link's destructor).
// FUNCTION: 0x43a1f0
Order::~Order()
{
    if (flags & 2) {
        g_missionOrderTableBegin[kind.index].notify(unit, this, 2);
    }
    if (flags_42 & 0x400000) {
        Unit* obj = unit;
        int index = g_seams.units->FindScript((CobScript*)obj->script, "StopBuilding");
        g_seams.units->StartScriptWithArgsByIndex((CobScript*)obj->script, index, 0, 0, 0, 0, 0, 0, 0);
        SendScriptCallNoArgs(obj, index);
        flags_42 &= ~0x400000;
    }
    if (((Owner_0043a1f0 *)unit->motion) != 0) {
        Slot_0043a1f0* slot = ((Owner_0043a1f0 *)unit->motion)->slot;
        if (slot->current != 0 && slot->current == attached) {
            if (attached != 0) {
                slot->Attach(0);
                delete (OrderFx*)attached;
                attached = 0;
            }
        } else {
            delete (OrderFx*)attached;
            attached = 0;
        }
    }
    if (!(flags_42 & 0x10000)) {
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, 3);
    }
    g_seams.units->UnitRefUnlink((UnitRef*)&link);
}

// The real resolver at 0x43a360 (matched in its own file), defined here without
// its annotation as in the original file, which /Ob2 inlines into the constructor.
short __stdcall ResolveUnitTypeKey(HapiBank* file, unsigned short id)
{
    char key[0x80];
    sprintf(key, "UTYPENAME%4d", id);
    if (file->HasItem(key))
        return g_seams.units->FindUnitTypeId(file->GetStringItem(key, 0));
    int i, n = 0;
    unsigned short k = 0;
    for (i = 1; i < g_game->unitDefCount; i++, n++) {
        if (!(((UnitType *)g_game->defs)[(unsigned short)i].flags & 0x20)) {
            if (k == id)
                return n;
            k++;
        }
    }
    return 0;
}

static unsigned char KindByName_0043a420(char* s)
{
    Entry_0043a1f0* e = LowerBoundOrderTypes(g_missionOrderTableBegin, g_missionOrderTableEnd, s, OrderTypeNameLess, 0);
    if (e == g_missionOrderTableEnd || _strcmpi(e->name, s) != 0)
        return 0;
    return (unsigned char)(e - g_missionOrderTableBegin);
}

static unsigned char KindByIndex_0043a420(unsigned char want)
{
    // Stays a helper with idx declared before k: inline, the scan keeps its counter in ECX.
    int idx = 0;
    int k = 0;
    for (Entry_0043a1f0* p = g_missionOrderTableBegin; p <= g_missionOrderTableEnd; p++, idx++) {
        if (!(p->flag14 & 1)) {
            if (k == want)
                break;
            k++;
        }
    }
    return (unsigned char)idx;
}

// The constructor of Order that loads the object from the parsed
// text file (the other one is 0x43a0c0; 0x43a970 is the matching writer).
// data/symbols.csv still names this address Order::Order
// (the name its caller 0x487080 uses), but the vtables it stores are
// Order's, so it is that class's second constructor.
//
// The kind is resolved by two helpers, one per branch, each
// returning an unsigned char: by name through the sorted kind table
// (lower_bound), or, for files without a "<name>_name" key, by counting the
// entries without flag bit 0. The real resolver
// ResolveUnitTypeKey is defined above.
// The case-3 attachment constructor is AirManeuverOrder's second constructor,
// 0x44e7d0, which shares its name with 0x44e740; data/aliases.csv has a row
// for it.
// FUNCTION: 0x43a420
Order::Order(Unit* punit, HapiBank* file, char* name)
    : link(0, 0)
{
    link.SetValue(this);
    link.SetUnit(0);
    next = 0;
    attached = 0;
    created = (unsigned int)g_game->ticks;
    if (file == 0)
        return;
    if (name == 0)
        return;
    if (strlen(name) > 0x1f)
        return;

    file->OpenNamedBox(name);
    file->SeekBox(0);
    SaveDesc_0043a1f0 desc;
    if (file->ReadBox(&desc, 0x3a) != 0x3a)
        return;

    char buf1[0x80];
    sprintf(buf1, "%s%s", name, "_name");
    {
        char* s = file->GetStringItem(buf1, 0);
        if (s != 0)
            desc.kind = KindByName_0043a420(s);
        else
            desc.kind = KindByIndex_0043a420(desc.kind);
    }

    Unit* u;
    if (desc.unitType == 0)
        u = 0;
    else
        u = g_game->units + desc.unitType;
    if (punit != u)
        return;
    if (desc.kind == 0)
        return;

    unit = punit;
    link.SetUnit(g_seams.units->LoadUnit(desc.ownerType, file));

    kind.index = desc.kind;
    state = desc.flag5;
    flags = desc.flags6;
    wakeFrame = desc.last_id;
    pos = *(Vec3*)&desc.pos;
    start = *(Point16*)&desc.start;
    cached = *(Point16*)&desc.cached;
    unitType = desc.field_26;
    radius = desc.radius;
    progress = desc.progress;
    flags_42 = desc.flags;
    subFlags = desc.subFlags;

    char* sname = g_missionOrderTableBegin[desc.kind].name;
    if (strcmp(sname, "MobileBuild") == 0 || strcmp(sname, "VTOL_MobileBuild") == 0 ||
        strcmp(sname, "BuildingBuild") == 0) {
        unitType = (unsigned short)ResolveUnitTypeKey(file, (unsigned short)unitType);
    }

    char buf3[0x20];
    sprintf(buf3, "%s%s", name, "g");
    switch (desc.field_4) {
    case 2:
        attached = new (kSim) Class_0044de80((int)this, file, buf3);
        return;
    case 3:
        attached = new (kSim) AirManeuverOrder((int)this, file, buf3);
        return;
    case 4:
        attached = new (kSim) ApproachRadius((int)this, file, buf3);
        return;
    case 5:
        attached = new (kSim) RingApproach((int)this, file, buf3);
        return;
    case 6:
        attached = new (kSim) PointMarker((int)this, file, buf3);
        return;
    case 0:
        attached = 0;
        return;
    case 1:
        attached = 0;
        return;
    default:
        attached = 0;
        return;
    }
}

// Serialises one build-queue object (Order) into the parsed text file
// under the key `name`: a raw 0x3a-byte snapshot, a "<name>_name" string value
// pointing at the kind's name, a "UTYPENAME<id>" value when the object is a
// build kind, and finally "<name>g" handed to the attached object.
// FUNCTION: 0x43a970
int Order::SerializeToSave(Unit* punit, HapiBank* file, char* name)
{
    if (unit->id != punit->id || !file || !name)
        return 0;
    if (strlen(name) > 0x1f)
        return 0;

    SaveDesc_0043a1f0 desc;
    // A local copy of the pointer and the test written as `== 0` is what the
    // original wants here: the copy stops if-conversion, and the polarity
    // gives the `jne` over the zero store instead of the `je` after it.
    Unit* u = unit;
    if (u == 0)
        desc.unitType = 0;
    else
        desc.unitType = u->id;
    // The owner is tested for null twice on purpose: mixing the copy with a
    // fresh read of link.owner keeps the compiler from dropping the third test.
    Unit* o = link.owner;
    desc.ownerType = (o != 0 && (link.owner->flags & 0x10000000) != 0 && link.owner != 0) ? o->id : 0;
    desc.field_4 = attached ? ((OrderFx*)attached)->GetType() : 0;
    desc.kind = kind.index;
    desc.flag5 = state;
    desc.flags6 = flags;
    desc.last_id = wakeFrame;
    desc.pos = *(Vec3_0043a1f0*)&pos;
    desc.start = *(int*)&start;
    desc.cached = *(int*)&cached;
    desc.field_26 = unitType;
    desc.radius = radius;
    desc.progress = progress;
    desc.flags = flags_42;
    desc.subFlags = subFlags;

    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&desc, 0x3a);

    char buf1[0x80];
    sprintf(buf1, "%s%s", name, "_name");
    file->SetStringItem(buf1, g_missionOrderTableBegin[kind.index].name);

    char* s = g_missionOrderTableBegin[kind.index].name;
    if (strcmp(s, "MobileBuild") == 0 || strcmp(s, "VTOL_MobileBuild") == 0 ||
        strcmp(s, "BuildingBuild") == 0) {
        unsigned short id = unitType;
        char buf2[0x80];
        sprintf(buf2, "UTYPENAME%4d", id);
        if (!file->HasItem(buf2) && id >= 1 && id < g_game->unitDefCount)
            file->SetStringItem(buf2, ((UnitType *)g_game->defs)[id].name);
    }

    if (desc.field_4 != 0) {
        char buf3[0x20];
        sprintf(buf3, "%s%s", name, "g");
        ((OrderFx*)attached)->SerializeToSave(this, file, buf3);
    }
    return 1;
}

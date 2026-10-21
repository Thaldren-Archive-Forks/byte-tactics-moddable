// Decompiled by Opus and deepseek-v4.1-flash. Names are provisional.

#include <vector>
#include "../state/sim_allocator.h"
#include "../state/block.h"
#include <algorithm>

#include "squad.h"

#include "../orders/mission_type.h"

#pragma pack(push, 1)
struct Unit;

#include "../network/player.h"

#include "unit.h"
#pragma pack(pop)

void __cdecl GameFreeThunk(int* p);
#include "../../interfaces/seams.h"

// Allocates the owner's ten squads and constructs each in place with the
// owner and its index (Squad's constructor, inlined here).
// FUNCTION: 0x480190
void __stdcall CreateSquads(Player* owner)
{
    owner->groups = (Group*)SimAllocTagged("SQUADS", 10 * sizeof(Squad));
    for (int i = 0; i < 10; i++)
    {
        Squad* squad = &((Squad*)owner->groups)[i];
        squad->owner = (int)owner;
        squad->index = i;
        squad->field_8 = 0;
        squad->field_c = 0;
        new (&squad->items) SimVector<int>;
    }
}

// Destroys the owner's ten squads allocated by 0x480190 and frees them.
// FUNCTION: 0x4801f0
void __stdcall FreeSquads(Player* owner)
{
    if (owner->groups) {
        for (int i = 0; i < 10; i++)
            ((Squad*)owner->groups)[i].items.~vector();
        GameFreeThunk((int*)owner->groups);
        owner->groups = 0;
    }
}

// Removes `u` from `v` by moving the last element into its slot and erasing
// the last element. Kept as an inline helper: the
// original translation unit had this as its own small function.
static inline void RemoveFast(SimVector<Unit*>& v, Unit* u)
{
    SimVector<Unit*>::iterator it = std::find(v.begin(), v.end(), u);
    if (it != v.end()) {
        SimVector<Unit*>::iterator last = v.end() - 1;
        *it = *last;
        // erase(end() - 1), not pop_back().
        v.erase(last);
    }
}

// Moves a unit to the unit group (squad) `index`: first removes it from the
// group it is currently in (unit+0xac, -1 means none), then adds it to the
// new group, then records the new group index. Each player owns ten
// 0x20-byte Squad squads at player+0x78 (built by 0x480190); the
// squad holds its units in the SimVector<Unit*> at +0x10.
//
// The removal itself overwrites the found slot with the last element and then
// erases the last element (order is not preserved).
// FUNCTION: 0x480250
void __stdcall SetUnitSquad(Unit* unit, int index)
{
    // RemoveFast stays a separate inline helper: a flat body inlines push_back's size() calls too.
    if (unit->group != -1)
        RemoveFast((SimVector<Unit*>&)((Squad*)((Player *)unit->player)->groups)[unit->group].items, unit);
    if (index != -1)
        ((SimVector<Unit*>&)((Squad*)((Player *)unit->player)->groups)[index].items).push_back(unit);
    unit->group = index;
}

// For every active unit in the array owned by `owner` whose field at +0xac
// equals `key`, asks GetOrderType for an order kind and hands it, with the
// remaining arguments, to AddOrder.
// FUNCTION: 0x480460
void __stdcall OrderSquad(Player* owner, int key, unsigned char mode, int remove,
                            Unit* target, int flags, int param_7, int param_8)
{
    for (Unit* u = owner->unitsBegin; u <= owner->unitsEnd; u++) {
        if (((short)u->unitDefIndex) != 0 && u->group == key) {
            MissionType kind = g_seams.orders->GetOrderType(mode, u, target, (Vec3*)flags);
            g_seams.orders->AddOrder(kind, remove, u, target, (Vec3*)flags, param_7, param_8);
        }
    }
}

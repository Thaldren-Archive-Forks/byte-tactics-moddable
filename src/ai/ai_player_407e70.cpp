// Decompiled by Claude Opus 5.5, deepseek-v4.1-flash and Opus. Names are
// provisional.
// Slot 0 of SpatialTimer (vtable 0x4fc9a0), derived from SquadTimer
// (the family is listed in ai_player.cpp, whose declarations this copies).
// Sets next to 30..179 ticks from now. When the group has units, moves the
// probe point by the step (one time in ten it restarts from best with a new
// random direction of length 0x140 map units), and when the owner can see or
// has explored the probe point, keeps it as the new best if a random roll
// favours its SumUnitRatingsInRange score. Then orders every unit whose def has
// flag4 set, and that is active or can reach best (WeaponCanReachPos), to move to
// best with the order GetOrderType picks.
//
// The explored-map test is the inlined player method 0x475470 describes: a
// {data, width, height} ByteMap at +0x7c.
#include <vector>

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

struct Position_00408090 {              // 16.16 fixed point; only high words read
    short xFrac;
    short x;                            // +0x2
    short yFrac;
    short y;                            // +0x6
    short zFrac;
    short z;                            // +0xa
};

#include "../network/player.h"

int __stdcall IsPointVisible(Player* player, Position_00408090* pos);

static inline int IsExplored(Player* player, Position_00408090* pos)
{
    // Plain int: an unsigned tx shortens the shift to 16 bits.
    int tx = pos->x >> 5;
    int ty = (pos->z - (pos->y >> 1)) >> 5;
    if ((unsigned int)tx < (unsigned int)player->exploredWidth && (unsigned int)ty < (unsigned int)player->exploredHeight &&
        player->explored[(unsigned int)player->exploredWidth * ty + tx])
        return 1;
    return 0;
}

static inline int IsVisible(Player* player, Position_00408090* pos)
{
    if ((g_game->mapFlags & 2) == 2)
        return IsExplored(player, pos);
    return IsPointVisible(player, pos);
}

#include "../units/unit_def.h"

// The flags at +0x245 read as the original's bitfield; the header keeps the
// plain word.
#pragma pack(push, 1)
struct UnitDefFlags_00407e90 {
    char unknown_0[0x245];
    unsigned int unknown_bits : 4;
    unsigned int flag4 : 1;             // +0x245 bit 4
};

#pragma pack(pop)

#include "../units/unit.h"

#include "../orders/mission_type.h"

#include "squad_manager.h"

#include "group.h"
#include "squad_timer.h"

static inline Vec3 MakeVec3(int ax, int ay, int az)
{
    Vec3 v;
    v.x = ax;
    v.y = ay;
    v.z = az;
    return v;
}

int __stdcall RandomInt(int range);
int __cdecl FUN_004b70ef(int angle, int distance);
int __cdecl FUN_004b7123(int angle, int distance);

static inline Vec3 Offset(int angle, int distance)
{
    Vec3 v;
    v.x = -FUN_004b70ef(angle, distance);
    v.y = 0;
    v.z = -FUN_004b7123(angle, distance);
    return v;
}

int __stdcall SumUnitRatingsInRange(int index, Vec3* pos, int range);
#include "../../interfaces/seams.h"

// Constructor 0x407d40 defined again, unannotated: emits the vtable and the
// scalar deleting destructor.
// FUNCTION: 0x407e70 ??_GSpatialTimer@@UAEPAXI@Z
SpatialTimer::SpatialTimer(SquadManager* p, void* q)
    : SquadTimer(p, (Group*)q)
{
    // Half of g_game's mapWidthWorld and mapHeightWorld, in 16.16 fixed point.
    int x = (int)(g_game->baseX / 2 * 65536.0);
    best = MakeVec3(x, 0, (int)(g_game->baseY / 2 * 65536.0));
    x = (int)(g_game->baseX / 2 * 65536.0);
    probe = MakeVec3(x, 0, (int)(g_game->baseY / 2 * 65536.0));
    x = (int)(g_game->baseX / 2 * 65536.0);
    step = MakeVec3(x, 0, (int)(g_game->baseY / 2 * 65536.0));
    bestRating = 0;
}

// FUNCTION: 0x407e90
void SpatialTimer::OnTimer()
{
    // Computed first or the sum folds into one lea.
    int delay = RandomInt(150) + 30;
    next = g_game->ticks + delay;
    if (group->units.empty())
        return;
    // Unused on purpose: it emits the operator delete call after the loop.
    std::vector<Unit*> unused;
    if (RandomInt(10) == 0) {
        probe = best;
        int angle = RandomInt(0x10000);
        // Offset() keeps the call order and the zero y.
        step = Offset(angle, 0x1400000);
    }
    probe += step;
    if (IsVisible((Player*)group->player, (Position_00408090*)&probe)) {
        int r = SumUnitRatingsInRange(player, &probe, 0xa0);
        // Operand order: the bestRating roll is called first.
        if (RandomInt(r) > RandomInt(bestRating)) {
            bestRating = r;
            best = probe;
        }
    }
    for (std::vector<Unit*>::iterator it = group->units.begin();
         it != group->units.end(); ++it) {
        Unit* u = *it;
        if (((UnitDefFlags_00407e90*)u->def)->flag4) {
            if (u->motion || g_seams.weapons->WeaponCanReachPos(u, (Vec3*)&u->pos, (Vec3*)&best, 0)) {
                MissionType kind = g_seams.orders->GetOrderType(3, u, 0, (Vec3*)&best);
                if (kind.index)
                    g_seams.orders->AddOrder(kind, 0, u, 0, (Vec3*)&best, 0, 0);
            }
        }
    }
}

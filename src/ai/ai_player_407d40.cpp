// Decompiled by GPT-6, finished by deepseek-v4.1-flash, verified by GPT-6.1-sol, finished by space-bunny-free, edited by deepseek-v4.1, finished by deepseek-v4.1-flash. Names are provisional.
// The rest of the class is in ai_player_407e70.cpp.
#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

struct SquadManager;  // the struct key stays in the mangled name of 0x407d40
#include "squad_manager.h"

#include "squad_timer.h"

SquadTimer::SquadTimer(SquadManager* p, void* q)
    : owner(p), group((Group*)q), next(0), player(p->index) {}

static inline Vec3 MakeVec3(int ax, int ay, int az)
{
    Vec3 v;
    v.x = ax;
    v.y = ay;
    v.z = az;
    return v;
}

// FUNCTION: 0x407d40
// FUNCTION: 0x407e70 ??_GSpatialTimer@@UAEPAXI@Z
SpatialTimer::SpatialTimer(SquadManager* p, void* q)
    : SquadTimer(p, (Group*)q)
{
    int x = (int)(g_game->baseX / 2 * 65536.0);
    best = MakeVec3(x, 0, (int)(g_game->baseY / 2 * 65536.0));
    x = (int)(g_game->baseX / 2 * 65536.0);
    probe = MakeVec3(x, 0, (int)(g_game->baseY / 2 * 65536.0));
    x = (int)(g_game->baseX / 2 * 65536.0);
    step = MakeVec3(x, 0, (int)(g_game->baseY / 2 * 65536.0));
    bestRating = 0;
}

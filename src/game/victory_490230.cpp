// Decompiled by deepseek-v4.1-flash. Names are provisional.
// The mission's defeat check. It does nothing unless the mission is active
// (the dword at +0x88, set to 1 by the constructor, 0x48df90), then asks the game mode
// which test to run: 1 runs the victory conditions (0x48fed0),
// 2 runs the "every other player is dead or allied" loop (0x48ffd0)
// and 3 calls CheckAlliedVictory.

#include "../map/mission.h"

#include "victory_conditions.h"

#include "../network/player.h"
#include "../state/block.h"
#include "../../interfaces/seams.h"

#pragma pack(push, 1)
#include "game.h"
#pragma pack(pop)

extern Game* g_game;

// 0x490080 is a __thiscall method in the original (the caller loads ecx with
// this), but its own file matched it as a free function, so it is declared
// here as a __fastcall free function: that still passes the first argument in
// ecx and gives the linker the established name CheckAlliedVictory.
int __fastcall CheckAlliedVictory(void* self);

inline int MissionConditions::AllVictoryConditionsMet()
{
    if (victoryCount == 0) {
        victory[victoryCount] = new (kSim) VictoryDestroyAllUnits;
        victoryCount++;
    }
    for (int i = 0; i < victoryCount; i++) {
        if (!victory[i]->IsSatisfied())
            return 0;
    }
    return 1;
}

// FUNCTION: 0x490230
int MissionConditions::CheckVictory()
{
    if (active == 0)
        return 0;
    switch (g_seams.map->GetGameType(g_game->net)) {
    case 1:
        return AllVictoryConditionsMet();
    case 2: {
        unsigned char player = g_game->localPlayer;
        Player* me = &g_game->players[player];
        for (unsigned char i = 0; i < 10; i++) {
            // Separate unsigned char copy of the counter: puts it in the base slot
            // and keeps the zero-extension after the player test.
            unsigned char j = i;
            if (i == g_game->localPlayer)
                continue;
            if (me->allied[j])
                continue;
            if (g_game->players[j].unitCount != 0)
                return 0;
        }
        return 1;
    }
    case 3:
        return CheckAlliedVictory(this);
    }
    return 0;
}

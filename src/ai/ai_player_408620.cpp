// Decompiled by DeepSeek V4.1 Flash. Names are provisional.
#include "win_types.h"
#include <vector>

#pragma pack(push, 1)
struct Item_00408620 {                 // 0x249 bytes
    char unknown_0[0x20];
    char name[0x249 - 0x20];           // +0x20
};

#include "../game/game.h"
#pragma pack(pop)

#include "../units/unit.h"

extern Game* g_game;

unsigned short __stdcall ChooseBuildOption(int player, Unit* unit);
void __stdcall QueueBuildOrder(char* name, Unit* unit, int count);

#include "group.h"
#include "squad_timer.h"

// FUNCTION: 0x408620
void BuildTimer::TryIssueIdleFactoryBuildOrder(Unit* unit)
{
    if (unit->list != 0)
        return;
    unsigned short idx = ChooseBuildOption(player, unit);
    if (idx != 0)
        QueueBuildOrder(((Item_00408620 *)g_game->defs)[idx].name, unit, 1);
}

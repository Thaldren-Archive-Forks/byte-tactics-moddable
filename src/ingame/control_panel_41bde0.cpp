// Decompiled by DeepSeek V4.1 Flash, finished by Claude Opus 5.5. Names are provisional.
// Steps the selected unit's build menu to the next page (wrapping to page 1),
// or closes it after the last page when toggle is set.
// Needed: without it the sub-object pointer is allocated to edx, not esi.
// Kept its own file: in control_panel.cpp the include set and prelude move
// that pointer from esi to edx.
#include <stdlib.h>

#pragma pack(push, 1)
struct Sub_0041bde0 {
    char unknown_0[0x22e];
    unsigned char pageCount;             // +0x22e
};

#include "../units/unit.h"

#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;
void __stdcall PlaySoundByName(char* name, int param);

// FUNCTION: 0x41bde0
void __stdcall StepBuildMenuPage(int toggle)
{
    unsigned short index = g_game->unitIndex;
    if (index != 0) {
        Unit* unit = &g_game->units[index];
        if (unit->unitDefIndex == 0)
            unit = 0;
        if (unit != 0) {
            if (toggle != 0) {
                if (!unit->menu.menuOpen) {
                    unit->menu.menuOpen = 1;
                    unit->menu.page = 1;
                } else if (unit->menu.page == ((Sub_0041bde0*)unit->def)->pageCount - 1) {
                    unit->menu.menuOpen = 0;
                } else {
                    unit->menu.page++;
                }
            } else {
                // Compared as page == pageCount - 1; keeps the original load order.
                if (unit->menu.page == ((Sub_0041bde0*)unit->def)->pageCount - 1)
                    unit->menu.page = 1;
                else
                    unit->menu.page++;
                unit->menu.menuOpen = 1;
            }
            g_game->ordersPanelFlags |= 0x10;
        }
    }
    PlaySoundByName("nextbuildmenu", 0);
}

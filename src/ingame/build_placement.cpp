// Decompiled by deepseek-v4.1-flash, space-bunny-free, GPT-6.1-sol and Haiku. Names are provisional.

#include <stdlib.h>

#pragma pack(push, 1)
#include "../gui/mouse_event.h"

#include "../util/vec3.h"

struct Struct_00499200_531 {
    int next;                          // +0x0, the previous layer
    int entries;                       // +0x4, the layer's gadget array
};

#include "../map/mission.h"

#include "../sound/sound.h"

#include "../game/game.h"
#pragma pack(pop)

#include "../orders/mission_type.h"
#include "../../interfaces/seams.h"

extern Game* g_game;

void BeginMouseScroll();
void UpdatePlacementGhostValidity();
void DispatchOrdersPanelPageFlags();
void UpdateMouseScroll();
void CenterCameraOnRadarClick();
void __stdcall SetEndGameState(int a);
void BlankScreen();
int BroadcastPendingViewState();
void ResetChatHudIndices();
void ApplySlotsToGamePlayers();
void ClearSelection();
int __stdcall SelectUnitsInBox(void* p);
unsigned short __stdcall PickUnitUnderCursor();
int __stdcall ResolveCursorModeForSelection(char mode);
void ShutdownIngameSystems();
void __stdcall PopUntilNamedLayout(int a);
void MainLoopTick();
void __stdcall UpdateCursorWorldPos(MouseEvent* p);
void __stdcall IssueMobileBuildOrders(MouseEvent* arg);
void __stdcall PlaySoundByName(char* name, int broadcast);
void __stdcall ClickSelectHoverUnit(MouseEvent* arg);
void __stdcall IssueOrderToSelection(void* a, unsigned char b, MissionType kind, Vec3* d, int e, int f);
int __stdcall FindGadgetIndexBySubstring(int value, char* name);
void __stdcall ClearGroupStatus(void* obj, int index);
void __stdcall CloseTopScreen(void* a);
void __stdcall SetCursorAnimation(void* a, void* b);
void __stdcall SetCloseHandler(void* a, int b);
int GetTicks();
void ClearKeyQueue();
int __stdcall IsKeyDown(int a);
void EndGameFrame();
void RunEndGameState(void);
void MenuFrame();
void __cdecl LeaveNetGameCallback();

// FUNCTION: 0x498f70
void __stdcall HandleLeftClick(MouseEvent* mouse)
{
    int index;

    if (g_game->orderMode == 0xe) {
        if (g_game->inputFlags & 0x40) {
            IssueMobileBuildOrders(mouse);
            PlaySoundByName("oktobuild", 0);
            if (mouse->keyFlags & 4) {
                g_game->inputFlags |= 0x20;
                return;
            }
            g_game->orderMode = 1;
            g_game->inputFlags &= 0xdf;
            index = FindGadgetIndexBySubstring(g_game->layer->entries, "STOP");
            if (index != -1) {
                ClearGroupStatus(g_game->message, index);
            }
        } else {
            PlaySoundByName("notoktobuild", 0);
        }
        return;
    }
    if ((signed char)g_game->cursorMode == 0xf) {
        ClickSelectHoverUnit(mouse);
        return;
    }
    if ((signed char)g_game->cursorMode >= 0x11) {
        if (g_game->interfaceType == 1 && g_game->orderMode == 1) {
            ClearSelection();
            PopUntilNamedLayout(1);
        }
        return;
    }
    {
        MissionType kind;
        kind.index = 0;
        IssueOrderToSelection(mouse, g_game->orderMode, kind, &g_game->pos, 0, 0);
    }
    if (mouse->keyFlags & 4) {
        g_game->inputFlags |= 0x20;
        return;
    }
    g_game->orderMode = 1;
    g_game->inputFlags &= 0xdf;
    index = FindGadgetIndexBySubstring(g_game->layer->entries, "STOP");
    if (index != -1) {
        ClearGroupStatus(g_game->message, index);
    }
}


// Order-button handler: when the game is not in order mode, selects the STOP
// order (inlined body of 0x495860); otherwise dispatches on the order flags
// (+0x2cc6): bit 1 cancels the current order, bit 0 switches to the 0x13
// (STOP) selection, bit 2 hands a zero order kind to IssueOrderToSelection.
// FUNCTION: 0x499100
void __stdcall HandleRightClick(MouseEvent* mouse)
{
    if (g_game->orderMode != 1) {
        g_game->orderMode = 1;
        g_game->inputFlags &= 0xdf;
        int index = FindGadgetIndexBySubstring(g_game->layer->entries, "STOP");
        if (index != -1) {
            ClearGroupStatus(g_game->message, index);
        }
        return;
    }
    if (g_game->interfaceType == 0) {
        if (g_game->inputFlags & 2) {
            if (mouse->keyFlags & 8) {
                BeginMouseScroll();
                return;
            }
            ClearSelection();
            PopUntilNamedLayout(1);
            return;
        }
        if (g_game->inputFlags & 1) {
            g_game->inputFlags = g_game->inputFlags | 0x10;
            if ((signed char)g_game->cursorMode != 0x13) {
                (*(signed char*)&g_game->cursorMode) = 0x13;
                SetCursorAnimation((void*)g_game->message, g_game->cursorModeSeq[0x13]);
                return;
            }
        }
    } else if (g_game->inputFlags & 4) {
        MissionType kind;
        IssueOrderToSelection(mouse, 1, kind, &g_game->pos, 0, 0);
    }
}


static inline void SetCursor(int n)
{
    if ((signed char)g_game->cursorMode != n) {
        (*(signed char*)&g_game->cursorMode) = n;
        SetCursorAnimation(g_game->message, g_game->cursorModeSeq[n]);
    }
}

// Main-loop frame handler. Copies the 24-byte mouse record off g_game,
// feeds it to the camera update, then runs the order/selection state machine
// off the flags byte at +0x2cc6 and the mouse message stored in the block.
// Advances the frame queues and, on the network/skirmish paths, flips the
// end-of-frame hooks.
// FUNCTION: 0x499200
void BattleFrame(void)
{
    MouseEvent mouse = (*(MouseEvent*)&g_game->view);
    UpdateCursorWorldPos(&mouse);

    unsigned char flags = g_game->inputFlags;
    if ((flags & 2) != 0 && g_game->orderMode == 0xe) {
        UpdatePlacementGhostValidity();
    } else if ((flags & 2) == 0 && (flags & 1) == 0) {
        SetCursor(0x13);
    } else {
        g_game->hoverUnitId = PickUnitUnderCursor();
        SetCursor(ResolveCursorModeForSelection(g_game->orderMode));
    }

    DispatchOrdersPanelPageFlags();
    if ((g_game->inputFlags & 0x20) != 0) {
        if (IsKeyDown(0xf9) == 0) {
            g_game->orderMode = 1;
            g_game->inputFlags &= 0xdf;
            int index = FindGadgetIndexBySubstring(g_game->layer->entries, "STOP");
            if (index != -1) {
                ClearGroupStatus(g_game->message, index);
            }
        }
    }

    flags = g_game->inputFlags;
    if ((flags & 0x10) != 0) {
        if (g_game->interfaceType == 0) {
            if (mouse.msg == 0x205) {
                g_game->inputFlags = flags & 0xef;
            } else {
                CenterCameraOnRadarClick();
            }
        } else {
            if (mouse.msg == 0x202) {
                g_game->inputFlags = flags & 0xef;
            } else {
                CenterCameraOnRadarClick();
            }
        }
    } else if (g_game->middleScrollActive != 0) {
        UpdateMouseScroll();
    } else if (mouse.msg == 0x204) {
        HandleRightClick(&mouse);
    } else if (g_game->orderMode != 1) {
        if (mouse.msg == 0x201) {
            HandleLeftClick(&mouse);
        }
    } else if ((flags & 8) != 0) {
        if (mouse.msg == 0x202) {
            g_game->inputFlags = flags & 0xf7;
            int dx = g_game->boxStartX - g_game->boxEndX;
            int dz = g_game->boxStartZ - g_game->boxEndZ;
            dx = abs(dx);
            dz = abs(dz);
            int now = GetTicks();
            if (g_game->boxStartTick + 0x19 > now && dx < 0x20 && dz < 0x20) {
                HandleLeftClick(&mouse);
            } else if (SelectUnitsInBox(&mouse) == 0) {
                ClearSelection();
                PopUntilNamedLayout(1);
            }
        } else {
            g_game->boxEndX = g_game->xWhole;
            g_game->boxEndHeight = g_game->yWhole;
            g_game->boxEndZ = g_game->zWhole;
        }
    } else if (mouse.msg == 0x201) {
        if ((flags & 2) != 0) {
            g_game->inputFlags = flags | 8;
            g_game->boxStartTick = GetTicks();
            g_game->boxStartX = g_game->xWhole;
            g_game->boxStartHeight = g_game->yWhole;
            g_game->boxStartZ = g_game->zWhole;
            g_game->boxEndX = g_game->xWhole;
            g_game->boxEndHeight = g_game->yWhole;
            g_game->boxEndZ = g_game->zWhole;
            SetCursor(0x13);
        } else if (g_game->interfaceType == 1) {
            if ((flags & 1) != 0) {
                g_game->inputFlags = flags | 0x10;
                SetCursor(0x13);
            }
        } else if ((flags & 1) != 0) {
            HandleLeftClick(&mouse);
        }
    }

    MainLoopTick();
    {
        unsigned short unit = g_game->unitIndex;
        if (unit != 0 && *(short*)((char *)g_game->units + unit * 0x118 + 0xa6) == 0) {
            PopUntilNamedLayout(0);
        }
    }

    if ((*(Flags16_00499200*)&g_game->endGameFlags_3923b).bits.b2 || (*(Flags16_00499200*)&g_game->endGameFlags_3923b).bits.b4) {
        if (g_seams.map->GetGameType(g_game->net) != 3 ||
            (g_seams.map->GetGameType(g_game->net) == 3 &&
             BroadcastPendingViewState() != 0)) {
            SetCursor(0x13);
            PopUntilNamedLayout(1);
            CloseTopScreen(g_game->message);
            if (g_seams.map->GetGameType(g_game->net) == 3) {
                ResetChatHudIndices();
                MainLoopTick();
            }
            ShutdownIngameSystems();
            ClearKeyQueue();
            ((Sound *)g_game->field_10)->SetTrackCategory(4);
            g_game->mode_391f1 = 7;
            g_game->handler = EndGameFrame;
            SetCloseHandler(LeaveNetGameCallback, 0);
            SetEndGameState(0);
        }
    }

    if (g_game->restartMissionRequest != 0) {
        // Each arm keeps its own copy of the hook stores, and `|= 4` in both: the compiler merges them.
        if (g_seams.map->GetGameType(g_game->net) == 1) {
            ShutdownIngameSystems();
            PopUntilNamedLayout(1);
            CloseTopScreen(g_game->message);
            BlankScreen();
            int a = g_seams.map->GetMissionIndex(g_game->net);
            char* b = g_seams.map->GetCampaignName(g_game->net);
            g_seams.map->LoadCampaign(g_game->net, 0, b);
            if (g_seams.map->SelectMission(g_game->net, 0, a) != 0) {
                g_game->netGameFlags_2a44.bits.b3 = 1;
                g_game->netGameFlags_2a44.value |= 4;
            }
            g_game->mode_391f1 = 2;
            g_game->handler = MenuFrame;
            SetCloseHandler(LeaveNetGameCallback, 0);
        } else {
            unsigned int saved = g_game->field_2a3c;
            ShutdownIngameSystems();
            PopUntilNamedLayout(1);
            CloseTopScreen(g_game->message);
            BlankScreen();
            SetCursor(0x14);
            g_game->field_2a3c = saved;
            g_seams.map->LoadMissionByName(g_game->net, 0, (char *)g_game->options + 0x11c);
            ApplySlotsToGamePlayers();
            g_game->netGameFlags_2a44.value |= 4;
            g_game->mode_391f1 = 2;
            g_game->handler = MenuFrame;
            SetCloseHandler(LeaveNetGameCallback, 0);
        }
        ((Sound *)g_game->field_10)->SetTrackCategory(4);
    }
}

// FUNCTION: 0x499880
void EndGameFrame(void)
{
    RunEndGameState();
}

// Decompiled by Opus, DeepSeek V4.1 Flash, Claude Opus 5.5, Haiku, deepseek-v4.1-flash, GPT-6.1-sol, deepseek-v4.1, space-bunny-free, fledge-alpha-free, GPT-6 and Sonnet. Names are provisional.
// The in-game control panel: the order and build menu handling (the order-mode
// state, the build menu tables, the order buttons and the build progress), the
// damage and build progress helpers, the build menu paging, the order-mode key
// handling, the commander lookup, and the camera position, follow and scroll
// state. The module's files gathered in address order; 0x41b2e0 and 0x41bde0
// keep their own files (their own include sets decide their matches).
#include <windows.h>
#include "../../interfaces/host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Unused here: the symbol ids <io.h> adds put OpenBuildMenuGui and
// AddBuildProgress in the symbol-id windows they match in (docs/c2-regalloc.md);
// the same is true in sound/samples.cpp and sound/cd_audio.cpp.
#include <io.h>

#pragma pack(push, 1)

#include "../util/vec3.h"

#include "../orders/mission_type.h"

struct Option_00419560;

struct Pair_00419560 {
    int a;
    int b;
};

struct Unit;
struct Player;
struct PlayerInfo;
struct Nano_0041b8d0;
struct Gadget;
struct Layer;
struct Gui;
struct BuildList_0041ace0;

#include "../game/unit_resources.h"

struct Nano_0041b8d0 {
    char unknown_0[0x10];
    int drawFrame;                     // +0x10
};

struct UnitTypeFlagsBits {
    unsigned int unknown_0 : 6;        // +0x241 bits 0-5
    unsigned int flag_6 : 1;           // bit 6
    unsigned int unknown_7 : 4;        // bits 7-10
    unsigned int flag_11 : 1;          // bit 11
    unsigned int unknown_12 : 6;       // bits 12-17
    unsigned int flag_18 : 1;          // bit 18
    unsigned int unknown_19 : 5;       // bits 19-23
    unsigned int flag_24 : 1;          // bit 24
    unsigned int unknown_25 : 7;       // bits 25-31
};

union UnitTypeFlags {
    unsigned int raw;
    UnitTypeFlagsBits bits;
};

struct UnitTypeFlags2 {
    unsigned int canMoveOrder : 1;     // +0x245 bit 0
    unsigned int canFireOrder : 1;     // bit 1
    unsigned int canOnOff : 1;         // bit 2
    unsigned int canStop : 1;          // bit 3
    unsigned int canAttack : 1;        // bit 4
    unsigned int canDefend : 1;        // bit 5
    unsigned int canPatrol : 1;        // bit 6
    unsigned int canMove : 1;          // bit 7
    unsigned int canLoad : 1;          // bit 8
    unsigned int canRepair : 1;        // bit 9
    unsigned int canReclaim : 1;       // bit 10
    unsigned int bit11 : 1;
    unsigned int canCapture : 1;       // bit 12
    unsigned int canCloak : 1;         // bit 13
    unsigned int canBlast : 1;         // bit 14
    unsigned int bits15 : 17;
};

// UnitDef (units/unit_def.h, 0x249 bytes) with its flag words read as bit fields,
// which the header lacks: the unit's own type and a build menu entry.
struct UnitType {
    char unknown_0[0x20];
    char unitname[0x20];               // +0x20
    char unknown_40[0x111 - 0x40];
    unsigned int field_111;            // +0x111
    char unknown_115[0x14a - 0x115];
    Point16 origin;                   // +0x14a
    char unknown_14e[0x156 - 0x14e];
    int ids;                           // +0x156
    char unknown_15a[0x186 - 0x15a];
    float energyCost;                  // +0x186 (TDF buildcostenergy)
    float metalCost;                   // +0x18a (TDF buildcostmetal)
    char unknown_18e[0x1ea - 0x18e];
    int buildtime;                     // +0x1ea
    char unknown_1ee[0x1fa - 0x1ee];
    unsigned int maxHealth;            // +0x1fa
    char unknown_1fe[0x21e - 0x1fe];
    short id;                          // +0x21e
    char unknown_220[0x22e - 0x220];
    unsigned char buildMenuPageCount;  // +0x22e
    unsigned char mobile;              // +0x22f
    char unknown_230[0x241 - 0x230];
    UnitTypeFlags flags;               // +0x241
    UnitTypeFlags2 flags2;             // +0x245
};

#include "../gui/gadget.h"
#include "../gui/layer.h"
#include "../gui/gui.h"

// Unused here: a real function declared to keep the file's symbol count (docs/c2-regalloc.md).
void StepAllGafSequences();

#include "../network/player_info.h"

#include "../network/player.h"

#include "../units/unit.h"

// A unit type's extra build menu entry (0x25 bytes).
struct BuildEntry_0041ace0 {
    short typeId;                      // +0x00
    unsigned char page;                // +0x02
    unsigned char slot;                // +0x03
    char name[0x21];                   // +0x04
};

struct BuildList_0041ace0 {            // 0xbd bytes
    int count;                         // +0x00
    BuildEntry_0041ace0 entries[5];    // +0x04
};

// The mouse record at g_game+0x2c76: copied by 0x41cc60, read by 0x41cd50.
#include "../gui/mouse_event.h"

struct CursorState {
    MouseEvent start;                 // +0x0, the mouse when the scroll began
    int flag;                          // +0x18
    POINT center;                      // +0x1c
    POINT savedScroll;                 // +0x24
};

struct Follow_0041ca10 {
    int unknown_0;                     // +0x0
    Vec3 pos;                          // +0x4
};

#include "../game/game.h"

#pragma pack(pop)

// GLOBAL: 0x511de8
extern Game* g_game;

extern int g_messageBytesReceived;
extern int g_messageBytesSent;
extern int g_packetBytesSent;
extern int DAT_00511c20;
extern int g_packetsSent;
extern int g_packetsReceived;
extern int g_packetBytesReceived;
extern int g_messageCountByType[45][2];
extern int g_messageBytesByType[45][2];
extern Option_00419560 g_consoleCommands;
extern Option_00419560 g_cheatCommands;
extern Option_00419560 g_debugCommands;

// The second part's own views and externs: the display its functions read.
#pragma pack(push, 1)
struct Display_0041ce90 {
    char unknown_0[0x40];
    HWND hwnd;                         // +0x40
    char unknown_44[0xf0 - 0x44];
    unsigned char flags_f0;            // +0xf0
};
#pragma pack(pop)

Unit* __stdcall FindNextSelectedUnit(Unit*, int);
unsigned int* __stdcall GetCategoryMask(char* name);
void __stdcall CalcRadarViewportRect(void* rect);
int GetScreenWidth();
int GetScreenHeight();
void __stdcall SetCursorPosition(int x, int y);
void HideSoftwareCursor();
void ShowSoftwareCursor();
void __stdcall GetCurrentMouseEvent(MouseEvent* mouse);
Display_0041ce90* GetDisplay();

void __stdcall RegisterCommands(Option_00419560* option);
void __stdcall SetDefaultCommandHandler(void (__stdcall* callback)(int), int mask);
void __stdcall DefaultCommandHandler(int args);

void __stdcall GetGadgetName(Gadget* entries, char* name, int index);
void __stdcall IssueOrCancelOrder(MissionType kind, int remove, Unit* owner,
                            int id, Vec3* pos, int typeId, int amount);
int __stdcall CanBuildAt(UnitType* type, Point16 cell, int a, Player* player);
int GetBuildSiteHeight(void);
int __stdcall GetFootprintHeight(UnitType* unit, Point16 cell);
Gadget* __stdcall FindGadgetOrNull(Gadget* entries, char* name);
unsigned short __stdcall FindUnitTypeId(char* name);
int __stdcall SumQueuedBuildCount(void* owner, int index);
void __stdcall MarkChanged(void* obj);
int __stdcall FindGadgetIndexBySubstring(int value, char* name);
int __stdcall FindGadgetIndexBySubstring(Gadget* entries, char* name);
void __stdcall SetGadgetStatus(Gui* menu, int index, int value);
void __stdcall PlaySoundByName(char* name, int broadcast);
void __stdcall AdjustBuildCount(MissionType kind, Unit* unit, int id, int count);
void __stdcall IssueOrderToSelection(void* a, int b, MissionType kind,
                                     int d, int e, int f);
void __stdcall SetGrayedOut(Gui* menu, int index, int value);
void __stdcall SetGadgetActive(Gui* menu, int index, char value);
void __stdcall RenderLayer(Gui* menu, int value);
void __stdcall SetGadgetActiveByName(Gui* menu, char* name, int value);
void __stdcall ClearSelectedGadget(Gui* menu);
int __stdcall HandleOrdersPanelClick(Gui* menu, Gadget* entries);
int __stdcall HandleOrderButtonClick(Gui* menu, Gadget* entries);
int __stdcall IsKeyDown(int key);
int __stdcall GetClickMode(Gui* menu);
void __stdcall QueueBuildOrder(char* name, Unit* unit, int count);
void __stdcall RefreshBuildCountTexts(Gui* menu, Unit* unit);
void __stdcall BuildDataPath(char* out, const char* dir, const char* name, const char* ext);
int __stdcall HAPI_FileLengthByName(char* path);
Layer* __stdcall LoadGuiLayer(Gui* menu, const char* name, int flags);
void __stdcall HandleBuildPanelClick(Gui* menu);
void __stdcall RefreshOrderButtons(Unit* unit);
int __stdcall IsScreenNamed(Gui* menu, const char* name);
void __stdcall AttachUnitToPiece(Unit* unit, Unit* target, char p3, char p4);
void __stdcall BroadcastBuilderLink(Unit* obj, Unit* target);
void __stdcall FinishConstruction(Unit* builder, Unit* unit);
int __stdcall AddBuildProgress(Unit* builder, Unit* unit, float amount);

static inline Point16 WorldToCell(Vec3 v, Point16 origin)
{
    Point16 c;
    c.x = (v.x - (origin.x << 19) + 0x80000) >> 20;
    c.y = (v.z - (origin.y << 19) + 0x80000) >> 20;
    return c;
}

static inline void CellToWorld(Point16 origin, Point16 c, Vec3* v)
{
    v->x = (origin.x + c.x * 2) << 19;
    v->z = (origin.y + c.y * 2) << 19;
}

// FUNCTION: 0x419560
void InitCommands()
{
    DAT_00511c20 = g_game->sideNames_37f5b[4].field_224;
    g_messageBytesReceived = 0;
    g_messageBytesSent = 0;
    // The second field of the second table is cleared through a walking
    // pointer: the original keeps a separate induction pointer for it.
    Pair_00419560* p = (Pair_00419560*)&g_messageBytesByType[1];
    for (int i = 0; i < 44; i++) {
        g_messageCountByType[i + 1][0] = 0;
        g_messageCountByType[i + 1][1] = 0;
        g_messageBytesByType[i + 1][0] = 0;
        p->b = 0;
        p++;
    }
    g_packetsSent = 0;
    g_packetBytesSent = 0;
    g_packetsReceived = 0;
    g_packetBytesReceived = 0;
    RegisterCommands(&g_consoleCommands);
    RegisterCommands(&g_cheatCommands);
    RegisterCommands(&g_debugCommands);
    SetDefaultCommandHandler(DefaultCommandHandler, 4);
}

// Copies the 32-character name of entry `index` of the table at
// g_game+0x1439b (0x249-byte entries) into `dest`. <windows.h> fixes the
// operand order of the address lea (tools/headers.py).
// FUNCTION: 0x4195f0
void __stdcall CopyMenuEntryName(char* dest, unsigned short index)
{
    strncpy(dest, ((UnitType *)g_game->defs)[index].unitname, 0x20);
    dest[0x1f] = 0;
}

// Returns whether the name of menu entry `index` contains `text`.
// FUNCTION: 0x419630
int __stdcall MenuEntryNameContains(Gadget* entries, char* text, int index)
{
    char name[32];
    GetGadgetName(entries, name, index);
    return strstr(name, text) != 0;
}

// Snaps the position at +0x2caa to the centre of its cell for the unit
// type selected at +0x2cc4, then passes it to IssueOrCancelOrder as a
// "MOBILEBUILD" (def flag bit 11 clear) or "VTOL_MOBILEBUILD" (bit 11 set)
// order for each of the local player's units with flag 0x10 whose def has
// flag 0x40. Bit 2 of the argument's keyFlags is passed through.
// FUNCTION: 0x419670
void __stdcall IssueMobileBuildOrders(Arg_00419670* arg)
{
    unsigned int remove = (arg->keyFlags >> 2) & 1;
    unsigned short index = g_game->buildTypeIndex;
    UnitType* def = &((UnitType *)g_game->defs)[index];
    Vec3 pos = g_game->pos;
    // origin read once into a local: passing def->origin to each helper changes the frame.
    Point16 origin = def->origin;
    Point16 cell = WorldToCell(pos, origin);
    CellToWorld(origin, cell, &pos);
    pos.y = g_game->boxStartHeight << 16;

    unsigned char team = g_game->localPlayer;
    Player* p = &g_game->players[team];
    for (Unit* u = p->unitsBegin; u <= p->unitsEnd; u++) {
        if ((u->flags & 0x10) && (((UnitType*)u->def)->flags.raw & 0x40)) {
            if (!(((UnitType*)u->def)->flags.raw & 0x800)) {
                IssueOrCancelOrder("MOBILEBUILD", remove, u, 0, &pos, index, 0);
            } else {
                UnitTypeFlagsBits flags = ((UnitType*)u->def)->flags.bits;
                if (flags.flag_11) {
                    IssueOrCancelOrder("VTOL_MOBILEBUILD", remove, u, 0, &pos, index, 0);
                }
            }
        }
    }
}

// FUNCTION: 0x4197d0
int UpdatePlacementGhostValidity(void)
{
    UnitType* item = &((UnitType *)g_game->defs)[g_game->buildTypeIndex];
    Point16 cell = WorldToCell(g_game->pos, item->origin);
    g_game->boxStartX = cell.x << 4;
    g_game->boxStartZ = cell.y << 4;
    Point16 origin = item->origin;
    g_game->boxEndX = (origin.x << 4) + g_game->boxStartX;
    g_game->boxEndZ = (origin.y << 4) + g_game->boxStartZ;
    g_game->inputFlags_2cc6.bits.b6 = CanBuildAt(item, cell, 0, &g_game->players[g_game->localPlayer]);
    unsigned char r;
    if (g_game->inputFlags_2cc6.value & 0x40)
        r = GetBuildSiteHeight();
    else
        r = GetFootprintHeight(item, cell);
    g_game->boxStartHeight = r;
    g_game->boxEndHeight = r;
    return (g_game->inputFlags_2cc6.value >> 6) & 1;
}

// Finds the GUI entry named after list entry `index` of the game's 0x249-byte
// table at +0x1439b and sets its text at +0xb6 to "+<n>", or clears it
// when n is 0.
// FUNCTION: 0x419940
void __stdcall SetBuildCountText(Gui* obj, unsigned short index, int n)
{
    Gadget* e = FindGadgetOrNull(obj->layer->entries, ((UnitType *)g_game->defs)[index].unitname);
    if (e) {
        if (n)
            sprintf(e->u.text, "+%d", n);
        else
            e->u.text[0] = 0;
    }
}

// Refreshes the text of the menu entries owned by the unit's owner. The menu
// keeps a table of 0x15b-byte entries whose first entry stores the entry count
// as a short at +0xb6 (the same offset entry 1..n use for their text). Entries
// whose byte at +0 is 1 have either a name at +2 looked up with FindUnitTypeId
// (printing "+<amount>") or show the unit's own count at +0x1e. Entry 0 holds
// only the count, so the loop starts at entry 1.
// FUNCTION: 0x4199b0
void __stdcall RefreshBuildCountTexts(Gui* menu, Unit* unit)
{
    Gadget* e = menu->layer->entries;
    // Read as an int so it is sign-extended once.
    int count = e->u.count;
    e++;
    for (int i = 1; i < count + 1; i++) {
        char* text = e->u.text;
        if (e->type == 1) {
            if (e->commonAttribs & 4) {
                unsigned short v = FindUnitTypeId(e->name);
                if (v != 0) {
                    int r = SumQueuedBuildCount(unit, v);
                    if (r != 0)
                        sprintf(text, "+%d", r);
                    else
                        text[0] = 0;
                }
            } else if (e->commonAttribs & 8) {
                int n = unit->weapons[0].stockpile;
                int r = SumQueuedBuildCount(unit, 0);
                text[0] = 0;
                if (n != 0)
                    sprintf(text, "%d", n);
                if (r != 0)
                    sprintf(text + strlen(text), " +%d", r);
            }
        }
        // Incremented after the reads, not before.
        e++;
    }
    MarkChanged(&g_game->gui);
}

// Sets the "ONOFF" menu entry from bit 0 of the unit's flags at +0x10e
// (compare 0x495860).
// FUNCTION: 0x419ac0
void __stdcall UpdateOnOffButton(Unit* unit)
{
    int index = FindGadgetIndexBySubstring((int)g_game->gui.layer->entries, "ONOFF");
    if (index != -1) {
        SetGadgetStatus(&g_game->gui, index, unit->view10e.onOff);
    }
}

// FUNCTION: 0x419b00
void __stdcall QueueBuildOrder(char* name, Unit* unit, int count)
{
    if (unit->playerIndex == g_game->playerIndex) {
        if (count > 0)
            PlaySoundByName("addbuild", 0);
        else
            PlaySoundByName("subbuild", 0);
    }
    if (strstr(name, "MAKENUKE") != 0 || strstr(name, "MAKEANTI") != 0) {
        AdjustBuildCount(MissionType("BUILDWEAPON"), unit, 0, count);
        return;
    }
    unsigned short id = FindUnitTypeId(name);
    if (id == 0)
        return;
    int mobile = unit->motion;
    const char* kind = mobile ? "MOBILEBUILD" : "BUILDINGBUILD";
    AdjustBuildCount(MissionType(kind), unit, id, count);
}

// FUNCTION: 0x419bc0
void __stdcall SetOrderIntent(unsigned char mode)
{
    g_game->orderMode = mode;
    g_game->inputFlags_2cc6.value = g_game->inputFlags_2cc6.value & 0xf7;
}

// Handles a click on an order button: finds which order the button's name
// contains and selects that order mode (SetOrderIntent inlined), plays the
// "immediateorders" or "specialorders" sound and returns 1; returns 0 when the
// name is no order. STOP issues the stop order at once.
static inline void SetOrderMode(unsigned char mode)
{
    g_game->orderMode = mode;
    g_game->inputFlags_2cc6.value = g_game->inputFlags_2cc6.value & 0xf7;
}

// FUNCTION: 0x419be0
int __stdcall HandleOrderButtonClick(Gadget* button, Gadget* entries)
{
    char name[32];
    void* orders = g_game->orders;
    // Uninitialised unless the entry is type 1, as in the original: a `: button` fallback changes the code.
    Gadget* e;
    if (entries[button->index].type == 1) e = &entries[button->index];

    GetGadgetName(entries, name, button->index);
    if (strstr(name, "MOVE")) {
        if (e->status == 0) {
            SetOrderMode(1);
        } else {
            SetOrderMode(2);
        }
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, button->index);
    if (strstr(name, "STOP")) {
        SetOrderMode(1);
        IssueOrderToSelection(orders, 0, "STOP", 0, 0, 0);
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, button->index);
    if (strstr(name, "ATTACK")) {
        if (e->status == 0) {
            SetOrderMode(1);
        } else {
            SetOrderMode(3);
        }
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, button->index);
    if (strstr(name, "BLAST")) {
        if (e->status == 0) {
            SetOrderMode(1);
        } else {
            SetOrderMode(4);
        }
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, button->index);
    if (strstr(name, "DEFEND")) {
        if (e->status == 0) {
            SetOrderMode(1);
        } else {
            SetOrderMode(7);
        }
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, button->index);
    if (strstr(name, "REPAIR")) {
        if (e->status == 0) {
            SetOrderMode(1);
        } else {
            SetOrderMode(8);
        }
        PlaySoundByName("specialorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, button->index);
    if (strstr(name, "PATROL")) {
        if (e->status == 0) {
            SetOrderMode(1);
        } else {
            SetOrderMode(9);
        }
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, button->index);
    if (strstr(name, "RECLAIM")) {
        if (e->status == 0) {
            SetOrderMode(1);
        } else {
            SetOrderMode(0xc);
        }
        PlaySoundByName("specialorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, button->index);
    if (strstr(name, "CAPTURE")) {
        if (e->status == 0) {
            SetOrderMode(1);
        } else {
            SetOrderMode(0xd);
        }
        PlaySoundByName("specialorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, button->index);
    if (strstr(name, "UNLOAD")) {
        if (e->status == 0) {
            SetOrderMode(1);
        } else {
            SetOrderMode(5);
        }
        PlaySoundByName("specialorders", 0);
        return 1;
    }
    GetGadgetName(entries, name, button->index);
    if (strstr(name, "LOAD")) {
        if (e->status == 0) {
            SetOrderMode(1);
        } else {
            SetOrderMode(6);
        }
        PlaySoundByName("immediateorders", 0);
        return 1;
    }
    return 0;
}

// Refreshes the order buttons of the unit menu: the BUILD/ORDERS page toggle
// for the selected unit, the multi-state CLOAK, ONOFF, MOVEORD and FIREORD
// buttons from the selection's combined order state, and disables every order
// button the selection cannot use.
// FUNCTION: 0x41a120
void __stdcall RefreshOrderButtons(Unit* unit)
{
    Gui* menu = &g_game->gui;
    int layer = (int)g_game->gui.layer->entries;
    int index;

    index = FindGadgetIndexBySubstring(layer, "BUILD");
    if (index != -1) {
        if (unit && ((UnitType*)unit->def)->buildMenuPageCount) {
            SetGadgetStatus(menu, index, unit->menu.menuOpen);
        } else {
            SetGrayedOut(menu, index, 1);
        }
    }
    index = FindGadgetIndexBySubstring(layer, "ORDERS");
    if (index != -1) {
        if (unit && ((UnitType*)unit->def)->buildMenuPageCount) {
            SetGadgetStatus(menu, index, !unit->menu.menuOpen);
        } else {
            SetGrayedOut(menu, index, 1);
        }
    }
    index = FindGadgetIndexBySubstring(layer, "CLOAK");
    if (index != -1) {
        if (g_game->ordersPanelFlags_37ebe_6.bits.cloak == 3) {
            SetGrayedOut(menu, index, 1);
        } else {
            SetGadgetStatus(menu, index, g_game->ordersPanelFlags_37ebe_6.bits.cloak);
        }
    }
    index = FindGadgetIndexBySubstring(layer, "ONOFF");
    if (index != -1) {
        if (g_game->ordersPanelFlags_37ebe_6.bits.onOff == 3) {
            SetGrayedOut(menu, index, 1);
        } else {
            SetGadgetStatus(menu, index, g_game->ordersPanelFlags_37ebe_6.bits.onOff);
        }
    }
    index = FindGadgetIndexBySubstring(layer, "MOVEORD");
    if (index != -1) {
        if (g_game->ordersPanelFlags_37ebe_6.bits.moveOrder == 4) {
            SetGrayedOut(menu, index, 1);
        } else {
            SetGadgetStatus(menu, index, g_game->ordersPanelFlags_37ebe_6.bits.moveOrder);
        }
    }
    index = FindGadgetIndexBySubstring(layer, "FIREORD");
    if (index != -1) {
        if (g_game->ordersPanelFlags_37ebe_6.bits.fireOrder == 4) {
            SetGrayedOut(menu, index, 1);
        } else {
            SetGadgetStatus(menu, index, g_game->ordersPanelFlags_37ebe_6.bits.fireOrder);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canMove) {
        index = FindGadgetIndexBySubstring(layer, "MOVE");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canStop) {
        index = FindGadgetIndexBySubstring(layer, "STOP");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canAttack) {
        index = FindGadgetIndexBySubstring(layer, "ATTACK");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canDefend) {
        index = FindGadgetIndexBySubstring(layer, "DEFEND");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canPatrol) {
        index = FindGadgetIndexBySubstring(layer, "PATROL");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canReclaim) {
        index = FindGadgetIndexBySubstring(layer, "RECLAIM");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canRepair) {
        index = FindGadgetIndexBySubstring(layer, "REPAIR");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canCapture) {
        index = FindGadgetIndexBySubstring(layer, "CAPTURE");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
    }
    if (!g_game->ordersPanelFlags_37ebe_6.bits.canLoad) {
        index = FindGadgetIndexBySubstring(layer, "LOAD");
        if (index != -1) {
            SetGadgetActive(menu, index, 0);
        }
        index = FindGadgetIndexBySubstring(layer, "UNLOAD");
        if (index != -1) {
            SetGrayedOut(menu, index, 1);
        }
        if (!g_game->ordersPanelFlags_37ebe_6.bits.canBlast) {
            index = FindGadgetIndexBySubstring(layer, "BLAST");
            if (index != -1) {
                SetGrayedOut(menu, index, 1);
            }
        }
    } else {
        index = FindGadgetIndexBySubstring(layer, "BLAST");
        if (index != -1) {
            SetGadgetActive(menu, index, 0);
        }
    }
}

// Click handler for the unit orders panel: cycles the move order, fire order,
// activation or cloak setting named by the clicked entry, sends the matching
// order and updates the button. Returns 0 when the entry is none of these.
// Inlined copy of MenuEntryNameContains.
static inline int Contains(Gadget* entries, char* text, int index)
{
    char name[32];
    GetGadgetName(entries, name, index);
    return strstr(name, text) != 0;
}

// FUNCTION: 0x41a490
int __stdcall HandleOrdersPanelClick(Gui* menu, Gadget* entries)
{
    Unit* unit = &g_game->units[g_game->unitIndex];
    void* orders = g_game->orders;
    int index = menu->hotGadgetIndex;

    if (Contains(entries, "MOVEORD", index)) {
        switch (g_game->ordersPanelFlags_37ebe_6.bits.moveOrder) {
        case 0:
            IssueOrderToSelection(orders, 0, "STANDING_MOVEORDER", 0, 1, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.moveOrder = 1;
            break;
        case 1:
            IssueOrderToSelection(orders, 0, "STANDING_MOVEORDER", 0, 2, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.moveOrder = 2;
            break;
        case 2:
        case 3:
            IssueOrderToSelection(orders, 0, "STANDING_MOVEORDER", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.moveOrder = 0;
            break;
        }
        PlaySoundByName("setmoveorders", 0);
        SetGadgetStatus(&g_game->gui, index, g_game->ordersPanelFlags_37ebe_6.bits.moveOrder);
    } else if (Contains(entries, "FIREORD", index)) {
        switch (g_game->ordersPanelFlags_37ebe_6.bits.fireOrder) {
        case 0:
            IssueOrderToSelection(orders, 0, "STANDING_FIREORDER", 0, 1, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.fireOrder = 1;
            break;
        case 1:
            IssueOrderToSelection(orders, 0, "STANDING_FIREORDER", 0, 2, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.fireOrder = 2;
            break;
        case 2:
        case 3:
            IssueOrderToSelection(orders, 0, "STANDING_FIREORDER", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.fireOrder = 0;
            break;
        }
        PlaySoundByName("setfireorders", 0);
        SetGadgetStatus(&g_game->gui, index, g_game->ordersPanelFlags_37ebe_6.bits.fireOrder);
    } else if (Contains(entries, "STATUS", index) || Contains(entries, "ONOFF", index)) {
        switch (g_game->ordersPanelFlags_37ebe_6.bits.onOff) {
        case 0:
            IssueOrderToSelection(orders, 0, "ACTIVATE", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.onOff = 1;
            break;
        case 1:
            IssueOrderToSelection(orders, 0, "DEACTIVATE", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.onOff = 0;
            break;
        case 2:
            IssueOrderToSelection(orders, 0, "ACTIVATE", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.onOff = 1;
            break;
        }
        PlaySoundByName("specialorders", 0);
        SetGadgetStatus(&g_game->gui, index, g_game->ordersPanelFlags_37ebe_6.bits.onOff);
    } else if (Contains(entries, "CLOAK", index)) {
        if (g_game->ordersPanelFlags_37ebe_6.bits.cloak) {
            IssueOrderToSelection(orders, 0, "CLOAK_OFF", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.cloak = 0;
        } else {
            IssueOrderToSelection(orders, 0, "CLOAK_ON", 0, 0, 0);
            g_game->ordersPanelFlags_37ebe_6.bits.cloak = 1;
        }
        PlaySoundByName("specialorders", 0);
        SetGadgetStatus(&g_game->gui, index, g_game->ordersPanelFlags_37ebe_6.bits.cloak);
    } else {
        return 0;
    }
    RefreshOrderButtons(g_game->unitIndex ? unit : 0);
    RenderLayer(&g_game->gui, 0x40);
    return 1;
}

// Builds the "<unit name>PREV" / "<unit name>NEXT" gadget names for the
// skeleton of the unit's owner and pushes them into the GUI.
// FUNCTION: 0x41a920
void __stdcall SetPrevNextGadgetNames(Unit* unit)
{
    char buf[256];
    if (((UnitType*)unit->def)->buildMenuPageCount < 2) {
        sprintf(buf, "%sPREV", g_game->sideNames_37f5b[unit->player->info->side].name);
        SetGadgetActiveByName(&g_game->gui, buf, 0);
        sprintf(buf, "%sNEXT", g_game->sideNames_37f5b[unit->player->info->side].name);
        SetGadgetActiveByName(&g_game->gui, buf, 0);
    }
}

// Click handler of the unit build/orders panel (installed by 0x41ace0 and
// 0x41b0f0): PREV/NEXT/ORDERS/BUILD buttons set request flags, a unit-type
// entry whose type has mobile == 0 switches to mode 0xe with that type,
// the orders buttons go to HandleOrdersPanelClick, and any other entry adds or removes
// build queue entries (5 at a time when IsKeyDown(0xf9) is set).
// FUNCTION: 0x41aa00
void __stdcall HandleBuildPanelClick(Gui* menu)
{
    if (menu->hotGadgetIndex != -1) {
        Gadget* entries = menu->layer->entries;
        // char[17]: a [20] buffer is placed above the name buffer.
        char idName[17];
        GetGadgetName(entries, idName, menu->hotGadgetIndex);
        idName[16] = 0;
        unsigned short id = FindUnitTypeId(idName);
        Unit* unit = &g_game->units[g_game->unitIndex];
        char name[32];
        GetGadgetName(entries, name, menu->hotGadgetIndex);
        if (strstr(name, "PREV")) {
            g_game->ordersPanelFlags_37ebe_6.bits.prev = 1;
        } else if (strstr(name, "NEXT")) {
            g_game->ordersPanelFlags_37ebe_6.bits.next = 1;
        } else if (strstr(name, "ORDERS")) {
            g_game->ordersPanelFlags_37ebe_6.bits.orders = 1;
            PlaySoundByName("ordersbutton", 0);
        } else if (strstr(name, "BUILD")) {
            g_game->ordersPanelFlags_37ebe_6.bits.build = 1;
            PlaySoundByName("buildbutton", 0);
        } else if (id != 0 && ((UnitType *)g_game->defs)[id].mobile == 0) {
            g_game->orderMode = 0xe;
            g_game->buildTypeIndex = id;
            PlaySoundByName("addbuild", 0);
        } else if (!HandleOrdersPanelClick(menu, entries) && !HandleOrderButtonClick(menu, entries)) {
            // Own nested if: inside the && chain the bit test is compiled differently.
            if (unit->selected) {
                char text[256];
                GetGadgetName(entries, text, menu->hotGadgetIndex);
                if (IsKeyDown(0xf9)) {
                    if (GetClickMode(menu) == 1)
                        QueueBuildOrder(text, unit, 5);
                    else
                        QueueBuildOrder(text, unit, -5);
                } else {
                    if (GetClickMode(menu) == 1)
                        QueueBuildOrder(text, unit, 1);
                    else
                        QueueBuildOrder(text, unit, -1);
                }
                if (unit->immobile
                    || (((UnitType*)unit->weapons[0].weapon)->field_111 & 0x10000000))
                    RefreshBuildCountTexts(menu, unit);
                MarkChanged(&g_game->gui);
            }
        }
        // One call after the chain, not one per branch.
        ClearSelectedGadget(menu);
    }
}

static inline Gadget* Entries(Gadget* t)
{
    return (Gadget*)((char*)t + 2);
}

// FUNCTION: 0x41ac90
void __stdcall DisableUnavailableBuildMenuEntries(Gui* obj)
{
    Gadget* t = obj->layer->entries;
    int n = t->u.count;
    for (int i = 0; i < n; i++) {
        // +0x28 (Gadget::tab) holds the entry flags here: bit 2 marks an entry that
        // names a unit, which is greyed out when its type id is 0.
        if (Entries(t)[i].tab & 4) {
            SetGrayedOut(obj, i, FindUnitTypeId((char*)&Entries(t)[i]) == 0);
        }
    }
}

// Inlined copy of SetPrevNextGadgetNames.
static inline void SetPrevNext(Unit* unit)
{
    char buf[256];
    if (((UnitType*)unit->def)->buildMenuPageCount < 2) {
        sprintf(buf, "%sPREV", g_game->sideNames_37f5b[unit->player->info->side].name);
        SetGadgetActiveByName(&g_game->gui, buf, 0);
        sprintf(buf, "%sNEXT", g_game->sideNames_37f5b[unit->player->info->side].name);
        SetGadgetActiveByName(&g_game->gui, buf, 0);
    }
}

// Inlined copy of DisableUnavailableBuildMenuEntries.
static inline void UpdateCounts(Gui* menu)
{
    Gadget* entry = menu->layer->entries;
    int n = entry->u.count;
    int i = 0;
    if (n <= 0)
        return;
    entry = (Gadget*)((char*)entry + 2);
    do {
        // +0x28 (Gadget::tab) holds the entry flags here: bit 2 marks an entry that
        // names a unit, which is greyed out when its type id is 0.
        if (entry[i].tab & 4)
            SetGrayedOut(menu, i, FindUnitTypeId((char*)&entry[i]) == 0);
    } while (++i < n);
}

// Keep the table pointer in entry form while reading its count. The guarded
// do/while places the header advance after the empty-list test.
// FUNCTION: 0x41ace0
void __stdcall OpenBuildMenuGui(Unit* unit, char* guiName, int page)
{
    if (unit->buildLeft == 0.0f) {
        Player* player = &g_game->players[g_game->localPlayer];
        int found = 0;
        char path[256];
        char name[256];
        BuildDataPath(path, "guis", guiName, "GUI");
        if (HAPI_FileLengthByName(path) == 0)
            sprintf(name, "%sDL", g_game->sideNames_37f5b[player->info->side].name);
        else
            strcpy(name, guiName);
        Layer* layer = LoadGuiLayer(&g_game->gui, name, 0);
        if (layer != 0) {
            layer->handler = (void (__stdcall*)(void*))HandleBuildPanelClick;
            layer->data = 0;
            for (int i = 0; i < g_game->buildListCount; i++) {
                for (int j = 0; j < g_game->buildLists[i].count; j++) {
                    BuildList_0041ace0* lists = g_game->buildLists;
                    // The entry in a local: the address lea's base and index order follows it.
                    BuildEntry_0041ace0* entry = &lists[i].entries[j];
                    if (((UnitType*)unit->def)->id == entry->typeId
                        && entry->page - 1 == page) {
                        char* src = entry->name;
                        Gadget* e = &layer->entries[entry->slot + 4];
                        e->flag_bit = 0;
                        e->commonAttribs = 4;
                        strcpy(e->name, src);
                        found = 1;
                        e->shown = 1;
                    }
                }
            }
            if (found) {
                RenderLayer(&g_game->gui, 2);
                RenderLayer(&g_game->gui, 1);
            }
            SetPrevNext(unit);
            RefreshOrderButtons(unit);
            RefreshBuildCountTexts(&g_game->gui, unit);
            if (unit->flags & 0x20000000) {
                int index = FindGadgetIndexBySubstring(g_game->gui.layer->entries, "ONOFF");
                if (index != -1)
                    SetGadgetStatus(&g_game->gui, index, unit->view10e.onOff);
            }
            if (page != 0)
                UpdateCounts(&g_game->gui);
            RenderLayer(&g_game->gui, 0x40);
            g_game->unitIndex = unit->id;
            g_game->unitDefIndex = unit->unitDefIndex;
        }
    }
}

// Opens the side-specific "GEN.GUI" dialog (ARMGEN.GUI / COREGEN.GUI) with
// HandleBuildPanelClick as its handler and remembers the selected unit's two ids.
// FUNCTION: 0x41b0f0
void __stdcall OpenGeneratorDialog(Unit* unit)
{
    char name[256];
    sprintf(name, "%sGEN.GUI",
            g_game->sideNames_37f5b[g_game->players[g_game->localPlayer].info->side].name);
    Layer* gadget = LoadGuiLayer(&g_game->gui, name, 0);
    if (gadget != 0) {
        gadget->handler = (void (__stdcall*)(void*))HandleBuildPanelClick;
        gadget->data = 0;
        RefreshOrderButtons(unit);
        RenderLayer(&g_game->gui, 0x40);
        if (unit != 0) {
            g_game->unitIndex = unit->id;
            g_game->unitDefIndex = unit->unitDefIndex;
        } else {
            g_game->unitIndex = 0;
            g_game->unitDefIndex = 0;
        }
    }
}

// FUNCTION: 0x41b200
int __stdcall GetBuildMenuPage(Unit* obj)
{
    if (obj->menu.menuOpen) {
        return obj->menu.page;
    }
    return 0;
}

// Builds "<name><n>.GUI" for entry `index` of the table at g_game+0x1439b
// (0x249-byte entries) into `dest`. <windows.h> fixes the operand order of
// the address lea (tools/headers.py).
// FUNCTION: 0x41b230
void __stdcall BuildEntryGuiName(char* dest, unsigned short index, int n)
{
    char name[256];
    strncpy(name, ((UnitType *)g_game->defs)[index].unitname, 0x20);
    name[0x1f] = 0;
    sprintf(dest, "%s%d.GUI", name, n);
}

// FUNCTION: 0x41b2a0
Unit* GetBuildMenuFocusUnit()
{
    unsigned short index = g_game->unitIndex;
    if (index) {
        Unit* unit = &g_game->units[index];
        if (unit->unitDefIndex)
            return unit;
    }
    return 0;
}

// Finished-construction notification: marks the target as no longer being
// built (clears its build progress and sets flag 0x2000), updates the
// BUILDER.GUI panel or starts the builder's nanolathe, syncs two unit-type
// flag bits and the selection flag.
// FUNCTION: 0x41b8d0
void __stdcall FinishConstruction(Unit* unit, Unit* target)
{
    if (unit && (unit->flags & 0x10000000) && ((UnitType*)unit->def)->ids != 0
        && target && (target->flags & 0x10000000)) {
        ((Nano_0041b8d0*)target->state)->drawFrame = 0;
        target->buildLeft = 0;
        // In place, not through a local: the OR result is reused for the 0x20000000 test.
        target->flags |= 0x2000;
        if (target->player->active != 0
            && (target->player->type == 1 || target->player->type == 2)) {
            if (target->flags & 0x20000000) {
                if (IsScreenNamed(&g_game->gui, "BUILDER.GUI"))
                    MarkChanged(&g_game->gui);
            } else {
                if (target->carrier != 0)
                    AttachUnitToPiece(target, 0, -1, 1);
            }
        }
        if (((UnitType*)target->def)->flags.bits.flag_18)
            target->SetStateBits(1, 1);
        if (g_game->unitIndex == unit->id)
            RefreshBuildCountTexts(&g_game->gui, unit);
        if (((UnitType*)target->def)->flags.bits.flag_24) {
            target->lastDamageType = 7;
            target->flags |= 0x4000;
        }
        if (target->player->active != 0
            && (target->player->type == 1 || target->player->type == 2))
            BroadcastBuilderLink(unit, target);
        if ((unit->flags & 0x10) || (target->flags & 0x10))
            g_game->ordersPanelFlags_37ebe_6.raw |= 0x10;
    }
}

#include "../../interfaces/seams.h"

// Adds build progress to a unit under construction: `amount` build points
// (negative when it is being taken apart) move the remaining fraction at
// +0x104 towards 0 (done) or 1 (nothing built), clamped to [0, 1]. Building
// charges the builder's resource store (RequestEnergyAndMetal, energy first) for the
// energy and metal share of the step and only proceeds when the store accepts it.
// Unbuilding adds the metal share to the unit's float at +0xd4 (times 0.5 or
// 0.7 for a type 2 player when g_game+0x37eee is 0 or 1) and destroys the
// unit through DamageUnit once nothing is left. Either way the unit's hit
// points follow the progress, and a finished unit goes to FinishConstruction.
// FUNCTION: 0x41ba60
int __stdcall AddBuildProgress(Unit* builder, Unit* unit, float amount)
{
    int result = 0;
    if (unit->buildLeft == 0.0f)
        return 0;
    if (amount >= 0.0f)
        ((unsigned char*)&unit->netDirtyFlags)[1] |= 0x80;
    if (amount == 0.0f)
        return 0;
    UnitType* type = (UnitType*)unit->def;
    float prev = unit->buildLeft;
#ifdef __clang__
    volatile float roundedNext = min(max(prev - amount / type->buildtime, 0.0f), 1.0f);
    float next = roundedNext;
#else
    float next = min(max(prev - amount / type->buildtime, 0.0f), 1.0f);
#endif
#ifdef __clang__
    volatile float roundedStep = prev - next;
    float step = roundedStep;
    volatile float roundedCharges[2] = {type->energyCost * step, type->metalCost * step};
    float energyCharge = roundedCharges[0];
    float metalCharge = roundedCharges[1];
#else
    float step = prev - next;
    float energyCharge = type->energyCost * step;
    float metalCharge = type->metalCost * step;
#endif
    int hp = (int)(prev * type->maxHealth) - (int)(next * type->maxHealth);
    if (amount < 0.0f) {
        float refund = -metalCharge;
        // Through a float&: writing unit->resourceSlot.metalMake directly moves the health load and compare.
        float& store = unit->resourceSlot.metalMake;
        if (unit->resourceSlot.player->active != 0 && unit->resourceSlot.player->type == 2) {
            switch (g_game->difficulty) {
            case 0:
                store += refund * 0.5;
                break;
            case 1:
                store += refund * 0.7;
                break;
            default:
                store += refund;
                break;
            }
        } else {
            store += refund;
        }
        // health is set before remaining; the other order also changes the build path.
        unit->health = max(hp + unit->health, 0);
        unit->buildLeft = next;
        unit->flags |= 0x2000;
        if (next >= 1.0f)
            g_seams.weapons->DamageUnit(unit, unit, 30000, 9, 0);
    } else if (g_seams.economy->RequestEnergyAndMetal(&builder->resourceSlot, energyCharge, metalCharge)) {
        unit->health = min(hp + unit->health, ((UnitType*)unit->def)->maxHealth);
        unit->buildLeft = next;
        unit->flags |= 0x2000;
        result = 1;
    }
    if (unit->buildLeft == 0.0f)
        FinishConstruction(builder, unit);
    return result;
}

// FUNCTION: 0x41bcd0
void __stdcall ApplyUnfinishedBuildDecay(Unit* unit, int ticks)
{
    float val = -((float)(((UnitType*)unit->def)->buildtime * ticks) / ((UnitType*)unit->def)->energyCost);
    AddBuildProgress(unit, unit, val);
}

// FUNCTION: 0x41bd10
int __stdcall AddRepairProgress(Unit* builder, Unit* unit, float f)
{
    int result = 0;
    UnitType* s = (UnitType*)unit->def;
    int max = s->maxHealth;
    int v = s->buildtime;
    if (unit->health >= max)
        return 0;
#ifdef __clang__
    int n1 = (int)(((double)max * f - 1.0f) / v + 1.0f);
    int n2 = (int)(((double)s->energyCost * f - 1.0f) / v + 1.0f);
#else
    int n1 = (int)((max * f - 1.0f) / v + 1.0f);
    int n2 = (int)((s->energyCost * f - 1.0f) / v + 1.0f);
#endif
    if (n1 >= 1)
        n1 = 1;
    if (n2 >= 1)
        n2 = 1;
    if (g_seams.economy->RequestEnergy(&builder->resourceSlot, &builder->resourceSlot, (float)n2)) {
        g_seams.weapons->DamageUnit(builder, unit, n1, 10, 0);
        result = 1;
    }
    return result;
}

// The 3-bit page field (bits 23-25) is set from the type's buildMenuPageCount, or
// stepped down with wraparound when it is already past the first page.
static inline unsigned int SetPage(Unit* u, unsigned int f)
{
    return (((((UnitType*)u->def)->buildMenuPageCount + 0x1ff) << 23) ^ f) & 0x3800000 ^ f;
}

static inline unsigned int StepPage(unsigned int f)
{
    return (((f & 0xff800000) - 1) ^ f) & 0x3800000 ^ f;
}

// FUNCTION: 0x41bf10
void __stdcall StepBuildMenuPageBack(int toggle)
{
    unsigned short index = g_game->unitIndex;
    Unit* u;
    if (index == 0)
        goto nextbuild;
    u = &g_game->units[index];
    if (u->unitDefIndex == 0)
        u = 0;
    if (u == 0)
        goto nextbuild;

    // Each branch declares its own f: a single shared local made MSVC put the
    // field in eax and the unit in ecx, swapping every later operand.
    if (toggle) {
        unsigned int f = u->flags;
        if ((f & 0x400000) == 0) {
            f |= 0x400000;
            u->flags = f;
            u->flags = SetPage(u, f);
        } else if ((f & 0x3800000) == 0x800000) {
            u->flags = f & 0xffbfffff;
        } else {
            u->flags = StepPage(f);
        }
    } else {
        unsigned int f = u->flags;
        if ((f & 0x3800000) < 0x1000000)
            u->flags = SetPage(u, f);
        else
            u->flags = StepPage(f);
        u->flags |= 0x400000;
    }
    g_game->ordersPanelFlags_37ebe_6.lo |= 0x10;

nextbuild:
    PlaySoundByName("nextbuildmenu", 0);
}

// Opens the selected unit's build menu at the given page (closes it for 0),
// if the unit has that many pages.
// Needed: without it the page count is loaded before the page.

// FUNCTION: 0x41c060
void __stdcall OpenBuildMenuPage(int page)
{
    // Unit lookup written out in place: a GetSelectedUnit helper changes the code.
    unsigned short index = g_game->unitIndex;
    if (index != 0) {
        Unit* unit = &g_game->units[index];
        if (unit->unitDefIndex == 0)
            unit = 0;
        if (unit != 0) {
            if (page < ((UnitType*)unit->def)->buildMenuPageCount) {
                unit->menu.menuOpen = page > 0;
                if (page != 0)
                    unit->menu.page = page;
                g_game->ordersPanelFlags_37ebe_6.lo |= 0x10;
                PlaySoundByName("nextbuildmenu", 0);
            }
        }
    }
}

// FUNCTION: 0x41c110
void __stdcall MarkSelectionOrdersDirty(Unit* unit)
{
    if (unit->playerIndex == g_game->localPlayer && (*(unsigned char*)&unit->flags & 0x10)) {
        g_game->ordersPanelFlags_37ebe_6.raw |= 0x10;
    }
}

// FUNCTION: 0x41c150
void __stdcall UpdateBuildMenuIfFocusUnit(Unit* unit)
{
    Game* g = g_game;
    unsigned short val1 = g->unitIndex;
    unsigned short val2 = unit->id;
    if (val1 == val2) {
        RefreshBuildCountTexts(&g->gui, unit);
    }
}

// 0x41bde0 keeps its own file (this file's prelude moves its sub-object
// pointer from esi to edx); its declaration stands in for the definition.
void __stdcall StepBuildMenuPage(int toggle);

// The unit at g_game->unitIndex, or 0 when the index is empty or the slot is
// not live (unitDefIndex == 0). Inlined at both call sites below.
static Unit* GetSelectedUnit()
{
    unsigned short index = g_game->unitIndex;
    if (index) {
        Unit* unit = &g_game->units[index];
        if (unit->unitDefIndex)
            return unit;
    }
    return 0;
}

// FUNCTION: 0x41c180
void DispatchOrdersPanelPageFlags()
{
    if (g_game->ordersPanelFlags_37ebe_6.bits.next) {
        g_game->ordersPanelFlags_37ebe_6.bits.next = 0;
        StepBuildMenuPage(0);
        return;
    }
    if (g_game->ordersPanelFlags_37ebe_6.bits.prev) {
        g_game->ordersPanelFlags_37ebe_6.bits.prev = 0;
        StepBuildMenuPageBack(0);
        return;
    }
    if (g_game->ordersPanelFlags_37ebe_6.bits.build) {
        g_game->ordersPanelFlags_37ebe_6.bits.build = 0;
        Unit* unit = GetSelectedUnit();
        if (unit) {
            unit->flags |= 0x400000;
            g_game->ordersPanelFlags_37ebe_6.bits.flag_4 = 1;
        }
        return;
    }
    if (g_game->ordersPanelFlags_37ebe_6.bits.orders) {
        g_game->ordersPanelFlags_37ebe_6.bits.orders = 0;
        Unit* unit = GetSelectedUnit();
        if (unit) {
            unit->flags &= ~0x400000;
            g_game->ordersPanelFlags_37ebe_6.bits.flag_4 = 1;
        }
    }
}

// FUNCTION: 0x41c2b0
void ResetCameraState()
{
    char saved = g_game->scrollSpeed;
    memset(&(*(Unit **)&g_game->value_142f3), 0, 0x5c);
    g_game->scrollSpeed = saved;
}

// FUNCTION: 0x41c2e0
void __stdcall CycleCameraFollow(int param)
{
    Unit* val = (Unit *)g_game->value_142f3;
    Unit* result = FindNextSelectedUnit(val, param);
    (*(Unit **)&g_game->value_142f3) = result;
}

// Returns the bit set of the unit types in the named category.
// The bit test was an inlined helper taking the type as unsigned short.
static inline int TestBit(unsigned int* set, unsigned short n)
{
    return set[n >> 5] & (1 << (n & 0x1f));
}

// Finds the local player's commander: the last of the player's units whose
// type is in the "Commander" unit-type set.
// FUNCTION: 0x41c310
void FindLocalCommander()
{
    Player* p = &g_game->players[g_game->localPlayer];
    unsigned int* set = GetCategoryMask("Commander");
    for (Unit* u = p->unitsBegin; u <= p->unitsEnd; u++) {
        if (TestBit(set, u->unitDefIndex)) {
            (*(Unit **)&g_game->value_142f3) = u;
        }
    }
}

// FUNCTION: 0x41c390
void __cdecl ClearCameraFollowState()
{
    (*(unsigned short*)&g_game->value_1434b) = 0;
    (*(Unit **)&g_game->value_142f3) = 0;
    (*(Follow_0041ca10 **)&g_game->tracked) = 0;
}

// The original calls this out of line from the camera functions.
#pragma auto_inline(off)
// FUNCTION: 0x41c3c0
void ClampCameraPosition()
{
    int maxX = g_game->mapWidth - g_game->viewWidth;
    int maxY = g_game->mapHeight - g_game->viewHeight;
    if (g_game->scrollX < 0) {
        g_game->scrollX = 0;
    } else if (g_game->scrollX > maxX) {
        g_game->scrollX = maxX;
    }
    if (g_game->scrollY < 0) {
        g_game->scrollY = 0;
    } else if (g_game->scrollY > maxY) {
        g_game->scrollY = maxY;
    }
    CalcRadarViewportRect(g_game->field_142cb_142cb);
}
#pragma auto_inline(on)

// Same clamp as ClampCameraPosition, on the second position pair.
// FUNCTION: 0x41c450
void ClampCameraTarget()
{
    int maxX = g_game->mapWidth - g_game->viewWidth;
    int maxY = g_game->mapHeight - g_game->viewHeight;
    if (g_game->x2 < 0) {
        g_game->x2 = 0;
    } else if (g_game->x2 > maxX) {
        g_game->x2 = maxX;
    }
    if (g_game->y2 < 0) {
        g_game->y2 = 0;
    } else if (g_game->y2 > maxY) {
        g_game->y2 = maxY;
    }
}

// FUNCTION: 0x41c4c0
void __stdcall SetCameraPosition(int x, int y, int instant)
{
    if (instant != 0) {
        g_game->x2 = x;
        g_game->y2 = y;
        int maxX = g_game->mapWidth - g_game->viewWidth;
        int maxY = g_game->mapHeight - g_game->viewHeight;
        if (g_game->x2 < 0) {
            g_game->x2 = 0;
        } else if (g_game->x2 > maxX) {
            g_game->x2 = maxX;
        }
        if (g_game->y2 < 0) {
            g_game->y2 = 0;
        } else if (g_game->y2 > maxY) {
            g_game->y2 = maxY;
        }
    } else {
        g_game->scrollX = x;
        g_game->scrollY = y;
        g_game->viewDirtyFlags.lo |= 2;
        ClampCameraPosition();
        g_game->x2 = g_game->scrollX;
        g_game->y2 = g_game->scrollY;
    }
    g_game->mapFlags &= 0xfff7;
}

// FUNCTION: 0x41c5e0
void __stdcall StartScreenShake(int dx, int dy, int value)
{
    if ((g_game->uiOptionFlags_37f2f_1 & 0x10) == 0) {
        g_game->value_1432f = value;
        g_game->value_14333 = value;
        if (dx != 0) {
            g_game->sum_x = dx;
        }
        if (dy != 0) {
            g_game->sum_y = dy;
        }
        if (value > 0) {
            g_game->flags_1434e |= 1;
        }
    }
}

// FUNCTION: 0x41c640
void __stdcall AccumulateScreenShake(int dx, int dy, int value)
{
    if ((g_game->uiOptionFlags_37f2f_1 & 0x10) == 0) {
        if ((g_game->flags_1434e & 1) == 0) {
            g_game->sum_x = 0;
            g_game->sum_y = 0;
        }
        g_game->value_1432f = (g_game->value_1432f + value) / 2;
        g_game->value_14333 = g_game->value_1432f;
        g_game->sum_x += dx;
        g_game->sum_y += dy;
        if (g_game->value_1432f > 0) {
            g_game->flags_1434e |= 1;
        }
    }
}

// The original calls this out of line from UpdateCameraFollow.
#pragma auto_inline(off)
int HarnessViewRand();

// FUNCTION: 0x41c6f0
void UpdateScreenShake()
{
    if ((g_game->flags_1434e & 1) != 0) {
        int count = g_game->value_14333;
        if (count > 0) {
            int dx = g_game->sum_x * count / g_game->value_1432f;
            int dy = g_game->sum_y * count / g_game->value_1432f;
            int rx = (int)(((__int64)HarnessViewRand() * dx) / 32768) - dx / 2;
            int ry = (int)(((__int64)HarnessViewRand() * dy) / 32768) - dy / 2;
            g_game->scrollX += rx;
            g_game->scrollY += ry;
            g_game->value_14333--;
        } else {
            g_game->flags_1434e &= 0xfe;
        }
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x41c7c0
void __stdcall CenterCameraOnPoint(int a, int b, int c)
{
    int cy = b - g_game->viewHeight / 2;
    int cx = a - g_game->viewWidth / 2;
    if (c != 0) {
        g_game->x2 = cx;
        g_game->y2 = cy;
        int maxX = g_game->mapWidth - g_game->viewWidth;
        int maxY = g_game->mapHeight - g_game->viewHeight;
        if (g_game->x2 < 0) {
            g_game->x2 = 0;
        } else if (g_game->x2 > maxX) {
            g_game->x2 = maxX;
        }
        if (g_game->y2 < 0) {
            g_game->y2 = 0;
        } else if (g_game->y2 > maxY) {
            g_game->y2 = maxY;
        }
    } else {
        g_game->scrollX = cx;
        g_game->scrollY = cy;
        g_game->viewDirtyFlags.lo |= 2;
        ClampCameraPosition();
        g_game->x2 = g_game->scrollX;
        g_game->y2 = g_game->scrollY;
    }
    g_game->mapFlags &= 0xfff7;
}

// FUNCTION: 0x41c8e0
void __stdcall CenterCameraOnMapPosition(Vec3* p, int param_2)
{
    int cy = (short)((p->z - (p->y >> 1)) >> 16) - g_game->viewHeight / 2;
    int cx = (short)(p->x >> 16) - g_game->viewWidth / 2;
    if (param_2 != 0) {
        g_game->x2 = cx;
        g_game->y2 = cy;
        int maxX = g_game->mapWidth - g_game->viewWidth;
        int maxY = g_game->mapHeight - g_game->viewHeight;
        if (g_game->x2 < 0) {
            g_game->x2 = 0;
        } else if (g_game->x2 > maxX) {
            g_game->x2 = maxX;
        }
        if (g_game->y2 < 0) {
            g_game->y2 = 0;
        } else if (g_game->y2 > maxY) {
            g_game->y2 = maxY;
        }
    } else {
        g_game->scrollX = cx;
        g_game->scrollY = cy;
        g_game->viewDirtyFlags.lo |= 2;
        ClampCameraPosition();
        g_game->x2 = g_game->scrollX;
        g_game->y2 = g_game->scrollY;
    }
    g_game->mapFlags &= 0xfff7;
}

// SetTarget/ClampTarget stay helpers: they give the original's load order.
static inline void SetTarget(int x, int y)
{
    g_game->x2 = x;
    g_game->y2 = y;
}

static inline void ClampTarget()
{
    int maxX = g_game->mapWidth - g_game->viewWidth;
    int maxY = g_game->mapHeight - g_game->viewHeight;
    if (g_game->x2 < 0) {
        g_game->x2 = 0;
    } else if (g_game->x2 > maxX) {
        g_game->x2 = maxX;
    }
    if (g_game->y2 < 0) {
        g_game->y2 = 0;
    } else if (g_game->y2 > maxY) {
        g_game->y2 = maxY;
    }
}

// Stays an inline helper written as `cur + -320`: other forms change the arithmetic.
static inline int Approach(int cur, int target)
{
    int d = cur - target;
    if (d > 0) {
        if (d > 320)
            return cur + -320;
    } else {
        if (d < -320)
            return cur + 320;
    }
    return cur - d / 2;
}

// Camera follow: picks a target position (a pending jump at +0x1433f while
// its counter runs, else the followed object at +0x142f7, else the followed
// unit at +0x142f3 while it is still alive), centres the scroll target on it
// and clamps it to the map, then moves the scroll position halfway towards
// the target (at most 320 pixels per axis) and re-clamps it.

// Needed: without it the view centre x is computed in the wrong order.
// FUNCTION: 0x41ca10
void UpdateCameraFollow()
{
    Vec3* p = 0;
    if ((unsigned short)g_game->value_1434b != 0) {
        (*(unsigned short*)&g_game->value_1434b)--;
        p = &g_game->trackedPos;
    } else if ((Follow_0041ca10 *)g_game->tracked != 0) {
        p = &((Follow_0041ca10 *)g_game->tracked)->pos;
    } else if ((Unit *)g_game->value_142f3 != 0) {
        if (((Unit *)g_game->value_142f3)->flags & 0x10000000) {
            p = &((Unit *)g_game->value_142f3)->pos;
        } else {
            (*(unsigned short*)&g_game->value_1434b) = 0;
            (*(Unit **)&g_game->value_142f3) = 0;
            (*(Follow_0041ca10 **)&g_game->tracked) = 0;
        }
    }
    if (p != 0) {
        SetTarget((short)(p->x >> 16) - g_game->viewWidth / 2,
                  (short)((p->z - (p->y >> 1)) >> 16) - g_game->viewHeight / 2);
        ClampTarget();
        g_game->mapFlags &= 0xfff7;
    }
    if (g_game->scrollX != g_game->x2) {
        g_game->viewDirtyFlags.raw |= 2;
        g_game->mapFlags &= 0xfff7;
        g_game->scrollX = Approach(g_game->scrollX, g_game->x2);
    }
    if (g_game->scrollY != g_game->y2) {
        g_game->viewDirtyFlags.raw |= 2;
        g_game->mapFlags &= 0xfff7;
        g_game->scrollY = Approach(g_game->scrollY, g_game->y2);
    }
    UpdateScreenShake();
    ClampCameraPosition();
}

// FUNCTION: 0x41cc60
void BeginMouseScroll()
{
    (*(unsigned short*)&g_game->value_1434b) = 0;
    (*(Unit **)&g_game->value_142f3) = 0;
    (*(Follow_0041ca10 **)&g_game->tracked) = 0;
    HideSoftwareCursor();
    CursorState* cursor = &(*(CursorState*)&g_game->cursor);
    cursor->flag = 1;
    cursor->start = (*(MouseEvent*)&g_game->view);
    cursor->savedScroll.x = g_game->scrollX / 16;
    cursor->savedScroll.y = g_game->scrollY / 16;
    cursor->center.x = GetScreenWidth() / 2;
    cursor->center.y = GetScreenHeight() / 2;
    SetCursorPosition(cursor->center.x, cursor->center.y);
}

// Clears a flag in the cursor state held in the game object and warps the
// mouse back to the saved position. The state is reached through a pointer
// local, which keeps the `add eax, 0x2cc7` the original has.

// FUNCTION: 0x41cd20
void EndMouseScroll()
{
    CursorState* cursor = &(*(CursorState*)&g_game->cursor);
    cursor->flag = 0;
    SetCursorPosition(cursor->start.x, cursor->start.y);
    ShowSoftwareCursor();
}

// The same scroll-and-clamp sequence is written out in 0x41c7c0, 0x41c8e0,
// 0x41d0f0 and 0x41d1f0.
static inline void ScrollTo(int x, int y)
{
    g_game->scrollX = x;
    g_game->scrollY = y;
    g_game->viewDirtyFlags.raw |= 2;
    ClampCameraPosition();
    g_game->x2 = g_game->scrollX;
    g_game->y2 = g_game->scrollY;
    g_game->mapFlags &= 0xfff7;
}

// Mouse scrolling: moves the view by a quarter of the mouse offset from the
// centre point, saves the new scroll position (in 16-pixel units), warps the
// OS cursor back to the centre, and ends the scroll (restoring the cursor to
// where it started) once the button is released. 0x41cc60 starts it.

// FUNCTION: 0x41cd50
void UpdateMouseScroll()
{
    // All fields go through one pointer c, and the quarter offsets keep their own
    // locals dx, dy: both fix the load order and register use.
    CursorState* c = &(*(CursorState*)&g_game->cursor);
    MouseEvent r = (*(MouseEvent*)&g_game->view);
    int dx = (r.x - c->center.x) / 4;
    int x = (dx + c->savedScroll.x) * 16;
    int dy = (r.y - c->center.y) / 4;
    int y = (dy + c->savedScroll.y) * 16;
    ScrollTo(x, y);
    c->savedScroll.x = g_game->scrollX / 16;
    c->savedScroll.y = g_game->scrollY / 16;
    SetCursorPosition(c->center.x, c->center.y);
    if (!(r.keyFlags & 2)) {
        CursorState* cursor = &(*(CursorState*)&g_game->cursor);
        cursor->flag = 0;
        SetCursorPosition(cursor->start.x, cursor->start.y);
        ShowSoftwareCursor();
    }
}

// Unused, but needed: changes how the scroll speed multiply is compiled.
#include <string>

// Edge and arrow-key scrolling: moves the view by the scroll speed (capped at
// 0x80) when an arrow key is held (unless the TALK.GUI chat box is open) or
// the mouse sits on the edge of the screen, then clamps and saves it.

// FUNCTION: 0x41ce90
void UpdateEdgeScroll()
{
    MouseEvent mouse;
    POINT pt;
    // Through a local: the direct spelling loads the scale into ebx.
    Game* g = g_game;
    int speed = g->scrollSpeed * g->scrollScale;
    if (speed > 0x80)
        speed = 0x80;
    if (speed == 0)
        return;
    GetCurrentMouseEvent(&mouse);
    Display_0041ce90* d = GetDisplay();
    if (!(d->flags_f0 & 2)) {
        host_cursor_get((int*)&pt.x, (int*)&pt.y);
        int w = g_game->width_37e1f;
        int h = g_game->field_37e23;
        if ((pt.x >= w || pt.y >= h) && pt.x < w + 100 && pt.y < h + 100
            && host_window_has_focus()) {
            mouse.x = pt.x;
            if (pt.x >= w)
                mouse.x = w - 1;
            mouse.y = pt.y;
            if (pt.y >= h)
                mouse.y = h - 1;
        }
    } else {
        int w = g_game->width_37e1f;
        int h = g_game->field_37e23;
        if (mouse.x >= w)
            mouse.x = w - 1;
        if (mouse.y >= h)
            mouse.y = h - 1;
    }
    int talk = IsScreenNamed(&g_game->gui, "TALK.GUI");
    int x = g_game->scrollX;
    int y = g_game->scrollY;
    if ((IsKeyDown(0xf4) && !talk) || (mouse.x == 0 && mouse.y < g_game->field_37e23))
        x -= speed;
    else if ((IsKeyDown(0xf6) && !talk) || mouse.x == g_game->width_37e1f - 1)
        x += speed;
    if ((IsKeyDown(0xf5) && !talk) || (mouse.y == 0 && mouse.x < g_game->width_37e1f))
        y -= speed;
    else if ((IsKeyDown(0xf7) && !talk) || mouse.y == g_game->field_37e23 - 1)
        y += speed;
    if (g_game->scrollX != x || g_game->scrollY != y) {
        g_game->scrollX = x;
        g_game->scrollY = y;
        g_game->viewDirtyFlags.lo |= 2;
        ClampCameraPosition();
        g_game->x2 = g_game->scrollX;
        g_game->y2 = g_game->scrollY;
        g_game->mapFlags &= 0xfff7;
        (*(unsigned short*)&g_game->value_1434b) = 0;
        (*(Unit **)&g_game->value_142f3) = 0;
        (*(Follow_0041ca10 **)&g_game->tracked) = 0;
    }
}

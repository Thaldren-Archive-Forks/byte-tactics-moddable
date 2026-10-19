// Decompiled by space-bunny-free, Sonnet 5.5, deepseek-v4.1-flash, GPT-6.1-sol, deepseek-v4.1, mimo-v2.6-pro, DeepSeek V4.1 Flash, GPT-6, claude-opus-5-5, Opus, Haiku, Space Bunny Free, opus and Claude Opus 5.5. Names are provisional.
// The player slots (g_game + 0x1b63, 0x14b bytes each): starting, resetting,
// initialising and freeing them, the metal and energy transfers, the per-tick
// player update, the unit line-of-sight loop and the camera setup.
#include <stdio.h>
#include "../state/block.h"
#include <string.h>

#include "../map/mission.h"
#include "mission_conditions.h"
#include "../game/unit_resources.h"

struct Player;
struct Unit;
class Pathfinder;

#pragma pack(push, 1)

#include "../ai/squad_manager.h"

#include "../util/vec3.h"

struct Map {
    char unknown_0[0x620];
    int field_620;                     // +0x620
};

// The original declares the tag as a struct here (the decorated name of
// CanPlaceUnitFootprint), so forward-declare it before the header.
struct UnitDef;
#include "../units/unit_def.h"

// A unit. Only the fields these functions use are named.
#include "../units/unit.h"

#include "../network/player_info.h"
#include "../../interfaces/seams.h"

#include "../network/player.h"

struct SkirmishPlayerSlot {            // 0x18 bytes
    int controller;                    // +0x0
    char unknown_4[0xc - 0x4];
    int metal;                         // +0xc
    int energy;                        // +0x10
    int unknown_14;
};

struct Form {
    char unknown_0[0xcc];
    char choice1[0x10];                // +0xcc
    char choice2[0x20];                // +0xdc
};

struct Gadget;

struct Screen {
    char unknown_0[4];
    Form* form;                        // +0x4
    void (__stdcall *callback)(Gadget*); // +0x8
};

struct Gadget {
    char unknown_0[0x18];
    Screen* screen;                    // +0x18
    char unknown_1c[0x60 - 0x1c];
    int selected;                      // +0x60
};

#include "game.h"
#pragma pack(pop)

extern Game* g_game;

const char* __stdcall Translate(const char* text);

// FUNCTION: 0x464290
void __stdcall SetupPlayerSlot(int player, char type)
{
    Player* p = &g_game->players[player & 0xff];
    // Declared after p: puts the pointer in the addressing-mode index slot.
    int idx = player & 0xff;

    memset(p->allied, 0, 11);
    memset(p->alliedBy, 0, 11);
    memset(p->shareLos, 0, 11);
    memset(p->shareVision, 0, 11);
    memset(p->shareMapping, 0, 11);
    int t = type;
    p->type = t;
    if (t != 3) {
        p->info->kind = t;
    }
    p->alliedBy[idx] = 1;
    p->ai = 0;
    p->index = (char)player;
    p->rank = (char)player;
    p->allied[idx] = 1;
    p->active = 1;
    p->startPos = (char)player;
    p->alliance = 5;
    p->info->bit5 = 0;
    p->id = player & 0xff;
    p->lobbyDataSynced = 0;
    p->rejectReason = 0;

    if (p->active != 0 && (p->type == 1 || p->type == 2)) {
        p->info->memory = ((Map*)g_game->unknown_c)->field_620 / 0x100000 + 1;
    }

    if (g_seams.map->GetGameType(g_game->net) == 1) {
        if (type == 1) {
            sprintf(p->name, Translate("Player"));
        } else if (type == 2) {
            // Tested as `== 0`: keeps the Arm arm inline, matching the block layout.
            if (p->info->side == 0) {
                sprintf(p->name, "Arm");
            } else {
                sprintf(p->name, "Core");
            }
        }
        strcpy(p->fullName, p->name);
    }

    if (g_seams.map->GetGameType(g_game->net) == 2) {
        if (type == 1) {
            sprintf(p->name, Translate("Player"));
        } else if (type == 2) {
            if (p->info->side == 0) {
                sprintf(p->name, Translate("Arm"));
            } else {
                sprintf(p->name, Translate("Core"));
            }
        }
        strcpy(p->fullName, p->name);
    }
}

extern char DAT_005119b8[];

// FUNCTION: 0x4644d0
void ResetPlayerSlots()
{
    Player* p;
    int i;

    for (i = 0; i <= 10; i++) {
        PlayerInfo* t = g_game->players[i].info;
        memset(&g_game->players[i], 0, 0x14b);
        g_game->players[i].info = t;
    }

    g_game->localPlayer = 0;
    g_game->playerIndex = 0;
    memset(g_game->playerIds, 0, 0x2c);

    p = &g_game->players[0];
    for (i = 0; i <= 10; i++, p++) {
        memset(p->allied, 0, 11);
        memset(p->alliedBy, 0, 11);
        memset(p->info, 0, 0xb9);
        sprintf(p->name, "Player %d First", i);
        sprintf(p->fullName, "Player %d Second", i);
        p->allied[i] = 1;
        p->alliedBy[i] = 1;
        // Inlined helper: separate stores let the reload of p->info be hoisted.
        p->Clear();
        p->info->kind = 0;
        p->info->memory = 0;
        p->info->started = 0;
        p->info->host = 0;
        p->info->color = (char)i;
        p->info->side = 0;
        strcpy(p->info->map, DAT_005119b8);
        p->ai = 0;
        p->lobbyDataSynced = 0;
        p->syncTick = 0;
        p->unitsBegin = 0;
        p->unitsEnd = 0;
        p->firstIndex = 0;
        p->lastIndex = 0;
        p->unitCount = 0;
        // Stays before unitsCreated = 0.
        p->keepaliveFlags &= 0xfe;
        p->unitsCreated = 0;
        p->id = -1;
        p->index = 10;
        p->alliance = 5;
        p->info->bit5 = 0;
    }

    g_game->tail = 0;
    g_game->head = 0;
}


void __cdecl operator delete(void* p);

// FUNCTION: 0x464700
void __stdcall InitPlayerSlot(Player* p)
{
    p->updateTime = (unsigned int)g_game->ticks;
    p->winLoseTime = (unsigned int)g_game->ticks;
    // The third stamp runs here but is written after the clears: C2 has to number its location after theirs.
    goto stamp;
back:
    p->totalEnergyProducedLo = 0;
    p->totalMetalProducedLo = 0;
    p->totalEnergyConsumedLo = 0;
    p->totalMetalConsumedLo = 0;
    p->energyWastedLo = 0;
    p->metalWastedLo = 0;
    p->energy = 0;
    p->energyIncome = 0;
    p->energyUsage = 0;
    p->metal = 0;
    p->metalIncome = 0;
    p->metalUsage = 0;
    p->energyCapacity = 0;
    p->metalCapacity = 0;
    p->totalEnergyProducedHi = 0;
    p->totalMetalProducedHi = 0;
    p->totalEnergyConsumedHi = 0;
    p->totalMetalConsumedHi = 0;
    p->shareEnergy = 0;
    p->shareMetal = 0;
    p->energyWastedHi = 0;
    p->metalWastedHi = 0;
    goto guard;
stamp:
    p->displayTimer = (unsigned int)g_game->ticks;
    goto back;
guard:
    if (!p->econ) {
        p->econ = new (kSim) UnitResources;
    }
    g_seams.economy->Reset(p->econ, p->index);
    p->flags &= 0xfffe;
    p->kills = 0;
    p->losses = 0;
    p->commanderKills = 0;
    p->commanderLosses = 0;
    p->unused_102 = p->unused_100 = -1;
    // w declared first: puts height/2 in edi and width/2 in ebx.
    int w, h;
    h = g_game->height / 2;
    w = g_game->width / 2;
    p->exploredWidth = w;
    p->exploredHeight = h;
    operator delete(p->explored);
    int& sz = p->exploredSize;
    sz = (h * w + 7) & ~7;
    p->explored = sz ? (unsigned char*)SimAlloc(sz) : 0;
    // Both references are load-bearing (`sz` also for the first block). With `memset(p->buffer, 0, p->exploredSize)`
    // MSVC propagates the phi and the size into the inlined memset, which puts
    // the phi in edi and stores it from edi; the original reloads [esi+0x7c]
    // into edi and keeps the phi in eax. Reading the fields through references
    // stops that propagation, and it also stops the scheduler from hoisting
    // the size load `mov ecx, [esi+0x88]` above the phi store (with the size
    // read directly the load comes first, the original has it second).
    unsigned char*& bref = p->explored;
    memset(bref, 0, sz);
    g_seams.ai->CreateSquads(p);
    // Keep `||` with both tests as written: the branch layout depends on it.
    if (!p->active || p->type != 3) {
        p->ai = new (kSim) SquadManager(p);
        g_seams.ai->CreatePlayerAI(p->index);
    }
}

class CommandArgs {
public:
    char unknown_0[0xd0];
    int count;
    CommandArgs* InitArgs();
};

char* __stdcall HAPI_LoadFile(const char* name, int* size);
int __stdcall ExecuteCommandText(char* text, int len, CommandArgs* vars, int param_4);
void __cdecl GameFreeThunk(char* text);

// FUNCTION: 0x4648e0
void LoadDefaultAIScript()
{
    int size;
    char* name = g_seams.map->GetNameSlot(g_game->net, 7);
    char* text = HAPI_LoadFile(name, &size);
    if (text == 0) {
        text = HAPI_LoadFile("ai\\default.txt", &size);
    }
    if (text != 0) {
        CommandArgs vars;
        vars.InitArgs();
        ExecuteCommandText(text, size, &vars, -1);
        GameFreeThunk(text);
    }
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active && p->type == 2) {
            g_seams.ai->ParseDownloadableAiWeightScripts(i);
            g_seams.ai->ReparseAiWeightScriptsIfLimitNotSticky(i);
        }
    }
}

// A char loop counter gives the separate countdown register (edi = 10), as
// in 0x403100; the entry pointer is computed before the test.
// FUNCTION: 0x464990
void InitPlayers()
{
    for (char i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->type) {
            InitPlayerSlot(p);
        }
    }
    LoadDefaultAIScript();
}

// FUNCTION: 0x4649d0
void RebuildAIFeatureCells()
{
    for (char i = 0; i < 10; i++) {
        if (g_game->players[i].ai) {
            g_seams.ai->RebuildFeatureCells(i);
        }
    }
}

// The loop bound is `p <= g_game->players + 10`, one past the last slot, so
// the body also runs once on the fields that follow the array (the `ja` guard
// only skips the loop when the array is empty). The original really does
// that.
// FUNCTION: 0x464a00
void FreePlayers()
{
    for (Player* p = g_game->players; p <= g_game->players + 10; p++) {
        // Fields reached as indices off q: keeps two induction variables.
        int* q = (int*)((char*)p + 0x84);
        q[-1] = 0;
        q[0] = 0;
        delete[] ((void**)q)[-2];
        q[1] = 0;
        ((void**)q)[-2] = 0;
        g_seams.ai->FreeSquads(p);
        if (((SquadManager**)q)[-4]) {
            g_seams.ai->DestroyPlayerAI(((unsigned char*)q)[0xc2]);
            if (SquadManager* u = ((SquadManager**)q)[-4]) {
                g_seams.ai->SquadManagerDeleteTimers(u, 0);
                delete u;
            }
            ((SquadManager**)q)[-4] = 0;
        }
        if (((void**)q)[0x1a]) {
            delete[] ((void**)q)[0x1a];
            ((void**)q)[0x1a] = 0;
        }
    }
}

// FUNCTION: 0x464ab0
float __stdcall GetEnergyIncome(void* param_1)
{
    return *(float*)((char*)param_1 + 0x90);
}

// Unused here: the symbol ids these declarations take keep the allocation
// (docs/c2-regalloc.md).
void RegisterUnitOrders(void);
void RegisterGroundOrders(void);
void ResetAIPlayers(void);
int FUN_00490200();

// FUNCTION: 0x464ac0
float __stdcall GetEnergyUsage(Player* param_1)
{
    return param_1->energyUsage;
}

// FUNCTION: 0x464ad0
float __stdcall GetNetEnergy(void* param_1)
{
    return *(float*)((char*)param_1 + 0x90) - *(float*)((char*)param_1 + 0x94);
}

// FUNCTION: 0x464af0
float __stdcall GetMetalIncome(void* param_1)
{
    return *(float*)((char*)param_1 + 0x9c);
}

// FUNCTION: 0x464b00
float __stdcall GetMetalUsage(void* param_1)
{
    return *(float*)((char*)param_1 + 0xa0);
}

struct Class_464b10 {
    char unknown_0[0x9c];
    float metalIncome;
    float metalUsage;
};

// FUNCTION: 0x464b10
float __stdcall GetNetMetal(Class_464b10* param_1)
{
    return param_1->metalIncome - param_1->metalUsage;
}

void __stdcall SendShareMetal(unsigned char from, unsigned char to, int value);

// Moves `amount` metal from player `from` to player `to`, the metal twin of
// 0x464c60 (which moves energy). It is clamped to what `from` has stored,
// taken out of `from`'s economy object (only when `flag` is set) and added to
// `to`'s economy object. An AI player (type 2) on easy or medium only counts
// 0.5 or 0.7 of it, and the transfer is announced over the network when `flag`
// is set. Same shape as 0x464c60, which moves energy with UnitResources.
// FUNCTION: 0x464b30
void __stdcall TransferMetal(unsigned char from, unsigned char to, float amount, int flag)
{
    if (from == 10)
        return;
    if (to == 10)
        return;
    if (flag) {
        float cap = g_game->players[from].energy;
        if (amount > cap)
            amount = cap;
    }
    if (amount == 0.0f)
        return;
    Player* player;
    // Receiver read inside both arms of the flag test: homes `to` in ebx.
    player = flag ? (g_seams.economy->SpendEnergy(g_game->players[from].econ, amount),
                     g_game->players[to].econ->player)
                  : g_game->players[to].econ->player;
    if (player->active != 0 && player->type == 2) {
        switch (g_game->difficulty) {
        case 0:
            g_game->players[to].econ->energyMake
                = g_game->players[to].econ->energyMake - amount * -0.5;
            break;
        case 1:
            g_game->players[to].econ->energyMake
                = g_game->players[to].econ->energyMake - amount * -0.7;
            break;
        default: {
            // Add goes through a local float: fixes the x87 operand order.
            float m = g_game->players[to].econ->energyMake;
            m += amount;
            g_game->players[to].econ->energyMake = m;
            break;
        }
        }
    } else {
        // Add goes through a local float: fixes the x87 operand order.
        float m = g_game->players[to].econ->energyMake;
        m += amount;
        g_game->players[to].econ->energyMake = m;
    }
    if (flag)
        SendShareMetal(from, to, *(int*)&amount);
}

void __stdcall SendShareEnergy(unsigned char from, unsigned char to, int value);

// Moves `amount` energy from player `from` to player `to`. The amount is first
// clamped to what `from` has stored, then (only when `flag` is set) taken out of
// `from`'s economy object and added to `to`'s. An AI player (type 2) on easy or
// medium only counts 0.5 or 0.7 of it, and the transfer is announced over the
// network when `flag` is set (the amount travels as its bit pattern).
// FUNCTION: 0x464c60
void __stdcall TransferEnergy(unsigned char from, unsigned char to, float amount, int flag)
{
    if (from == 10)
        return;
    if (to == 10)
        return;
    if (flag) {
        float cap = g_game->players[from].metal;
        if (amount > cap)
            amount = cap;
    }
    if (amount == 0.0f)
        return;
    Player* player;
    // Receiver read inside both arms of the flag test: homes `to` in ebx.
    player = flag ? (g_seams.economy->SpendMetal(g_game->players[from].econ, amount), g_game->players[to].econ->player)
                  : g_game->players[to].econ->player;
    if (player->active != 0 && player->type == 2) {
        switch (g_game->difficulty) {
        case 0:
            g_game->players[to].econ->metalMake += amount * 0.5;
            break;
        case 1:
            g_game->players[to].econ->metalMake += amount * 0.7;
            break;
        default:
            // Full `players[to].econ` expression: keeps the store from folding
            // into the load.
            g_game->players[to].econ->metalMake = amount + g_game->players[to].econ->metalMake;
            break;
        }
    } else {
        g_game->players[to].econ->metalMake = amount + g_game->players[to].econ->metalMake;
    }
    if (flag)
        SendShareEnergy(from, to, *(int*)&amount);
}


// Calls UpdateUnitLineOfSight for every unit in the range whose flag 0x10000000 is set.
// FUNCTION: 0x464da0
void __stdcall RefreshOwnedUnitsLineOfSight(Player* range)
{
    for (Unit* u = range->unitsBegin; u <= range->unitsEnd; u++) {
        if (u->flags & 0x10000000) {
            g_seams.los_radar->UpdateUnitLineOfSight(u);
        }
    }
}

void BroadcastPlayerInfo();
void __stdcall ReportGameEvent(int param_1);
void __stdcall PlaySoundByName(char* str, int flag);
int __stdcall IsGadgetNamed(int param1, int param2, char* name);
void __stdcall ClearSelectedGadget(void* param_1);

// FUNCTION: 0x464de0
void __stdcall ContinueWatchingCallback(Gadget* gadget)
{
    int screen = (int)gadget->screen->form;
    if (gadget->selected == -1)
        return;
    PlaySoundByName("BigButton", 0);
    if (IsGadgetNamed(screen, gadget->selected, "CHOICE1")) {
        BroadcastPlayerInfo();
        g_game->bits.flag4 = 0;
        ReportGameEvent(4);
        return;
    }
    if (!IsGadgetNamed(screen, gadget->selected, "CHOICE2")) {
        ClearSelectedGadget(gadget);
        return;
    }
    g_game->bits.flag2 = 1;
    g_game->bits.flag4 = 0;
}

extern char g_choice2GadgetName[];
extern char g_choice1GadgetName[];
extern char g_titleGadgetName[];
extern char g_noText[];
extern char g_yesText[];
extern char g_yesNoGuiName[];
extern char g_continueWatchingTitle[];

Screen* __stdcall LoadGuiLayer(Menu* menu, char* name, int value);
void __stdcall SetKeyboardInput(Menu* menu, int flag);
void __stdcall SetTranslatedTextByName(Menu* menu, char* name, char* text, int value);
void __stdcall RenderLayer(Menu* menu, int value);

// FUNCTION: 0x464e70
void ShowContinueWatchingDialog()
{
    Screen* screen = LoadGuiLayer(&g_game->gui_519, g_yesNoGuiName, 0x900);
    if (screen) {
        Form* form;
        SetKeyboardInput(&g_game->gui_519, 1);
        form = screen->form;
        SetTranslatedTextByName(&g_game->gui_519, g_choice1GadgetName, g_yesText, 0);
        SetTranslatedTextByName(&g_game->gui_519, g_choice2GadgetName, g_noText, 0);
        SetTranslatedTextByName(&g_game->gui_519, g_titleGadgetName, g_continueWatchingTitle, 0);
        strcpy(form->choice1, g_choice1GadgetName);
        strcpy(form->choice2, g_choice2GadgetName);
        screen->callback = ContinueWatchingCallback;
        RenderLayer(&g_game->gui_519, 0x40);
    }
}

extern int g_economyPacketTick;

unsigned char __stdcall FindHostSlot();
int __stdcall RandomInt(int range);
Unit* __stdcall CreateUnit(unsigned char player, unsigned short typeId,
                           Vec3 pos, int a, int b, int c);
void __stdcall SetStartingStorageBonus(Player* player, int height, int width);
void __stdcall FocusCommander(int on);
void __stdcall SendPlayerEconomy(Player* player, int a, int b);
int __stdcall CountCombatPlayers();
int __stdcall CountActiveAIPlayers();
void __stdcall OpenMessageBox(Menu* menu, const char* text, int a, int b, int c);

static int loopCond_00464f80(unsigned char i)
{
    if (i >= 0xa)
        return 0;
    return 1;
}

// The loop's latch test. It has to be a second, separately spelled inlined
// helper: with the same expression at both test sites MSVC folds one of them
// away, and the original keeps both.
static int more_00464f80(unsigned char i)
{
    if (i < 0xa)
        return 1;
    return 0;
}

// The three scale constants, named so that each lands in .rdata as its own
// object of exactly the original's width, and in this order: MSVC 5 emits a
// float LITERAL in an 8-byte slot but a named static const float in 4, and
// literals come after statics, so with 100.0f spelled as a literal it is the
// first .rdata object and the checker reads its slot's 4 padding bytes as
// part of it (the original's next constant is another function's 12700.0f).
static const double kNegSeven = -0.7;
static const double kNegHalf = -0.5;
static const float kHundred = 100.0f;

void __stdcall UpdatePlayers()
{
    g_seams.players->UpdatePlayersOnStream(RAND_STREAM_MAIN);
}

// FUNCTION: 0x464f80
void __stdcall UpdatePlayersOnStream(RandStream stream)
{
    g_seams.pathfinding->RunSearches((Pathfinder*)g_game->pathfinder);
    // bl is declared before the guard and pi before the first goto, or the jump
    // is rejected.
    unsigned char bl = 0;
    // `for (;;)` with loopCond at the top and more() at next_bl: keeps both the
    // entry guard and the latch test.
    for (;;) {
        if (!loopCond_00464f80(bl))
            goto next_bl;
        Player* pi;
        // Array form, not through pi: gives the original's load-then-lea order.
        if (g_game->players[bl].active == 0)
            goto next_bl;
        pi = &g_game->players[bl];

        {
            unsigned char t = pi->type;
            if (t != 1 && t != 2 && t != 3)
                goto next_bl;
        }
        if (pi->index == 0xa)
            goto next_bl;
        {
            // Fresh pointer for the whole second group: stops it being merged with
            // the first.
            Player* pi2 = &g_game->players[bl];
            if (pi2->active == 0)
                goto next_bl;
            unsigned char t2 = pi2->type;
            if (t2 != 1 && t2 != 2 && t2 != 3)
                goto next_bl;
            if (pi2->index == 0xa)
                goto next_bl;
        }

        if (pi->ai != 0)
            g_seams.ai->SquadManagerTickIfActive(pi->ai, 0);

        g_seams.ai->UpdatePlayerAI(bl);

        {
            Unit* u = pi->unitsBegin;
            while (u <= pi->unitsEnd) {
                if (u->flags & 0x10000000)
                    g_seams.los_radar->UpdateUnitLineOfSight(u);
                u = (Unit*)((char*)u + 0x118);
            }
        }

        if (bl == g_game->playerIndex)
            g_seams.los_radar->DrawRadarUnits();

        if ((unsigned int)pi->updateTime > (unsigned int)g_game->ticks)
            goto next_bl;
        pi->updateTime += 0x1e;

        if (bl == g_game->localPlayer) {
            if (g_seams.map->GetGameType(g_game->net) == 1) {
                if (g_game->list_391ed->CheckVictory() == 0) {
                    if (g_game->list_391ed->CheckDefeat(stream) != 0) {
                        if (g_game->endGameCountdown < 0) {
                            g_game->endGameCountdown = 4;
                        } else {
                            g_game->endGameCountdown--;
                            if (g_game->endGameCountdown < 0) {
                                g_game->endGameFlags |= 4;
                                g_game->endGameFlags &= 0xffef;
                                g_game->bits.flag6 = 1;
                            }
                        }
                    }
                } else {
                    // This is a second copy of the countdown_extra block, and
                    // the duplication is load-bearing: with a `goto` here MSVC
                    // 5 leaves the `mov eax,[g_game]` reload after the `jne`
                    // and the jump lands on it, where the original hoists the
                    // reload above the branch and jumps past it. Written out
                    // twice, MSVC tail-merges the copies and hoists it.
                    if (g_game->endGameCountdown < 0) {
                        g_game->endGameCountdown = 4;
                    } else {
                        g_game->endGameCountdown--;
                        if (g_game->endGameCountdown < 0) {
                            g_game->endGameFlags |= 4;
                            g_game->bits.flag4 = 1;
                            g_game->bits.flag5 = 1;
                        }
                    }
                    goto skip508;
                }
            } else if ((pi->active == 0 ||
                        (pi->info->flags_9b & 0x40) == 0) &&
                       g_game->list_391ed->CheckDefeat(stream) != 0) {
                if (g_game->endGameCountdown < 0) {
                    g_game->endGameCountdown = 4;
                } else {
                    g_game->endGameCountdown--;
                    if (g_game->endGameCountdown < 0) {
                        if (g_game->commanderDeath == 2) {
                            PlayerInfo* self =
                                g_game->players[FindHostSlot()].info;
                            // `unsigned int` with the 0xffff mask: avoids a spilled raw result.
                            unsigned int typeId;
                            typeId = g_seams.units->FindUnitTypeId(
                                &g_game->startPos[0x232 *
                                    g_game->players[g_game->localPlayer].info->side]) & 0xffff;
                            int bound = 9999;
                            int typeOff = typeId * 0x249;
                            Vec3 pos;
                            do {
                                int cx = g_game->baseX / 10;
                                int cy = g_game->baseY / 10;
                                pos.x = (RandomInt(g_game->baseX - 2 * cx) + cx) << 16;
                                pos.y = 0;
                                pos.z = (RandomInt(g_game->baseY - 2 * cy) + cy) << 16;
                                // Declared in the order hh, hits, zacc, outer, hw: places the
                                // `shl` where the original has it.
                                int hh = g_game->height << 16;
                                int hits = 0;
                                unsigned int zacc =
                                    (unsigned int)pos.z - (unsigned int)hh;
                                int outer = 3;
                                int hw = g_game->width << 16;
                                do {
                                    unsigned int xacc =
                                        (unsigned int)pos.x - (unsigned int)hw;
                                    int inner = 3;
                                    Point16 cell;
                                    cell.y = zacc >> 20;
                                    do {
                                        cell.x = xacc >> 20;
                                        if (g_seams.map->CanPlaceUnitFootprint(
                                                (UnitDef*)((char*)g_game->defs + typeOff),
                                                0, cell, 1) != 0)
                                            hits++;
                                        xacc += hw;
                                    } while (--inner != 0);
                                    zacc += hh;
                                } while (--outer != 0);
                                if (hits >= 9 && (short)g_seams.weapons->FindFeatureAtPos(&pos, 0, 0) == -1) {
                                    if (g_game->net->lavaWorld == 0)
                                        break;
                                    if (g_seams.map->GetCellMeanHeight(&pos) >
                                        (int)g_game->seaLevel)
                                        break;
                                }
                            } while (--bound > 0);

                            {
                                Unit* unit = g_seams.units->CreateUnit(
                                    g_game->localPlayer, typeId, pos, 1, 1, 0);
                                SetStartingStorageBonus(pi,
                                             self->metal * 100,
                                             self->energy * 100);
                                {
                                    // The slot is written through a local
                                    // pointer because that is what makes MSVC 5
                                    // re-read unit->owner in the next block:
                                    // it cannot prove the store disjoint from
                                    // it. Written as `unit->resourceSlot = f` the
                                    // pointer is forwarded from the first block
                                    // instead and the second `mov eax,
                                    // [esi+0xec]` disappears.
                                    float* slot = &unit->energy;
                                    float f = (float)self->energy * kHundred;
                                    if (unit->owner->active != 0 &&
                                        unit->owner->type == 2) {
                                        switch (g_game->difficulty) {
                                        case 0: f = *slot - f * kNegHalf; break;
                                        case 1: f = *slot - f * kNegSeven; break;
                                        default: f = *slot + f; break;
                                        }
                                    } else {
                                        f = *slot + f;
                                    }
                                    *slot = f;
                                }
                                {
                                    float* slot = &unit->metal;
                                    float f = (float)self->metal * kHundred;
                                    if (unit->owner->active != 0 &&
                                        unit->owner->type == 2) {
                                        switch (g_game->difficulty) {
                                        case 0: f = *slot - f * kNegHalf; break;
                                        case 1: f = *slot - f * kNegSeven; break;
                                        default: f = *slot + f; break;
                                        }
                                    } else {
                                        f = *slot + f;
                                    }
                                    *slot = f;
                                }
                                g_seams.los_radar->RecalculateLineOfSight(1);
                                FocusCommander(1);
                            }
                        } else {
                            goto watch_check;
                        }
                    }
                }
            } else {
                goto check230;
            }
        }

    skip508:
        if (pi->active != 0) {
            unsigned char t = pi->type;
            if ((t == 1 || t == 2 || t == 3) && pi->index != 0xa) {
                if ((pi->unitCount != 0 || pi->unitsCreated == 0) &&
                    (t == 1 || t == 2)) {
                    if ((g_game->endGameFlags & 4) == 0 &&
                        g_game->endGameCountdown < 0) {
                        g_seams.economy->UpdatePlayerEconomy(pi);
                    }
                }
            }
        }

        if (bl == g_game->playerIndex) {
            g_seams.los_radar->UpdateSensorRadarAndCloak();
            g_seams.los_radar->UpdateRadarMapped();
            if (g_seams.map->GetGameType(g_game->net) == 3) {
                g_economyPacketTick++;
                if ((g_economyPacketTick & 3) == 0)
                    SendPlayerEconomy(pi, 0, 0);
            }
        }
        goto next_bl;

    watch_check:
        if (g_seams.map->GetGameType(g_game->net) == 3 &&
            pi->rejectReason == 0) {
            if ((g_game->players[FindHostSlot()].info->flags_9b & 0x80) != 0 ||
                CountActiveAIPlayers() > 0) {
                pi->info->bit6 = 1;
                if (bl == g_game->localPlayer) {
                    g_game->mapFlags &= 0xfffe;
                    g_game->mapFlags &= 0xfffd;
                    g_seams.los_radar->RecalculateLineOfSight(1);
                    BroadcastPlayerInfo();
                    if (CountActiveAIPlayers() == 0) {
                        Screen* dlg = LoadGuiLayer(&g_game->gui_519, "YESORNO.GUI", 0x900);
                        if (dlg != 0) {
                            SetKeyboardInput(&g_game->gui_519, 1);
                            Form* w = dlg->form;
                            SetTranslatedTextByName(&g_game->gui_519, "CHOICE1", "Yes", 0);
                            SetTranslatedTextByName(&g_game->gui_519, "CHOICE2", "No", 0);
                            SetTranslatedTextByName(&g_game->gui_519, "TITLE",
                                         "You're out!  Continue Watching?", 0);
                            strcpy(w->choice1, "CHOICE1");
                            strcpy(w->choice2, "CHOICE2");
                            dlg->callback = ContinueWatchingCallback;
                            RenderLayer(&g_game->gui_519, 0x40);
                        }
                        goto skip508;
                    }
                    if (CountCombatPlayers() <= 0)
                        goto skip508;
                    OpenMessageBox(&g_game->gui_519,
                                 Translate("You are placed in watch mode because you are hosting AI players which are still alive.  If you exit, they will be terminated."),
                                 500, 1, 1);
                    g_game->endGameFlags &= 0xffef;
                    goto skip508;
                }
                goto skip508;
            }
        }

    flags82e:
        g_game->endGameFlags |= 4;
        g_game->endGameFlags &= 0xffef;
        if (pi->rejectReason == 0)
            g_game->bits.flag6 = 1;
        goto skip508;

    check230:
        if (g_game->list_391ed->CheckVictory() != 0)
            goto countdown_extra;
        goto skip508;

    countdown_extra:
        if (g_game->endGameCountdown < 0) {
            g_game->endGameCountdown = 4;
        } else {
            g_game->endGameCountdown--;
            if (g_game->endGameCountdown < 0) {
                g_game->endGameFlags |= 4;
                g_game->bits.flag4 = 1;
                g_game->bits.flag5 = 1;
            }
        }
        goto skip508;

    next_bl:
        if (!more_00464f80(++bl))
            break;
    }

    if (g_seams.map->GetGameType(g_game->net) == 3 &&
        g_game->commanderDeath != 2 &&
        CountCombatPlayers() == 0) {
        if (g_game->endGameCountdown < 0) {
            g_game->endGameCountdown = 4;
            return;
        }
        g_game->endGameCountdown--;
        if (g_game->endGameCountdown < 0) {
            g_game->endGameFlags |= 4;
            g_game->endGameFlags &= 0xffef;
            g_game->bits.flag6 = 1;
        }
    }
}

// 0x4658e0 (players_4658e0.cpp) and 0x465ac0 (players_465ac0.cpp) stay in
// files of their own: each matches only with that file's header set and
// symbol count.

// Sets each player's two camera floats (player +0x8c and +0x98) from the game
// mode: mode 1 copies the net player's stored floats, mode 2 the per-slot
// integers converted to float, mode 3 the colour-index player's info values
// times 100 (read from players[index] but stored into players[i], as the
// original does, so a player with a colour index other than its own slot
// gets the other player's view position).
// FUNCTION: 0x465e30
void InitPlayerResources()
{
    for (int i = 0; i < 10; i++) {
        Player* player = &g_game->players[i];
        if (g_game->pendingSaveStore == 0) {
            switch (g_seams.map->GetGameType(g_game->net)) {
            case 1:
                SetStartingStorageBonus(player, (int)g_game->net->startMetal[i],
                             (int)g_game->net->startEnergy[i]);
                player->energy = g_game->net->startEnergy[i];
                player->metal = g_game->net->startMetal[i];
                break;
            case 2:
                player->energy = (float)((SkirmishPlayerSlot*)g_game->options)[i].energy;
                player->metal = (float)((SkirmishPlayerSlot*)g_game->options)[i].metal;
                break;
            case 3: {
                int index = FindHostSlot();
                if (index == 10)
                    index = i;
                Player* other = &g_game->players[index];
                player->energy = other->info->energy * 100;
                player->metal = other->info->metal * 100;
                break;
            }
            }
        }
    }
}

#include "../util/hapi_bank.h"

// Reads each player's "Controller" value from its "Player%i" section.
// FUNCTION: 0x465fb0
void __stdcall LoadPlayerControllers(HapiBank* file)
{
    char name[16];
    for (int i = 0; i < 10; i++) {
        Player* player = &g_game->players[i];
        int* slot = &((SkirmishPlayerSlot*)g_game->options)[i].controller;
        sprintf(name, "Player%i", i);
        if (file->OpenAccount(name)) {
            *slot = file->GetIntegerItem("Controller", 0);
            player->type = *slot;
        } else {
            *slot = 0;
            player->type = 0;
        }
    }
}

// FUNCTION: 0x466580
void UpdateBlink()
{
    if (g_game->blinkTimer > 0) {
        g_game->blinkTimer--;
        return;
    }
    g_game->blinkTimer = 7;
    g_game->bits_142f1.blinkOn = !g_game->bits_142f1.blinkOn;
}

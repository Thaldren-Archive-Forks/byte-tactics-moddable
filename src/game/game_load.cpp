// Decompiled by deepseek-v4.1-flash, GPT-6.1-sol, deepseek-v4.1, Claude Fable 5.1,
// GPT-6 Astra, space-bunny-free, Claude Opus 5.5, GPT-6, claude-opus-5-5 and Opus.
// Names are provisional.
//
// One Game from the four views (the menu object at +0x519 and the byte and word
// forms of the loading flags at +0x38d75 sit in anonymous unions, as do the
// player records, the loaded/slots block and the map rectangles), one Mission,
// one PacketManager and the module's classes.
//
// <memory.h> stays before <windows.h>: it fixes the map-name block's x87 schedule.
#include <memory.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <time.h>
#include "../../interfaces/host.h"
#include "../state/game_rand.h"
// Needed: without it the x87 schedule of the map-name block differs.
#include <ddraw.h>

#pragma pack(push, 1)

#include "game.h"

union Fixed_497180 {
    int i;                              // 16.16
    struct {
        short frac;
        short whole;
    } h;
};

struct FixedPos_497180 {
    Fixed_497180 x;
    Fixed_497180 y;
    Fixed_497180 z;
};

struct RecFlag_497180 {
    unsigned short started : 1;
    unsigned short : 15;
};

struct Display_00497f40 {
    char unknown_0[0x40];
    HWND hwnd;                          // +0x40
};

struct Sub_497180 {
    char unknown_0[0x10];
};

struct Gadget_497180 {
    char unknown_0[0x8];
    void (__stdcall* handler)(Gadget_497180*);   // +0x8
    char* owner;                                 // +0xc
};


#include "../network/player_info.h"

// Unused here: real functions declared to keep the file's symbol count (docs/c2-regalloc.md).
int CheckDirectXVersion(int, int, int, int, int);
void EnumPlayersCallback(int, int, int, int, int);
int AimCobStub(int, int, int, int);
void EmitBubbles(int, int, int, short);
void DrawLine(int, int, int, int, int, int);



// The ten player records at g_game+0x1b63 are 0x14b bytes. Three functions view
// them under different names; the fields that disagree over the same bytes sit
// in anonymous unions.
struct PlayerRec_00497f40 {             // 0x14b bytes, array at g_game+0x1b63
    union {
        int present;                    // +0x0
        int active;
    };
    unsigned int id;                    // +0x4
    char unknown_8[0x20 - 0x8];
    unsigned char percent;              // +0x20
    unsigned char keepaliveFlags;       // +0x21
    char unknown_22[0x27 - 0x22];
    PlayerInfo* data;                   // +0x27
    char name[0x73 - 0x2b];             // +0x2b
    union {
        unsigned char team;             // +0x73
        unsigned char control;
    };
    char unknown_74[0xdc - 0x74];
    float size1;                        // +0xdc
    float size2;                        // +0xe0
    char unknown_e4[0x146 - 0xe4];
    unsigned char kind;                 // +0x146
    unsigned char startPos;             // +0x147
    char unknown_148[0x149 - 0x148];
    RecFlag_497180 flags_149;           // +0x149
};

typedef PlayerRec_00497f40 PlayerInfo_00497f40;
typedef PlayerRec_00497f40 PlayerRec_00497ce0;

struct Point_00498cd0 {
    int x;                              // +0x0
    int y;                              // +0x4
};

struct View_00498cd0 {
    char unknown_0[0x2c];
    int x;                              // +0x2c
    int y;                              // +0x30
};

struct Fixed_00498d00 {
    unsigned short frac;                // +0x0
    short whole;                        // +0x2
};

struct Pos_00498d00 {
    Fixed_00498d00 x;                   // +0x0
    Fixed_00498d00 y;                   // +0x4
    Fixed_00498d00 z;                   // +0x8
};

struct Rect_00498d00 {
    int x;                              // +0x0
    int y;                              // +0x4
    int unknown_8[4];
};

struct Pos_00498da0 {
    unsigned int x;                     // +0x0
    unsigned int y;                     // +0x4
    unsigned int z;                     // +0x8
};

#include "../graphics/rect.h"

struct Point_00498da0 {
    short x;                            // +0x0
    short y;                            // +0x2
};

struct BitFlags_00498da0 {
    unsigned char b0:1;
    unsigned char b1:1;
    unsigned char b2:1;
    unsigned char b3:1;
    unsigned char b4:1;
    unsigned char b5:1;
    unsigned char b6:1;
    unsigned char b7:1;
};

union Flags_00498da0 {
    unsigned char value;
    BitFlags_00498da0 bits;
};

struct View_00498da0 {
    int x;                              // +0x0
    int y;                              // +0x4
    char unknown_8[0x18 - 0x8];
};

#include "../map/cell.h"
#include "../util/vec3.h"

struct Slot_00497180 {                  // 0x18 bytes
    int kind;                           // +0x0, 1 or 2 when the slot is in use
    char unknown_4[0x18 - 0x4];
};

struct Options_00497180 {               // at g_game+0x29a0
    Slot_00497180 slots[10];            // +0x0
    char unknown_f0[0x108 - 0xf0];
    Settings_00496e10 settings;         // +0x108
    int fixedloc;                       // +0x118
};


// One view of the game state. The ranges two views name differently sit in
// anonymous unions, so each function keeps the names it matched with.
class Sound;
#include "../util/hapi_bank.h"


#pragma pack(pop)

extern Game* g_game;

extern unsigned char g_scorePanelKillFlash[10];
extern unsigned char g_scorePanelLossFlash[10];
extern "C" int g_loadingBarFlashAlpha;
extern "C" int DAT_0051e6cc;
extern "C" int g_loadingBarFlashDecayTick;
extern int g_usePacketManager;
extern "C" unsigned char g_loadingBarPrevPercent, DAT_0051e821, DAT_0051e822;
extern "C" unsigned char DAT_0051e823, DAT_0051e824, DAT_0051e825;
extern int g_nonCampaignGame;

#include "../map/mission.h"

#include "../network/packet_manager.h"

extern PacketManager g_packetManager;

#include "../sound/sound.h"
// Unused here: the symbol ids these declarations take keep the allocation of LoadMatch,
// LoadingScreenFrame and OffsetWorldPosFromView (docs/c2-regalloc.md).
int GetHostDpid();
void RegisterUnitOrders(void);
void RegisterGroundOrders(void);
void EnableAICommands(void);
void RegisterAICommands(void);
void ResetAIPlayers(void);
void RegisterVtolOrders(void);
void StepAllGafSequences(void);
void ResetNetStats(void);
void InitCommands(void);
void RefreshSelectionOrders(void);
void DispatchOrdersPanelPageFlags(void);
void ResetCameraState(void);
void FindLocalCommander(void);
void ClampCameraPosition(void);
void ClampCameraTarget(void);
void UpdateScreenShake(void);
void UpdateCameraFollow(void);
void BeginMouseScroll(void);
void EndMouseScroll(void);
void UpdateMouseScroll(void);
void UpdateEdgeScroll(void);
void CenterCameraOnRadarClick(void);
void RegisterDataArchives(void);
void InitMissionStatus(void);
void ScheduleFadeTick(void);
void InitExplosions(void);
void FreeExplosions(void);
void UpdateExplosions(void);

void __stdcall SeedRandom(int x);
unsigned HarnessGameSeed(unsigned live);
unsigned HarnessCrtSeed(unsigned live);
int __stdcall RandomInt(int x);
unsigned char __stdcall FindHostSlot();
void ApplyUseOnlyUnits();
void HandleNetPackets();
void ApplySlotsToGamePlayers();
void LoadBattleAssets();
void __stdcall LoadSavedGameState(void* mission);
void CreateMissionUnits();
void CenterCameraOnStartPosition();
void __stdcall LoadPictureCached(const char* name, int a, int b, int c);
void BroadcastPlayerInfo();
void UpdateNetGameInfo();
void __stdcall ReportGameEvent(int x);
void __stdcall SetCameraPosition(int x, int y, int z);
void __stdcall SpawnCommanderAtStartPos(int team, int startpos);
void __stdcall CreateUnit(unsigned char team, unsigned short id, FixedPos_497180 pos, int a,
    int b, int c);
Gadget_497180* __stdcall LoadGuiLayer(Sub_497180* sub, const char* name, int flags);
void __stdcall HandleMain2LayoutEvent(Gadget_497180* gadget);
void __cdecl operator delete(void* p);

void __stdcall RenderLayer(Menu_00497ce0* menu, int value);
void __stdcall MarkLayerChanged(Menu_00497ce0* menu);
void __stdcall BlitMenuLayers(Menu_00497ce0* menu, int a, int b);
void __stdcall FillRectangle(void* surface, void* rect, int color);
void __stdcall DrawTextClipped(void* surface, const char* text, int x, int y, int len, int flag);
char* __stdcall Translate(char* s);

void LoadThreadMain();
void BattleFrame();
void __cdecl HandleBattleQuitPrompt(int);
void __cdecl BlankScreen();
void __cdecl FreePictureCache();
void __cdecl SaveSettings();
void __cdecl SendLoadProgress();
void __cdecl OnlineUnload();
void __cdecl DrawLightBars();
void __cdecl StopAllSounds();
void __cdecl MainLoopTick();
void __cdecl ShowSoftwareCursor();
void __stdcall UnlockScreen(void*);
void __cdecl RestoreScreen();
void __cdecl FlipScreen();
int __cdecl IsCdPlaying();
void __cdecl GameFreeThunk(void*);
void __stdcall BuildDataPath(void*, char*, char*, char*);
void __stdcall SendProbe(unsigned int, int);
void __stdcall DrawSyncStatus(void*);
void __stdcall DisableKeyCommands(void*);
int __stdcall GetTextPixelWidth(char*);
void __stdcall CloseTopScreen(void*);
void __stdcall SetCursorAnimation(void*, void*);
void __stdcall RemapPaletteToClosestIndices(void*, void*, void*);
void __stdcall SetCloseHandler(void (__cdecl *)(int), int);
void __stdcall SetResolution(int, int);
void __stdcall FatalError(char*);
void* __stdcall GetGafFrame(void*, int);
void __stdcall DrawFrame(void*, void*, int, int);
void* __stdcall FindGafEntry(int, char*);
void __stdcall SetPaletteColors(void*, int, int);
void* __stdcall HAPI_LoadFile(void*, unsigned int*);
void __stdcall SetTextColors(int, int);
void __stdcall SetFont(int);
int __stdcall LockScreen(void*);
void __stdcall SetRestoreSurface(int);
void __stdcall SetOffscreenSurface(void*);
void* __stdcall AllocSurface(char*, int, int);
void __stdcall DrawSurface(void*, void*, int, int);
void __stdcall HAPINET_guaranteepackets(int);
int __cdecl GetScreenWidth();
int __cdecl GetScreenHeight();
int __cdecl GetTextKeyColor();
int __cdecl GetFontHeight();
int __cdecl AssignStartPositions();
unsigned int __cdecl GetTicks();
char* __cdecl GetPreferredLanguage();

#include "../../interfaces/seams.h"
int __stdcall PointInRect(Rect* r, int x, int y);


// Game start: seeds the random generators, loads the match settings for the
// current network mode (1 = skirmish defaults, 2 = multiplayer host block,
// 3 = joined game, from the host player's record), runs the between-missions
// summary, spawns each player's commander and opens the MAIN2 GUI.
//
// Kept as written, each for the registers it gives:
// - Cases 1 and 2 are inlined calls of ApplyMissionOptionFlags (matched on its own in
//   0x496e10.cpp, zero callers in the exe because /Ob2 inlined every call):
//   it copies the settings block's first int to +0x37ef6 and three int flags
//   into bits 2, 0 and 1 of the view-flags word at +0x14281. The inlined
//   `int -> 1-bit field` assignments are what mask with the CSE'd constant 1
//   (`mov edi,1` at the switch head, `and ebx,edi`), while the hand-written
//   `(w & ~M) | (b & M)` lanes narrow to `and bl,1; movzx si,bl`.
// - Case 3 and the post-wait block copy BITFIELDS of the player's
//   `unsigned short` flags at +0x9b: bits 8, 9 and 10 sit in the byte at
//   +0x9c, and `view->bit1 = pf->b9` (same bit position on both sides) is what
//   MSVC 5 compiles to the zero-extended byte load plus `and ebx,2` with no
//   shift; the 2-bit field at bits 11-12 and the 1-bit field at 13 give the
//   `shr ecx,0xb; and ecx,3` and `shr ecx,0xd; and ecx,1` extracts.
// - Case 2's statement order is copy 0x37eec -> 0x37ee6 first, then
//   g_nonCampaignGame = 1, then the settings block; the store through g_game makes
//   MSVC reload g_game for the block, as the original does.
// - The commander spawn loop is `for (int i = 0; i < 10; i++)` indexing
//   0x14b-byte records; MSVC strength-reduces it to the byte offset in ebx and
//   that puts g_game in the SIB base. A hand-written `off += 0x14b` loop swaps
//   the base and index.
// Also kept: the loading flags at +0x38d75 written as volatile
// (the field the guide names); the +0x9b bit-6 test as a 1-bit bitfield; the
// mission-count loop with `i++` before `def += 6`; the ten-player walks with
// `(unsigned char)i` so the multiply is not strength-reduced; the
// std::random_shuffle call; rec+0x149 as a 1-bit bitfield (direct `or byte`);
// `pos.x.i` before `pos.y.i = 0`.
// Inlined into cases 1 and 2 below (matched on its own in 0x496e10.cpp).
inline void __stdcall ApplyMissionOptionFlags(Settings_00496e10* s)
{
    g_game->commanderDeath = s->value;
    g_game->mapFlags_14281.bit2 = s->flag_c;
    g_game->mapFlags_14281.bit0 = s->flag_4;
    g_game->mapFlags_14281.bit1 = s->flag_8;
}

// FUNCTION: 0x497180
void __cdecl LoadMatch(void*)
{
    unsigned long long perfCount;
    FixedPos_497180 pos;
    FixedPos_497180 start;
    int order[10];

    perfCount = host_time_counter();
    SeedRandom(HarnessGameSeed((unsigned int)perfCount + (unsigned int)(perfCount >> 32)));
    GameSrand(RAND_STREAM_LOAD, HarnessCrtSeed((unsigned)time(NULL)));
    g_game->ticks = 0;

    // Six of these casts stay: taking more than two out in this function moves
    // the symbol state that LoadingScreenFrame's allocation depends on
    // (docs/c2-regalloc.md).
    switch (g_seams.map->GetGameType(((Mission*)g_game->net))) {
    case 1:
        g_nonCampaignGame = 0;
        ApplyMissionOptionFlags(&g_game->singleSettings);
        ApplyUseOnlyUnits();
        break;
    case 2:
        g_game->field_37ee6 = g_game->maxUnits_37eec;
        g_nonCampaignGame = 1;
        ApplyMissionOptionFlags(&((Options_00497180*)g_game->options)->settings);
        break;
    case 3: {
        g_game->field_37ee6 = g_game->maxUnits_37eec;
        g_nonCampaignGame = 1;
        g_game->otherFlags &= 0xfffe;

        int sel = FindHostSlot();
        unsigned char cur = g_game->localPlayer;
        if ((*(PlayerRec_00497f40(*)[10])&g_game->players)[cur].keepaliveFlags & 2) {
            do {
                PlayerInfo* p = (*(PlayerRec_00497f40(*)[10])&g_game->players)[g_game->localPlayer].data;
                if (g_usePacketManager)
                    g_packetManager.SendAllQueued(1);
                HandleNetPackets();
                sel = FindHostSlot();
                host_sleep_ms(0x32);
                if (sel == 10)
                    continue;
                if (p->color == 0xff)
                    continue;
                if (p->unknown_8f == 0)
                    continue;
                break;
            } while (1);
            host_sleep_ms(0x32);
        }

        g_seams.map->LoadMissionByName(((Mission*)g_game->net), 0, (char*)(*(PlayerRec_00497f40(*)[10])&g_game->players)[sel].data);
        if (FindHostSlot() == 10)
            break;

        int sel2 = FindHostSlot();
        PlayerInfo* p2 = (*(PlayerRec_00497f40(*)[10])&g_game->players)[sel2].data;
        g_nonCampaignGame = p2->cheating;
        g_game->commanderDeath = p2->commander;
        g_game->mapFlags_14281.bit1 = p2->los;
        g_game->mapFlags_14281.bit2 = p2->losType;
        g_game->mapFlags_14281.bit0 = p2->mapping;
        g_game->field_37ee6 = p2->maxUnits;
        break;
    }
    default:
        break;
    }

    if (((HapiBank*)g_game->pendingSaveStore) != 0) {
        ((HapiBank*)g_game->pendingSaveStore)->OpenAccount("summary");
        if (((HapiBank*)g_game->pendingSaveStore)->HasItem("BetweenMissions") == 0) {
            g_seams.players->LoadPlayerControllers(((HapiBank*)g_game->pendingSaveStore));
            if (g_seams.map->GetGameType(((Mission*)g_game->net)) == 2) {
                int count = 0;
                Slot_00497180* def = ((Options_00497180*)g_game->options)->slots;
                int i = 0;
                while (i < 10) {
                    if (def->kind == 1 || def->kind == 2)
                        count = i + 1;
                    i++;
                    def++;
                }
                int cur = g_game->playerCount;
                if (count > cur)
                    cur = count;
                g_game->playerCount = cur;
                ApplySlotsToGamePlayers();
            }
        }
    }

    LoadBattleAssets();

    if (g_seams.map->GetGameType(g_game->net) != 1) {
        if (g_seams.map->GetGameType(g_game->net) == 3) {
            (*(volatile union LoadFlags_00497f40*)&g_game->loadingFlags).value |= 4;
            while (((*(volatile union LoadFlags_00497f40*)&g_game->loadingFlags).value & 8) == 0)
                host_sleep_ms(0x32);

            int sel = FindHostSlot();
            PlayerInfo* pl = (*(PlayerRec_00497f40(*)[10])&g_game->players)[sel].data;
            g_game->mapFlags_14281.bit0 = pl->mapping;
            g_game->mapFlags_14281.bit1 = pl->los;
            g_game->mapFlags_14281.bit2 = pl->losType;
            g_game->commanderDeath = pl->commander;

            for (int i = 0; i < 10; i++) {
                PlayerRec_00497f40* rec = &(*(PlayerRec_00497f40(*)[10])&g_game->players)[i];
                if (rec->active == 0)
                    continue;
                unsigned char st = rec->team;
                if (st != 1 && st != 2)
                    continue;
                pos.x.i = (RandomInt(g_game->baseX - 0xa0) + 0x50) << 16;
                pos.y.i = 0;
                pos.z.i = (RandomInt(g_game->baseY - 0xa0) + 0x50) << 16;
                if (rec->active != 0 && (rec->data->flags_9b & 0x40))
                    continue;
                PlayerInfo* pl2 = rec->data;
                int side = pl2->side;
                int which = rec->startPos;
                g_seams.map->GetStartPosition(((Mission*)g_game->net), 0, (Vec3*)&pos, which);
                if (rec->active != 0 && rec->team == 1)
                    start = pos;
                unsigned short id = g_seams.units->FindUnitTypeId(g_game->sideNames[side].name + 4);
                g_seams.units->CreateUnit(rec->kind, id, *(Vec3*)&pos, 1, 1, 0);
                int s1 = pl->energy * 100;
                int s2 = pl->metal * 100;
                rec->flags_149.started = 1;
                rec->size1 = (float)(s1 >= 200 ? s1 : 200);
                rec->size2 = (float)(s2 >= 200 ? s2 : 200);
            }

            unsigned char li = g_game->localPlayer;
            PlayerInfo* lp = (*(PlayerRec_00497f40(*)[10])&g_game->players)[li].data;
            int cx;
            int cz;
            if (lp->bit6) {
                g_game->mapFlags &= 0xfffe;
                g_game->mapFlags &= 0xfffd;
                cx = g_game->viewWidth / 2;
                cz = g_game->viewHeight / 2;
            } else {
                cx = start.x.h.whole - g_game->viewWidth / 2;
                cz = start.z.h.whole - g_game->viewHeight / 2;
            }
            SetCameraPosition(cx, cz, 0);
            ReportGameEvent(6);
        } else if (g_seams.map->GetGameType(((Mission*)g_game->net)) == 2 &&
            ((HapiBank*)g_game->pendingSaveStore) == 0) {
            if (((Options_00497180*)g_game->options)->fixedloc != 0) {
                for (int i1 = 0; i1 < 10; i1++) {
                    if ((unsigned char)i1 < 10) {
                        PlayerRec_00497f40* rec = &(*(PlayerRec_00497f40(*)[10])&g_game->players)[(unsigned char)i1];
                        if (rec->active != 0) {
                            unsigned char st = rec->team;
                            if ((st == 1 || st == 2 || st == 3) &&
                                rec->kind != 10)
                                SpawnCommanderAtStartPos(i1, i1);
                        }
                    }
                }
            } else {
                for (int i2 = 0; i2 < 10; i2++)
                    order[i2] = -1;
                int n = 0;
                for (int i3 = 0; i3 < 10; i3++) {
                    if ((unsigned char)i3 < 10) {
                        PlayerRec_00497f40* rec = &(*(PlayerRec_00497f40(*)[10])&g_game->players)[(unsigned char)i3];
                        if (rec->active != 0) {
                            unsigned char st = rec->team;
                            if ((st == 1 || st == 2 || st == 3) &&
                                rec->kind != 10)
                                order[n++] = i3;
                        }
                    }
                }
                if (n > 2 || (int)(((__int64)GameRand(RAND_STREAM_LOAD) * 2) / 0x8000) != 0) {
                    GameShuffleDraw draw(RAND_STREAM_LOAD);
                    std::random_shuffle(order, order + n, draw);
                }
                int k = 0;
                for (int i4 = 0; i4 < 10; i4++) {
                    if ((unsigned char)i4 < 10) {
                        PlayerRec_00497f40* rec = &(*(PlayerRec_00497f40(*)[10])&g_game->players)[(unsigned char)i4];
                        if (rec->active != 0) {
                            unsigned char st = rec->team;
                            if ((st == 1 || st == 2 || st == 3) &&
                                rec->kind != 10)
                                SpawnCommanderAtStartPos(i4, order[k++]);
                        }
                    }
                }
            }
            g_seams.players->InitPlayerResources();
        }
    }

    g_seams.los_radar->RecalculateLineOfSight(1);

    if (((HapiBank*)g_game->pendingSaveStore) != 0) {
        ((HapiBank*)g_game->pendingSaveStore)->OpenAccount("summary");
        if (((HapiBank*)g_game->pendingSaveStore)->HasItem("BetweenMissions") == 0) {
            LoadSavedGameState(((HapiBank*)g_game->pendingSaveStore));
            goto tail;
        }
    } else if (g_seams.map->GetGameType(((Mission*)g_game->net)) != 1) {
        goto tail;
    }
    CreateMissionUnits();
    CenterCameraOnStartPosition();

tail:
    {
        int pnum = g_game->playerIndex;
        PlayerRec_00497f40* rec = &(*(PlayerRec_00497f40(*)[10])&g_game->players)[pnum];
        LoadPictureCached(0, 0, 0, 0);
        PlayerInfo* pl = rec->data;
        int side = pl->side;
        sprintf(g_game->name, "%sMAIN2.GUI", g_game->sideNames[side].name);
    }
    Gadget_497180* gadget =
        LoadGuiLayer((Sub_497180*)&g_game->menu_519_18, g_game->name, 0x20);
    gadget->handler = HandleMain2LayoutEvent;
    gadget->owner = (char*)g_game;

    PlayerRec_00497f40* currec = &(*(PlayerRec_00497f40(*)[10])&g_game->players)[g_game->localPlayer];
    currec->data->flags_9b |= 0x10;
    BroadcastPlayerInfo();
    UpdateNetGameInfo();
    g_seams.players->UpdatePlayersOnStream(RAND_STREAM_LOAD);
    g_seams.players->InitPlayerResources();

    HapiBank* mission = ((HapiBank*)g_game->pendingSaveStore);
    if (mission != 0) {
        mission->CloseBank();
        operator delete(mission);
        g_game->pendingSaveStore = (int)(0);
    }
    g_seams.players->RebuildAIFeatureCells();

    (*(volatile union LoadFlags_00497f40*)&g_game->loadingFlags).value |= 2;
}
//
// Kept as written, each for the registers it gives:
// - Vec3::operator- is the explicit-component form the matched sibling 0x413d80
//   (same translation unit) uses. That makes the state 3 block byte exact, but
//   on its own it ties `range` and `order` at priority 130 (c2prio), and range
//   wins the tie on its +0x40 key, so order and range trade esi and edi.
// - `int ok = AddBuildProgress(...); if (ok)` adds a candidate to a block that
//   references order, which raises order to 134 and gives it esi again.
// - The operand order of the six bounds adds (pos.x + min.x and so on) follows
//   the symbol ids, so it moves with the headers and with code-neutral
//   spellings. What puts all six in place (found by the permuter): <memory.h>
//   plus <windows.h>, the state 0 test written as two nested ifs, and an empty
//   `do {} while (0);` in UnitRef::Get(), a debug check that compiles to
//   nothing. Without the do-while, or without the `ok` local, the registers
//   come out differently.
// Reading the record address through a named base (a temp_intro the permuter
// found) is what keeps g_game in the SIB base and the offset in the index.
static inline char* PlayerRecordAt(Game* game, int off) { return (char*)(*(PlayerRec_00497f40(*)[10])&game->players) + off; }

// FUNCTION: 0x497ce0
void __stdcall DrawSyncStatus(void* surface)
{
    int off;
    extern Game* g_game;

    RenderLayer(&g_game->menu_519_18, 0x40);
    MarkLayerChanged(&g_game->menu_519_18);
    BlitMenuLayers(&g_game->menu_519_18, 0, 0);

    const char* text;
    if (g_game->netBits.synced) {
        text = Translate("Synchronization complete");
    } else {
        int countA = 0;
        int countB = 0;
        for (int i = 0; i < 10; i++) {
            PlayerRec_00497ce0* p = &(*(PlayerRec_00497f40(*)[10])&g_game->players)[i];
            if (p->present && (p->team == 1 || p->team == 2 || p->team == 3) && p->kind != 10)
                countA++;
            if (p->present && (p->team == 1 || p->team == 2 || p->team == 3) && p->kind != 10 &&
                p->percent == 100 && g_game->loaded[i] != 0)
                countB++;
        }

        int x;
        int slot = 620 / countA;
        Rect r;
        r.left = 10;
        r.top = 420;
        r.bottom = 435;
        off = 0;
        x = 11;
        for (; off < 10 * (int)sizeof(PlayerRec_00497ce0); off += sizeof(PlayerRec_00497ce0)) {
            Game* game = g_game;
            PlayerRec_00497ce0* q = (PlayerRec_00497ce0*)PlayerRecordAt(game, off);
            if (!(q->present && (q->team == 1 || q->team == 2 || q->team == 3) && q->kind != 10))
                continue;
            {
                r.left = x;
                r.right = slot + x - 2;
                FillRectangle(surface, &r, g_game->field_dcf);
                int pc = q->percent;
                r.right = r.left + (pc * (slot - 2)) / 100;
                FillRectangle(surface, &r, g_game->field_dd5);
                DrawTextClipped(surface, q->name, r.left, 420, slot - 2, 0);
                x += slot;
            }
        }

        const char* pr;
        int ready = countB == 1;
        if (ready)
            pr = Translate("player ready");
        else
            pr = Translate("players ready");
        char buf[128];
        sprintf(buf, "%s.  %i %s", Translate("Waiting for other players"), countB, pr);
        text = buf;
    }

    DrawTextClipped(surface, text, 10, 400, -1, 0);
}
// Draws one loading bar: its label, then the filled rect and the lightbar frame.
// The flash alpha is passed as base plus slot: a pointer already offset by the
// caller changes the register choice.
static inline void DrawLoadingBar(void* gadget, void* lightbar, int index, unsigned char* prev, unsigned char* alphas, int slot, char* label, int y, int* rect)
{
    unsigned int color;
    int flash;
    color = g_game->colors[(*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[index] < 100 ? 12 : 10];
    SetTextColors(color, GetTextKeyColor());
    if ((*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[index] == 100 && *prev != 100) {
        alphas[slot] = 0x1e;
    }
    flash = alphas[slot];
    *prev = (*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[index];
    DrawTextClipped(gadget, (char*)Translate(label), 0x5a, y, -1, flash);
    // Each bar's rect is written left, right, top, bottom.
    rect[0] = 0xcd;
    rect[2] = ((int)(*(volatile unsigned char(*)[6])&g_game->progress_38d6f)[index] * 7) / 2 + 0xcd;
    rect[1] = y;
    rect[3] = y + 0x14;
    FillRectangle(gadget, rect, color);
    DrawFrame(gadget, lightbar, rect[0], rect[1]);
}
// The loading-screen frame: on the first call it starts the loader thread
// (LoadThreadMain), once the loader sets the "loaded" bit it restores the game
// screen and installs the game frame handler (BattleFrame), and otherwise it
// draws the six progress bars.
// Unused here: the symbol ids these declarations take keep the allocation
// (docs/c2-regalloc.md).
extern int Pad_497f40_e0;
// FUNCTION: 0x497f40
void LoadingScreenFrame(void)
{
    struct Surface { int width, height; char unknown_8[0x28]; };
    Surface gadget;
    char buf[128];
    char aux[256];
    void* surfaceHandle;
    int i;
    unsigned int color;
    int textWidth;
    unsigned int stamp;
    int rect[4];
    PlayerInfo_00497f40* pi;
    char namebuf[100];

    if (!(*(volatile union LoadFlags_00497f40*)&g_game->loadingFlags).bits.started) {
        while (((int)g_game->layer) != 0) {
            CloseTopScreen(&g_game->field_519);
        }
        DisableKeyCommands(&g_game->field_519);
        if (((unsigned char)g_game->cursorMode) != 0x14) {
            g_game->cursorMode = (char)(0x14);
            SetCursorAnimation(&g_game->field_519, (void*)g_game->cursorHourglass);
        }
        SetFont(g_game->fontComix);
        SetPaletteColors(g_game->paletteRgba, 0, 0x100);
        if (g_seams.map->GetGameType(g_game->net) != 2) {
            SaveSettings();
        }
        while (((int)g_game->layer) != 0) {
            CloseTopScreen(&g_game->field_519);
        }
        BlankScreen();
        g_game->width_37e1f = 0x280;
        g_game->field_37e23 = 0x1e0;
        if (GetScreenWidth() != 0x280 || GetScreenHeight() != 0x1e0) {
            GameFreeThunk((void*)g_game->screen);
            g_game->screen = 0;
            SetRestoreSurface(0);
            RestoreScreen();
            SetWindowPos(((Display_00497f40*)g_game->unknown_c)->hwnd, 0, 0, 0, 0x280, 0x1e0, 4);
            SetResolution(0x280, 0x1e0);
            g_game->screen = (int)AllocSurface("OFFSCREEN", g_game->width_37e1f, g_game->field_37e23);
            SetRestoreSurface(g_game->screen);
            SetOffscreenSurface((void*)g_game->screen);
        }
        BuildDataPath(aux, "palettes", "guipal", "PAL");
        surfaceHandle = HAPI_LoadFile((unsigned int*)aux, 0);
        RemapPaletteToClosestIndices(&g_game->field_519, g_game->paletteRgba, surfaceHandle);
        GameFreeThunk(surfaceHandle);
        g_game->lastTick = GetTicks();
        g_game->frames_38a3b = 0;
        g_game->ticks = 0;
        g_game->streak = 0;
        g_game->endGameCountdown = (short)0xffff;
        g_game->width_37e1f = g_game->width_37f1b;
        g_game->field_37e23 = g_game->height_37f1f;
        g_game->viewCullMinX = 0x80;
        g_game->viewCullMinY = 0x20;
        g_game->viewCullMaxX = g_game->width_37e1f - 1;
        g_game->viewCullMaxY = g_game->field_37e23 - 0x21;
        g_game->viewWidth = g_game->viewCullMaxX - g_game->viewCullMinX + 1;
        g_game->viewHeight = g_game->viewCullMaxY - g_game->viewCullMinY + 1;
        LoadPictureCached("loadgame2bg", 0, 0, 0);
        memset(&g_game->slots_29a4, 0, sizeof(g_game->slots_29a4));
        for (i = 0; i < 10; i++) {
            pi = &(*(PlayerRec_00497f40(*)[10])&g_game->players)[i];
            if (pi->active == 0 || (pi->control != 1 && pi->control != 2)) {
                g_game->slots_29a4.inGame[i] = 0;
            } else {
                g_game->slots_29a4.inGame[i] = 1;
            }
            g_game->slots_29a4.flag40[i] = (pi->active != 0 && (pi->data->flags_9b & 0x40) != 0) ? 1 : 0;
        }
        if (!host_thread_start((void (__cdecl*)(void*))LoadThreadMain, 0, 0)) {
            FatalError("Unable to start the loading thread!");
        }
        // Four memsets (10, 10, 6, 6 bytes); the stage bytes get one 8-byte memset.
        memset(g_scorePanelKillFlash, 0, 10);
        memset(g_scorePanelLossFlash, 0, 10);
        memset(&g_loadingBarFlashAlpha, 0, 6);
        memset(&g_loadingBarPrevPercent, 0, 6);
        (*(volatile union LoadFlags_00497f40*)&g_game->loadingFlags).bits.started = 1;
        OnlineUnload();
    }
    if ((*(volatile union LoadFlags_00497f40*)&g_game->loadingFlags).bits.loaded) {
        StopAllSounds();
        BlankScreen();
        FreePictureCache();
        if (GetScreenWidth() != g_game->width_37f1b || GetScreenHeight() != g_game->height_37f1f) {
            GameFreeThunk((void*)g_game->screen);
            g_game->screen = 0;
            SetRestoreSurface(0);
            RestoreScreen();
            SetWindowPos(((Display_00497f40*)g_game->unknown_c)->hwnd, 0, 0, 0, g_game->width_37f1b,
                         g_game->height_37f1f, 4);
            SetResolution(g_game->width_37f1b, g_game->height_37f1f);
            g_game->screen = (int)AllocSurface("OFFSCREEN", g_game->width_37e1f, g_game->field_37e23);
            SetRestoreSurface(g_game->screen);
        }
        DrawLightBars();
        MainLoopTick();
        ShowSoftwareCursor();
        g_game->mode_391f1 = 6;
        g_game->handler = BattleFrame;
        SetCloseHandler(HandleBattleQuitPrompt, 0);
        g_game->field_589 = 0;
        memset((void*)(*(volatile unsigned char(*)[6])&g_game->progress_38d6f), 0, 8);
        ((Sound*)g_game->field_10)->SetTrackCategory(0);
        if (!((Sound*)g_game->field_10)->IsCdPlaying()) {
            ((Sound*)g_game->field_10)->PlayNextTrack();
        }
        // Index players[i], not explicit offsets: keeps the SIB base and index order.
        for (i = 0; i < 10; i++) {
            // Store past rect: a byte local would be dead-store eliminated.
            if ((*(PlayerRec_00497f40(*)[10])&g_game->players)[i].active != 0)
                ((unsigned char*)rect)[19] = (*(PlayerRec_00497f40(*)[10])&g_game->players)[i].control;
        }
        return;
    }
    for (i = 0; i < 10; i++) {
        if ((*(PlayerRec_00497f40(*)[10])&g_game->players)[i].active != 0
            && ((*(PlayerRec_00497f40(*)[10])&g_game->players)[i].control == 1 || (*(PlayerRec_00497f40(*)[10])&g_game->players)[i].control == 2)) {
            SendProbe((*(PlayerRec_00497f40(*)[10])&g_game->players)[i].id, 0);
            if (g_usePacketManager != 0)
                (&g_packetManager)->SendAllQueued(1);
        }
    }
    HandleNetPackets();
    // Nested, not `b2 && AssignStartPositions()`: that folds to a test on the byte.
    if ((*(volatile union LoadFlags_00497f40*)&g_game->loadingFlags).bits.b2) {
        if (AssignStartPositions() != 0) {
            (*(volatile union LoadFlags_00497f40*)&g_game->loadingFlags).bits.b2 = 0;
            (*(volatile union LoadFlags_00497f40*)&g_game->loadingFlags).bits.b3 = 1;
            HAPINET_guaranteepackets(0);
        }
    }
    if (g_usePacketManager != 0) {
        (&g_packetManager)->SendAllQueued(1);
    }
    SetOffscreenSurface((void*)g_game->screen);
    // The result stays in a local: it gives the compare against a register.
    int ok = LockScreen(&gadget);
    if (ok != 0) {
        color = g_game->colors[15];
        stamp = GetTicks();
        if (g_loadingBarFlashDecayTick < (int)stamp) {
            g_loadingBarFlashDecayTick = GetTicks();
            for (i = 0; i < 6; i++) {
                if (((char*)&g_loadingBarFlashAlpha)[i] != 0) {
                    ((char*)&g_loadingBarFlashAlpha)[i] -= 2;
                }
            }
        }
        SetFont(g_game->fontComix);
        DrawSurface(&gadget, (void*)((int)g_game->surface), 0, 0);
        if (g_seams.map->GetGameType(g_game->net) != 1) {
            SetTextColors(color, 0xfe);
            // Local for the strncpy source: the call comes before the length push.
            char* name = g_seams.map->GetMissionName(g_game->net);
            strncpy(namebuf, name, 100);
            namebuf[99] = 0;
            if (GetPreferredLanguage() != 0 && _strcmpi((const char*)GetPreferredLanguage(), "english") != 0) {
                _strlwr(namebuf);
            }
            wsprintfA(buf, "%s: %s", (char*)Translate("Map"), (char*)Translate(namebuf));
            textWidth = GetTextPixelWidth(buf);
            {
                int x = gadget.width / 2 - textWidth / 2;
                DrawTextClipped(&gadget, buf, x,
                             (int)((double)gadget.height - (double)GetFontHeight() * 1.5), -1, 0);
            }
        }
        {
            void* light = FindGafEntry(((int)g_game->gaf), "LIGHTBAR");
            void* lightbar = GetGafFrame(light, 0);
            *((short*)lightbar + 3) = 0;
            *((short*)lightbar + 2) = 0;
            DrawLoadingBar(&gadget, lightbar, 0, &g_loadingBarPrevPercent, (unsigned char*)&g_loadingBarFlashAlpha, 0, "Textures", 0x87, rect);
            DrawLoadingBar(&gadget, lightbar, 1, &DAT_0051e821, (unsigned char*)&g_loadingBarFlashAlpha, 1, "Terrain", 0xb1, rect);
            DrawLoadingBar(&gadget, lightbar, 2, &DAT_0051e822, (unsigned char*)&g_loadingBarFlashAlpha, 2, "Units", 0xda, rect);
            DrawLoadingBar(&gadget, lightbar, 3, &DAT_0051e823, (unsigned char*)&g_loadingBarFlashAlpha, 3, "Animation", 0x106, rect);
            DrawLoadingBar(&gadget, lightbar, 4, &DAT_0051e824, (unsigned char*)&DAT_0051e6cc, 0, "3D Data", 0x130, rect);
            DrawFrame(&gadget, lightbar, rect[0] + *((short*)lightbar + 2),
                         rect[1] + *((short*)lightbar + 3));
            DrawLoadingBar(&gadget, lightbar, 5, &DAT_0051e825, (unsigned char*)&DAT_0051e6cc, 1, "Explosions", 0x15b, rect);
        }
        if (g_seams.map->GetGameType(g_game->net) == 3) {
            DrawSyncStatus(&gadget);
            SendLoadProgress();
        }
        UnlockScreen(&gadget);
        FlipScreen();
    }
    host_sleep_ms(200);
}
// Converts a point by the view origin at +0x2c/+0x30 (less a 0x80 by 0x20
// border) and passes it on to ClampWorldPosToTerrain.
// FUNCTION: 0x498cd0
void __stdcall OffsetWorldPosFromView(Point_00498cd0* p, View_00498cd0* view, int param_3)
{
    g_seams.los_radar->ClampWorldPosToTerrain((view->x - 0x80) + p->x, (view->y - 0x20) + p->y, (Vec3*)param_3);
}
// Converts the screen position stored at +0x2c76 into a 16.16 world position
// (scaled from the view origin), with the height taken from the ground there.
#include <string.h>
// FUNCTION: 0x498d00
void __stdcall MinimapCursorToWorldPos(Pos_00498d00* out)
{
    Rect_00498d00 r = (*(struct Rect_00498d00*)&g_game->view);
    int dx = r.x - g_game->posX;
    int dy = r.y - g_game->posY;
    memset(out, 0, sizeof(Pos_00498d00));
    out->x.whole = g_game->mapWidth * dx / g_game->width_142eb;
    out->z.whole = g_game->mapHeight * dy / g_game->screen_h;
    out->y.whole = g_seams.units->GetGroundHeight((Vec3*)out);
}

// Turns a screen point into a world position. The caller copies the 24-byte
// view rectangle at g_game+0x2c76 onto its own stack and passes the address, so
// this function sees a snapshot of the view.
//
// If the point is inside the rect at +0x142bb and bit 3 of the flags byte is
// clear, the screen offset from the view origin is simply scaled by world size
// over screen size. Otherwise the point is clamped to the visible limit rect at
// +0x37e27 and measured from the world origin, which lets the camera sit still
// at the edge of the map. The three low flag bits record which of the two
// happened: bit 0 direct, bit 1 inside the limit rect, bit 2 both. Then the
// point is converted to a map position, the cell it landed in is stored, and
// that cell's feature id is stored with it.
//
// Layout notes, for whoever reads the neighbours of this code:
// - the view rectangle is 0x18 bytes (int x, int y, then 16 bytes this
//   function never looks at); only x and y are read.
// - +0x2caa is the world position, three 20.12 fixed-point values. The
//   conversion back to a cell is an unsigned `>> 20` into a pair of shorts at
//   +0x2c8e, written as one 4-byte copy, and the feature id of that cell goes
//   to +0x2cbc.
// - +0x2cc6 is one byte of flags reached through a bitfield union. Writing bit
//   0 hoists the byte into bl and stores it after the arithmetic, while
//   clearing bit 1 and setting bit 2 are straight-to-memory masks. Reading the
//   two bits back for bit 2 gives the `test cl, 3` the original does.
// - +0x37e27 is the visible limit rect (left, top, right, bottom). Its address
//   is taken once and kept in a register, because the same rect is both the
//   clamp bounds and the argument of the second PointInRect call.
// - +0x1422b and +0x1422f are the world size, +0x142e7..+0x142ed the view
//   origin and screen size as shorts, +0x1431f and +0x14323 the world origin
//   the clamped point is measured from.
// - the MIN(MAX()) nesting is not decoration: the original expands the max
//   twice, once for the comparison and once for the value it keeps.
// - `Pos v = *p` with only v.x and v.z read is what leaves the store of the
//   middle word in the frame; MSVC folds the other two into their consumers.
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

// FUNCTION: 0x498da0
void __stdcall UpdateCursorWorldPos(View_00498da0* r)
{
    int mx, my;

    if (PointInRect(&g_game->viewLimit, r->x, r->y) && !((*(union Flags_00498da0*)&g_game->inputFlags_2cc6).value & 8)) {
        mx = (r->x - g_game->posX) * g_game->mapWidth / g_game->width_142eb;
        my = (r->y - g_game->posY) * g_game->mapHeight / g_game->screen_h;
        (*(union Flags_00498da0*)&g_game->inputFlags_2cc6).value |= 1;
        (*(union Flags_00498da0*)&g_game->inputFlags_2cc6).value &= ~2;
    } else {
        Rect* lim = &g_game->rect;
        mx = g_game->scrollX + MIN(MAX(r->x, lim->left), lim->right) - lim->left;
        my = g_game->scrollY + MIN(MAX(r->y, lim->top), lim->bottom) - lim->top;
        (*(union Flags_00498da0*)&g_game->inputFlags_2cc6).value &= ~1;
        (*(union Flags_00498da0*)&g_game->inputFlags_2cc6).bits.b1 = PointInRect(lim, r->x, r->y);
    }
    (*(union Flags_00498da0*)&g_game->inputFlags_2cc6).bits.b2 = (*(union Flags_00498da0*)&g_game->inputFlags_2cc6).bits.b0 || (*(union Flags_00498da0*)&g_game->inputFlags_2cc6).bits.b1;
    g_seams.los_radar->ClampWorldPosToTerrain(mx, my, (Vec3*)&(*(struct Pos_00498da0*)&g_game->pos));
    {
        Pos_00498da0* p = &(*(struct Pos_00498da0*)&g_game->pos);
        Point_00498da0 pt;
        Pos_00498da0 v = *p;
        pt.x = (short)(v.x >> 20);
        pt.y = (short)(v.z >> 20);
        (*(struct Point_00498da0*)&g_game->point) = pt;
    }
    g_game->cellFeature = g_seams.features->GetCellFeature(g_seams.los_radar->GetMapCell((*(struct Point_00498da0*)&g_game->point).x, (*(struct Point_00498da0*)&g_game->point).y));
}

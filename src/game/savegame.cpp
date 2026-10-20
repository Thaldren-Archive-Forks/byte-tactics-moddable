// Decompiled by deepseek-v4.1, deepseek-v4.1-flash, Sonnet, Sonnet 5.5, Haiku, space-bunny-free and GPT-6. Names are provisional.
// The saved game screens: the info panel, the load and save click handlers,
// the saved-game list and the two screen openers.

#include <stdio.h>
#include <string.h>
#include <time.h>

class Mission;

#include "../util/hapi_bank.h"

// Pack 1: the 4-byte pad after the pointer at +0x391e9 would shift later fields.
#pragma pack(push, 1)
#include "../gui/gadget.h"
#include "../gui/gui.h"
#include "../gui/layer.h"
#include "../../interfaces/seams.h"

// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
int DrawTriangleContours(int, int, int, int, int, int, int);
int DrawRangeCircle(int, int, int, int, int, int, int);
int UpdatePlayerEconomy(int);

// A player's side block, the SideData behind PlayerState::pSideData (Thaldren
// SideData); the byte at +0x95 is the side id the campaign setup writes.
struct SideData {
    char unknown_0[0x95];
    unsigned char bSideId;               // +0x95
};

// The skirmish setup block at g_game+0x29a0 (Thaldren SkirmishInfo, #6518).
struct SkirmishInfo {
    char unknown_0[0x108];
    int commanderDeath;                  // +0x108
    int mapping;                         // +0x10c
    int lineOfSight;                     // +0x110
    int losType;                         // +0x114
    int fixedLocations;                  // +0x118
    char aMapName[0x100];                // +0x11c
};

struct Entry_004931d0 {
    char unknown_0[0xce];
    void* handler;                     // +0xce
};

struct Gadget_004931d0 {
    char unknown_0[4];
    Entry_004931d0* info;              // +0x4
    void (__stdcall* handler)(Gui*);   // +0x8
    void* context;                     // +0xc
};

// The DELETE block's path buffer and the count it rebuilds the "GAMES" list
// with, plus the "GAMES" entry the GAMES/LOAD/GAMENAME block looks up and
// never reads. One local, so that the buffer's address is the aggregate's.
struct Save_00492df0 {
    int games;                           // +0, stored, never read
    int count;                           // +4
    char path[0x100];                    // +8
};

// One view of the game state. The GUI object at +0x519 is read as a Gui
// (0x491ec0, 0x493060), as a raw byte pointer (0x492b10) and as the message
// buffer (0x492360, 0x492df0); the flags at +0x2a44 are read both as the
// b0..b3 bitfield union (0x492360) and as bits0/bit2/bits3 (0x492df0).
union Flags_2a44 {
    unsigned short value;
    struct {
        unsigned short b0 : 1;
        unsigned short b1 : 1;
        unsigned short b2 : 1;
        unsigned short b3 : 1;
        unsigned short rest : 12;
    };
    struct {
        unsigned short bits0_2a44 : 2;
        unsigned short bit2_2a44 : 1;
        unsigned short bits3_2a44 : 13;
    };
};

union Flags_3923b {
    unsigned short value;
    struct {
        unsigned short b0 : 1;
        unsigned short b1 : 1;
        unsigned short b2 : 1;
        unsigned short b3 : 1;
        unsigned short rest : 12;
    };
};

#include "game.h"
#pragma pack(pop)

extern Game* g_game;

#include "../map/mission.h"
#include "../state/sim_globals.h"

extern char* g_savegameDir;
extern const char g_invalidSavegameText[]; // "Invalid savegame file"
extern char g_savegameRadarFrame[];
extern char* g_savegameFileNames;
extern char* g_savegameDescs;
extern char* g_savegameSideList;
extern char* g_savegameRadarPreview;

void __stdcall MarkChanged(Gui* menu);
Gadget* __stdcall FindGadgetChecked(Gadget* entries, char* name);
Gadget* __stdcall FindGadgetChecked(Gadget* entries, char* name);
Gadget* __stdcall FindGadgetChecked(Gadget* entries, char* name);
Gadget* __stdcall FindGadgetChecked(Gadget* entries, char* name);
Entry_004931d0* __stdcall FindGadgetChecked(void* entries, char* name);
int __stdcall FindGadgetIndex(Gadget* entries, char* name, int type);
int __stdcall FindGadgetIndex(Gadget* entries, char* name, int type);
int __stdcall FindGadgetIndex(Gadget* entries, char* name, int type);
void __stdcall SetGadgetText(Gui* menu, int index, char* text);
Gadget* __stdcall FindGadgetChecked_E(Gadget* entries, char* name);
void __stdcall SetGadgetActiveByName(void* menu, char* name, int value);
void __stdcall SetTranslatedTextByName(void* menu, char* name, char* text, int param_4);
char* __stdcall SkipTextLines(char* text, int n);
void __stdcall FrameFromSurface(void* dst, void* src);
void __stdcall FreeSurface(void* image);
void* __stdcall LoadSurface(HapiBank* obj);
HapiBank* __stdcall OpenSummaryBank(char* name);
void __stdcall FreeSummaryBank(void* handle);
HapiBank* __stdcall OpenHapiBank(char* path);
void __stdcall PlaySoundByName(char* name, int param);
void __stdcall ShutdownIngameSystems();
void __stdcall PopUntilNamedLayout(int flag);
void MenuFrame();
void __stdcall DisableKeyCommands(void* menu);
void __stdcall EnableKeyCommands(void* menu);
void __stdcall SetKeyboardInput(void* menu, int value);
void __stdcall ConfigureListBoxByName(void* menu, char* name, void* text, int value, int flag);
void __stdcall BeginTextEdit(void* menu, int index);
void __stdcall RenderLayer(void* menu, int value);
void __stdcall LoadPictureCached(const char* name, int a, int b, int c);
void __stdcall MakeDirectoryPath(char* path);
void __stdcall CloseTopScreen(char* sub);
void __stdcall RemoveFile(char* path);
char* __stdcall BuildDataPath(char* buf, char* dir, char* name, char* ext);
char* __stdcall BuildSideList();
int __stdcall SaveGameFile(char* path, char* description, int param_3);
int __stdcall CountDirectoryEntries(const char* path, int flag);
void __stdcall ScanDirectory(char* path, char* list, char* sizes, int mode, int flag, int what);
void __cdecl InitMissionStatus();
void OrLabelAttribs();
void __stdcall BlankScreen();
void __stdcall RegisterDataArchives();
void __stdcall SetFrontendState(int code, int line, char* file);
char __stdcall FindGameCdDrive(int flag);
void __stdcall SetCloseHandler(void (__cdecl *callback)(int), int param);
void __cdecl LeaveNetGameCallback(int param);
void __cdecl GameFreeThunk(void* p);
void* __cdecl GameAllocIgnoreTag(const char* name, unsigned int size);
void __cdecl operator delete(void* p);
void* __cdecl operator new(unsigned int size);
Layer* __stdcall LoadGuiLayer(Gui* menu, const char* name, int flags);
Gadget_004931d0* __stdcall LoadGuiLayer(char* sub, const char* name, int flags);
int __stdcall IsCurrentGadgetNamed(Gui* gadget, char* name);
int __stdcall IsCurrentGadgetNamed(Gui* gadget, char* name);
void __stdcall ClearSelectedGadget(Gui* menu);
void __stdcall ClearSelectedGadget(Gui* menu);
void __stdcall SetCursorAnimation(void* menu, void* data);
void __stdcall SetDescListCleanupFlag(Gui* menu, int flag);
const char* __stdcall Translate(const char* key);
void __stdcall OpenMessageBox(void* menu, const char* message, int a, int b, int c);
void __stdcall LoadGameScreenHandler(Gui* gadget);
void __stdcall SaveGameScreenHandler(Gui* gadget);
void __stdcall SavedGameSelectHandler(int a, int b);
char* __stdcall ListSavedGames(int* count);
void ShowSavedGameInfo();

// Stays an inline helper, with the field test outside it: gives the original's
// registers at both delete sites.
__inline void DeleteSave_00492360(HapiBank* obj)
{
    if (obj) {
        obj->CloseBank();
        operator delete(obj);
    }
}

// FUNCTION: 0x491ec0
void __stdcall ShowSavedGameInfo()
{
    Gui* menu = &g_game->gui;
    Layer* layer = g_game->gui.layer;
    Gadget* entries = layer->entries;
    Gadget* games = FindGadgetChecked(entries, "GAMES");
    if (games == 0)
        return;

    int index = FindGadgetIndex(entries, "GAMENAME", 3);
    char path[0x100];
    struct Buf { int gametype; char name[0x34]; char* diffs[3]; } b;
#define gametype b.gametype
#define name b.name
#define diffs b.diffs

    char* desc;
    if (games->u.list.selected > -1
        && (desc = SkipTextLines(g_savegameDescs, games->u.list.selected)) != 0
        && strlen(desc) != 0) {
        SetGadgetText(menu, index, desc);
        char* fname = SkipTextLines(g_savegameFileNames, games->u.list.selected);
        sprintf(path, "%s\\%s", g_savegameDir, fname);
        HapiBank* file = OpenSummaryBank(path);
        if (file != 0) {
            file->OpenNamedBox("Radar Image");
            Gadget* radar = FindGadgetChecked_E(menu->layer->entries, "RADAR");
            if (g_savegameRadarPreview != 0)
                FreeSurface(g_savegameRadarPreview);
            g_savegameRadarPreview = (char*)LoadSurface(file);
            if (g_savegameRadarPreview != 0) {
                FrameFromSurface(g_savegameRadarFrame, g_savegameRadarPreview);
                radar->u.list.field_c2 = g_savegameRadarFrame;
            }
            SetGadgetActiveByName(menu, "RADAR", g_savegameRadarPreview != 0);

            int players = file->GetIntegerItem("Players", 0);
            gametype = file->GetIntegerItem("Gametype", 0);
            if (players != 0) {
                if (gametype == 1)
                    strcpy(name, "Single");
                else
                    sprintf(name, "Skirmish (%d players)", players);
            } else {
                strcpy(name, "???");
            }
            SetTranslatedTextByName(menu, "GAMETYPE", name, 0);

            if (gametype == 1) {
                char* campaign = file->GetStringItem("Campaign", 0);
                if (campaign != 0) {
                    strcpy(name, campaign);
                    SetTranslatedTextByName(menu, "CAMPAIGN", name, 0);
                    SetGadgetActiveByName(menu, "CAMPTEXT", 1);
                    SetGadgetActiveByName(menu, "CAMPAIGN", 1);
                }
                char* mission = file->GetStringItem("Mission", 0);
                if (mission != 0) {
                    strcpy(name, mission);
                    SetTranslatedTextByName(menu, "MISSION", name, 0);
                }
            } else {
                SetGadgetActiveByName(menu, "CAMPTEXT", 0);
                SetGadgetActiveByName(menu, "CAMPAIGN", 0);
                char* mission = file->GetStringItem("Map", 0);
                if (mission != 0) {
                    strcpy(name, mission);
                    SetTranslatedTextByName(menu, "MISSION", name, 0);
                }
            }

            int time = file->GetIntegerItem("Game Time", 0);
            sprintf(name, "%02d:%02d:%02d", time / 108000, time / 1800 % 60,
                    time / 30 % 60);
            SetTranslatedTextByName(menu, "TIME", name, 0);

            if (g_savegameSideList != 0) {
                int side = file->GetIntegerItem("Side", 0);
                strcpy(name, SkipTextLines(g_savegameSideList, side));
            } else {
                strcpy(name, "???");
            }
            SetTranslatedTextByName(menu, "SIDE", name, 0);

            diffs[0] = "Easy";
            diffs[1] = "Medium";
            diffs[2] = "Hard";
            sprintf(name, "%s",
                    diffs[file->GetIntegerItem("Difficulty", 0)]);
            SetTranslatedTextByName(menu, "DIFF", name, 0);
            FreeSummaryBank(file);
            goto done;
        }
    }
    {
        SetGadgetText(menu, index, DAT_005119b8);
        SetTranslatedTextByName(menu, "SIDE", DAT_005119b8, 0);
        SetTranslatedTextByName(menu, "DIFF", DAT_005119b8, 0);
        SetTranslatedTextByName(menu, "MISSION", DAT_005119b8, 0);
        SetTranslatedTextByName(menu, "CAMPAIGN", DAT_005119b8, 0);
        SetTranslatedTextByName(menu, "GAMETYPE", DAT_005119b8, 0);
        SetTranslatedTextByName(menu, "TIME", DAT_005119b8, 0);
    }
done:
    MarkChanged(&g_game->gui);
#undef gametype
#undef name
#undef diffs
}

// FUNCTION: 0x492330
void __stdcall ShowInvalidSavegameDialog(void* param_1)
{
    OpenMessageBox(param_1, Translate(g_invalidSavegameText), 0x140, 1, 1);
    ClearSelectedGadget((Gui*)param_1);
}

// Load game screen click handler (sibling of 0x492df0, save game screen).
// FUNCTION: 0x492360
void __stdcall LoadGameScreenHandler(Gui* gadget)
{
    Gadget* entries = gadget->layer->entries;
    char buf[0x104];

    if (gadget->hotGadgetIndex == -1)
        return;

    if (IsCurrentGadgetNamed(gadget, "CANCEL")) {
        if ((*(union Flags_2a44*)&g_game->netGameFlags_2a44).b2)
            DisableKeyCommands(g_game->message_519);
        if (g_savegameFileNames)
            GameFreeThunk(g_savegameFileNames);
        if (g_savegameDescs)
            GameFreeThunk(g_savegameDescs);
        if (g_savegameSideList)
            GameFreeThunk(g_savegameSideList);
        g_savegameSideList = 0;
        g_savegameDescs = 0;
        g_savegameFileNames = 0;
        if (g_savegameRadarPreview)
            GameFreeThunk(g_savegameRadarPreview);
        g_savegameRadarPreview = 0;
        PlaySoundByName("Previous", 0);
        return;
    }
    if (!IsCurrentGadgetNamed(gadget, "LOAD") && !IsCurrentGadgetNamed(gadget, "GAMES")) {
        if (gadget->hotGadgetIndex != -1)
            ClearSelectedGadget(gadget);
        return;
    }
    Gadget* e = FindGadgetChecked(entries, "GAMES");
    sprintf(buf, "%s\\%s", g_savegameDir, SkipTextLines(g_savegameFileNames, e->u.list.selected));
    HapiBank* save = OpenSummaryBank(buf);
    if (save != 0) {
        int type = save->GetIntegerItem("Gametype", 0);
        FreeSummaryBank(save);
        switch (type) {
        case 1:
            if (!FindGameCdDrive(0)) {
                OpenMessageBox(g_game->message_519,
                    Translate("Please insert the Campaign CD (Disc 2) and try again"),
                    200, 1, 1);
                ClearSelectedGadget((Gui*)g_game->message_519);
                return;
            }
            break;
        case 2:
            if (!FindGameCdDrive(1)) {
                OpenMessageBox(g_game->message_519,
                    Translate("Please insert the Multiplayer CD (Disc 1) and try again"),
                    200, 1, 1);
                ClearSelectedGadget((Gui*)g_game->message_519);
                return;
            }
            break;
        default:
            goto invalid;
        }
        RegisterDataArchives();
        BlankScreen();
        if (((unsigned char)g_game->cursorMode) != 20) {
            g_game->cursorMode = (char)(20);
            SetCursorAnimation(g_game->message_519, ((void*)g_game->cursorHourglass));
        }
        if ((*(union Flags_2a44*)&g_game->netGameFlags_2a44).b2)
            DisableKeyCommands(g_game->message_519);
        PlaySoundByName("SMLBUTTON", 0);
        e = FindGadgetChecked(entries, "GAMES");
        sprintf(g_game->saveName_38c6b, "%s\\%s", g_savegameDir,
                SkipTextLines(g_savegameFileNames, e->u.list.selected));
        if ((*(union Flags_2a44*)&g_game->netGameFlags_2a44).b2)
            ShutdownIngameSystems();
        (*(union Flags_3923b*)&g_game->endGameFlags_3923b).b3 = 1;

        g_game->pendingSaveStore = (int)(OpenHapiBank(g_game->saveName_38c6b));
        if (((HapiBank*)g_game->pendingSaveStore) == 0)
            goto invalid;
        (((HapiBank*)g_game->pendingSaveStore))->OpenAccount("summary");
        g_seams.map->SetMissionType((((HapiBank*)g_game->pendingSaveStore))->GetIntegerItem("Gametype", 0));
        char* campaign = (((HapiBank*)g_game->pendingSaveStore))->GetStringItem("Campaign", 0);
        if (campaign != 0)
            g_seams.map->LoadCampaign(g_game->net, 0, campaign);
        g_game->flag_37ef2 = (((HapiBank*)g_game->pendingSaveStore))->GetIntegerItem("Side", 0);
        g_game->difficulty = (((HapiBank*)g_game->pendingSaveStore))->GetIntegerItem("Difficulty", 0);
        if (g_seams.map->GetGameType(g_game->net) == 1) {
            if (g_game->flag_37ef2 == 0) {
                g_game->p1b8a->bSideId = 0;
                g_game->p1cd5->bSideId = 1;
            } else {
                g_game->p1b8a->bSideId = 1;
                g_game->p1cd5->bSideId = 0;
            }
        }
        char* mission = (((HapiBank*)g_game->pendingSaveStore))->GetStringItem("Mission", 0);
        if (mission == 0)
            goto invalid;
        if (strlen(mission) == 0)
            goto invalid;
        if (g_seams.map->LoadMissionByName(g_game->net, 0, mission) == 0)
            goto invalid;
        strcpy((char*)&((SkirmishInfo*)g_game->options)->aMapName[0], mission);
        char* thumbs = (((HapiBank*)g_game->pendingSaveStore))->GetStringItem("Thumbs", 0);
        strncpy(g_game->campaignMissionStatus, thumbs, 0x19);
        if (strlen(g_game->campaignMissionStatus) != 0x19)
            InitMissionStatus();
        if (g_seams.map->GetGameType(g_game->net) == 2) {
            (((HapiBank*)g_game->pendingSaveStore))->OpenAccount("summary");
            g_game->numPlayers =
                (unsigned short)((short)(((HapiBank*)g_game->pendingSaveStore))->GetIntegerItem("Players", 0));
            ((SkirmishInfo*)g_game->options)->commanderDeath =
                (((HapiBank*)g_game->pendingSaveStore))->GetIntegerItem("CommanderDeath", 1);
            ((SkirmishInfo*)g_game->options)->fixedLocations =
                (((HapiBank*)g_game->pendingSaveStore))->GetIntegerItem("Location", 1);
            ((SkirmishInfo*)g_game->options)->mapping =
                (((HapiBank*)g_game->pendingSaveStore))->GetIntegerItem("Mapping", 1);
            ((SkirmishInfo*)g_game->options)->lineOfSight =
                (((HapiBank*)g_game->pendingSaveStore))->GetIntegerItem("LineOfSight", 1);
            ((SkirmishInfo*)g_game->options)->losType =
                (((HapiBank*)g_game->pendingSaveStore))->GetIntegerItem("LineOfSightType", 1);
        }
        (*(union Flags_2a44*)&g_game->netGameFlags_2a44).b2 = 1;
        g_game->mode_391f1 = 2;
        g_game->handler = MenuFrame;
        SetCloseHandler(LeaveNetGameCallback, 0);
        if (g_savegameFileNames)
            GameFreeThunk(g_savegameFileNames);
        if (g_savegameDescs)
            GameFreeThunk(g_savegameDescs);
        if (g_savegameSideList)
            GameFreeThunk(g_savegameSideList);
        g_savegameSideList = 0;
        g_savegameDescs = 0;
        g_savegameFileNames = 0;
        if (g_savegameRadarPreview)
            GameFreeThunk(g_savegameRadarPreview);
        g_savegameRadarPreview = 0;
        PopUntilNamedLayout(1);
        if (g_seams.map->GetGameType(g_game->net) == 1 &&
            (((HapiBank*)g_game->pendingSaveStore))->HasItem("BetweenMissions")) {
            (*(union Flags_2a44*)&g_game->netGameFlags_2a44).b3 = 1;
            if (((HapiBank*)g_game->pendingSaveStore)) {
                DeleteSave_00492360(((HapiBank*)g_game->pendingSaveStore));
                g_game->pendingSaveStore = (int)(0);
            }
            (*(union Flags_2a44*)&g_game->netGameFlags_2a44).b2 = 0;
            g_game->mode_391f1 = 2;
            g_game->handler = MenuFrame;
            SetCloseHandler(LeaveNetGameCallback, 0);
            SetFrontendState(0xe, 0x48c, "c:\\cavedog\\wargame\\wargame.cpp");
        }
        BlankScreen();
        return;
    }
invalid:
    if (((HapiBank*)g_game->pendingSaveStore)) {
        DeleteSave_00492360(((HapiBank*)g_game->pendingSaveStore));
    }
    g_game->pendingSaveStore = (int)(0);
    OpenMessageBox(gadget, Translate("Invalid savegame file"), 0x140, 1, 1);
    ClearSelectedGadget(gadget);
}

// Lists the saved games: builds the search pattern for the "SAV" files in the
// save directory, asks how many there are, and fills two tables: the file
// names (0x100 bytes each) and, packed one after another, each save's
// "Description" line. A file without a description is dropped by shifting the
// remaining names down. Publishes the descriptions in the "GAMES" menu and
// returns the names table (0 when there are no saves); the count goes back
// through the parameter.
// FUNCTION: 0x492b10
char* __stdcall ListSavedGames(int* count)
{
    char buf[0x100];
    int found;
    char* copy;
    int i;
    char* dp;

    BuildDataPath(buf, g_savegameDir, "*", "SAV");
    *count = CountDirectoryEntries(buf, 0);
    if (*count == 0) {
        ConfigureListBoxByName(g_game->menu, "GAMES", DAT_005119b8, 0, 0);
        return 0;
    }
    g_savegameFileNames = (char*)GameAllocIgnoreTag("SAVEGAME NAMES", *count << 8);
    g_savegameDescs = (char*)GameAllocIgnoreTag("SAVEGAME DESCS", *count << 6);
    memset(g_savegameDescs, 0, *count << 6);
    memset(g_savegameFileNames, 0, *count << 8);
    ScanDirectory(buf, g_savegameFileNames, 0, 0, 0, 1);
    dp = g_savegameDescs;
    found = 0;
    copy = (char*)GameAllocIgnoreTag("SAVEGAME2", *count << 8);
    memcpy(copy, g_savegameFileNames, *count << 8);
    for (i = 0; i < *count; i++) {
        sprintf(buf, "%s\\%s", g_savegameDir, SkipTextLines(copy, i));
        HapiBank* file = OpenSummaryBank(buf);
        char* desc = 0;
        if (file)
            desc = file->GetStringItem("Description", 0);
        if (file && desc) {
            strcpy(buf, desc);
            strcpy(dp, buf);
            dp += strlen(buf) + 1;
            found++;
            file->CloseBank();
            delete file;
        } else {
            char* d = SkipTextLines(g_savegameFileNames, found);
            for (int j = i + 1; j < *count; j++) {
                char* next = SkipTextLines(copy, j);
                strcpy(d, next);
                d += strlen(next) + 1;
            }
        }
    }
    GameFreeThunk(copy);
    ConfigureListBoxByName(g_game->menu, "GAMES", g_savegameDescs, found, 0);
    *count = found;
    return found ? g_savegameFileNames : 0;
}

// FUNCTION: 0x492de0
void __stdcall SavedGameSelectHandler(int arg1, int arg2)
{
    ShowSavedGameInfo();
}

// Click handler of the save game screen. On close (field +0x60 == -1) it
// frees the four save lists that 0x492b10 built. CANCEL goes back; DELETE
// removes the chosen save file, rebuilds the list and redraws; GAMES, LOAD and
// GAMENAME all take the name typed in the GAMENAME box and, when it is not
// empty, write the save under that name with a timestamp.
// FUNCTION: 0x492df0
void __stdcall SaveGameScreenHandler(Gui* gadget)
{
    Gadget* entries = gadget->layer->entries;
    Save_00492df0 save;
    if (gadget->hotGadgetIndex == -1) {
        SetDescListCleanupFlag(gadget, 1);
        if (g_savegameFileNames)
            GameFreeThunk(g_savegameFileNames);
        if (g_savegameDescs)
            GameFreeThunk(g_savegameDescs);
        if (g_savegameSideList)
            GameFreeThunk(g_savegameSideList);
        g_savegameSideList = 0;
        g_savegameDescs = 0;
        g_savegameFileNames = 0;
        if (g_savegameRadarPreview)
            GameFreeThunk(g_savegameRadarPreview);
        g_savegameRadarPreview = 0;
        return;
    }
    if (IsCurrentGadgetNamed(gadget, "CANCEL")) {
        if ((*(union Flags_2a44*)&g_game->netGameFlags_2a44).bit2_2a44)
            DisableKeyCommands(g_game->message_519);
        PlaySoundByName("Previous", 0);
        return;
    }
    if (IsCurrentGadgetNamed(gadget, "DELETE")) {
        PlaySoundByName("SmallButton", 0);
        Gadget* e = FindGadgetChecked(entries, "GAMES");
        sprintf(save.path, "%s\\%s", g_savegameDir, SkipTextLines(g_savegameFileNames, e->u.list.selected));
        RemoveFile(save.path);
        ListSavedGames(&save.count);
        ClearSelectedGadget(gadget);
        ShowSavedGameInfo();
        return;
    }
    if (!IsCurrentGadgetNamed(gadget, "GAMES") && !IsCurrentGadgetNamed(gadget, "LOAD") &&
        !IsCurrentGadgetNamed(gadget, "GAMENAME")) {
        if (gadget->hotGadgetIndex != -1)
            ClearSelectedGadget(gadget);
        return;
    }
    if ((*(union Flags_2a44*)&g_game->netGameFlags_2a44).bit2_2a44)
        DisableKeyCommands(g_game->message_519);
    PlaySoundByName("smlbutton", 0);
    int index = FindGadgetIndex(entries, "GAMENAME", 3);
    char* text = entries[index].u.text;
    if (strlen(text) != 0) {
        save.games = (int)FindGadgetChecked(entries, "GAMES");
        BuildDataPath(g_game->saveName_38c6b, g_savegameDir, text, "SAV");
        SaveGameFile(g_game->saveName_38c6b, text, time(0));
    }
}

// FUNCTION: 0x493060
void ShowSaveGameScreen()
{
    int local;

    g_game->bits_38a51_2.flag_38a51 |= 1;
    Layer* layer = LoadGuiLayer(&g_game->gui, "LOADGAME.GUI", 0x880);
    layer->handler = (void (__stdcall*)(void*))SaveGameScreenHandler;
    layer->data = g_game;
    LoadPictureCached("DSAVEGAME2", 0, 0, 0);
    MakeDirectoryPath(g_savegameDir);
    ListSavedGames(&local);
    SetTranslatedTextByName(&g_game->gui, "TITLE", "Save Game", 0);
    if (local == 0) {
        SetGadgetActiveByName(&g_game->gui, "DELETE", 0);
    }
    Gadget* games = FindGadgetChecked(layer->entries, "GAMES");
    if (games != 0) {
        games->u.list.callback = (void (__stdcall*)(Gui*, Gadget*))SavedGameSelectHandler;
    }
    int index = FindGadgetIndex(layer->entries, "GAMENAME", 3);
    layer->entries[index].attribs |= 2;
    g_savegameSideList = BuildSideList();
    ShowSavedGameInfo();
    BeginTextEdit(&g_game->gui, index);
    SetKeyboardInput(&g_game->gui, 1);
    OrLabelAttribs();
    SetGadgetActiveByName(&g_game->gui, "LoadGame", 0);
    EnableKeyCommands(&g_game->gui);
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x4931d0
void ShowLoadGameScreen()
{
    Gadget_004931d0* gadget = LoadGuiLayer((char*)&g_game->message_519[0], "LOADGAME.GUI", 0x980);
    gadget->handler = LoadGameScreenHandler;
    gadget->context = g_game;
    LoadPictureCached("DLOADGAME2", 0, 0, 0);
    int count;
    if (ListSavedGames(&count) == 0) {
        CloseTopScreen((char*)&g_game->message_519[0]);
        OpenMessageBox((char*)&g_game->message_519[0],
                     Translate("There are no saved games to choose from"),
                     0x140, 1, 1);
        return;
    }
    g_savegameSideList = BuildSideList();
    ConfigureListBoxByName((char*)&g_game->message_519[0], "GAMES", g_savegameDescs, count, 0);
    SetGadgetActiveByName((char*)&g_game->message_519[0], "DELETE", 0);
    SetGadgetActiveByName((char*)&g_game->message_519[0], "GAMENAME", 0);
    Entry_004931d0* entry = FindGadgetChecked(gadget->info, "GAMES");
    if (entry != 0) {
        entry->handler = (void*)SavedGameSelectHandler;
    }
    ShowSavedGameInfo();
    SetKeyboardInput((char*)&g_game->message_519[0], 1);
    OrLabelAttribs();
    SetGadgetActiveByName((char*)&g_game->message_519[0], "SaveGame", 0);
    EnableKeyCommands((char*)&g_game->message_519[0]);
    RenderLayer((char*)&g_game->message_519[0], 0x40);
    ((char*)g_game)[0x38a51] |= 1;
}

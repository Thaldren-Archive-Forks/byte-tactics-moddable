// Decompiled by Sonnet, Opus, Space Bunny Free, deepseek-v4.1-flash, deepseek-v4.1, Sonnet 5.5 and space-bunny-free. Names are provisional.
// The game state: the offscreen surface, the game mode and its frame handler,
// the wind, the game speed, the CD list settings in the registry, start-up and
// shutdown of the game, the reset for a new match and the selection of a
// screen.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "../map/mission.h"
#include "../../interfaces/seams.h"

struct Display_00491a70;
struct Src_004ab400;

#pragma pack(push, 1)
#include "../gui/gui.h"

#include "../network/player.h"

#include "game.h"

class Sound;

#pragma pack(pop)

// GLOBAL: 0x511de8
extern Game* g_game;

extern const char g_offscreenSurfaceName[];  // "OFFSCREEN"

int __stdcall AllocSurface(const char* name, int width, int height);
void __stdcall SetRestoreSurface(int surface);

// FUNCTION: 0x490ac0
void CreateOffscreenSurface()
{
    g_game->screen = AllocSurface(g_offscreenSurfaceName, g_game->width_37e1f, g_game->field_37e23);
    SetRestoreSurface(g_game->screen);
}

// Releases the offscreen object created by 0x490ac0.
void __cdecl GameFreeThunk(int* p);
void RestoreScreen();

// FUNCTION: 0x490b00
void FreeOffscreenSurface()
{
    GameFreeThunk((int*)g_game->screen);
    g_game->screen = 0;
    SetRestoreSurface(0);
    RestoreScreen();
}

// Stores the new game mode in g_game->frontendState (+0x391f1) and installs the state
// handler for it in g_game->handler (+0x391f5), through the 0x490c14 jump
// table. Mode 6 also gets a different quit callback from SetCloseHandler.
void __stdcall SetCloseHandler(void (__cdecl *callback)(int), int param);
void __cdecl LeaveNetGameCallback(int param);
void __cdecl HandleBattleQuitPrompt(int param);
void InitFrame();
void ReturnToMainMenuFrame();
void MenuFrame();
void PreBattleFrame();
void CampaignSetupFrame();
void LoadingScreenFrame();
void BattleFrame();
void EndGameFrame();

// FUNCTION: 0x490b30
void __stdcall SetGameMode(int param)
{
    g_game->mode_391f1 = param;
    switch (param) {
    case 0:
        g_game->handler = InitFrame;
        break;
    case 1:
        g_game->handler = ReturnToMainMenuFrame;
        break;
    case 2:
        g_game->handler = MenuFrame;
        break;
    case 3:
        g_game->handler = PreBattleFrame;
        break;
    case 4:
        g_game->handler = CampaignSetupFrame;
        break;
    case 5:
        g_game->handler = LoadingScreenFrame;
        break;
    case 6:
        g_game->handler = BattleFrame;
        break;
    case 7:
        g_game->handler = EndGameFrame;
        break;
    default:
        g_game->handler = 0;
        break;
    }
    if (param == 6) {
        SetCloseHandler(HandleBattleQuitPrompt, 0);
    } else {
        SetCloseHandler(LeaveNetGameCallback, 0);
    }
}

// The wind update. While the per-tick counter at +0x37ec4 is below the tick
// count at +0x38a47 it jitters the wind direction by a random amount, picks a
// random wind speed in the range at +0x1425b..+0x1425f and, when that speed is
// not zero, a fresh direction, then turns direction and speed into the two wind
// vector components and a 0..1 strength. Once the counter has caught up with the
// tick count the wind is switched off instead.

// Fixed-point trig helpers, written in assembly: the angle is a short.
int __cdecl FUN_004b70ef(short angle, int scale);
int __cdecl FUN_004b7123(short angle, int scale);
int __stdcall RandomInt(int range);

// FUNCTION: 0x490c40
void __cdecl UpdateWind()
{
    if (g_game->windCounter < ((unsigned int)g_game->ticks)) {
        // The __int64 cast keeps the _allmul/_alldiv calls; keep this one
        // expression with no temporary.
        g_game->windCounter += ((int)((__int64)rand() * 10 / 0x8000) + 5) * 30;

        // Separate statements: one expression changes how the low bound is added.
        int range = g_game->windSpeedMax - g_game->windSpeedMin;
        int n = RandomInt(range);
        g_game->windSpeed = g_game->windSpeedMin + n;
        if (g_game->windSpeed != 0)
            g_game->windDirection = RandomInt(0x10000);

        g_game->windX = -FUN_004b70ef(g_game->windDirection, g_game->windSpeed) * 2;
        g_game->windZ = -FUN_004b7123(g_game->windDirection, g_game->windSpeed) * 2;

        g_game->windStrength = (float)g_game->windSpeed / (float)g_game->windSpeedDenominator;
        if (1.0 < g_game->windStrength)
            g_game->windStrength = 1.0f;

        g_game->windEnabled = 1;
    } else {
        g_game->windEnabled = 0;
    }
}

// FUNCTION: 0x490da0
void ResetGameSpeed()
{
    if (g_seams.map->GetGameType(g_game->net) == 3) {
        g_game->gameSpeed = (unsigned short)(10);
        g_game->effectiveGameSpeed = 10;
    }
    g_game->carry = 0;
}


char* __stdcall Translate(char* text);
int GetLocalDpid();
int __stdcall BroadcastPacket(int player, void* data, int size);
void __stdcall AddMessage(char* text, int key, int value, int last);

// FUNCTION: 0x490df0
void __stdcall SetGameSpeed(int speed, int broadcast)
{
    if (speed > 0x14)
        speed = 0x14;
    if (speed < 1)
        speed = 1;
    if (speed != g_game->gameSpeed) {
        char buf[100];
        int d = speed - 10;
        if (d == 0) {
            strcpy(buf, Translate("Game Speed Normal"));
        } else {
            sprintf(buf, "%s  %c%d\n", Translate("Game Speed"),
                    (d > 0) ? '+' : ' ', d);
        }
        AddMessage(buf, 2, 0, 10);
    }
    g_game->gameSpeed = speed;
    g_game->effectiveGameSpeed = speed;
    if (broadcast) {
        char data[3];
        data[0] = 0x19;
        data[1] = 1;
        data[2] = (char)speed;
        BroadcastPacket(GetLocalDpid(), data, 3);
    }
}

// FUNCTION: 0x490ee0
void IncreaseGameSpeed()
{
    unsigned short value = g_game->gameSpeed;
    if (value < 0x14) {
        SetGameSpeed(value + 1, 1);
    }
}

// The counterpart of 0x490ee0: steps a 16-bit game setting down by one.
// FUNCTION: 0x490f10
void DecreaseGameSpeed()
{
    unsigned short value = g_game->gameSpeed;
    if (value > 1) {
        SetGameSpeed(value - 1, 1);
    }
}

struct CdLists_490f80 {
    char unknown_0[0x24];
    unsigned char tracks[0xaa0 - 0x24]; // +0x24
};

extern CdLists_490f80 g_cdListsDiscEntries;

int __stdcall ReadGameRegistryValue(const char* key, void* buf, unsigned int* size);

// FUNCTION: 0x490f40
void LoadCdLists(void)
{
    unsigned int size = 0xaa0;
    if (ReadGameRegistryValue("CDLISTS", &g_cdListsDiscEntries, &size) == 0) {
        memset(&g_cdListsDiscEntries, 0, sizeof(g_cdListsDiscEntries));
    }
}

// Copies the per-track bytes from the CD object into the CD-list settings
// block and saves it to the registry under "CDLISTS".
class Sound {
public:
    void SetTrackCategory(int mode);
    int GetDiscSerial();
    int IsFirstTrackData();
    int GetTrackCategory();
    int SetCdCallback(void (*cb)());
    void CopyTrackTypeTable(const void* src);
    int GetTrackCount();
    unsigned char GetCategoryOfTrack(int index);
    int SetPlaybackOrder(int value);
    void PlayNextTrack();
    int StopCdAudio();
    void EnableCdAudio(int on);
};

void __stdcall WriteGameRegistryValue(void* key, void* buf, int value);

// FUNCTION: 0x490f80
void SaveCdLists()
{
    for (int i = 0; i < ((Sound*)g_game->field_10)->GetTrackCount(); i++) {
        g_cdListsDiscEntries.tracks[i] = ((Sound*)g_game->field_10)->GetCategoryOfTrack(i + 1);
    }
    WriteGameRegistryValue((void*)"CDLISTS", &g_cdListsDiscEntries, sizeof(g_cdListsDiscEntries));
}

// Refreshes the CD-list table at g_cdListsDiscEntries from the CD object: it saves and
// restores the object's track table across an MCI close/open, looks the current
// disc id up in the 20-entry table and, when found, moves that entry to the
// front and re-reads its track bytes. A disc not in the table is inserted at the
// front after shifting the others up, provided the drive reports 16 audio
// tracks. The final cleanup call depends on the game mode.


extern int DAT_0051e848;
extern int g_cachedCdTrackTypes;
extern int DAT_0051e850;
extern int DAT_0051e854;
extern int DAT_0051e858;
extern int* g_savegameSideList;

// FUNCTION: 0x490fe0
void ReopenCdAudio()
{
    char tracks[16] = {1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    char buf[0x88];

    int saved = ((Sound*)g_game->field_10)->GetTrackCategory();
    mciSendStringA("stop cdaudio", 0, 0, 0);
    mciSendStringA("close cdaudio", 0, 0, 0);
    mciSendStringA("open cdaudio", 0, 0, 0);
    ((Sound*)g_game->field_10)->EnableCdAudio(g_game->musicMode & 1);
    ((Sound*)g_game->field_10)->SetPlaybackOrder(g_game->cdMode);
    ((Sound*)g_game->field_10)->SetTrackCategory(saved);

    int id = ((Sound*)g_game->field_10)->GetDiscSerial();
    int index = 0;
    int* slot = &DAT_0051e848;
    // Test *slot != id first, with the break as its own block: else the loop is rotated.
    while (1) {
        if (*slot != id) {
            slot = (int*)((char*)slot + 0x88);
            index++;
            if ((int)slot < (int)&g_cdListsDiscEntries + 0x20 + (int)sizeof(g_cdListsDiscEntries))
                continue;
            goto newdisc;
        }
        break;
    }
    {
        memcpy(buf, (char*)&g_cdListsDiscEntries + index * 0x88, 0x88);
        for (int j = index; j > 0; j--)
            memcpy((char*)&g_cdListsDiscEntries + j * 0x88,
                   (char*)&g_cdListsDiscEntries + (j - 1) * 0x88, 0x88);
        memcpy(&g_cdListsDiscEntries, buf, 0x88);
        ((Sound*)g_game->field_10)->CopyTrackTypeTable(&g_cachedCdTrackTypes);
    }
newdisc:
    if (index == 0x14) {
        if (((Sound*)g_game->field_10)->GetTrackCount() == 0x10) {
            if (((Sound*)g_game->field_10)->IsFirstTrackData() != 0) {
                ((Sound*)g_game->field_10)->CopyTrackTypeTable(tracks);
            }
        }
        // Downward pointer walk against the addresses, not an index loop.
        int p = (int)&g_cdListsDiscEntries + 0xa18;
        for (; p > (int)&g_cdListsDiscEntries; p -= 0x88)
            memcpy((void*)p, (void*)(p - 0x88), 0x88);
        g_cachedCdTrackTypes = *(int*)&tracks[0];
        DAT_0051e850 = *(int*)&tracks[4];
        DAT_0051e854 = *(int*)&tracks[8];
        DAT_0051e858 = *(int*)&tracks[12];
        DAT_0051e848 = id;
    }
    if ((g_game->netGameFlags & 4) != 0 && g_game->mode_391f1 == 6)
        ((Sound*)g_game->field_10)->PlayNextTrack();
    else
        ((Sound*)g_game->field_10)->StopCdAudio();
}

extern const char g_skirmishInfoTag[];     // "SkirmishInfo"
extern const char g_cdListsKey[];          // "CDLISTS"
extern const char g_guisDirName[];         // "guis"
extern const char g_animsDirName[];        // "anims"
extern const char g_fontsDirName[];        // "fonts"
extern const char g_commonGuiName[];       // "commongui"
extern const char g_hattFont12Name[];      // "hattfont12"
extern const char g_hattFont11Name[];      // "hattfont11"
extern const char g_unitLimitKey[];        // "UnitLimit"

int GetScreenWidth();
int GetScreenHeight();
void __stdcall SetPageFlipping(int enable);
void LoadGameResources();
void InitSound();
void ResetFrontendState();
void __stdcall InitPacketTables(void* param_1);
void LoadGameFonts();
void LoadDefaultPalette();
void LoadAllSound();
void __stdcall LoadAlphaTable(void* palette);
void __stdcall LoadShadeTable(void* palette);
void __stdcall LoadLightTable(void* palette);
void __stdcall MakeGrayTable(void* palette);
void __stdcall MakeBlueTable(void* palette);
void* __cdecl GameAllocIgnoreTag(const char* name, unsigned int size);
void LoadSettings();
void ApplyBrightnessAndVolume();
void LoadSideData();
void LoadLogos();
void __stdcall SetBrightness(float brightness);
void __stdcall SetCurrentGuiContext(void* ctx);
void __stdcall SetGuiPath(void* obj, const char* name);
void __stdcall SetAnimsPath(void* obj, const char* name);
void __stdcall SetFontsPath(void* obj, const char* name);
void __stdcall SetDefaultFont(void* param_1, int param_2);
void* __stdcall GetGafFrame(void* gaf, int frame);
void __stdcall InitCursorFrames(void* p, void* value);
void __stdcall LoadGafFile(void* obj, const char* name);
void __stdcall LoadGafIntoSlot(void* obj, const char* name, int index);
void __stdcall SetTextKeyColor(int value);
void __stdcall SetFont(int font);
void ClearPictureCache();
int __stdcall GetPreferenceInt(const char* name, int defaultValue);

// FUNCTION: 0x491200
void InitGame()
{
    unsigned int size;

    // Each GlobalMemoryStatus call in its own nested block: sets the dwLength
    // store position and shares one stack slot.
    {
        MEMORYSTATUS mem;
        mem.dwLength = 0x20;
        GlobalMemoryStatus(&mem);
    }
    g_game->width_37e1f = GetScreenWidth();
    g_game->field_37e23 = GetScreenHeight();
    g_game->screen = AllocSurface(g_offscreenSurfaceName, g_game->width_37e1f,
                                      g_game->field_37e23);
    SetRestoreSurface(g_game->screen);
    g_game->endGameFlags &= 0xfffe;
    g_game->endGameFlags &= 0xfffd;
    g_game->restartMissionRequest = 0;
    g_seams.map->SetMissionType(0);
    if (g_seams.map->GetGameType(g_game->net) == 3) {
        g_game->gameSpeed = (unsigned short)(10);
        g_game->effectiveGameSpeed = 10;
    }
    g_game->carry = 0;
    g_game->field_37ee6 = (unsigned short)(((short)g_game->maxUnits_37eec));
    g_game->maxUnits = (unsigned short)(((short)g_game->field_37ee6));
    g_game->mode = 3;
    g_game->uiOptionFlags &= 0xfdff;
    g_game->uiOptionFlags &= 0xff7f;
    g_game->uiOptionFlags &= 0xfeff;
    SetPageFlipping(0);
    LoadGameResources();
    InitSound();
    ResetFrontendState();
    InitPacketTables(g_game->chatHudRing);
    g_seams.orders->RegisterAllOrderTypes();
    LoadGameFonts();
    LoadDefaultPalette();
    LoadAllSound();
    LoadAlphaTable(g_game->field_143a7);
    LoadShadeTable(g_game->field_143a7);
    LoadLightTable(g_game->field_143a7);
    MakeGrayTable(g_game->field_143a7);
    MakeBlueTable(g_game->field_143a7);
    g_game->options = (Entry_00479620*)(GameAllocIgnoreTag(g_skirmishInfoTag, 0x22c));
    LoadSettings();
    size = 0xaa0;
    int ok = ReadGameRegistryValue(g_cdListsKey, &g_cdListsDiscEntries, &size);
    if (ok == 0)
        memset(&g_cdListsDiscEntries, 0, 0xaa0);
    ((Sound*)g_game->field_10)->EnableCdAudio(g_game->musicMode & 1);
    ((Sound*)g_game->field_10)->SetPlaybackOrder(g_game->cdMode);
    ((Sound*)g_game->field_10)->SetCdCallback(ReopenCdAudio);
    ReopenCdAudio();
    ((Sound*)g_game->field_10)->SetTrackCategory(0);
    ApplyBrightnessAndVolume();
    LoadSideData();
    LoadLogos();
    SetBrightness(0.5 - g_game->brightness * -0.041666668f);
    SetCurrentGuiContext(&g_game->gui);
    SetGuiPath(&g_game->gui, g_guisDirName);
    SetAnimsPath(&g_game->gui, g_animsDirName);
    SetFontsPath(&g_game->gui, g_fontsDirName);
    SetDefaultFont(&g_game->gui, g_game->fontComix);
    InitCursorFrames(&g_game->gui, GetGafFrame(g_game->cursorNormal, 0));
    LoadGafFile(&g_game->gui, g_commonGuiName);
    LoadGafIntoSlot(&g_game->gui, g_hattFont12Name, 0);
    LoadGafIntoSlot(&g_game->gui, g_hattFont11Name, 1);
    g_game->gui.language = g_game->gui.values[0];
    SetTextKeyColor(0xfe);
    SetFont(g_game->fontComix);
    g_game->debugMode = 0;
    g_game->movieCaptureIndex = 0;
    g_game->valueSet = 0;
    g_game->valueSet_38c63 = 0;
    g_game->unusedAfterMovieOutputRateDirty = 0;
    g_game->pendingSaveStore = 0;
    g_game->moviePlayer = (void*)(0);
    g_game->mode_391f1 = 0;
    g_game->handler = InitFrame;
    SetCloseHandler(LeaveNetGameCallback, 0);
    {
        MEMORYSTATUS mem2;
        mem2.dwLength = 0x20;
        GlobalMemoryStatus(&mem2);
    }
    ClearPictureCache();
    g_game->gui.clearQuickKeys = 1;
    int limit = GetPreferenceInt(g_unitLimitKey, 0xfa);
    if (limit > 500)
        limit = 500;
    else if (limit < 20)
        limit = 20;
    g_game->maxUnits_37eec = (unsigned short)(limit);
}

// Saves the per-track bytes from the CD object into the CD-list settings
// block and writes it to the registry under "CDLISTS", then releases the
// CD-list buffers, the track buffer and the rest of the game state.
struct Obj_004aeda0;
struct Obj_004aef80;
struct Class_00452370;

void FreePictureCache();
void __stdcall FreeGafSlot(Obj_004aeda0* obj, int i);
void __stdcall FreeCommonGuiGaf(Obj_004aef80* obj);
void FreeLogos();
void FreeSideFonts();
void FreeSounds();
void FreeGameFonts();
void ShutdownSound();
void FreeAnimFiles();
void __stdcall ReleasePacketData(Class_00452370* obj);

// FUNCTION: 0x4916a0
void ShutdownGame(void)
{
    for (int i = 0; i < ((Sound*)g_game->field_10)->GetTrackCount(); i++) {
        g_cdListsDiscEntries.tracks[i] = ((Sound*)g_game->field_10)->GetCategoryOfTrack(i + 1);
    }
    WriteGameRegistryValue((void*)"CDLISTS", &g_cdListsDiscEntries, 0xaa0);
    FreePictureCache();
    FreeGafSlot((Obj_004aeda0*)&g_game->gui, 1);
    FreeGafSlot((Obj_004aeda0*)&g_game->gui, 0);
    FreeCommonGuiGaf((Obj_004aef80*)&g_game->gui);
    FreeLogos();
    FreeSideFonts();
    FreeSounds();
    FreeGameFonts();
    ShutdownSound();
    FreeAnimFiles();
    GameFreeThunk((int*)g_game->screen);
    g_game->screen = 0;
    SetRestoreSurface(0);
    RestoreScreen();
    g_seams.orders->ClearOrderTypeTable();
    GameFreeThunk((int*)((void*)g_game->options));
    g_game->options = (Entry_00479620*)(0);
    g_seams.units->FreeUnitInfo();
    ReleasePacketData((Class_00452370*)g_game->chatHudRing);
    g_seams.map->FreeOtaEnumCacheAndMission();
}

#include "../../interfaces/seams.h"

// Resets the game state for a new match: clears the scratch fields and flags
// in g_game, runs the per-subsystem reset functions, allocates the three
// transform point buffers, and zeroes the per-player counters and the input
// history.
extern unsigned int g_scorePanelSlidePos;
extern unsigned int g_cdActivityRingWriteIdx;
extern int g_cdActivitySampleRing[30];

void ResetChatHudIndices();
void LoadLightBar();
void ResetSpeech();
void LoadTextureGafs();
void CreateParticleLists();
void ResetCameraState();
void InitCommands();
unsigned int GetTicks();

// FUNCTION: 0x4917d0
void LoadBattleAssets()
{
    ResetChatHudIndices();
    memset(&g_game->chatRecipients_2bf1_b, 0, 11);
    g_game->orderMode = 1;
    g_game->buildTypeIndex = 0;
    g_game->bit5_2cc6 = 0;
    g_game->bit6_2cc6 = 0;
    g_game->hoverUnitId = 0;
    g_game->bits_37ebe.bit0_37ebe = 0;
    g_game->bits_37ebe.bit11_37ebe = 0;
    g_game->restartMissionRequest = 0;
    g_game->bits_37f2f_1.bit9_37f2f = 0;
    g_game->bits_37f2f_1.bit7_37f2f = 0;
    g_game->bits_37f2f_1.bit8_37f2f = 0;
    LoadLightBar();
    ResetSpeech();
    LoadTextureGafs();
    g_seams.features->LoadFeatureFileList();
    g_seams.units->InitUnitCategories();
    g_seams.features->NopAfterFeatureEnum();
    CreateParticleLists();
    g_seams.weapons->AllocWeaponArray();
    g_seams.weapons->LoadWeaponTypes();
    g_seams.los_radar->LoadTntMap();
    ResetCameraState();
    g_seams.units->LoadUnitTypes();
    g_seams.units->LoadDownloadMenus();
    g_seams.units->AllocateUnitMemory();
    g_seams.features->ResolveFeatureLinks();
    g_seams.features->FreeFeatureFileList();
    g_seams.map->BuildAllPassMaps();
    g_game->windSpeedDenominator = 5000;
    g_game->windCounter = 0;
    g_seams.wind_meteors->UpdateWind();
    g_game->xform = GameAllocIgnoreTag("TEMP XFORM PTS", 0x960);
    g_game->projected = GameAllocIgnoreTag("TEMP PROJECTED PTS", 0x640);
    g_game->field_1438b = GameAllocIgnoreTag("ASSEM PTS", 0xa0);
    g_game->lastTick = GetTicks();
    g_game->ticks = (int)(0);
    if (g_seams.map->GetGameType(g_game->net) == 3) {
        g_game->gameSpeed = (unsigned short)(10);
        g_game->effectiveGameSpeed = 10;
    }
    g_game->carry = 0;
    g_seams.wind_meteors->InitMeteors();
    g_seams.los_radar->InitRadar();
    g_seams.players->InitPlayers();
    g_seams.orders->CreatePathfinder();
    g_seams.weapons->InitExplosions();
    InitCommands();
    for (int i = 0; i < 10; i++)
        g_game->players[i].unitsCreated = 0;
    g_game->bits.flag2 = 0;
    g_game->bits.flag4 = 0;
    g_game->bits.flag5 = 0;
    g_game->bits.flag6 = 0;
    g_game->bits_2bee_2.flags_2bee = 0;
    g_game->field_2a46 = 0xff;
    g_cdActivityRingWriteIdx = 0;
    memset(g_cdActivitySampleRing, 0, sizeof(g_cdActivitySampleRing));
    g_scorePanelSlidePos = 0;
}

struct Display_00491a70 {
    char unknown_0[0x40];
    HWND hwnd;                         // +0x40
};

void __stdcall SetResolution(int x, int y);
void __stdcall SetOffscreenSurface(int surface);

// FUNCTION: 0x491a70
void Force640x480Surfaces()
{
    g_game->width_37e1f = 0x280;
    g_game->field_37e23 = 0x1e0;
    if (GetScreenWidth() != 0x280 || GetScreenHeight() != 0x1e0) {
        GameFreeThunk((int*)g_game->screen);
        g_game->screen = 0;
        SetRestoreSurface(0);
        RestoreScreen();
        SetWindowPos(((Display_00491a70*)g_game->unknown_c)->hwnd, 0, 0, 0, 0x280, 0x1e0, 4);
        SetResolution(0x280, 0x1e0);
        g_game->screen = AllocSurface(g_offscreenSurfaceName, g_game->width_37e1f, g_game->field_37e23);
        SetRestoreSurface(g_game->screen);
        SetOffscreenSurface(g_game->screen);
    }
}

void __cdecl CollectEndGameStats();
void DestroyParticleLists();
void FreeTextureGafs();
void CloseNetSession();

// FUNCTION: 0x491b60
void ShutdownIngameSystems()
{
    g_game->flags_2a44 &= 0xfffb;
    ((Sound*)g_game->field_10)->StopCdAudio();
    ((Sound*)g_game->field_10)->SetTrackCategory(4);
    CollectEndGameStats();
    g_seams.wind_meteors->EmptyShutdownPreCleanup();
    g_seams.units->FreeUnitMemory();
    DestroyParticleLists();
    g_seams.weapons->FreeExplosions();
    g_seams.orders->DestroyPathfinder();
    g_seams.players->FreePlayers();
    g_seams.los_radar->FreeRadar();
    g_seams.los_radar->FreeMapResources();
    GameFreeThunk((int*)g_game->field_1438b);
    GameFreeThunk((int*)g_game->projected);
    GameFreeThunk((int*)g_game->xform);
    g_game->field_1438b = 0;
    g_game->projected = 0;
    g_game->xform = 0;
    g_seams.units->FreeDownloadMenus();
    g_seams.units->FreeUnitTypes();
    g_seams.weapons->FreeWeaponTypes();
    FreeTextureGafs();
    g_seams.weapons->FreeWeaponArray();
    g_seams.units->FreeMovementClasses();
    g_seams.units->FreeUnitCategories();
    if (g_seams.map->GetGameType(g_game->net) == 3) {
        CloseNetSession();
    }
}

void BlankScreen();
void RemoveLocalPlayers();
void __stdcall QuitApp(const char*);

// FUNCTION: 0x491c60
void ShutdownIngameAndQuit()
{
    BlankScreen();
    RemoveLocalPlayers();
    ShutdownIngameSystems();
    QuitApp(0);
}

// Selects entry n (a signed byte at +0x2cbe) and hands the matching table
// entry at +0x1487f to SetCursorAnimation for the object at +0x519.
struct Obj_004ab400;

void __stdcall SetCursorAnimation(Obj_004ab400* p, Src_004ab400* src);

// FUNCTION: 0x491c80
void __stdcall SetCursorMode(int n)
{
    if (((signed char)g_game->cursorMode) != n) {
        g_game->cursorMode = (char)(n);
        SetCursorAnimation((Obj_004ab400*)&g_game->gui, g_game->table[n]);
    }
}

// Picks the order/state table entry at +0x1487f and hands it to
// SetCursorAnimation for the object at +0x519, mirroring 0x491c80.
int __cdecl UpdatePlacementGhostValidity(void);
unsigned short __cdecl PickUnitUnderCursor(void);
int __stdcall ResolveCursorModeForSelection(unsigned char mode);

// FUNCTION: 0x491cc0
void __stdcall UpdateBattleHoverMode(int unused)
{
    unsigned char flags = g_game->inputFlags;

    if ((flags & 2) != 0 && g_game->orderMode == 0xe) {
        UpdatePlacementGhostValidity();
        return;
    }
    if ((flags & 2) == 0 && (flags & 1) == 0) {
        if (((signed char)g_game->cursorMode) != 0x13) {
            g_game->cursorMode = (char)(0x13);
            SetCursorAnimation((Obj_004ab400*)&g_game->gui, g_game->table[0x13]);
        }
        return;
    }
    g_game->hoverUnitId = PickUnitUnderCursor();
    int n = ResolveCursorModeForSelection(g_game->orderMode);
    if (((signed char)g_game->cursorMode) != n) {
        g_game->cursorMode = (char)(n);
        SetCursorAnimation((Obj_004ab400*)&g_game->gui, g_game->table[n]);
    }
}

int __stdcall IsScreenNamed(Gui* queue, char* name);
void __stdcall CloseTopScreen(Gui* queue);

// FUNCTION: 0x491d70
int __stdcall PopUntilNamedLayout(int force)
{
    unsigned short flags = g_game->flags_37ebe;
    if (((flags & 0x800) || (flags & 0x65) || (g_game->lobbyUiDirtyFlags & 0xe0)) && force == 0) {
        g_game->flags_37ebe = flags | 0x10;
        return 0;
    }
    g_game->unitIndex = (unsigned short)(0);
    while (g_game->gui.layer) {
        if (IsScreenNamed(&g_game->gui, g_game->name))
            return 1;
        CloseTopScreen(&g_game->gui);
    }
    return 0;
}

// Compare 0x491d70.
// FUNCTION: 0x491e10
void CloseTopScreenIfNotNamed()
{
    if (!IsScreenNamed(&g_game->gui, g_game->name)) {
        g_game->unitIndex = (unsigned short)(0);
        CloseTopScreen(&g_game->gui);
    }
}

extern int* g_savegameFileNames;
extern int* g_savegameDescs;
extern int* g_savegameRadarPreview;

// FUNCTION: 0x491e50
void FreePreviewCaches()
{
    if (g_savegameFileNames != 0) {
        GameFreeThunk(g_savegameFileNames);
    }
    if (g_savegameDescs != 0) {
        GameFreeThunk(g_savegameDescs);
    }
    if (g_savegameSideList != 0) {
        GameFreeThunk(g_savegameSideList);
    }
    g_savegameSideList = 0;
    g_savegameDescs = 0;
    g_savegameFileNames = 0;
    if (g_savegameRadarPreview != 0) {
        GameFreeThunk(g_savegameRadarPreview);
    }
    g_savegameRadarPreview = 0;
}

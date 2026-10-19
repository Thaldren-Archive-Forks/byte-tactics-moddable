// Decompiled by Opus, deepseek-v4.1-flash, GPT-6, deepseek-v4.1 and space-bunny-free. Names are provisional.

// Full <windows.h> pulls in <rpc.h>/<ole2.h>, whose declarations flip the
// base/index order of a SIB address in 0x431740.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../map/mission.h"

#pragma pack(push, 1)
struct SkirmishPlayer {
    int controller;      // +0x00
    int side;            // +0x04
    int allyGroup;       // +0x08
    int metal;           // +0x0c
    int energy;          // +0x10
    int color;           // +0x14
};

struct Options {
    SkirmishPlayer players[10];         // +0x00, 0x18 bytes each
    char unknown_f0[0x108 - 10 * 0x18];
    int skirmishCommanderDeath;         // +0x108
    int skirmishMapping;                // +0x10c
    int skirmishLineOfSight;            // +0x110
    int skirmishLOSType;                // +0x114
    int fixedLocations;                 // +0x118
    char skirmishMap[0x228 - 0x11c];    // +0x11c
    int skirmishDifficulty;             // +0x228
};

struct Flags_0042f9a0 {
    unsigned short damagebars : 1;       // bit 0
    unsigned short antiAlias : 1;        // bit 1
    unsigned short shadows : 1;          // bit 2
    unsigned short vehicleShadows : 1;   // bit 3
    unsigned short featureShadows : 1;   // bit 4
    unsigned short shading : 1;          // bit 5
    unsigned short ditheredFog : 1;      // bit 6
    unsigned short unused7 : 1;          // bit 7
    unsigned short switchAlt : 1;        // bit 8
};

struct SoundFlags {
    unsigned short soundMode : 3;        // bits 0..2
    unsigned short restoreVolume : 1;    // bit 3
    unsigned short ackfx : 1;            // bit 4
    unsigned short buildfx : 1;          // bit 5
    unsigned short speechfx : 1;         // bit 6
};

struct MusicFlags_0042f9a0 {
    unsigned short musicmode : 1;        // bit 0
};

#include "game.h"

#include "../units/unit_def.h"

#include "../sound/sound.h"
#include "../../interfaces/seams.h"
#include "../host/settings_io.h"

#pragma pack(pop)

extern Game* g_game;
extern char g_onlineLobbyPlayerName[];
extern char g_cmdlineHostGameName[];

int __stdcall GetWindowsUserName(void* buf);
int __cdecl IsOnlineConfigLoaded();
void __cdecl ProtectBlockReadWrite(void* p);
void __cdecl ProtectBlockReadOnly(void* p);

// FUNCTION: 0x42f910
void __stdcall SaveTrackSettings(unsigned char* tracks)
{
    char name[12];
    int i;

    for (i = 0; i < 10; i++) {
        sprintf(name, "track%d", i);
        SettingSetInt((void*)"Total Annihilation", name, tracks[i]);
    }
}

// Writes a value under the game's registry key (see WriteRegistryBinary); the
// sibling of ReadGameRegistryValue.
// FUNCTION: 0x42f960
void __stdcall WriteGameRegistryValue(void* key, void* buf, int value)
{
    SettingSetBlob((void*)"Total Annihilation", key, buf, value);
}

// Reads a value from the game's registry key (see ReadRegistryValue/0x4b6880).
// FUNCTION: 0x42f980
int __stdcall ReadGameRegistryValue(const char* key, void* buf, unsigned int* size)
{
    return SettingGetBlob("Total Annihilation", key, buf, size);
}

// Reads a DWORD under the game key into g_game->*field; a missing value stores
// the default and writes it back. value is the caller's scratch local (one of
// the helper's own moves the frame), and the named ok keeps the compare against
// the hoisted zero register.
static inline void ReadSettingInt(const char* key, int Game::* field, int defaultValue, int& value)
{
    int ok = SettingGetInt((void*)"Total Annihilation", (void*)key, &value);
    if (ok != 0) {
        g_game->*field = value;
    } else {
        g_game->*field = defaultValue;
        SettingSetInt((void*)"Total Annihilation", (void*)key, g_game->*field);
    }
}

// Same, but a missing value is not written back.
static inline void ReadSettingIntOr(const char* key, int Game::* field, int defaultValue, int& value)
{
    if (SettingGetInt((void*)"Total Annihilation", (void*)key, &value) != 0) {
        g_game->*field = value;
    } else {
        g_game->*field = defaultValue;
    }
}

// Same for g_game->options.
static inline void ReadSettingInt(const char* key, int Options::* field, int defaultValue, int& value)
{
    int ok = SettingGetInt((void*)"Total Annihilation", (void*)key, &value);
    if (ok != 0) {
        ((Options*)g_game->options)->*field = value;
    } else {
        ((Options*)g_game->options)->*field = defaultValue;
        SettingSetInt((void*)"Total Annihilation", (void*)key, ((Options*)g_game->options)->*field);
    }
}

// Reads one per-player skirmish value; a missing value is not written back.
static inline void ReadPlayerSetting(const char* key, int SkirmishPlayer::* field, int defaultValue, int& value, int i)
{
    if (SettingGetInt((void*)"Total Annihilation\\Skirmish", (void*)key, &value) != 0) {
        ((Options*)g_game->options)->players[i].*field = value;
    } else {
        ((Options*)g_game->options)->players[i].*field = defaultValue;
    }
}

// Reads up to DitheredFog, PlayMovie and AllMissions go through a named local;
// every other site compares the call directly.
// FUNCTION: 0x42f9a0
void LoadSettings()
{
    int value;
    char name[32];
    char buf[256];
    int i;

    int ok1 = SettingGetInt((void*)"Total Annihilation", (void*)"Interface Type", &value);
    if (ok1 != 0) {
        if (value > 1) value = 1;
        g_game->interfaceType = value;
    } else {
        g_game->interfaceType = 0;
        SettingSetInt((void*)"Total Annihilation", (void*)"Interface Type", g_game->interfaceType);
    }
    ReadSettingInt("DisplaymodeWidth", &Game::width_37f1b, 0x280, value);
    ReadSettingInt("DisplaymodeHeight", &Game::height_37f1f, 0x1e0, value);
    ReadSettingInt("side", &Game::flag_37ef2, 0, value);
    int ok5 = SettingGetInt((void*)"Total Annihilation", (void*)"Difficulty", &value);
    if (ok5 != 0) {
        g_game->difficulty = value & 0xffff;
    } else {
        g_game->difficulty = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"Difficulty", g_game->difficulty);
    }
    int ok6 = SettingGetInt((void*)"Total Annihilation", (void*)"scrollspeed", &value);
    if (ok6 != 0) {
        g_game->scrollSpeed = (unsigned char)value;
    } else {
        g_game->scrollSpeed = 0x20;
        SettingSetInt((void*)"Total Annihilation", (void*)"scrollspeed", g_game->scrollSpeed);
    }
    ReadSettingInt("SingleCommanderDeath", &Game::singleCommanderDeath, 1, value);
    ReadSettingInt("SingleMapping", &Game::singleMapping, 1, value);
    ReadSettingInt("SingleLineOfSight", &Game::singleLineOfSight, 1, value);
    ReadSettingInt("SingleLOSType", &Game::singleLOSType, 1, value);
    ReadSettingInt("screenchat", &Game::screenchat, 1, value);
    int ok12 = SettingGetInt((void*)"Total Annihilation", (void*)"damagebars", &value);
    if (ok12 != 0) {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).damagebars = value;
    } else {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).damagebars = 0;
        SettingSetInt((void*)"Total Annihilation", (void*)"damagebars", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).damagebars);
    }
    int ok13 = SettingGetInt((void*)"Total Annihilation", (void*)"Sound Mode", &value);
    if (ok13 != 0) {
        if (value == 2) {
            ((Sound*)g_game->field_10)->Enable3D();
        } else {
            ((Sound*)g_game->field_10)->Disable3D();
        }
        (*(struct SoundFlags*)&g_game->soundFlags).soundMode = value;
    } else {
        SettingSetInt((void*)"Total Annihilation", (void*)"Sound Mode",
                     (((Sound*)g_game->field_10)->Is3DEnabled() != 0) + 1);
        (*(struct SoundFlags*)&g_game->soundFlags).soundMode = 1;
    }
    int ok14 = SettingGetInt((void*)"Total Annihilation", (void*)"MixingBuffers", &value);
    if (ok14 != 0) {
        ((Sound*)g_game->field_10)->SetMaxBuffers(value);
    } else {
        ((Sound*)g_game->field_10)->SetMaxBuffers(8);
    }
    int ok15 = SettingGetInt((void*)"Total Annihilation", (void*)"RestoreVolume", &value);
    if (ok15 != 0) {
        (*(struct SoundFlags*)&g_game->soundFlags).restoreVolume = value;
    } else {
        (*(struct SoundFlags*)&g_game->soundFlags).restoreVolume = 0;
    }
    if ((*(struct SoundFlags*)&g_game->soundFlags).restoreVolume) {
        int ok16 = SettingGetInt((void*)"Total Annihilation", (void*)"WaveOutVolume", &value);
        if (ok16 != 0) {
            ((Sound*)g_game->field_10)->SetWaveVolume(value);
        }
        int ok17 = SettingGetInt((void*)"Total Annihilation", (void*)"CDAudioVolume", &value);
        if (ok17 != 0) {
            ((Sound*)g_game->field_10)->SetAuxVolume(value, 0);
        }
    }
    int ok18 = SettingGetInt((void*)"Total Annihilation", (void*)"Anti-Alias", &value);
    if (ok18 != 0) {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).antiAlias = value;
    } else {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).antiAlias = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"Anti-Alias", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).antiAlias);
    }
    int ok19 = SettingGetInt((void*)"Total Annihilation", (void*)"Shadows", &value);
    if (ok19 != 0) {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).shadows = value;
    } else {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).shadows = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"Shadows", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).shadows);
    }
    int ok20 = SettingGetInt((void*)"Total Annihilation", (void*)"FeatureShadows", &value);
    if (ok20 != 0) {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).featureShadows = value;
    } else {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).featureShadows = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"FeatureShadows", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).featureShadows);
    }
    int ok21 = SettingGetInt((void*)"Total Annihilation", (void*)"VehicleShadows", &value);
    if (ok21 != 0) {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).vehicleShadows = value;
    } else {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).vehicleShadows = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"VehicleShadows", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).vehicleShadows);
    }
    int ok22 = SettingGetInt((void*)"Total Annihilation", (void*)"Shading", &value);
    if (ok22 != 0) {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).shading = value;
    } else {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).shading = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"Shading", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).shading);
    }
    int ok23 = SettingGetInt((void*)"Total Annihilation", (void*)"DitheredFog", &value);
    if (ok23 != 0) {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).ditheredFog = value;
    } else {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).ditheredFog = 0;
        SettingSetInt((void*)"Total Annihilation", (void*)"DitheredFog", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).ditheredFog);
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"Gamma", &value) != 0) {
        if (value == 10) {
            g_game->brightness = 0xc;
        } else {
            g_game->brightness = value;
        }
    } else {
        g_game->brightness = 0xc;
        SettingSetInt((void*)"Total Annihilation", (void*)"Gamma", g_game->brightness);
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"SwitchAlt", &value) != 0) {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).switchAlt = value;
    } else {
        (*(struct Flags_0042f9a0*)&g_game->flags_37f06).switchAlt = 0;
        SettingSetInt((void*)"Total Annihilation", (void*)"SwitchAlt", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).switchAlt);
    }
    value = 0xb;
    if (SettingGetString((void*)"Total Annihilation", (void*)"Password", g_game->password_2be3_1176a, &value) == 0) {
        g_game->password_2be3_1176a[0] = 0;
    }
    if (IsOnlineConfigLoaded() != 0 && g_onlineLobbyPlayerName[0] != 0) {
        g_game->nickname[0] = 0;
        strncat(g_game->nickname, g_onlineLobbyPlayerName, 0x10);
    } else {
        value = 0x11;
        if (SettingGetString((void*)"Total Annihilation", (void*)"Nickname", g_game->nickname, &value) == 0) {
            g_game->nickname[0] = 0;
        }
    }
    if (g_cmdlineHostGameName[0] != 0) {
        g_game->gameName_2bc1[0] = 0;
        strncat(g_game->gameName_2bc1, g_cmdlineHostGameName, 0x10);
    } else {
        value = 0x11;
        if (SettingGetString((void*)"Total Annihilation", (void*)"Game Name", g_game->gameName_2bc1, &value) == 0) {
            g_game->gameName_2bc1[0] = 0;
        }
    }
    value = 0x100;
    if (SettingGetString((void*)"Total Annihilation", (void*)"Image Output Directory",
                     g_game->imageOutputDirectory, &value) == 0) {
        if (GetWindowsUserName(buf) == 0) {
            strcpy(buf, "user_images");
        }
        sprintf(g_game->imageOutputDirectory, "%s\\%s", ((char*)g_game->unknown_c) + 0x628, buf);
    }
    ReadSettingIntOr("Movie Output Rate", &Game::movieOutputRate, 10, value);
    ReadSettingIntOr("textlines", &Game::textLines, 10, value);
    ReadSettingIntOr("textscroll", &Game::textScroll, 10, value);
    ReadSettingIntOr("mousespeed", &Game::mousespeed, 10, value);
    if (SettingGetInt((void*)"Total Annihilation", (void*)"gamespeed", &value) != 0) {
        g_game->gameSpeed = (unsigned short)value;
    } else {
        g_game->gameSpeed = 10;
    }
    g_game->effectiveGameSpeed = (short)(g_game->gameSpeed);
    if (SettingGetInt((void*)"Total Annihilation", (void*)"unitchat", &value) != 0) {
        g_game->unitChat = (unsigned char)value;
    } else {
        g_game->unitChat = 10;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"unitchattext", &value) != 0) {
        g_game->unitChatText = (unsigned char)value;
    } else {
        g_game->unitChatText = 5;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"musicmode", &value) != 0) {
        (*(struct MusicFlags_0042f9a0*)&g_game->flags_37f14_37f14).musicmode = value;
    } else {
        (*(struct MusicFlags_0042f9a0*)&g_game->flags_37f14_37f14).musicmode = 1;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"cdmode", &value) != 0) {
        g_game->cdMode = (unsigned char)value;
    } else {
        g_game->cdMode = 4;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"ackfx", &value) != 0) {
        (*(struct SoundFlags*)&g_game->soundFlags).ackfx = value;
    } else {
        (*(struct SoundFlags*)&g_game->soundFlags).ackfx = 1;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"buildfx", &value) != 0) {
        (*(struct SoundFlags*)&g_game->soundFlags).buildfx = value;
    } else {
        (*(struct SoundFlags*)&g_game->soundFlags).buildfx = 1;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"speechfx", &value) != 0) {
        (*(struct SoundFlags*)&g_game->soundFlags).speechfx = value;
    } else {
        (*(struct SoundFlags*)&g_game->soundFlags).speechfx = 1;
    }
    ReadSettingIntOr("fxvol", &Game::volume1, 0x1b, value);
    ReadSettingIntOr("musicvol", &Game::volume2, 0x20, value);
    if (SettingGetInt((void*)"Total Annihilation", (void*)"clock", &value) != 0) {
        g_game->uiOptionFlags_37f2f.clock = value;
    } else {
        g_game->uiOptionFlags_37f2f.clock = 0;
    }
    if (SettingGetInt((void*)"Total Annihilation", (void*)"NumSkirmishPlayers", &value) != 0) {
        if (value > 1 && value <= 10) {
            g_game->playerCount = value;
        } else {
            g_game->playerCount = value;
        }
    } else {
        g_game->playerCount = 4;
    }
    ReadSettingInt("MultiCommanderDeath", &Game::multiCommanderDeath, 1, value);
    ReadSettingInt("MultiMapping", &Game::multiMapping, 1, value);
    ReadSettingInt("MultiLineOfSight", &Game::multiLineOfSight, 1, value);
    ReadSettingInt("MultiLOSType", &Game::multiLOSType, 1, value);
    ReadSettingInt("SkirmishCommanderDeath", &Options::skirmishCommanderDeath, 1, value);
    ReadSettingInt("SkirmishMapping", &Options::skirmishMapping, 1, value);
    ReadSettingInt("SkirmishLineOfSight", &Options::skirmishLineOfSight, 1, value);
    ReadSettingInt("SkirmishLOSType", &Options::skirmishLOSType, 1, value);
    if (SettingGetInt((void*)"Total Annihilation", (void*)"SkirmishDifficulty", &value) != 0) {
        ((Options*)g_game->options)->skirmishDifficulty = value & 0xffff;
    } else {
        ((Options*)g_game->options)->skirmishDifficulty = 1;
        SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishDifficulty",
                     ((Options*)g_game->options)->skirmishDifficulty);
    }
    ReadSettingInt("SkirmishLocation", &Options::fixedLocations, 1, value);
    value = 0x100;
    if (SettingGetString((void*)"Total Annihilation", (void*)"SkirmishMap",
                     ((Options*)g_game->options)->skirmishMap, &value) == 0) {
        g_seams.map->SetMissionType(2);
        g_seams.map->RefreshMapList(g_game->net, 0, 0);
        strncpy(((Options*)g_game->options)->skirmishMap,
                g_seams.map->GetMissionName(g_game->net), 0x100);
        g_seams.map->SetMissionType(0);
        SettingSetString((void*)"Total Annihilation", (void*)"SkirmishMap", ((Options*)g_game->options)->skirmishMap);
    }
    for (i = 0; i < g_game->playerCount; i++) {
        wsprintfA(name, "Player%dController", i);
        ReadPlayerSetting(name, &SkirmishPlayer::controller, 0, value, i);
        wsprintfA(name, "Player%dSide", i);
        ReadPlayerSetting(name, &SkirmishPlayer::side, i % 2, value, i);
        wsprintfA(name, "Player%dColor", i);
        ReadPlayerSetting(name, &SkirmishPlayer::color, i, value, i);
        wsprintfA(name, "Player%dAllyGroup", i);
        ReadPlayerSetting(name, &SkirmishPlayer::allyGroup, 5, value, i);
        wsprintfA(name, "Player%dMetal", i);
        ReadPlayerSetting(name, &SkirmishPlayer::metal, 1000, value, i);
        wsprintfA(name, "Player%dEnergy", i);
        ReadPlayerSetting(name, &SkirmishPlayer::energy, 1000, value, i);
    }
    int ok26 = SettingGetInt((void*)"Total Annihilation", (void*)"PlayMovie", &value);
    if (ok26 != 0) {
        g_game->playMovie = value;
    } else {
        g_game->playMovie = 1;
    }
    int ok27 = SettingGetInt((void*)"Total Annihilation", (void*)"DisplaymodeDepth", &value);
    if (ok27 == 0) {
        value = 0;
    }
    if (value == 0x100) {
        int ok28 = SettingGetInt((void*)"Total Annihilation", (void*)"Games", &value);
        if (ok28 == 0) value=0;
        if (value == 1) g_game->uiOptionFlags_37f2f.bit1=1;
        else g_game->uiOptionFlags_37f2f.bit1=0;
    } else g_game->uiOptionFlags_37f2f.bit1=0;
label_430e7f:
    g_game->uiOptionFlags_37f2f.bit2 = 1;
    g_game->uiOptionFlags_37f2f.bit3 = 1;
    g_game->uiOptionFlags_37f2f.bit4 = 0;
    int ok29 = SettingGetInt((void*)"Total Annihilation", (void*)"AllMissions", &value);
    if (ok29 != 0) {
        g_game->allMissionsFlags.allMissions = value;
        return;
    }
    g_game->allMissionsFlags.allMissions = 0;
}

// Writes every option back out to the config tree: the display, sound and
// render settings under the "Total Annihilation" section, the strings
// (password, nickname, game name, image output directory, skirmish map), and
// the per-player skirmish settings under "Total Annihilation\Skirmish".
// FUNCTION: 0x430f00
void SaveSettings()
{
    char name[32];
    int i;

    SettingSetInt((void*)"Total Annihilation", (void*)"Interface Type", g_game->interfaceType);
    SettingSetInt((void*)"Total Annihilation", (void*)"DisplaymodeWidth", g_game->width_37f1b);
    SettingSetInt((void*)"Total Annihilation", (void*)"DisplaymodeHeight", g_game->height_37f1f);
    SettingSetInt((void*)"Total Annihilation", (void*)"side", g_game->flag_37ef2);
    SettingSetInt((void*)"Total Annihilation", (void*)"FixedLocations", ((Options*)g_game->options)->fixedLocations);
    SettingSetInt((void*)"Total Annihilation", (void*)"scrollspeed", g_game->scrollSpeed);
    SettingSetInt((void*)"Total Annihilation", (void*)"SingleCommanderDeath", g_game->singleCommanderDeath);
    SettingSetInt((void*)"Total Annihilation", (void*)"SingleMapping", g_game->singleMapping);
    SettingSetInt((void*)"Total Annihilation", (void*)"SingleLineOfSight", g_game->singleLineOfSight);
    SettingSetInt((void*)"Total Annihilation", (void*)"SingleLOSType", g_game->singleLOSType);
    SettingSetInt((void*)"Total Annihilation", (void*)"screenchat", g_game->screenchat);
    SettingSetInt((void*)"Total Annihilation", (void*)"damagebars", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).damagebars);
    SettingSetInt((void*)"Total Annihilation", (void*)"Sound Mode", (*(struct SoundFlags*)&g_game->soundFlags).soundMode);
    SettingSetInt((void*)"Total Annihilation", (void*)"RestoreVolume", (*(struct SoundFlags*)&g_game->soundFlags).restoreVolume);
    SettingSetInt((void*)"Total Annihilation", (void*)"MixingBuffers",
                 ((Sound*)g_game->field_10)->GetMaxBuffers());
    if ((*(struct SoundFlags*)&g_game->soundFlags).restoreVolume) {
        SettingSetInt((void*)"Total Annihilation", (void*)"WaveOutVolume",
                     ((Sound*)g_game->field_10)->QueryWaveVolume());
        SettingSetInt((void*)"Total Annihilation", (void*)"CDAudioVolume",
                     ((Sound*)g_game->field_10)->QueryAuxVolume());
    }
    SettingSetInt((void*)"Total Annihilation", (void*)"Anti-Alias", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).antiAlias);
    SettingSetInt((void*)"Total Annihilation", (void*)"Shadows", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).shadows);
    SettingSetInt((void*)"Total Annihilation", (void*)"FeatureShadows", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).featureShadows);
    SettingSetInt((void*)"Total Annihilation", (void*)"VehicleShadows", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).vehicleShadows);
    SettingSetInt((void*)"Total Annihilation", (void*)"Shading", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).shading);
    SettingSetInt((void*)"Total Annihilation", (void*)"DitheredFog", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).ditheredFog);
    SettingSetInt((void*)"Total Annihilation", (void*)"Difficulty", g_game->difficulty);
    SettingSetInt((void*)"Total Annihilation", (void*)"Gamma", g_game->brightness);
    SettingSetInt((void*)"Total Annihilation", (void*)"SwitchAlt", (*(struct Flags_0042f9a0*)&g_game->flags_37f06).switchAlt);
    SettingSetString((void*)"Total Annihilation", (void*)"Password", g_game->password_2be3_1176a);
    SettingSetString((void*)"Total Annihilation", (void*)"Nickname", g_game->nickname);
    SettingSetString((void*)"Total Annihilation", (void*)"Game Name", g_game->gameName_2bc1);
    if (g_game->valueSet) {
        SettingSetString((void*)"Total Annihilation", (void*)"Image Output Directory",
                     g_game->imageOutputDirectory);
        g_game->valueSet = 0;
    }
    if (g_game->valueSet_38c63) {
        SettingSetInt((void*)"Total Annihilation", (void*)"Movie Output Rate", g_game->movieOutputRate);
        g_game->valueSet_38c63 = 0;
    }
    SettingSetInt((void*)"Total Annihilation", (void*)"unitchat", g_game->unitChat);
    SettingSetInt((void*)"Total Annihilation", (void*)"unitchattext", g_game->unitChatText);
    SettingSetInt((void*)"Total Annihilation", (void*)"textlines", g_game->textLines);
    SettingSetInt((void*)"Total Annihilation", (void*)"textscroll", g_game->textScroll);
    SettingSetInt((void*)"Total Annihilation", (void*)"mousespeed", g_game->mousespeed);
    SettingSetInt((void*)"Total Annihilation", (void*)"gamespeed", g_game->gameSpeed);
    SettingSetInt((void*)"Total Annihilation", (void*)"clock", g_game->uiOptionFlags_37f2f.clock);
    SettingSetInt((void*)"Total Annihilation", (void*)"musicmode", g_game->musicMode & 1);
    SettingSetInt((void*)"Total Annihilation", (void*)"cdmode", g_game->cdMode);
    SettingSetInt((void*)"Total Annihilation", (void*)"ackfx", (*(struct SoundFlags*)&g_game->soundFlags).ackfx);
    SettingSetInt((void*)"Total Annihilation", (void*)"buildfx", (*(struct SoundFlags*)&g_game->soundFlags).buildfx);
    SettingSetInt((void*)"Total Annihilation", (void*)"speechfx", (*(struct SoundFlags*)&g_game->soundFlags).speechfx);
    SettingSetInt((void*)"Total Annihilation", (void*)"fxvol", g_game->volume1);
    SettingSetInt((void*)"Total Annihilation", (void*)"musicvol", g_game->volume2);
    SettingSetInt((void*)"Total Annihilation", (void*)"MultiCommanderDeath", g_game->multiCommanderDeath);
    SettingSetInt((void*)"Total Annihilation", (void*)"MultiMapping", g_game->multiMapping);
    SettingSetInt((void*)"Total Annihilation", (void*)"MultiLineOfSight", g_game->multiLineOfSight);
    SettingSetInt((void*)"Total Annihilation", (void*)"MultiLOSType", g_game->multiLOSType);
    SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishCommanderDeath",
                 ((Options*)g_game->options)->skirmishCommanderDeath);
    SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishMapping", ((Options*)g_game->options)->skirmishMapping);
    SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishLineOfSight",
                 ((Options*)g_game->options)->skirmishLineOfSight);
    SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishLOSType", ((Options*)g_game->options)->skirmishLOSType);
    SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishLocation", ((Options*)g_game->options)->fixedLocations);
    SettingSetInt((void*)"Total Annihilation", (void*)"SkirmishDifficulty",
                 ((Options*)g_game->options)->skirmishDifficulty);
    SettingSetString((void*)"Total Annihilation", (void*)"SkirmishMap", ((Options*)g_game->options)->skirmishMap);
    for (i = 0; i < g_game->playerCount; i++) {
        wsprintfA(name, "Player%dController", i);
        SettingSetInt((void*)"Total Annihilation\\Skirmish", name,
                     ((Options*)g_game->options)->players[i].controller);
        wsprintfA(name, "Player%dSide", i);
        SettingSetInt((void*)"Total Annihilation\\Skirmish", name, ((Options*)g_game->options)->players[i].side);
        wsprintfA(name, "Player%dColor", i);
        SettingSetInt((void*)"Total Annihilation\\Skirmish", name, ((Options*)g_game->options)->players[i].color);
        wsprintfA(name, "Player%dAllyGroup", i);
        SettingSetInt((void*)"Total Annihilation\\Skirmish", name,
                     ((Options*)g_game->options)->players[i].allyGroup);
        wsprintfA(name, "Player%dMetal", i);
        SettingSetInt((void*)"Total Annihilation\\Skirmish", name, ((Options*)g_game->options)->players[i].metal);
        wsprintfA(name, "Player%dEnergy", i);
        SettingSetInt((void*)"Total Annihilation\\Skirmish", name,
                     ((Options*)g_game->options)->players[i].energy);
    }
    SettingSetInt((void*)"Total Annihilation", (void*)"PlayMovie", g_game->playMovie);
}

// Applies the unit-type restrictions listed in the embedded list entry that
// Mission returns for index 6: clears bit 23 (0x800000) on every unit
// type, then sets it on each type named by the entry.
// FUNCTION: 0x431740
void ApplyUseOnlyUnits()
{
    TdfFile parser;
    char name[256];
    char* file = g_seams.map->GetNameSlot(g_game->net, 6);
    if (file == 0)
        return;
    if (!parser.LoadFile(file))
        return;
    {
        ProtectBlockReadWrite(g_game->defs);
        for (int i = 1; i < g_game->count_1438f; i++)
            g_game->defs[i].flags1 &= 0xff7fffff;
        parser.ResetCurrentRecord();
        for (int j = 0; parser.SelectRecordAt(j); j++, parser.ResetCurrentRecord()) {
            parser.current->CopyRecordName(name, 0x100);
            for (int k = 0; k < g_game->count_1438f; k++) {
                if (_strcmpi(g_game->defs[k].unitname, name) == 0) {
                    g_game->defs[k].flags1 |= 0x800000;
                    break;
                }
            }
        }
        ProtectBlockReadOnly(g_game->defs);
    }
}

// Decompiled by Opus, space-bunny-free, DeepSeek V4.1 Flash and Sonnet. Names are provisional.
// The game file a saved game is written to and read from (a HapiBank named
// "Total Annihilation 3.0": a game summary plus each subsystem's state), and
// the campaign name and a few settings saved in the registry.
#include <stdio.h>
#include <string.h>

#include "../util/hapi_bank.h"
#include "../../interfaces/seams.h"
#include "mission_conditions.h"

class Mission;

#pragma pack(push, 1)
#include "../network/player_info.h"

struct PlayerEntry_004326b0 {
    PlayerInfo* info;                  // +0x00
    char unknown_4[0x14b - 4];
};

struct Options_004326b0 {
    char unknown_0[0x108];
    int commanderDeath;                // +0x108
    int mapping;                       // +0x10c
    int lineOfSight;                   // +0x110
    int lineOfSightType;               // +0x114
    int location;                      // +0x118
};

#include "game.h"
#pragma pack(pop)

#include "../map/mission.h"

extern Game* g_game;
extern char* g_saveBankName;          // "Total Annihilation 3.0"
extern char* g_summaryAccountName;    // "Summary"
extern char g_buildDateFormat[];      // "BUILD DATE: %s"
extern char g_buildTimeFormat[];      // "BUILD TIME: %s"
extern char g_maxUnitsKey[];          // "maxunits"
extern char g_campaignKey[];          // "Campaign"
extern char g_missionKey[];           // "Mission"
extern char g_mapKey[];               // "Map"
extern char g_difficultyKey[];        // "Difficulty"
extern char g_sideKey[];              // "Side"
extern char g_playersKey[];           // "Players"
extern char g_gameTypeKey[];          // "Gametype"
extern char g_thumbsKey[];            // "Thumbs"
extern char g_commanderDeathKey[];    // "CommanderDeath"
extern char g_locationKey[];          // "Location"
extern char g_mappingKey[];           // "Mapping"
extern char g_lineOfSightKey[];       // "LineOfSight"
extern char g_lineOfSightTypeKey[];   // "LineOfSightType"
extern char g_betweenMissionsKey[];   // "BetweenMissions"
extern char g_descriptionKey[];       // "Description"
extern char g_gameIdKey[];            // "Game ID"
extern char g_gameTimeKey[];          // "Game Time"
extern char g_radarImageBoxName[];    // "Radar Image"

void __stdcall ReadCameraPosition(HapiBank* file);

void __stdcall SaveSurface(void* surface, void* file);
void __stdcall WriteCameraPosition(void* file);

extern int __stdcall ReadRegistryData(const char* app, const char* key, char* buf, int* size);
extern void __stdcall WriteRegistryString(const char* app, const char* key, const char* value);
extern int __stdcall WriteRegistryDword(const char* param1, const char* param2, int param3);

// Allocates a 4-byte object, initialises it, and opens it with the given
// name and two global strings; frees it and returns 0 on failure.
// FUNCTION: 0x432520
HapiBank* __stdcall OpenSummaryBank(char* name)
{
    HapiBank* mem = new HapiBank;
    HapiBank* obj;
    if (mem != 0) {
        obj = mem->InitBank();
    } else {
        obj = 0;
    }
    if (obj->OpenBank(name, g_saveBankName, g_summaryAccountName) == 0) {
        if (obj != 0) {
            obj->CloseBank();
            delete obj;
        }
        return 0;
    }
    obj->OpenAccount(g_summaryAccountName);
    return obj;
}

// Deletes an object whose (out-of-line) destructor is CloseBank.
// FUNCTION: 0x432590
void __stdcall FreeSummaryBank(HapiBank* obj)
{
    if (obj) {
        obj->CloseBank();
        operator delete(obj);
    }
}

// Allocates a 4-byte object, initialises it, and opens it with the given
// name and a global string; frees it and returns 0 on failure.
// Sibling of 0x432520.
// FUNCTION: 0x4325b0
HapiBank* __stdcall OpenHapiBank(char* name)
{
    HapiBank* mem = new HapiBank;
    HapiBank* obj;
    if (mem != 0) {
        obj = mem->InitBank();
    } else {
        obj = 0;
    }
    if (obj->OpenBank(name, g_saveBankName, 0) == 0) {
        if (obj != 0) {
            obj->CloseBank();
            delete obj;
        }
        return 0;
    }
    return obj;
}

#include "../../interfaces/seams.h"

// Reads the game summary section and every subsystem's saved state.
// FUNCTION: 0x432610
int __stdcall LoadSavedGameState(HapiBank* file)
{
    file->OpenAccount(g_summaryAccountName);
    if (file->HasItem("maxunits"))
        g_game->maxUnits_37eec = file->GetIntegerItem("maxunits", 0);
    g_seams.players->LoadPlayers(file);
    ReadCameraPosition(file);
    g_seams.features->LoadFeatures(file);
    g_seams.map->LoadMetalPlotmap(file);
    g_seams.map->LoadPlayerFeaturesPlotmap(file);
    g_seams.map->LoadMappingData(file);
    g_seams.units->LoadUnits(file);
    g_seams.wind_meteors->LoadMeteors(file);
    g_seams.players->LoadConditions(g_game->list_391ed, 0, file);
    g_game->bits_37ebe_2.loaded = 1;
    return 1;
}

// Writes the game summary file named by argument 1 (the "Summary" section of
// a freshly built parse tree, plus the radar image and every subsystem's save
// state when the game is in state 6) and hands it to SaveBank, which
// writes the save file and returns its result. Argument 2 is an optional
// description string, argument 3 the game id.
// FUNCTION: 0x4326b0
int __stdcall SaveGameFile(char* path, char* description, int gameId)
{
    HapiBank file;
    char buf[256];

    (&file)->InitBank();
    file.NewBank();
    (&file)->OpenAccount(g_summaryAccountName);

    sprintf(buf, g_buildDateFormat, g_game->build_date);
    (&file)->SetIntegerItem(buf, 0);
    sprintf(buf, g_buildTimeFormat, g_game->build_time);
    (&file)->SetIntegerItem(buf, 0);
    (&file)->SetIntegerItem(g_maxUnitsKey, g_game->maxUnits_37eec);
    (&file)->SetStringItem(g_campaignKey, g_seams.map->GetCampaignName(g_game->net));
    if (g_game->mode_391f1 != 6) {
        g_seams.map->AdvanceMission(g_game->net);
    }
    (&file)->SetStringItem(g_missionKey, g_seams.map->GetMissionName(g_game->net));
    (&file)->SetStringItem(g_mapKey, g_seams.map->GetMissionName(g_game->net));
    (&file)->SetIntegerItem(g_difficultyKey, g_game->difficulty);
    (&file)->SetIntegerItem(g_sideKey, (*(PlayerEntry_004326b0(*)[10])&g_game->players_1b8a)[g_game->localPlayer].info->side);
    (&file)->SetIntegerItem(g_playersKey, g_game->field_2a3c);
    (&file)->SetIntegerItem(g_gameTypeKey, g_seams.map->GetGameType(g_game->net));
    (&file)->SetStringItem(g_thumbsKey, g_game->campaignMissionStatus_391cf);
    if (g_seams.map->GetGameType(g_game->net) == 2) {
        (&file)->SetIntegerItem(g_commanderDeathKey, ((Options_004326b0*)g_game->options)->commanderDeath);
        (&file)->SetIntegerItem(g_locationKey, ((Options_004326b0*)g_game->options)->location);
        (&file)->SetIntegerItem(g_mappingKey, ((Options_004326b0*)g_game->options)->mapping);
        (&file)->SetIntegerItem(g_lineOfSightKey, ((Options_004326b0*)g_game->options)->lineOfSight);
        (&file)->SetIntegerItem(g_lineOfSightTypeKey, ((Options_004326b0*)g_game->options)->lineOfSightType);
    }
    if (g_game->mode_391f1 != 6) {
        (&file)->SetIntegerItem(g_betweenMissionsKey, 1);
        g_seams.map->SelectMission(g_game->net, 0, g_game->mission);
    }
    if (description != 0) {
        (&file)->SetStringItem(g_descriptionKey, description);
    }
    (&file)->SetIntegerItem(g_gameIdKey, gameId);
    (&file)->SetIntegerItem(g_gameTimeKey, g_game->ticks);
    if (g_game->mode_391f1 == 6) {
        (&file)->OpenNamedBox(g_radarImageBoxName);
        SaveSurface(g_game->finalSurface, &file);
        WriteCameraPosition(&file);
        g_seams.players->SavePlayers(&file);
        g_seams.units->SaveUnits(&file);
        g_seams.map->SaveMappingData(&file);
        g_seams.features->SaveFeatures(&file);
        g_seams.map->SavePlayerFeaturesPlotmap(&file);
        g_seams.map->SaveMetalPlotmap(&file);
        g_seams.wind_meteors->SaveMeteors(&file);
        g_seams.players->SaveConditions(g_game->list_391ed, 0, &file);
    }
    int result = file.SaveBank(path, g_saveBankName, 1, 0);
    (&file)->CloseBank();
    return result;
}

// FUNCTION: 0x432a80
void __stdcall LoadCampaignRegistryName(int core, char* name)
{
    char key[64];
    if (core)
        strcpy(key, "CoreCamp");
    else
        strcpy(key, "ArmCamp");
    core = 26; // the original reuses the dead first argument slot for the size
    if (ReadRegistryData("Total Annihilation", key, name, &core) == 0) {
        memset(name, 0x55, 0x19);
        name[0x19] = 0;
    }
}

// FUNCTION: 0x432b00
void __stdcall SaveCampaignRegistryName(int core, char* name)
{
    char key[64];
    if (core)
        strcpy(key, "CoreCamp");
    else
        strcpy(key, "ArmCamp");
    name[0x19] = 0;
    WriteRegistryString("Total Annihilation", key, name);
}

// Saves the "AllMissions" flag (bit 0 of g_game+0x38d7f) to the registry.
// FUNCTION: 0x432b60
void SaveAllMissionsSetting()
{
    WriteRegistryDword("Total Annihilation", "AllMissions", g_game->flags_38d7f & 1);
}

// FUNCTION: 0x432b80
void SaveNumSkirmishPlayers()
{
    WriteRegistryDword("Total Annihilation", "NumSkirmishPlayers", g_game->playerCount);
}

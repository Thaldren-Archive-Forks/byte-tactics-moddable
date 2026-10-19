// Mission: one campaign or map (Thaldren's MapInfo, 0xec4 bytes), the object
// at g_game+0x391e9. The one declaration of the class: map_list.cpp defines
// the methods and the files that call them include this.
#ifndef MISSION_H
#define MISSION_H

#include "../util/tdf.h"

struct MissionRule;
struct Vec3_00437320;

struct MissionUnit {
    char* name;                        // +0x0
    char* ident;                       // +0x4
    char* initialMission;              // +0x8
    int x;                             // +0xc
    int y;                             // +0x10
    int z;                             // +0x14
    short angle;                       // +0x18
    short health;                      // +0x1a
    int creationCountdown;             // +0x1c
    short buildPriority;               // +0x20
    unsigned char player;              // +0x22
    union {
        struct {
            unsigned char initialGroup : 4;    // +0x23
            unsigned char missionCritical : 1;
            unsigned char aiIgnore : 1;
            unsigned char aiPriorityTarget : 1;
            unsigned char immunity : 1;
        };
        unsigned char flags;
    };
};

struct MissionFeature {
    char name[0x80];
    int x;                             // +0x80
    int y;                             // +0x84
};

class Mission {
public:
    int type;                          // +0x000
    char campaign[0x100];              // +0x004
    char names[9][0x100];              // +0x104
    int exists;                        // +0xa04
    TdfFile list;                      // +0xa08, the campaign file
    char missionName[0x100];           // +0xa14
    char text_b14[0x100];              // +0xb14
    char* briefing;                    // +0xc14
    int missionIndex;                  // +0xc18
    int tntChecksum;                   // +0xc1c
    int headerChecksum;                // +0xc20
    char description[0x80];            // +0xc24
    char planet[0x80];                 // +0xca4
    char* mapList;                     // +0xd24
    int mapCount;                      // +0xd28
    int multi;                         // +0xd2c
    int surfaceMetal;                  // +0xd30
    int minWindSpeed;                  // +0xd34
    int maxWindSpeed;                  // +0xd38
    int gravity;                       // +0xd3c
    float tidalStrength;               // +0xd40
    int lavaWorld;                     // +0xd44
    int noSeaLevelTrigger;             // +0xd48
    int waterDoesDamage;               // +0xd4c
    int waterDamage;                   // +0xd50
    float killMul;                     // +0xd54
    float timeMul;                     // +0xd58
    float startMetal[10];              // +0xd5c, per player
    float startEnergy[10];             // +0xd84, per player
    MissionUnit* units;                // +0xdac
    int unitCount;                     // +0xdb0
    MissionRule* rules;                // +0xdb4
    int ruleCount;                     // +0xdb8
    MissionFeature* features;          // +0xdbc
    int featureCount;                  // +0xdc0
    char memory[0x80];                 // +0xdc4
    char numPlayers[0x80];             // +0xe44

    Mission(int owner_);
    ~Mission();
    int GetGameType();
    void LoadCampaign(char* file);
    char* GetCampaignName();
    void LoadBriefing();
    char* GetBriefing();
    void SetNameSlot(int index, char* text);
    void BuildCampaignFilePath(int index, char* dir, char* name, char* ext);
    char* GetNameSlot(int index);
    int CountMissions();
    int BuildMissionList(char** out);
    int GetTerrainLength();
    char* GetDescription();
    char* GetPlanet();
    int GetTerrainSizeTier();
    int MissionExists(int index);
    int LoadMissionByName(char* map);
    int SelectMission(int index);
    char* GetTranslatedName();
    char* GetMissionName();
    bool HasMissionName();
    int GetMissionIndex();
    int AdvanceMission();
    void RefreshMapList(int multiplayer);
    int LoadMission(char* map);
    int SelectSchema(int type, TdfFile* parser, char* schema);
    void LoadMissionData(char* name, TdfFile* parser);
    void FreeMissionData();
    int CountStartPositions();
    int GetStartPosition(Vec3_00437320* out, int id);
    int ComputeMapChecksum();
};

#endif

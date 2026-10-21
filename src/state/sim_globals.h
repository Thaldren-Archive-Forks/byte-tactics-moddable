#ifndef STATE_SIM_GLOBALS_H
#define STATE_SIM_GLOBALS_H

#pragma pack(push, 8)

#include "win_types.h"
#include <stddef.h>
#include <memory>
#include "block.h"
#include "../graphics/memory_cache.h"
#include "../map/los_tables.h"
#include "../map/vector_00438480.h"
#include "../network/packet_manager.h"
#include "../quirks/quirks.h"
#include "../units/movement_class.h"
#include "../util/vec3.h"

struct ExplodedPiece;
struct PlayerAI;
struct WeaponDef;

struct SimGlobals {
    int g_aiCommandsEnabled = 1;
    char DAT_005119b8[8];
    PlayerAI* g_playerAI[10];
    int g_playerBudgetCap[10];
    int g_playerTickLoad[10];
    int g_budgetRefreshCounter;
    unsigned char DAT_00511a3c[4];
    int g_cdPathMismatch;
    ExplodedPiece* g_explodedPieces[100];
    CMemoryCache g_debrisMemCache;
    unsigned char DAT_00511fb4[4];
    int g_meteorNextStrikeTime;
    unsigned char DAT_005122ec[4];
    char g_meteorWeaponName[32];
    int g_meteorSpawnMagnitude;
    int g_meteorSpawnInterval;
    int g_meteorActive;
    int g_meteorStrikeEndTime;
    Point16 g_meteorOrigin;
    int g_meteorShowerDuration;
    WeaponDef* g_meteorWeapon;
    int g_meteorsEnabled;
    int g_meteorNextHitTime;
    Point16 g_meteorTarget;
    int g_meteorScheduleGap;
    unsigned char DAT_0051233c[4];
    Vector_00438480<int> g_missionOrderTableVec;
    unsigned char DAT_00512350[8];
    MovementClassTable g_movementClasses;
    unsigned char DAT_00512758[8];
    int (__stdcall* g_packetHandlers[45])(int);
    unsigned char DAT_00512ad4[4];
    int g_packetSizes[45];
    unsigned char DAT_00512b8c[4];
    int g_packetModes[45];
    PacketManager g_packetManager;
    int g_economyPacketTick;
    int g_lastPlaceHeight;
    int g_lastPlaceMetalSum;
    LosTables g_losTables;
    unsigned char DAT_0051e6b0[4];
    int g_mapLoadFlag;
    unsigned int g_cdMismatchDefeatTick;
    unsigned int g_randomSeed;
    unsigned char DAT_0051fc8c[4];
    Quirks g_quirks;
};

SimGlobals& SimCreate();

extern SimGlobals* g_simGlobals;

inline SimGlobals& Sim()
{
    return g_simGlobals != 0 ? *g_simGlobals : SimCreate();
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
static_assert(__builtin_offsetof(SimGlobals, g_playerAI) - __builtin_offsetof(SimGlobals, DAT_005119b8) == 8, "DAT_005119b8 is followed by g_playerAI");
static_assert(__builtin_offsetof(SimGlobals, g_playerBudgetCap) - __builtin_offsetof(SimGlobals, g_playerAI) == 40, "g_playerAI is followed by g_playerBudgetCap");
static_assert(__builtin_offsetof(SimGlobals, g_playerTickLoad) - __builtin_offsetof(SimGlobals, g_playerBudgetCap) == 40, "g_playerBudgetCap is followed by g_playerTickLoad");
static_assert(__builtin_offsetof(SimGlobals, g_budgetRefreshCounter) - __builtin_offsetof(SimGlobals, g_playerTickLoad) == 40, "g_playerTickLoad is followed by g_budgetRefreshCounter");
static_assert(__builtin_offsetof(SimGlobals, DAT_00511a3c) - __builtin_offsetof(SimGlobals, g_budgetRefreshCounter) == 4, "g_budgetRefreshCounter is followed by DAT_00511a3c");
static_assert(__builtin_offsetof(SimGlobals, g_debrisMemCache) - __builtin_offsetof(SimGlobals, g_explodedPieces) == 400, "g_explodedPieces is followed by g_debrisMemCache");
static_assert(__builtin_offsetof(SimGlobals, DAT_005122ec) - __builtin_offsetof(SimGlobals, g_meteorNextStrikeTime) == 4, "g_meteorNextStrikeTime is followed by DAT_005122ec");
static_assert(__builtin_offsetof(SimGlobals, g_meteorWeaponName) - __builtin_offsetof(SimGlobals, DAT_005122ec) == 4, "DAT_005122ec is followed by g_meteorWeaponName");
static_assert(__builtin_offsetof(SimGlobals, g_meteorSpawnMagnitude) - __builtin_offsetof(SimGlobals, g_meteorWeaponName) == 32, "g_meteorWeaponName is followed by g_meteorSpawnMagnitude");
static_assert(__builtin_offsetof(SimGlobals, g_meteorSpawnInterval) - __builtin_offsetof(SimGlobals, g_meteorSpawnMagnitude) == 4, "g_meteorSpawnMagnitude is followed by g_meteorSpawnInterval");
static_assert(__builtin_offsetof(SimGlobals, g_meteorActive) - __builtin_offsetof(SimGlobals, g_meteorSpawnInterval) == 4, "g_meteorSpawnInterval is followed by g_meteorActive");
static_assert(__builtin_offsetof(SimGlobals, g_meteorStrikeEndTime) - __builtin_offsetof(SimGlobals, g_meteorActive) == 4, "g_meteorActive is followed by g_meteorStrikeEndTime");
static_assert(__builtin_offsetof(SimGlobals, g_meteorOrigin) - __builtin_offsetof(SimGlobals, g_meteorStrikeEndTime) == 4, "g_meteorStrikeEndTime is followed by g_meteorOrigin");
static_assert(__builtin_offsetof(SimGlobals, g_meteorShowerDuration) - __builtin_offsetof(SimGlobals, g_meteorOrigin) == 4, "g_meteorOrigin is followed by g_meteorShowerDuration");
static_assert(__builtin_offsetof(SimGlobals, g_meteorWeapon) - __builtin_offsetof(SimGlobals, g_meteorShowerDuration) == 4, "g_meteorShowerDuration is followed by g_meteorWeapon");
static_assert(__builtin_offsetof(SimGlobals, g_meteorsEnabled) - __builtin_offsetof(SimGlobals, g_meteorWeapon) == 4, "g_meteorWeapon is followed by g_meteorsEnabled");
static_assert(__builtin_offsetof(SimGlobals, g_meteorNextHitTime) - __builtin_offsetof(SimGlobals, g_meteorsEnabled) == 4, "g_meteorsEnabled is followed by g_meteorNextHitTime");
static_assert(__builtin_offsetof(SimGlobals, g_meteorTarget) - __builtin_offsetof(SimGlobals, g_meteorNextHitTime) == 4, "g_meteorNextHitTime is followed by g_meteorTarget");
static_assert(__builtin_offsetof(SimGlobals, g_meteorScheduleGap) - __builtin_offsetof(SimGlobals, g_meteorTarget) == 4, "g_meteorTarget is followed by g_meteorScheduleGap");
static_assert(__builtin_offsetof(SimGlobals, DAT_0051233c) - __builtin_offsetof(SimGlobals, g_meteorScheduleGap) == 4, "g_meteorScheduleGap is followed by DAT_0051233c");
static_assert(__builtin_offsetof(SimGlobals, g_missionOrderTableVec) - __builtin_offsetof(SimGlobals, DAT_0051233c) == 4, "DAT_0051233c is followed by g_missionOrderTableVec");
static_assert(__builtin_offsetof(SimGlobals, DAT_00512350) - __builtin_offsetof(SimGlobals, g_missionOrderTableVec) == 16, "g_missionOrderTableVec is followed by DAT_00512350");
static_assert(__builtin_offsetof(SimGlobals, g_movementClasses) - __builtin_offsetof(SimGlobals, DAT_00512350) == 8, "DAT_00512350 is followed by g_movementClasses");
static_assert(__builtin_offsetof(SimGlobals, DAT_00512758) - __builtin_offsetof(SimGlobals, g_movementClasses) == 1024, "g_movementClasses is followed by DAT_00512758");
static_assert(__builtin_offsetof(SimGlobals, DAT_00512ad4) - __builtin_offsetof(SimGlobals, g_packetHandlers) == 180, "g_packetHandlers is followed by DAT_00512ad4");
static_assert(__builtin_offsetof(SimGlobals, g_packetSizes) - __builtin_offsetof(SimGlobals, DAT_00512ad4) == 4, "DAT_00512ad4 is followed by g_packetSizes");
static_assert(__builtin_offsetof(SimGlobals, DAT_00512b8c) - __builtin_offsetof(SimGlobals, g_packetSizes) == 180, "g_packetSizes is followed by DAT_00512b8c");
static_assert(__builtin_offsetof(SimGlobals, g_economyPacketTick) - __builtin_offsetof(SimGlobals, g_packetManager) == 46396, "g_packetManager is followed by g_economyPacketTick");
static_assert(__builtin_offsetof(SimGlobals, g_lastPlaceMetalSum) - __builtin_offsetof(SimGlobals, g_lastPlaceHeight) == 4, "g_lastPlaceHeight is followed by g_lastPlaceMetalSum");
static_assert(__builtin_offsetof(SimGlobals, DAT_0051e6b0) - __builtin_offsetof(SimGlobals, g_losTables) == 16, "g_losTables is followed by DAT_0051e6b0");
static_assert(__builtin_offsetof(SimGlobals, g_cdMismatchDefeatTick) - __builtin_offsetof(SimGlobals, g_mapLoadFlag) == 4, "g_mapLoadFlag is followed by g_cdMismatchDefeatTick");
static_assert(__builtin_offsetof(SimGlobals, DAT_0051fc8c) - __builtin_offsetof(SimGlobals, g_randomSeed) == 4, "g_randomSeed is followed by DAT_0051fc8c");
static_assert(sizeof(((SimGlobals*)0)->g_missionOrderTableVec) == 16, "g_missionOrderTableVec size");
static_assert(sizeof(((SimGlobals*)0)->g_movementClasses) == 1024, "g_movementClasses size");
static_assert(sizeof(((SimGlobals*)0)->g_packetManager) == 46396, "g_packetManager size");
static_assert(sizeof(((SimGlobals*)0)->g_losTables) == 16, "g_losTables size");
#pragma clang diagnostic pop

#pragma pack(pop)

#define g_aiCommandsEnabled (Sim().g_aiCommandsEnabled)
#define DAT_005119b8 (Sim().DAT_005119b8)
#define g_playerAI (Sim().g_playerAI)
#define g_playerBudgetCap (Sim().g_playerBudgetCap)
#define g_playerTickLoad (Sim().g_playerTickLoad)
#define g_budgetRefreshCounter (Sim().g_budgetRefreshCounter)
#define DAT_00511a3c (Sim().DAT_00511a3c)
#define g_cdPathMismatch (Sim().g_cdPathMismatch)
#define g_explodedPieces (Sim().g_explodedPieces)
#define g_debrisMemCache (Sim().g_debrisMemCache)
#define DAT_00511fb4 (Sim().DAT_00511fb4)
#define g_meteorNextStrikeTime (Sim().g_meteorNextStrikeTime)
#define DAT_005122ec (Sim().DAT_005122ec)
#define g_meteorWeaponName (Sim().g_meteorWeaponName)
#define g_meteorSpawnMagnitude (Sim().g_meteorSpawnMagnitude)
#define g_meteorSpawnInterval (Sim().g_meteorSpawnInterval)
#define g_meteorActive (Sim().g_meteorActive)
#define g_meteorStrikeEndTime (Sim().g_meteorStrikeEndTime)
#define g_meteorOrigin (Sim().g_meteorOrigin)
#define g_meteorShowerDuration (Sim().g_meteorShowerDuration)
#define g_meteorWeapon (Sim().g_meteorWeapon)
#define g_meteorsEnabled (Sim().g_meteorsEnabled)
#define g_meteorNextHitTime (Sim().g_meteorNextHitTime)
#define g_meteorTarget (Sim().g_meteorTarget)
#define g_meteorScheduleGap (Sim().g_meteorScheduleGap)
#define DAT_0051233c (Sim().DAT_0051233c)
#define g_missionOrderTableVec (Sim().g_missionOrderTableVec)
#define DAT_00512350 (Sim().DAT_00512350)
#define g_movementClasses (Sim().g_movementClasses)
#define DAT_00512758 (Sim().DAT_00512758)
#define g_packetHandlers (Sim().g_packetHandlers)
#define DAT_00512ad4 (Sim().DAT_00512ad4)
#define g_packetSizes (Sim().g_packetSizes)
#define DAT_00512b8c (Sim().DAT_00512b8c)
#define g_packetModes (Sim().g_packetModes)
#define g_packetManager (Sim().g_packetManager)
#define g_economyPacketTick (Sim().g_economyPacketTick)
#define g_lastPlaceHeight (Sim().g_lastPlaceHeight)
#define g_lastPlaceMetalSum (Sim().g_lastPlaceMetalSum)
#define g_losTables (Sim().g_losTables)
#define DAT_0051e6b0 (Sim().DAT_0051e6b0)
#define g_mapLoadFlag (Sim().g_mapLoadFlag)
#define g_cdMismatchDefeatTick (Sim().g_cdMismatchDefeatTick)
#define g_randomSeed (Sim().g_randomSeed)
#define DAT_0051fc8c (Sim().DAT_0051fc8c)
#define g_quirks (Sim().g_quirks)
typedef unsigned char SimType_DAT_005122f4[28];
#define DAT_005122f4 (*reinterpret_cast<SimType_DAT_005122f4*>(reinterpret_cast<char*>(&g_meteorWeaponName) + 0x4))
typedef char* SimType_g_missionOrderTableBegin;
#define g_missionOrderTableBegin (*reinterpret_cast<SimType_g_missionOrderTableBegin*>(reinterpret_cast<char*>(&g_missionOrderTableVec) + 0x4))
typedef int SimType_g_missionOrderTableEnd;
#define g_missionOrderTableEnd (*reinterpret_cast<SimType_g_missionOrderTableEnd*>(reinterpret_cast<char*>(&g_missionOrderTableVec) + 0x8))
typedef char SimType_DAT_00512370[];
#define DAT_00512370 (*reinterpret_cast<SimType_DAT_00512370*>(reinterpret_cast<char*>(&g_movementClasses) + 0x18))
typedef int (__stdcall* SimType_g_packetHandlerType2)(int);
#define g_packetHandlerType2 (*reinterpret_cast<SimType_g_packetHandlerType2*>(reinterpret_cast<char*>(&g_packetHandlers) + 0x8))
typedef PacketChannel SimType_g_packetChannels[];
#define g_packetChannels (*reinterpret_cast<SimType_g_packetChannels*>(reinterpret_cast<char*>(&g_packetManager) + 0x8))
typedef PacketReceiver SimType_g_packetManagerReceiver;
#define g_packetManagerReceiver (*reinterpret_cast<SimType_g_packetManagerReceiver*>(reinterpret_cast<char*>(&g_packetManager) + 0xb300))

#endif

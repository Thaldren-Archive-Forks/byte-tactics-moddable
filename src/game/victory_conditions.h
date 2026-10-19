#ifndef VICTORY_CONDITIONS_H
#define VICTORY_CONDITIONS_H

#include <string.h>
#include "mission_conditions.h"
#ifndef TA_TYPES_H
#include "../util/vec3.h"
#endif

struct Unit;
class HapiBank;

class Listener_0048ff40 {
public:
    virtual int VisitUnit(Unit* unit) = 0;
};

class Listener_0048f250 {
public:
    virtual void VisitUnit(Unit* unit) = 0;
};

class VictoryKillEnemyCommander : public MissionCondition {
public:
    virtual void OnUnitDied(Unit* unit);
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};

class VictoryDestroyAllUnits : public MissionCondition {
public:
    virtual int IsSatisfied();
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};

class VictoryKillAllMobileUnits : public MissionCondition, public Listener_0048ff40 {
public:
    virtual int VisitUnit(Unit* unit);
    int numUnits;
    VictoryKillAllMobileUnits() { numUnits = 0; }
    virtual void OnUnitDied(Unit* unit);
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};

#pragma pack(push, 2)
class VictoryBuildUnitType : public MissionCondition, public Listener_0048ff40 {
public:
    virtual int VisitUnit(Unit* unit);
    char name[0x20];
    short id;

    VictoryBuildUnitType(const char* text)
    {
        strcpy(name, text);
        id = 0;
    }
    virtual int IsSatisfied();
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};
#pragma pack(pop)

class VictoryCaptureUnitType : public MissionCondition {
public:
    char name[0x20];

    VictoryCaptureUnitType(const char* text) { strcpy(name, text); }
    virtual void OnUnitCaptured(Unit* unit);
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};

#pragma pack(push, 2)
class VictoryKillAllOfType : public MissionCondition, public Listener_0048ff40 {
public:
    virtual int VisitUnit(Unit* unit);
    char name[0x20];
    short id;
    int count;

    VictoryKillAllOfType(const char* text) { strcpy(name, text); }
    virtual void OnUnitDied(Unit* unit);
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};
#pragma pack(pop)

class VictoryKillUnitType : public MissionCondition {
public:
    char name[0x20];
    int numLeftToKill;

    VictoryKillUnitType(const char* text, int n)
    {
        strcpy(name, text);
        numLeftToKill = n;
    }
    virtual void OnUnitDied(Unit* unit);
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};

#pragma pack(push, 2)
class VictoryMoveUnitToRadius : public MissionCondition, public Listener_0048f250 {
public:
    virtual void VisitUnit(Unit* unit);
    char name[0x20];
    Vec3 pos;
    int radius;

    VictoryMoveUnitToRadius(const char* text, int x, int z, int r)
    {
        if (_strcmpi(text, "ANYTYPE") == 0)
            name[0] = 0;
        else
            strcpy(name, text);
        pos.x = x;
        pos.z = z;
        radius = r << 16;

        pos.y = 0x12345678;
    }
    virtual int IsSatisfied();
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};
#pragma pack(pop)

#pragma pack(push, 2)
class VictoryUnitTypePassesX : public MissionCondition, public Listener_0048ff40 {
public:
    virtual int VisitUnit(Unit* unit);
    char name[0x20];
    int x;

    VictoryUnitTypePassesX(const char* text, int v)
    {
        if (_strcmpi(text, "ANYTYPE") == 0)
            name[0] = 0;
        else
            strcpy(name, text);
        x = v >> 4;
    }
    virtual int IsSatisfied();
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};
#pragma pack(pop)

#pragma pack(push, 2)
class VictoryUnitTypePassesZ : public MissionCondition, public Listener_0048ff40 {
public:
    virtual int VisitUnit(Unit* unit);
    char name[0x20];
    int z;

    VictoryUnitTypePassesZ(const char* text, int v)
    {
        if (_strcmpi(text, "ANYTYPE") == 0)
            name[0] = 0;
        else
            strcpy(name, text);
        z = v >> 4;
    }
    virtual int IsSatisfied();
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};
#pragma pack(pop)

class VictoryTimerRunsOut : public MissionCondition {
public:
    unsigned int endTick;

    VictoryTimerRunsOut(int t) { endTick = t * 30; }
    virtual int IsSatisfied();
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};

class DefeatCommanderKilled : public MissionCondition {
public:
    virtual void OnUnitDied(Unit* unit);
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};

class DefeatAllUnitsKilled : public MissionCondition, public Listener_0048ff40 {
public:
    virtual int IsSatisfied();
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
    virtual int VisitUnit(Unit* unit);
};

#pragma pack(push, 2)
class DefeatAllUnitsKilledOfType : public MissionCondition, public Listener_0048ff40 {
public:
    virtual int VisitUnit(Unit* unit);
    char name[0x20];
    short id;
    int count;

    DefeatAllUnitsKilledOfType(const char* text) { strcpy(name, text); }
    virtual void OnUnitDied(Unit* unit);
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};
#pragma pack(pop)

class DefeatUnitTypeKilled : public MissionCondition {
public:
    char name[0x20];
    int numLeftToKill;

    DefeatUnitTypeKilled(const char* text, int n)
    {
        strcpy(name, text);
        numLeftToKill = n;
    }
    virtual void OnUnitDied(Unit* unit);
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};

class DefeatDeathTimerRunsOut : public MissionCondition {
public:
    int endTick;

    DefeatDeathTimerRunsOut(int t) { endTick = t * 30; }
    virtual int IsSatisfied();
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};

class DefeatAnyUnitPassesX : public MissionCondition, public Listener_0048ff40 {
public:
    virtual int VisitUnit(Unit* unit);
    int x;

    DefeatAnyUnitPassesX(int v) { x = v >> 4; }
    virtual int IsSatisfied();
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};

class DefeatAnyUnitPassesZ : public MissionCondition, public Listener_0048ff40 {
public:
    virtual int VisitUnit(Unit* unit);
    int z;

    DefeatAnyUnitPassesZ(int v) { z = v >> 4; }
    virtual int IsSatisfied();
    virtual void SaveState(HapiBank* file);
    virtual void LoadState(HapiBank* file);
};

#endif

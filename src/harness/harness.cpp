#include "win_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <time.h>
#include "../../interfaces/host.h"

#include "../../include/ta_types.h"
#include "../map/mission.h"
#include "condition_class.h"
#include "../../interfaces/seams.h"
#include "../quirks/quirks.h"
#include "../state/sim_globals.h"
#include "block_scan.h"
#include "../state/block.h"

extern Game* g_game;

void __stdcall SetMissionType(int owner);
void __stdcall ResetPlayerSlots(void);
void ApplySlotsToGamePlayers();
void InitMissionStatus();
char IsMemSet();
int GetMemSetValue();

enum { MAX_PLAYERS = 10 };

struct PlayerSpec {
    int named;
    int human;
    char side[64];
    int sideLine;
    int team;
    int color;
    int metal;
    int energy;
};

static int s_testMode = 0;
static unsigned s_gameSeed = 1;
static unsigned s_crtSeed = 1;
static unsigned s_viewSeed = 1;
static unsigned s_viewState = 1;
static unsigned s_fpuControlWord = 0x9001F;
static PlayerSpec s_players[MAX_PLAYERS];
static char s_map[0x104];
static int s_mapLine = 0;
static unsigned s_difficulty = 1;
static unsigned s_commanderDeath = 1;
static unsigned s_mapping = 1;
static unsigned s_lineOfSight = 1;
static unsigned s_losType = 1;
static unsigned s_fixedLocations = 1;
static int s_localPlayer = -1;
static int s_localPlayerLine = 0;
static unsigned s_ticks = 0;
static unsigned s_stepsPerFrame = 1;
static int s_started = 0;
static int s_dumpEnabled = 0;
static int s_blockScan = 0;
static char s_commit[64] = "-";
static char s_scenarioPath[512];
static FILE* s_trace = NULL;
static FILE* s_dump = NULL;
static unsigned s_dumpTick = 0;
static char s_scenarioQuirks[512];
static Quirks s_scenarioQuirkSet;
static char s_switchQuirks[512];
static char s_quirkSpec[512] = "3.1c";
static unsigned s_quirkHash = 0;
static unsigned s_quirkTamperTick = 0;
static int s_quirkTamper = 0;
static unsigned s_quirkReachTick = 0;
static int s_quirkReach = 0;

static void Fail(const char* text)
{
    host_log(text);
    host_exit(2);
}

static void FailLine(int lineNo, const char* what, const char* key, const char* detail)
{
    char msg[900];
    sprintf(msg, "testmode: scenario line %d: %s %s%s\n", lineNo, key, what, detail);
    Fail(msg);
}

static unsigned ParseNumber(int lineNo, const char* key, const char* value)
{
    char* end;
    unsigned number = strtoul(value, &end, 0);
    if (end == value || *end != '\0') {
        char detail[600];
        sprintf(detail, ", got \"%s\"", value);
        FailLine(lineNo, "needs a number", key, detail);
    }
    return number;
}

static unsigned ParseRange(int lineNo, const char* key, const char* value, unsigned max)
{
    unsigned number = ParseNumber(lineNo, key, value);
    if (number > max) {
        char what[64];
        sprintf(what, "must be at most %u", max);
        FailLine(lineNo, what, key, "");
    }
    return number;
}

static void InitPlayerSpecs()
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        PlayerSpec* p = &s_players[i];
        memset(p, 0, sizeof *p);
        p->team = 5;
        p->color = i;
        p->metal = 1000;
        p->energy = 1000;
    }
}

static void ReadPlayerKey(int lineNo, const char* key, const char* value)
{
    int slot = key[6] - '0';
    const char* field = key + 8;
    PlayerSpec* p = &s_players[slot];
    p->named = 1;
    if (strcmp(field, "controller") == 0) {
        if (strcmp(value, "ai") == 0)
            p->human = 0;
        else if (strcmp(value, "human") == 0)
            p->human = 1;
        else
            FailLine(lineNo, "must be ai or human", key, "");
    } else if (strcmp(field, "side") == 0) {
        if (strlen(value) >= sizeof p->side)
            FailLine(lineNo, "is too long", key, "");
        strcpy(p->side, value);
        p->sideLine = lineNo;
    } else if (strcmp(field, "team") == 0) {
        p->team = ParseRange(lineNo, key, value, 5);
    } else if (strcmp(field, "color") == 0) {
        p->color = ParseRange(lineNo, key, value, 9);
    } else if (strcmp(field, "metal") == 0) {
        p->metal = ParseNumber(lineNo, key, value);
    } else if (strcmp(field, "energy") == 0) {
        p->energy = ParseNumber(lineNo, key, value);
    } else {
        FailLine(lineNo, "is not a known key", key, "");
    }
}


enum { MAX_ORDERS = 256, ORDER_LINE_TEXT = 128 };

enum OrderKind { ORDER_MOVE, ORDER_ATTACK, ORDER_BUILD, ORDER_PATROL, ORDER_GUARD, ORDER_RECLAIM, ORDER_KINDS };

struct OrderMode {
    const char* name;
    unsigned char mode;
};

static const OrderMode s_orderModes[ORDER_KINDS] = {
    { "move", 2 }, { "attack", 3 }, { "build", 0xe }, { "patrol", 9 }, { "guard", 7 }, { "reclaim", 0xc }
};

struct UnitSelector {
    int slot;
    int all;
    int count;
    char type[40];
};

struct ScriptedOrder {
    int lineNo;
    unsigned tick;
    UnitSelector who;
    int kind;
    int hasUnitTarget;
    UnitSelector targetUnit;
    int x;
    int z;
    char buildType[40];
};

static ScriptedOrder s_orders[MAX_ORDERS];
static int s_orderCount = 0;

static char* NextToken(char** cursor)
{
    char* s = *cursor;
    while (*s == ' ' || *s == '\t')
        s++;
    if (*s == '\0')
        return NULL;
    char* start = s;
    while (*s != '\0' && *s != ' ' && *s != '\t')
        s++;
    if (*s != '\0')
        *s++ = '\0';
    *cursor = s;
    return start;
}

static void ParseSelector(int lineNo, const char* text, UnitSelector* out)
{
    char detail[200];
    const char* colon = strchr(text, ':');
    memset(out, 0, sizeof *out);
    if (colon == NULL || colon != text + 1 || !isdigit((unsigned char)text[0])) {
        sprintf(detail, ", \"%.60s\" is not <slot>:all or <slot>:<unit type>:<n>", text);
        FailLine(lineNo, "has a bad unit selector", "order", detail);
    }
    out->slot = text[0] - '0';
    const char* rest = colon + 1;
    if (strcmp(rest, "all") == 0) {
        out->all = 1;
        return;
    }
    const char* second = strchr(rest, ':');
    if (second == NULL || second == rest || (size_t)(second - rest) >= sizeof out->type) {
        sprintf(detail, ", \"%.60s\" is not <slot>:all or <slot>:<unit type>:<n>", text);
        FailLine(lineNo, "has a bad unit selector", "order", detail);
    }
    memcpy(out->type, rest, second - rest);
    out->type[second - rest] = '\0';
    char* end;
    long n = strtol(second + 1, &end, 10);
    if (second[1] == '\0' || *end != '\0' || n < 1 || n > 4000) {
        sprintf(detail, ", \"%.60s\" needs a count of 1 or more after the unit type", text);
        FailLine(lineNo, "has a bad unit selector", "order", detail);
    }
    out->count = (int)n;
}

static int ParseCoordinate(int lineNo, const char* text)
{
    char* end;
    long v = strtol(text, &end, 10);
    if (*text == '\0' || *end != '\0' || v < 0 || v > 65535) {
        char detail[100];
        sprintf(detail, ", \"%.40s\" is not a world coordinate", text);
        FailLine(lineNo, "has a bad position", "order", detail);
    }
    return (int)v;
}

static void ReadOrder(int lineNo, char* value)
{
    char* cursor = value;
    char* tickText = NextToken(&cursor);
    char* whoText = NextToken(&cursor);
    char* kindText = NextToken(&cursor);
    if (kindText == NULL)
        FailLine(lineNo, "needs <tick> <unit selector> <kind> <target>", "order", "");
    if (s_orderCount >= MAX_ORDERS)
        FailLine(lineNo, "is past the limit of 256 order lines", "order", "");
    ScriptedOrder* o = &s_orders[s_orderCount++];
    memset(o, 0, sizeof *o);
    o->lineNo = lineNo;
    o->tick = ParseNumber(lineNo, "order", tickText);
    if (o->tick == 0)
        FailLine(lineNo, "needs a tick of at least 1", "order", "");
    ParseSelector(lineNo, whoText, &o->who);
    o->kind = -1;
    for (int i = 0; i < ORDER_KINDS; i++) {
        if (strcmp(kindText, s_orderModes[i].name) == 0)
            o->kind = i;
    }
    if (o->kind < 0) {
        char detail[100];
        sprintf(detail, ", \"%.40s\" is not move, attack, build, patrol, guard or reclaim", kindText);
        FailLine(lineNo, "has an unknown kind", "order", detail);
    }
    if (o->kind == ORDER_BUILD) {
        char* type = NextToken(&cursor);
        if (type == NULL || strlen(type) >= sizeof o->buildType)
            FailLine(lineNo, "needs a unit type to build", "order", "");
        strcpy(o->buildType, type);
    }
    char* first = NextToken(&cursor);
    if (first == NULL)
        FailLine(lineNo, "needs a target", "order", "");
    if (strchr(first, ':') != NULL && o->kind != ORDER_BUILD) {
        o->hasUnitTarget = 1;
        ParseSelector(lineNo, first, &o->targetUnit);
        if (o->targetUnit.all)
            FailLine(lineNo, "needs a target selector of the form <slot>:<unit type>:<n>", "order", "");
    } else {
        char* second = NextToken(&cursor);
        if (second == NULL)
            FailLine(lineNo, "needs a target of x z or a unit selector", "order", "");
        o->x = ParseCoordinate(lineNo, first);
        o->z = ParseCoordinate(lineNo, second);
    }
    if (NextToken(&cursor) != NULL)
        FailLine(lineNo, "has extra words after the target", "order", "");
}

static void CheckOrders()
{
    for (int i = 0; i < s_orderCount; i++) {
        ScriptedOrder* o = &s_orders[i];
        char detail[160];
        const UnitSelector* sels[2] = { &o->who, o->hasUnitTarget ? &o->targetUnit : NULL };
        for (int k = 0; k < 2; k++) {
            if (sels[k] != NULL && !s_players[sels[k]->slot].named) {
                sprintf(detail, ", slot %d is not a player the scenario names", sels[k]->slot);
                FailLine(o->lineNo, "names a bad slot", "order", detail);
            }
        }
        if (!s_players[o->who.slot].human) {
            sprintf(detail, ", slot %d is not controller human", o->who.slot);
            FailLine(o->lineNo, "orders a slot that is not human", "order", detail);
        }
        if (o->tick > s_ticks)
            FailLine(o->lineNo, "has a tick after the last tick", "order", "");
    }
}

static void ReadKey(int lineNo, const char* key, char* value)
{
    if (strcmp(key, "game_seed") == 0)
        s_gameSeed = ParseNumber(lineNo, key, value);
    else if (strcmp(key, "crt_seed") == 0)
        s_crtSeed = ParseNumber(lineNo, key, value);
    else if (strcmp(key, "view_seed") == 0)
        s_viewSeed = ParseNumber(lineNo, key, value);
    else if (strcmp(key, "fpu_control_word") == 0)
        s_fpuControlWord = ParseNumber(lineNo, key, value);
    else if (strcmp(key, "map") == 0) {
        if (strlen(value) >= 0x100)
            FailLine(lineNo, "is too long", key, "");
        strcpy(s_map, value);
        s_mapLine = lineNo;
    } else if (strcmp(key, "difficulty") == 0)
        s_difficulty = ParseRange(lineNo, key, value, 2);
    else if (strcmp(key, "commander_death") == 0)
        s_commanderDeath = ParseRange(lineNo, key, value, 1);
    else if (strcmp(key, "mapping") == 0)
        s_mapping = ParseRange(lineNo, key, value, 1);
    else if (strcmp(key, "line_of_sight") == 0)
        s_lineOfSight = ParseRange(lineNo, key, value, 1);
    else if (strcmp(key, "los_type") == 0)
        s_losType = ParseRange(lineNo, key, value, 1);
    else if (strcmp(key, "start_positions") == 0) {
        if (strcmp(value, "fixed") == 0)
            s_fixedLocations = 1;
        else if (strcmp(value, "random") == 0)
            s_fixedLocations = 0;
        else
            FailLine(lineNo, "must be fixed or random", key, "");
    } else if (strcmp(key, "local_player") == 0)
    {
        s_localPlayer = ParseRange(lineNo, key, value, MAX_PLAYERS - 1);
        s_localPlayerLine = lineNo;
    }
    else if (strcmp(key, "steps_per_frame") == 0) {
        s_stepsPerFrame = ParseNumber(lineNo, key, value);
        if (s_stepsPerFrame < 1 || s_stepsPerFrame > 8)
            FailLine(lineNo, "must be from 1 to 8", key, "");
    } else if (strcmp(key, "ticks") == 0) {
        s_ticks = ParseNumber(lineNo, key, value);
        if (s_ticks == 0)
            FailLine(lineNo, "must be at least 1", key, "");
    } else if (strcmp(key, "quirks") == 0) {
        char err[200] = "spec too long";
        if (!QuirksParse(value, &s_scenarioQuirkSet, err, sizeof err) || !QuirksNormalise(value, s_scenarioQuirks, sizeof s_scenarioQuirks)) {
            char detail[240];
            sprintf(detail, ": %s", err);
            FailLine(lineNo, "is not a quirk spec", key, detail);
        }
    } else if (strcmp(key, "quirks_tamper") == 0) {
        s_quirkTamperTick = ParseNumber(lineNo, key, value);
        s_quirkTamper = 1;
    } else if (strcmp(key, "quirks_reach") == 0) {
        static const char* const reachFlags[] = { "units.ignore-null-death-record", "ai.patrol-null-enemy-skip", "units.skip-empty-yardmap" };
        char flag[64];
        unsigned tick;
        if (sscanf(value, "%u %63s", &tick, flag) != 2)
            FailLine(lineNo, "needs <tick> <flag name>", key, "");
        if (tick == 0)
            FailLine(lineNo, "needs a tick of at least 1", key, "");
        for (int f = 0; f < 3; f++) {
            if (strcmp(flag, reachFlags[f]) == 0) {
                s_quirkReachTick = tick;
                s_quirkReach = f + 1;
            }
        }
        if (s_quirkReach == 0)
            FailLine(lineNo, "names a flag with no guard to reach", key, "");
    } else if (strcmp(key, "order") == 0)
        ReadOrder(lineNo, value);
    else if (strncmp(key, "player", 6) == 0 && isdigit((unsigned char)key[6]) && key[7] == '.')
        ReadPlayerKey(lineNo, key, value);
    else
        FailLine(lineNo, "is not a known key", key, "");
}

static void CheckScenario()
{
    int named = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
        named += s_players[i].named;
    if (named < 2)
        Fail("testmode: scenario needs at least 2 players (player<N>.* keys, N 0 to 9)\n");
    if (s_ticks == 0)
        Fail("testmode: scenario needs a ticks line\n");
    if (s_map[0] == '\0')
        Fail("testmode: scenario needs a map line\n");
    CheckOrders();
    if (s_localPlayer >= 0 && !s_players[s_localPlayer].named) {
        char msg[128];
        sprintf(msg, "testmode: scenario line %d: local_player %d is not a player the scenario names\n",
                s_localPlayerLine, s_localPlayer);
        Fail(msg);
    }
}

static void ReadScenario(const char* path)
{
    char line[512];
    InitPlayerSpecs();
    FILE* f = fopen(path, "r");
    if (f == NULL) {
        char msg[700];
        sprintf(msg, "testmode: cannot read scenario file %s\n", path);
        Fail(msg);
    }
    int lineNo = 0;
    while (fgets(line, sizeof line, f) != NULL) {
        lineNo++;
        char* key = line;
        while (*key == ' ' || *key == '\t')
            key++;
        if (*key == '#' || *key == '\n' || *key == '\r' || *key == '\0')
            continue;
        char* end = key;
        while (*end != '\0' && !isspace((unsigned char)*end))
            end++;
        if (*end != '\0')
            *end++ = '\0';
        while (*end == ' ' || *end == '\t')
            end++;
        char* value = end;
        char* last = value + strlen(value);
        while (last > value && isspace((unsigned char)last[-1]))
            *--last = '\0';
        if (*value == '\0')
            FailLine(lineNo, "needs a value", key, "");
        ReadKey(lineNo, key, value);
    }
    fclose(f);
    CheckScenario();
}

static int FindSide(const char* name)
{
    int count = g_game->sides;
    for (int i = 0; i < count; i++) {
        if (_stricmp(g_game->names[i], name) == 0)
            return i;
    }
    return -1;
}

static void SetUpSkirmish()
{
    Table_0047ae60* t = (Table_0047ae60*)g_game->options;
    int sideCount = g_game->sides;
    int highest = 0;
    int active = 0;

    memset(t, 0, sizeof *t);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        PlayerSpec* p = &s_players[i];
        Player_0047ae60* slot = &t->players[i];
        slot->active = p->named ? (p->human ? 1 : 2) : 0;
        slot->shade = i % 2;
        if (p->side[0] != '\0') {
            int side = FindSide(p->side);
            if (side < 0) {
                char key[32];
                char detail[200];
                sprintf(key, "player%d.side", i);
                sprintf(detail, ", the game has no side \"%s\"", p->side);
                FailLine(p->sideLine, "names an unknown side", key, detail);
            }
            slot->shade = side;
        } else if (slot->shade >= sideCount) {
            slot->shade = 0;
        }
        slot->type = p->team;
        slot->metal = p->metal;
        slot->energy = p->energy;
        slot->color = p->color;
        if (p->named)
            highest = i;
        if (slot->active != 0)
            active++;
    }

    SetMissionType(2);
    Mission* mission = (Mission*)g_game->net;
    strcpy(t->mapName, s_map);
    t->difficulty = s_difficulty;
    t->commanderDeath = s_commanderDeath;
    t->mapping = s_mapping;
    t->lineOfSight = s_lineOfSight;
    t->losType = s_losType;
    t->fixedLocations = s_fixedLocations;
    g_game->difficulty = t->difficulty;

    if (mission->LoadMissionByName(t->mapName) == 0) {
        char msg[600];
        sprintf(msg, "testmode: scenario line %d: no map named \"%s\"\n", s_mapLine, t->mapName);
        Fail(msg);
    }
    if (mission->CountStartPositions() < active) {
        char msg[200];
        sprintf(msg, "testmode: map \"%s\" has fewer start positions than the scenario's %d players\n",
                t->mapName, active);
        Fail(msg);
    }

    g_game->playerCount = highest + 1;
    g_game->numPlayers = (unsigned short)active;
    ResetPlayerSlots();
    ApplySlotsToGamePlayers();
    InitMissionStatus();

    int local = s_localPlayer;
    if (local < 0) {
        local = 0;
        for (int i = MAX_PLAYERS - 1; i >= 0; i--) {
            if (t->players[i].active == 1)
                local = i;
        }
    }
    g_game->localPlayer = (unsigned char)local;
    g_game->playerIndex = (unsigned char)local;

    g_game->field_2bbe = 9;
    g_game->field_2bbf = 2;
    g_game->field_2bc0 = 2;
}

static void OpenTrace(unsigned controlWord);

static void ParseTestDump(char* cmdLine)
{
    for (char* p = cmdLine; *p != '\0'; p++) {
        if ((p[0] == '-' || p[0] == '/') && _strnicmp(p + 1, "testdump", 8) == 0
            && (p[9] == '\0' || isspace((unsigned char)p[9])) && (p == cmdLine || isspace((unsigned char)p[-1]))) {
            char* q = p + 9;
            while (*q == ' ' || *q == '\t')
                q++;
            char* end;
            unsigned tick = strtoul(q, &end, 0);
            if (end == q || (*end != '\0' && !isspace((unsigned char)*end)))
                Fail("testmode: -testdump needs a tick number\n");
            s_dumpEnabled = 1;
            s_dumpTick = tick;
            memset(p, ' ', end - p);
            return;
        }
    }
}

static void ParseBlockScan(char* cmdLine)
{
    for (char* p = cmdLine; *p != '\0'; p++) {
        if ((p[0] == '-' || p[0] == '/') && _strnicmp(p + 1, "blockscan", 9) == 0 && (p == cmdLine || isspace((unsigned char)p[-1]))) {
            char* q = p + 10;
            unsigned every = 1;
            if (*q == '=') {
                char* digits = ++q;
                every = 0;
                while (isdigit((unsigned char)*q) && every < 100000000)
                    every = every * 10 + (unsigned)(*q++ - '0');
                if (q == digits || every == 0)
                    continue;
            }
            if (*q != '\0' && !isspace((unsigned char)*q))
                continue;
            s_blockScan = (int)every;
            memset(p, ' ', (size_t)(q - p));
            return;
        }
    }
}

static void ParseQuirksSwitch(char* cmdLine)
{
    for (char* p = cmdLine; *p != '\0'; p++) {
        if ((p[0] == '-' || p[0] == '/') && _strnicmp(p + 1, "quirks=", 7) == 0 && (p == cmdLine || isspace((unsigned char)p[-1]))) {
            char* q = p + 8;
            char* start = q;
            if (*q == '"') {
                start = ++q;
                while (*q != '\0' && *q != '"')
                    q++;
            } else {
                while (*q != '\0' && !isspace((unsigned char)*q))
                    q++;
            }
            int n = (int)(q - start);
            if (n >= (int)sizeof s_switchQuirks)
                Fail("-quirks: spec is too long\n");
            memcpy(s_switchQuirks, start, n);
            s_switchQuirks[n] = '\0';
            if (*q == '"')
                q++;
            memset(p, ' ', q - p);
            return;
        }
    }
}

static void ResolveQuirks()
{
    Quirks set;
    char err[200];
    char msg[300];
    if (s_scenarioQuirks[0] != '\0') {
        set = s_scenarioQuirkSet;
        strcpy(s_quirkSpec, s_scenarioQuirks);
    } else if (s_switchQuirks[0] != '\0') {
        if (!QuirksParse(s_switchQuirks, &set, err, sizeof err)) {
            sprintf(msg, "-quirks: %s\n", err);
            Fail(msg);
        }
        QuirksNormalise(s_switchQuirks, s_quirkSpec, sizeof s_quirkSpec);
    } else {
        QuirksParse("3.1c", &set, err, sizeof err);
    }
    g_quirks = set;
    s_quirkHash = QuirksHash(g_quirks);
}

void HarnessInit(char* cmdLine)
{
    ParseBlockScan(cmdLine);
    if (s_blockScan)
        BlockScanEnable((unsigned)s_blockScan);
    InstallNativeSeams();
    ParseQuirksSwitch(cmdLine);
    char* p = cmdLine;
    char* flag = NULL;
    while (*p != '\0') {
        if ((p[0] == '-' || p[0] == '/') && _strnicmp(p + 1, "testmode", 8) == 0
            && (p[9] == '\0' || isspace((unsigned char)p[9])) && (p == cmdLine || isspace((unsigned char)p[-1]))) {
            flag = p;
            break;
        }
        p++;
    }
    if (flag == NULL) {
        s_viewState = (unsigned)time(0);
        ResolveQuirks();
        return;
    }
    ParseTestDump(cmdLine);

    char* q = flag + 9;
    while (*q == ' ' || *q == '\t')
        q++;
    char path[512];
    char* pathStart = q;
    int n = 0;
    if (*q == '"') {
        q++;
        pathStart = q;
        while (*q != '\0' && *q != '"')
            q++;
        n = (int)(q - pathStart);
        if (*q == '"')
            q++;
    } else {
        while (*q != '\0' && !isspace((unsigned char)*q))
            q++;
        n = (int)(q - pathStart);
    }
    if (n <= 0 || n >= (int)sizeof path)
        Fail("testmode: missing or too long scenario path\n");
    memcpy(path, pathStart, n);
    path[n] = '\0';
    strcpy(s_scenarioPath, path);
    memset(flag, ' ', q - flag);

    s_testMode = 1;
    ReadScenario(path);
    ResolveQuirks();
    s_viewState = s_viewSeed;

    unsigned cw = _control87(0, 0);
    if ((cw & ~_MCW_IC) != (s_fpuControlWord & ~_MCW_IC)) {
        char msg[256];
        sprintf(msg,
                "testmode: FPU control word (_control87) is 0x%05X (0x%05X without the infinity bit), "
                "scenario expects 0x%05X (0x%05X)\n",
                cw, cw & ~_MCW_IC, s_fpuControlWord, s_fpuControlWord & ~_MCW_IC);
        Fail(msg);
    }
    OpenTrace(cw);
    if (IsMemSet()) {
        char note[96];
        sprintf(note, "testmode: allocations are filled with 0x%08x\n", (unsigned)GetMemSetValue());
        host_log(note);
    }
}

int HarnessTestMode()
{
    return s_testMode;
}

unsigned HarnessStepsPerFrame()
{
    return s_stepsPerFrame;
}

unsigned HarnessGameSeed(unsigned live)
{
    return s_testMode ? s_gameSeed : live;
}

unsigned HarnessCrtSeed(unsigned live)
{
    return s_testMode ? s_crtSeed : live;
}

int HarnessViewRand()
{
    s_viewState = s_viewState * 214013 + 2531011;
    return (s_viewState >> 16) & 0x7fff;
}

void HarnessStartSkirmish()
{
    if (!s_testMode || s_started)
        return;
    s_started = 1;
    SetUpSkirmish();
}

void HarnessAfterStep()
{
    if (s_testMode && s_blockScan)
        BlockScanTick((unsigned)g_game->ticks, s_ticks);
    if (!s_testMode || (unsigned)g_game->ticks < s_ticks)
        return;
    char msg[64];
    if (s_trace != NULL)
        fclose(s_trace);
    if (s_dump != NULL)
        fclose(s_dump);
    s_trace = NULL;
    s_dump = NULL;
    if (s_blockScan)
        BlockScanWrite(strcmp(s_commit, "unknown") == 0 ? "-" : s_commit);
    sprintf(msg, "testmode: reached tick %u\n", s_ticks);
    host_log(msg);
    host_exit(0);
}

#include "trace_schema.h"

#include "../state/game_rand.h"

enum {
    GAME_PLAYERS = 0x1b63,
    GAME_WEAPONS = 0x2cf3,
    GAME_PROJECTILE_COUNT = 0x141f3,
    GAME_PROJECTILES = 0x141f7,
    GAME_SPOT_ENTRIES = 0x1420b,
    GAME_SPOT_USED_HEAD = 0x14213,
    GAME_SPOT_REST_HEAD = 0x14217,
    GAME_PATHFINDER = 0x14207,
    GAME_FEATURE_COUNT = 0x14253,
    GAME_FEATURES = 0x1426f,
    GAME_POOL_COUNT = 0x14351,
    GAME_MAP_WIDTH = 0x14233,
    GAME_MAP_HEIGHT = 0x14237,
    GAME_VISIBILITY = 0x14273,
    GAME_EYE_COUNT = 0x14277,
    GAME_EYES = 0x1427b,
    GAME_CELLS = 0x14287,
    GAME_SPATIAL_CELLS = 0x1429f,
    GAME_SPATIAL_WIDTH = 0x142a3,
    GAME_SPATIAL_HEIGHT = 0x142a7,
    GAME_SPATIAL_COUNT = 0x142ab,
    GAME_SPATIAL_OVERFLOW = 0x142b7,
    GAME_UNITS = 0x14357,
    GAME_DEF_COUNT = 0x1438f,
    GAME_DEFS = 0x1439b,
    GAME_DEBRIS_COUNT = 0x1491b,
    GAME_DEBRIS = 0x1491f,
    GAME_PIECES = 0x1ab9f,
    GAME_VERTICES = 0x1e88f,
    GAME_FACES = 0x2590f,
    GAME_TICK = 0x38a47
};

enum {
    UNIT_STRIDE = 0x118,
    UNITDEF_STRIDE = 0x249,
    WEAPONDEF_STRIDE = 0x115,
    PROJECTILE_STRIDE = 0x6b,
    FEATURE_STRIDE = 0x100,
    PIECE_STRIDE = 0x34,
    PLAYER_STRIDE = 0x14b,
    SPOT_STRIDE = 0x30,
    DEBRIS_STRIDE = 0x54,
    SPOT_CAPACITY = 0x800,
    PIECE_COUNT = 300,
    EXPLODED_SLOTS = 100,
    CELL_STRIDE = 13,
    CELL_FEATURE = 8,
    CELL_SPOT = 10,
    CELL_FLAGS = 12,
    CELL_FLAGS_DRAW_MARK = 4,
    CELL_FLAGS_UNUSED = 0x80,
    CELL_LIMIT = 1 << 22,
    REC_STRIDE = 10,
    REC_LIMIT = 1 << 20,
    EYE_STRIDE = 0x24,
    EYE_CAPACITY = 20,
    NODE_STRIDE = 0x14,
    NODE_LIMIT = 1 << 19,
    NODE_PENALTY = 0x10,
    NODE_G = 8,
    MOVECLASS_STRIDE = 0x20,
    MOVECLASS_COUNT = 32,
    PASSMAP_LIMIT = 1 << 22,
    PASSMAP_ROW_LIMIT = 4096,
    PLAYER_EXPLORED = 0x7c,
    PLAYER_EXPLORED_SIZE = 0x88,
    EXPLORED_LIMIT = 1 << 22,
    PF_POOL = 0x0,
    PF_ITEMS = 0x4,
    PF_FREE_HEAD = 0x8,
    PF_USED = 0xc,
    PF_COUNT = 0x14,
    PF_CELLS = 0x1c,
    PF_CELL_COUNT = 0x28,
    PF_DIRTY = 0x2c,
    PF_OBJECT = 0x58,
    UNIT_LIVE = 0xa6,
    UNIT_MOTION = 0x0,
    UNIT_LIST = 0x5c,
    UNIT_LIST2 = 0x60,
    UNIT_SCRIPT = 0x9a,
    UNIT_STATE = 0x9e,
    UNIT_HEAD = 0xa2,
    ORDER_KIND = 0x4,
    ORDER_TABLE_STRIDE = 0x19,
    ORDER_TABLE_NAME = 0x15,
    ORDER_LINK = 0x12,
    ORDER_NEXT = 0x4a,
    ORDER_ATTACHED = 0x52,
    FX_SOURCE = 0x4,
    PATHORDER_FLAGS = 0x8,
    PATHORDER_REF = 0x16,
    AIRMANEUVER_FLAGS = 0x8,
    GOAL_TARGET = 0x4,
    PATROL_COUNT = 0x18,
    PATROL_POINTS = 0xc,
    PATROL_POINT_LIMIT = 3,
    AISEARCH_COUNT = 0x5c,
    AISEARCH_POINTS = 0xc,
    AISEARCH_POINT_LIMIT = 20,
    SCRIPT_TABLE = 0x8,
    SCRIPT_STATICS = 0x10,
    SCRIPT_PIECES = 0x14,
    SCRIPT_CHANNELS = 0x1c,
    SCRIPT_OBJECT_STATE = 0x540,
    SCRIPT_CHANNEL_STRIDE = 0xa4,
    SCRIPT_CHANNEL_COUNT = 8,
    SCRIPT_STACK = 0x24,
    SCRIPT_STACK_LIMIT = 32,
    SCRIPT_PIECE_STRIDE = 0x4c,
    SCRIPT_PIECE_LIMIT = 512,
    SCRIPT_STATIC_LIMIT = 4096,
    TABLE_PIECE_COUNT = 0x8,
    TABLE_STATIC_COUNT = 0x10,
    OBJECT_STATE_PIECES = 0x22,
    OBJECT_STATE_STRIDE = 0x36,
    ORDER_CHAIN_LIMIT = 4096,
    TRACE_OBJECT_LIMIT = 1 << 16,
    TRACE_MAP_SIZE = 1 << 18
};

enum {
    MISSION_TYPE = 0x0,
    MISSION_TYPE_CAMPAIGN = 1,
    VECTOR_FIRST = 0x4,
    VECTOR_LAST = 0x8,
    VECTOR_LIMIT = 1 << 20,
    STRING_LIMIT = 255,
    PLAYER_SLOTS = 11,
    PLAYER_MANAGER = 0x74,
    PLAYER_ALLIED = 0x108,
    AI_CENTRE = 0x35,
    AI_CENTRE_SIZE = 12,
    PLAYER_SQUADS = 0x78,
    PLAYER_ECON = 0xec,
    SQUAD_TIMERS = 10,
    MANAGER_TIMERS = 0x11,
    MANAGER_SCOUT_TIMER = 8,
    SQUAD_SCOUT = 8,
    SCRIPT_SET_UNIT_VALUE_SLOT = 16,
    SCRIPT_VALUE_YARD_OPEN = 18,
    PACKET_UNIT_DEATH = 12,
    PACKET_UNIT_DEATH_SIZE = 11,
    SQUAD_COUNT = 10,
    SQUAD_STRIDE = 0x20,
    SQUAD_ITEMS = 0x10,
    GAME_CONDITIONS = 0x391ed,
    GAME_MISSION = 0x391e9,
    CONDITIONS_VICTORY = 0x0,
    CONDITIONS_VICTORY_COUNT = 0x40,
    CONDITIONS_DEFEAT = 0x44,
    CONDITIONS_DEFEAT_COUNT = 0x84,
    CONDITIONS_LIMIT = 16,
    WEAPON_TABLE = 0x64,
    WEAPON_TABLE_FIRST = 0x5,
    WEAPON_TABLE_LAST = 0x9,
    WEAPON_ENTRY_SIZE = 8,
    WEAPON_COUNT = 256,
    DEF_LIMIT = 1 << 14,
    DEF_FOOTPRINT_X = 0x14a,
    DEF_FOOTPRINT_Z = 0x14c,
    DEF_YARDMAP = 0x14e,
    DEF_ID_COUNT = 0x152,
    DEF_IDS = 0x156,
    DEF_YARD_LIMIT = 1 << 16,
    DEF_ID_LIMIT = 30,
    DEF_WEAPON_CATEGORIES = 0x231,
    DEF_CATEGORIES = 0x23d,
    DEF_MASK_SIZE = 0x40,
    MISSION_UNITS = 0xdac,
    MISSION_UNIT_COUNT = 0xdb0,
    MISSION_RULES = 0xdb4,
    MISSION_RULE_COUNT = 0xdb8,
    MISSION_FEATURES = 0xdbc,
    MISSION_FEATURE_COUNT = 0xdc0,
    MISSION_LIMIT = 1 << 16,
    MISSION_UNIT_SIZE = 0x24,
    MISSION_RULE_SIZE = 0xc,
    MISSION_FEATURE_SIZE = 0x88,
    MISSION_FEATURE_POS = 0x80
};

#define LINK_IN_FX 0x80000000u


static unsigned char* MoveClasses()
{
    return (unsigned char*)DAT_00512370 - 0x18;
}

typedef unsigned char* SimView_g_explodedPieces[EXPLODED_SLOTS];
#undef g_explodedPieces
#define g_explodedPieces (*reinterpret_cast<SimView_g_explodedPieces*>(&Sim().g_explodedPieces))
extern void* g_approachRadiusVtable[];
extern void* g_ringApproachVtable[];
extern void* g_pointMarkerVtable[];
extern void* g_pathOrderVtable[];
extern void* g_airManeuverOrderVtable[];
extern void* g_patrolGoalVtable[];
extern void* g_aiSearchGoalVtable[];
extern void* g_packedGoalVtable[];
extern void* g_liteGoalVtable[];
extern void* g_packedPosGoalVtable[];
typedef unsigned char* SimView_g_playerAI[MAX_PLAYERS];
#undef g_playerAI
#define g_playerAI (*reinterpret_cast<SimView_g_playerAI*>(&Sim().g_playerAI))

static const char* const s_phaseNames[9] = { "net", "units", "projectiles", "explosions", "players", "features",
                                             "wind", "meteors", "world" };
static const char* const s_targetNames[] = { "", "heap", "unit", "player", "unitdef", "weapondef", "projectile",
                                             "feature", "piece", "rec", "node", "moveclass", "order", "orderfx", "link" };

static int s_dumpNow = 0;
static unsigned s_traceTick = 0;
static const char* s_tracePhase = "";
static unsigned s_hashLo = 0;
static unsigned s_hashHi = 0;

static unsigned char* GameAt(unsigned offset)
{
    return (unsigned char*)g_game + offset;
}

static unsigned GameU32(unsigned offset)
{
    return *(unsigned*)GameAt(offset);
}


static unsigned s_orderKeys[TRACE_MAP_SIZE];
static unsigned s_orderIds[TRACE_MAP_SIZE];
static unsigned s_fxKeys[TRACE_MAP_SIZE];
static unsigned s_fxIds[TRACE_MAP_SIZE];
static unsigned s_linkKeys[TRACE_MAP_SIZE];
static unsigned s_linkIds[TRACE_MAP_SIZE];
static unsigned s_orderList[TRACE_OBJECT_LIMIT];
static unsigned s_fxList[TRACE_OBJECT_LIMIT];
static unsigned s_orderUsed[TRACE_OBJECT_LIMIT];
static unsigned s_fxUsed[TRACE_OBJECT_LIMIT];
static unsigned s_linkUsed[2 * TRACE_OBJECT_LIMIT];
static unsigned s_orderUsedCount = 0;
static unsigned s_fxUsedCount = 0;
static unsigned s_linkUsedCount = 0;
static unsigned s_orderCountSeen = 0;
static unsigned s_fxCountSeen = 0;

static unsigned MapSlot(const unsigned* keys, unsigned p)
{
    unsigned slot = (p >> 2) * 2654435761u >> 14;
    while (keys[slot] != 0 && keys[slot] != p)
        slot = (slot + 1) & (TRACE_MAP_SIZE - 1);
    return slot;
}

static int MapFind(const unsigned* keys, const unsigned* ids, unsigned p, unsigned* id)
{
    unsigned slot;
    if (p == 0)
        return 0;
    slot = MapSlot(keys, p);
    if (keys[slot] != p)
        return 0;
    *id = ids[slot];
    return 1;
}

static void MapAdd(unsigned* keys, unsigned* ids, unsigned* used, unsigned* usedCount, unsigned p, unsigned id)
{
    unsigned slot = MapSlot(keys, p);
    if (keys[slot] == 0)
        used[(*usedCount)++] = slot;
    keys[slot] = p;
    ids[slot] = id;
}

#define HASH_MULTIPLY \
    { \
        unsigned pc = (lo & 0xffff) * 435; \
        unsigned pa = (lo >> 16) * 435; \
        unsigned plo = pc + (pa << 16); \
        unsigned carry = plo < pc; \
        hi = hi * 435 + (pa >> 16) + carry + (lo << 8); \
        lo = plo; \
    }

static void HashBuf(const unsigned char* p, unsigned n)
{
    unsigned lo = s_hashLo;
    unsigned hi = s_hashHi;
    while (n >= 8) {
        lo ^= *(const unsigned*)p;
        hi ^= *(const unsigned*)(p + 4);
        HASH_MULTIPLY
        p += 8;
        n -= 8;
    }
    while (n > 0) {
        lo ^= *p++;
        HASH_MULTIPLY
        n--;
    }
    s_hashLo = lo;
    s_hashHi = hi;
}

static void HashU32(unsigned v)
{
    HashBuf((const unsigned char*)&v, 4);
}

static void HashByte(unsigned char v)
{
    HashBuf(&v, 1);
}

static void Resolve(unsigned blockBase, unsigned p, bool isRef, int target, unsigned* tag, unsigned* index)
{
    unsigned base = 0;
    unsigned stride = 1;
    unsigned count = 0;
    *tag = 0;
    *index = 0;
    if (p == 0)
        return;
    if (isRef)
        p += blockBase;
    *tag = TT_HEAP;
    switch (target) {
    case TT_UNIT:
        base = GameU32(GAME_UNITS);
        stride = UNIT_STRIDE;
        count = *(unsigned short*)GameAt(GAME_POOL_COUNT);
        break;
    case TT_PLAYER:
        base = (unsigned)GameAt(GAME_PLAYERS);
        stride = PLAYER_STRIDE;
        count = PLAYER_SLOTS;
        break;
    case TT_UNITDEF:
        base = GameU32(GAME_DEFS);
        stride = UNITDEF_STRIDE;
        count = GameU32(GAME_DEF_COUNT);
        break;
    case TT_WEAPONDEF:
        base = (unsigned)GameAt(GAME_WEAPONS);
        stride = WEAPONDEF_STRIDE;
        count = 256;
        break;
    case TT_PROJECTILE:
        base = GameU32(GAME_PROJECTILES);
        stride = PROJECTILE_STRIDE;
        count = GameU32(GAME_PROJECTILE_COUNT);
        break;
    case TT_FEATURE:
        base = GameU32(GAME_FEATURES);
        stride = FEATURE_STRIDE;
        count = GameU32(GAME_FEATURE_COUNT);
        break;
    case TT_PIECE:
        base = (unsigned)GameAt(GAME_PIECES);
        stride = PIECE_STRIDE;
        count = PIECE_COUNT;
        break;
    case TT_REC:
        if (p == GameU32(GAME_SPATIAL_OVERFLOW)) {
            *tag = TT_REC;
            *index = 0xffffffff;
            return;
        }
        base = GameU32(GAME_SPATIAL_CELLS);
        stride = REC_STRIDE;
        count = GameU32(GAME_SPATIAL_COUNT);
        break;
    case TT_NODE:
        if (GameU32(GAME_PATHFINDER) == 0)
            return;
        base = *(unsigned*)(GameU32(GAME_PATHFINDER) + PF_POOL);
        stride = NODE_STRIDE;
        count = *(unsigned*)(GameU32(GAME_PATHFINDER) + PF_USED);
        break;
    case TT_MOVECLASS:
        base = (unsigned)MoveClasses();
        stride = MOVECLASS_STRIDE;
        count = MOVECLASS_COUNT;
        break;
    case TT_ORDER:
        if (MapFind(s_orderKeys, s_orderIds, p, index))
            *tag = TT_ORDER;
        return;
    case TT_ORDERFX:
        if (MapFind(s_fxKeys, s_fxIds, p, index))
            *tag = TT_ORDERFX;
        return;
    case TT_LINK:
        if (MapFind(s_linkKeys, s_linkIds, p, index))
            *tag = TT_LINK;
        return;
    default:
        return;
    }
    if (base != 0 && p >= base && p - base < stride * count && (p - base) % stride == 0) {
        *tag = target;
        *index = (p - base) / stride;
    }
}

static void HashPointer(unsigned p, int target)
{
    unsigned tag;
    unsigned index;
    Resolve((unsigned)SimBlockBase(), p, false, target, &tag, &index);
    HashByte((unsigned char)tag);
    HashU32(index);
}

static unsigned ReadMasked(const unsigned char* p, const TraceField* f)
{
    unsigned v = 0;
    for (unsigned b = f->size; b > 0; b--)
        v = (v << 8) | p[b - 1];
    return v & f->mask;
}

static void HashField(const unsigned char* base, const TraceField* f)
{
    if (f->kind == TF_POINTER)
        HashPointer(*(const unsigned*)(base + f->offset), f->target);
    else if (f->mask != 0)
        HashU32(ReadMasked(base + f->offset, f));
    else
        HashBuf(base + f->offset, f->size);
}

static void HashPlan(const unsigned char* base, const TraceField* plan, unsigned n)
{
    for (unsigned k = 0; k < n; k++)
        HashField(base, &plan[k]);
}

static void DumpPrefix(const char* path)
{
    fprintf(s_dump, "%u %s %s ", s_traceTick, s_tracePhase, path);
}

static void DumpValue(const unsigned char* p, const TraceField* f)
{
    if (f->kind == TF_POINTER) {
        unsigned tag;
        unsigned index;
        Resolve((unsigned)SimBlockBase(), *(const unsigned*)p, false, f->target, &tag, &index);
        if (tag == 0)
            fputs("null", s_dump);
        else if (tag == TT_HEAP)
            fputs("heap", s_dump);
        else if (tag == TT_REC && index == 0xffffffff)
            fputs("rec:overflow", s_dump);
        else
            fprintf(s_dump, "%s:%u", s_targetNames[tag], index);
    } else if (f->kind == TF_SCALAR && f->mask != 0) {
        fprintf(s_dump, "0x%0*x", f->size * 2, ReadMasked(p, f));
    } else if (f->kind == TF_SCALAR) {
        fputs("0x", s_dump);
        for (unsigned b = f->size; b > 0; b--)
            fprintf(s_dump, "%02x", p[b - 1]);
    } else {
        fputs("hex:", s_dump);
        for (unsigned b = 0; b < f->size; b++)
            fprintf(s_dump, "%02x", p[b]);
    }
    fputc('\n', s_dump);
}

static void DumpFields(const char* path, const unsigned char* base, const TraceField* fields, unsigned n)
{
    char full[160];
    for (unsigned k = 0; k < n; k++) {
        const TraceField* f = &fields[k];
        if (f->name[0] != '\0')
            sprintf(full, "%s.%s", path, f->name);
        else
            sprintf(full, "%s.+0x%x", path, f->offset);
        DumpPrefix(full);
        DumpValue(base + f->offset, f);
    }
}

static void VisitElement(const char* path, const unsigned char* base, const TraceTable* t)
{
    HashPlan(base, t->plan, t->planCount);
    if (s_dumpNow)
        DumpFields(path, base, t->fields, t->fieldCount);
}

static void VisitU32(const char* path, unsigned value)
{
    HashU32(value);
    if (s_dumpNow) {
        DumpPrefix(path);
        fprintf(s_dump, "0x%08x\n", value);
    }
}

static void VisitRaw(const char* path, const unsigned char* p, unsigned n)
{
    HashBuf(p, n);
    if (s_dumpNow) {
        DumpPrefix(path);
        fputs("hex:", s_dump);
        for (unsigned b = 0; b < n; b++)
            fprintf(s_dump, "%02x", p[b]);
        fputc('\n', s_dump);
    }
}

static void EndSection()
{
    HashU32(0xffffffff);
}

static void VisitUnits()
{
    unsigned char* units = (unsigned char*)GameU32(GAME_UNITS);
    unsigned count = *(unsigned short*)GameAt(GAME_POOL_COUNT);
    char path[64];
    HashByte(1);
    for (unsigned i = 0; units != NULL && i < count; i++) {
        unsigned char* u = units + i * UNIT_STRIDE;
        if (*(unsigned short*)(u + 0xa6) == 0)
            continue;
        HashU32(i);
        sprintf(path, "units[%u]", i);
        VisitElement(path, u, &kUnitTable);
    }
    EndSection();
}

static void VisitProjectiles()
{
    unsigned char* projectiles = (unsigned char*)GameU32(GAME_PROJECTILES);
    unsigned count = GameU32(GAME_PROJECTILE_COUNT);
    char path[64];
    HashByte(2);
    for (unsigned i = 0; projectiles != NULL && i < count; i++) {
        unsigned char* p = projectiles + i * PROJECTILE_STRIDE;
        if (*(unsigned short*)(p + 0x69) & 2)
            continue;
        HashU32(i);
        sprintf(path, "projectiles[%u]", i);
        VisitElement(path, p, &kProjectileTable);
    }
    EndSection();
}

static void VisitExplosions()
{
    char path[64];
    unsigned char pieceUsed[PIECE_COUNT];
    unsigned debrisCount = GameU32(GAME_DEBRIS_COUNT);
    unsigned char* pieces = GameAt(GAME_PIECES);
    memset(pieceUsed, 0, sizeof pieceUsed);
    if (debrisCount > PIECE_COUNT)
        debrisCount = PIECE_COUNT;

    HashByte(3);
    for (unsigned slot = 0; slot < EXPLODED_SLOTS; slot++) {
        unsigned char* block = g_explodedPieces[slot];
        if (block == NULL)
            continue;
        HashU32(slot);
        sprintf(path, "explodedPieces[%u]", slot);
        VisitElement(path, block, &kExplodedPieceTable);
        sprintf(path, "explodedPieces[%u].rec", slot);
        VisitElement(path, block + 0x30, &kPieceRecTable);
    }
    EndSection();

    HashByte(4);
    for (unsigned d = 0; d < debrisCount; d++) {
        unsigned char* e = GameAt(GAME_DEBRIS) + d * DEBRIS_STRIDE;
        unsigned obj = *(unsigned*)e;
        if (obj == 0)
            continue;
        HashU32(d);
        sprintf(path, "debris[%u]", d);
        VisitElement(path, e, &kDebrisTable);
        if (obj >= (unsigned)pieces && (obj - (unsigned)pieces) % PIECE_STRIDE == 0
            && (obj - (unsigned)pieces) / PIECE_STRIDE < PIECE_COUNT)
            pieceUsed[(obj - (unsigned)pieces) / PIECE_STRIDE] = 1;
    }
    EndSection();

    HashByte(5);
    for (unsigned i = 0; i < PIECE_COUNT; i++) {
        if (!pieceUsed[i])
            continue;
        HashU32(i);
        sprintf(path, "pieces[%u]", i);
        VisitElement(path, pieces + i * PIECE_STRIDE, &kPieceTable);
        sprintf(path, "pieces[%u].vertices", i);
        VisitRaw(path, GameAt(GAME_VERTICES) + i * 96, 96);
        for (unsigned k = 0; k < 6; k++) {
            sprintf(path, "pieces[%u].faces[%u]", i, k);
            VisitElement(path, GameAt(GAME_FACES) + (i * 6 + k) * 0x20, &kFaceTable);
        }
    }
    EndSection();
}

static void VisitPlayers()
{
    char path[64];
    HashByte(6);
    for (unsigned i = 0; i < PLAYER_SLOTS; i++) {
        HashU32(i);
        sprintf(path, "players[%u]", i);
        VisitElement(path, GameAt(GAME_PLAYERS) + i * PLAYER_STRIDE, &kPlayerTable);
    }
    EndSection();
}

static void VisitFeatures()
{
    unsigned char* entries = (unsigned char*)GameU32(GAME_SPOT_ENTRIES);
    unsigned char list[SPOT_CAPACITY];
    unsigned featureCount = GameU32(GAME_FEATURE_COUNT);
    unsigned char* defs = (unsigned char*)GameU32(GAME_FEATURES);
    char path[64];
    memset(list, 0, sizeof list);
    HashByte(7);
    if (entries != NULL) {
        for (int which = 1; which <= 2; which++) {
            int idx = *(int*)GameAt(which == 1 ? GAME_SPOT_USED_HEAD : GAME_SPOT_REST_HEAD);
            for (int guard = 0; idx >= 0 && idx < SPOT_CAPACITY && guard < SPOT_CAPACITY; guard++) {
                list[idx] = (unsigned char)which;
                idx = *(short*)(entries + idx * SPOT_STRIDE);
            }
        }
        for (unsigned i = 0; i < SPOT_CAPACITY; i++) {
            if (list[i] == 0)
                continue;
            unsigned char* spot = entries + i * SPOT_STRIDE;
            unsigned feature = *(unsigned short*)(spot + 0x2c);
            const TraceTable* t = &kSpotObjectTable;
            if (feature < featureCount && defs != NULL && (*(unsigned short*)(defs + feature * FEATURE_STRIDE + 0xfe) & 1))
                t = &kSpotAnimTable;
            HashU32(i);
            HashByte(list[i]);
            sprintf(path, "features[%u]", i);
            if (s_dumpNow) {
                char name[80];
                sprintf(name, "%s.list", path);
                DumpPrefix(name);
                fprintf(s_dump, "0x%02x\n", list[i]);
            }
            VisitElement(path, spot, t);
        }
    }
    EndSection();
}

static void VisitFieldList(const char* prefix, const unsigned char* base, const TraceField* fields, unsigned n)
{
    for (unsigned k = 0; k < n; k++) {
        const TraceField* f = &fields[k];
        char path[96];
        HashField(base, f);
        if (s_dumpNow) {
            sprintf(path, "%s.%s", prefix, f->name);
            DumpPrefix(path);
            DumpValue(base + f->offset, f);
        }
    }
}

static void VisitGlobalList(const char* prefix, const TraceGlobal* list, unsigned n)
{
    for (unsigned k = 0; k < n; k++) {
        const TraceGlobal* g = &list[k];
        TraceField f;
        char path[96];
        f.name = g->name;
        f.offset = 0;
        f.size = g->size;
        f.kind = g->kind;
        f.target = g->target;
        f.mask = 0;
        const unsigned char* address = (const unsigned char*)g->address;
#ifdef TRACE_SIM_BASE
        if (g->relative)
            address = TRACE_SIM_BASE + (size_t)g->address;
#endif
        if (g->kind == TF_POINTER)
            HashPointer(*(const unsigned*)address, g->target);
        else
            HashBuf(address, g->size);
        if (s_dumpNow) {
            sprintf(path, "%s.%s", prefix, g->name);
            DumpPrefix(path);
            DumpValue(address, &f);
        }
    }
}

static void VisitWind()
{
    HashByte(8);
    VisitFieldList("wind", GameAt(0), kWindFields, kWindFieldCount);
    EndSection();
}

static void VisitMeteors()
{
    HashByte(9);
    VisitGlobalList("meteors", kMeteorGlobals, kMeteorGlobalsCount);
    EndSection();
}

static void VisitValue(const char* path, unsigned value, unsigned size)
{
    HashBuf((const unsigned char*)&value, size);
    if (s_dumpNow) {
        DumpPrefix(path);
        fprintf(s_dump, "0x%0*x\n", size * 2, value);
    }
}

static void VisitPointer(const char* path, unsigned value, int target)
{
    TraceField f;
    HashPointer(value, target);
    if (s_dumpNow) {
        f.name = "";
        f.offset = 0;
        f.size = 4;
        f.kind = TF_POINTER;
        f.target = (unsigned char)target;
        f.mask = 0;
        DumpPrefix(path);
        DumpValue((const unsigned char*)&value, &f);
    }
}

static void VisitBlob(const char* name, unsigned first, const unsigned char* p, unsigned stride, unsigned count)
{
    static const char digits[] = "0123456789abcdef";
    char path[96];
    char line[64];
    HashBuf(p, stride * count);
    if (!s_dumpNow)
        return;
    for (unsigned i = 0; i < count; i++) {
        const unsigned char* e = p + i * stride;
        unsigned b;
        unsigned len = 4;
        memcpy(line, "hex:", 4);
        for (b = 0; b < stride; b++) {
            line[len++] = digits[e[b] >> 4];
            line[len++] = digits[e[b] & 15];
        }
        line[len++] = '\n';
        line[len] = '\0';
        sprintf(path, "%s[%u]", name, first + i);
        DumpPrefix(path);
        fputs(line, s_dump);
    }
}

static unsigned MapCellCount()
{
    unsigned n = GameU32(GAME_MAP_WIDTH) * GameU32(GAME_MAP_HEIGHT);
    return n <= CELL_LIMIT ? n : 0;
}

static void VisitGameScalars()
{
    HashByte(10);
    VisitFieldList("game", GameAt(0), kGameScalarFields, kGameScalarFieldCount);
    VisitGlobalList("game", kScalarGlobals, kScalarGlobalsCount);
    EndSection();
}

static void VisitMapCells()
{
    unsigned char* cells = (unsigned char*)GameU32(GAME_CELLS);
    unsigned n = MapCellCount();
    HashByte(11);
    VisitValue("cells.count", n, 4);
    if (cells != NULL) {
        unsigned char* copy = (unsigned char*)malloc((size_t)CELL_STRIDE * n);
        if (copy == NULL)
            Fail("testmode: out of memory hashing the map cells\n");
        memcpy(copy, cells, (size_t)CELL_STRIDE * n);
        for (unsigned i = 0; i < n; i++) {
            unsigned char* c = copy + i * CELL_STRIDE;
            unsigned feature = c[CELL_FEATURE] | (c[CELL_FEATURE + 1] << 8);
            c[CELL_FLAGS] &= (unsigned char)~(CELL_FLAGS_DRAW_MARK | CELL_FLAGS_UNUSED);
            if (feature >= 0xfffb && feature != 0xfffe)
                c[CELL_SPOT] = c[CELL_SPOT + 1] = 0;
        }
        VisitBlob("cells", 0, copy, CELL_STRIDE, n);
        free(copy);
    }
    EndSection();
}

static void VisitVisibility()
{
    unsigned char* mask = (unsigned char*)GameU32(GAME_VISIBILITY);
    unsigned entries = MapCellCount() / 4;
    HashByte(12);
    VisitValue("visibility.count", entries, 4);
    if (mask != NULL)
        VisitBlob("visibility", 0, mask, 2, entries);
    EndSection();
}

static void VisitExplored()
{
    char path[64];
    HashByte(13);
    for (unsigned who = 0; who < MAX_PLAYERS; who++) {
        unsigned char* player = GameAt(GAME_PLAYERS) + who * PLAYER_STRIDE;
        unsigned char* cells = *(unsigned char**)(player + PLAYER_EXPLORED);
        unsigned size = *(unsigned*)(player + PLAYER_EXPLORED_SIZE);
        if (cells == NULL || size > EXPLORED_LIMIT)
            size = 0;
        sprintf(path, "players[%u].explored", who);
        VisitValue(path, size, 4);
        if (size != 0)
            VisitBlob(path, 0, cells, 1, size);
    }
    EndSection();
}

static void VisitSpatial()
{
    unsigned char* recs = (unsigned char*)GameU32(GAME_SPATIAL_CELLS);
    unsigned count = GameU32(GAME_SPATIAL_COUNT);
    unsigned char* overflow = (unsigned char*)GameU32(GAME_SPATIAL_OVERFLOW);
    char path[64];
    HashByte(14);
    VisitValue("spatial.width", GameU32(GAME_SPATIAL_WIDTH), 4);
    VisitValue("spatial.height", GameU32(GAME_SPATIAL_HEIGHT), 4);
    if (recs == NULL || count > REC_LIMIT)
        count = 0;
    VisitValue("spatial.count", count, 4);
    for (unsigned r = 0; r < count; r++) {
        sprintf(path, "spatial[%u]", r);
        VisitElement(path, recs + r * REC_STRIDE, &kRecTable);
    }
    HashByte(overflow != NULL);
    if (overflow != NULL)
        VisitElement("spatial.overflow", overflow, &kRecTable);
    EndSection();
}

static void VisitEyes()
{
    unsigned char* eyes = (unsigned char*)GameU32(GAME_EYES);
    unsigned count = GameU32(GAME_EYE_COUNT);
    char path[64];
    HashByte(15);
    if (eyes == NULL || count > EYE_CAPACITY)
        count = 0;
    VisitValue("eyes.count", count, 4);
    for (unsigned e = 0; e < count; e++) {
        sprintf(path, "eyes[%u]", e);
        VisitElement(path, eyes + e * EYE_STRIDE, &kEyeTable);
    }
    EndSection();
}

static unsigned char s_nodeFree[NODE_LIMIT];

static void VisitPathNodes(const unsigned char* pf)
{
    unsigned char* pool = *(unsigned char**)(pf + PF_POOL);
    unsigned char** items = *(unsigned char***)(pf + PF_ITEMS);
    int freeHead = *(int*)(pf + PF_FREE_HEAD);
    unsigned used = *(unsigned*)(pf + PF_USED);
    unsigned count = *(unsigned*)(pf + PF_COUNT);
    char path[96];
    if (pool == NULL || items == NULL || used > NODE_LIMIT || count > used) {
        used = 0;
        count = 0;
    }
    memset(s_nodeFree, 0, used);
    for (unsigned guard = 0; guard < used && freeHead >= 0 && (unsigned)freeHead < used; guard++) {
        if (s_nodeFree[freeHead])
            break;
        s_nodeFree[freeHead] = 1;
        freeHead = *(int*)(pool + freeHead * NODE_STRIDE);
    }
    for (unsigned node = 0; node < used; node++) {
        unsigned char* n = pool + node * NODE_STRIDE;
        if (s_nodeFree[node])
            continue;
        HashU32(node);
        sprintf(path, "pathfinder.pool[%u]", node);
        VisitElement(path, n, &kNodeTable);
        if (*(int*)(n + NODE_G) != 0) {
            strcat(path, ".penalty");
            VisitValue(path, *(unsigned short*)(n + NODE_PENALTY), 2);
        }
    }
    VisitValue("pathfinder.heap.count", count, 4);
    for (unsigned slot = 0; slot < count; slot++) {
        sprintf(path, "pathfinder.heap[%u]", slot);
        VisitPointer(path, (unsigned)items[slot], TT_NODE);
    }
}

static void VisitPathCells(const unsigned char* pf)
{
    unsigned char* cells = *(unsigned char**)(pf + PF_CELLS);
    unsigned* dirty = *(unsigned**)(pf + PF_DIRTY);
    unsigned count = *(unsigned*)(pf + PF_CELL_COUNT);
    char path[96];
    unsigned words;
    if (cells == NULL || dirty == NULL || count > CELL_LIMIT)
        count = 0;
    words = (count + 255) >> 8;
    for (unsigned w = 0; w < words; w++) {
        unsigned bits = dirty[w];
        if (bits == 0)
            continue;
        sprintf(path, "pathfinder.dirty[%u]", w);
        VisitU32(path, bits);
        for (unsigned bit = 0; bit < 32; bit++) {
            if (!((bits >> bit) & 1))
                continue;
            for (unsigned k = 0; k < 8; k++) {
                unsigned index = (w << 8) + (bit << 3) + k;
                const unsigned char* c = cells + index * 4;
                unsigned flags = c[0];
                unsigned state = flags & 3;
                if (index >= count)
                    break;
                sprintf(path, "pathfinder.cells[%u].flags", index);
                VisitValue(path, flags, 1);
                if ((flags & 8) || state == 1 || state == 2) {
                    sprintf(path, "pathfinder.cells[%u].dir", index);
                    VisitValue(path, c[1], 1);
                }
                if (state == 1 || state == 2) {
                    sprintf(path, "pathfinder.cells[%u].node", index);
                    VisitValue(path, *(const unsigned short*)(c + 2), 2);
                }
            }
        }
    }
}

static void VisitPathfinder()
{
    unsigned char* pf = (unsigned char*)GameU32(GAME_PATHFINDER);
    HashByte(16);
    HashByte(pf != NULL);
    if (pf != NULL) {
        VisitElement("pathfinder", pf, &kPathfinderTable);
        if (*(unsigned*)(pf + PF_OBJECT) != 0)
            VisitElement("pathfinder", pf, &kPathfinderSearchTable);
        VisitPathNodes(pf);
        VisitPathCells(pf);
    }
    EndSection();
    HashByte(17);
    VisitGlobalList("pathfinder", kPathGlobals, kPathGlobalsCount);
    EndSection();
}

static void VisitMoveClasses()
{
    unsigned char* classes = MoveClasses();
    char path[64];
    HashByte(18);
    for (unsigned c = 0; c < MOVECLASS_COUNT; c++) {
        sprintf(path, "moveClasses[%u]", c);
        VisitElement(path, classes + c * MOVECLASS_STRIDE, &kMoveClassTable);
    }
    EndSection();
}

static unsigned s_passRow[PASSMAP_ROW_LIMIT];

static void VisitPassMaps()
{
    unsigned char* classes = MoveClasses();
    char path[64];
    HashByte(19);
    for (unsigned c = 0; c < MOVECLASS_COUNT; c++) {
        unsigned char* e = classes + c * MOVECLASS_STRIDE;
        const unsigned* cells = *(const unsigned**)(e + 0x18);
        unsigned width = *(unsigned*)(e + 0x10);
        unsigned height = *(unsigned*)(e + 0x14);
        unsigned full = height >> 4;
        unsigned tail = height & 15;
        if (cells == NULL || width == 0 || height == 0 || width > PASSMAP_ROW_LIMIT
            || width * ((height + 15) >> 4) > PASSMAP_LIMIT)
            continue;
        HashU32(c);
        sprintf(path, "moveClasses[%u].passMap", c);
        VisitBlob(path, 0, (const unsigned char*)cells, 4, full * width);
        if (tail != 0) {
            unsigned mask = (1u << (tail * 2)) - 1;
            for (unsigned x = 0; x < width; x++)
                s_passRow[x] = cells[full * width + x] & mask;
            VisitBlob(path, full * width, (const unsigned char*)s_passRow, 4, width);
        }
    }
    EndSection();
}

enum FxClass { FX_UNKNOWN, FX_APPROACH, FX_RING, FX_MARKER, FX_PATH, FX_AIR };
enum GoalClass { GOAL_UNKNOWN, GOAL_PATROL, GOAL_AISEARCH, GOAL_PACKED, GOAL_LITE, GOAL_PACKEDPOS };

static unsigned ReadU32At(unsigned p, unsigned offset)
{
    return *(const unsigned*)(p + offset);
}

static int FxClassOf(unsigned fx)
{
    unsigned vptr = ReadU32At(fx, 0);
    if (vptr == (unsigned)g_approachRadiusVtable)
        return FX_APPROACH;
    if (vptr == (unsigned)g_ringApproachVtable)
        return FX_RING;
    if (vptr == (unsigned)g_pointMarkerVtable)
        return FX_MARKER;
    if (vptr == (unsigned)g_pathOrderVtable)
        return FX_PATH;
    if (vptr == (unsigned)g_airManeuverOrderVtable)
        return FX_AIR;
    return FX_UNKNOWN;
}

static int GoalClassOf(unsigned goal)
{
    unsigned vptr = ReadU32At(goal, 0);
    if (vptr == (unsigned)g_patrolGoalVtable)
        return GOAL_PATROL;
    if (vptr == (unsigned)g_aiSearchGoalVtable)
        return GOAL_AISEARCH;
    if (vptr == (unsigned)g_packedGoalVtable)
        return GOAL_PACKED;
    if (vptr == (unsigned)g_liteGoalVtable)
        return GOAL_LITE;
    if (vptr == (unsigned)g_packedPosGoalVtable)
        return GOAL_PACKEDPOS;
    return GOAL_UNKNOWN;
}

static void RegisterFx(unsigned fx)
{
    unsigned id;
    if (fx == 0 || MapFind(s_fxKeys, s_fxIds, fx, &id))
        return;
    if (s_fxCountSeen >= TRACE_OBJECT_LIMIT)
        Fail("testmode: more order objects than the trace can number\n");
    s_fxList[s_fxCountSeen] = fx;
    MapAdd(s_fxKeys, s_fxIds, s_fxUsed, &s_fxUsedCount, fx, s_fxCountSeen);
    if (FxClassOf(fx) == FX_PATH)
        MapAdd(s_linkKeys, s_linkIds, s_linkUsed, &s_linkUsedCount, fx + PATHORDER_REF, s_fxCountSeen | LINK_IN_FX);
    s_fxCountSeen++;
}

static void RegisterOrder(unsigned order)
{
    if (s_orderCountSeen >= TRACE_OBJECT_LIMIT)
        Fail("testmode: more orders than the trace can number\n");
    s_orderList[s_orderCountSeen] = order;
    MapAdd(s_orderKeys, s_orderIds, s_orderUsed, &s_orderUsedCount, order, s_orderCountSeen);
    MapAdd(s_linkKeys, s_linkIds, s_linkUsed, &s_linkUsedCount, order + ORDER_LINK, s_orderCountSeen);
    s_orderCountSeen++;
}

static int UnitIsLive(const unsigned char* u)
{
    return *(const unsigned short*)(u + UNIT_LIVE) != 0;
}

static void NumberUnitHeap(unsigned char* units, unsigned count)
{
    while (s_orderUsedCount > 0)
        s_orderKeys[s_orderUsed[--s_orderUsedCount]] = 0;
    while (s_fxUsedCount > 0)
        s_fxKeys[s_fxUsed[--s_fxUsedCount]] = 0;
    while (s_linkUsedCount > 0)
        s_linkKeys[s_linkUsed[--s_linkUsedCount]] = 0;
    s_orderCountSeen = 0;
    s_fxCountSeen = 0;
    for (unsigned i = 0; units != NULL && i < count; i++) {
        unsigned char* u = units + i * UNIT_STRIDE;
        if (!UnitIsLive(u))
            continue;
        for (unsigned which = 0; which < 2; which++) {
            unsigned order = ReadU32At((unsigned)u, which == 0 ? UNIT_LIST : UNIT_LIST2);
            for (unsigned guard = 0; order != 0 && guard < ORDER_CHAIN_LIMIT; guard++) {
                RegisterOrder(order);
                RegisterFx(ReadU32At(order, ORDER_ATTACHED));
                order = ReadU32At(order, ORDER_NEXT);
            }
        }
        unsigned motion = ReadU32At((unsigned)u, UNIT_MOTION);
        if (motion != 0 && ReadU32At(motion, 0) != 0)
            RegisterFx(ReadU32At(ReadU32At(motion, 0), GOAL_TARGET));
    }
}

typedef Elem_0043c390* SimView_g_missionOrderTableBegin;
#undef g_missionOrderTableBegin
#define g_missionOrderTableBegin (*reinterpret_cast<SimView_g_missionOrderTableBegin*>(reinterpret_cast<char*>(&g_missionOrderTableVec) + 0x4))

static const char* OrderKindName(unsigned order)
{
    return (const char*)ReadU32At((unsigned)g_missionOrderTableBegin + *(const unsigned char*)(order + ORDER_KIND) * ORDER_TABLE_STRIDE, ORDER_TABLE_NAME);
}

static int OrderPosYUnset(unsigned order)
{
    static const char* const kinds[] = { "MobileBuild", "VTOL_MobileBuild", "Reclaim", "VTOL_Reclaim" };
    const char* name = OrderKindName(order);
    for (unsigned i = 0; name != NULL && i < sizeof(kinds) / sizeof(kinds[0]); i++) {
        if (strcmp(name, kinds[i]) == 0)
            return 1;
    }
    return 0;
}

static int FxPosYUnset(unsigned fx)
{
    unsigned order = ReadU32At(fx, FX_SOURCE);
    const char* name = order != 0 ? OrderKindName(order) : NULL;
    return name != NULL && (strcmp(name, "VTOL_MobileBuild") == 0 || strcmp(name, "VTOL_Reclaim") == 0);
}

static void VisitOrders(unsigned char* units, unsigned count)
{
    char path[96];
    HashByte(20);
    for (unsigned i = 0; units != NULL && i < count; i++) {
        unsigned char* u = units + i * UNIT_STRIDE;
        if (!UnitIsLive(u))
            continue;
        HashU32(i);
        if (s_dumpNow)
            sprintf(path, "units[%u].head", i);
        VisitPointer(path, ReadU32At((unsigned)u, UNIT_HEAD), TT_LINK);
        for (unsigned which = 0; which < 2; which++) {
            unsigned order = ReadU32At((unsigned)u, which == 0 ? UNIT_LIST : UNIT_LIST2);
            HashByte((unsigned char)(which + 1));
            for (unsigned n = 0; order != 0 && n < ORDER_CHAIN_LIMIT; n++) {
                if (s_dumpNow)
                    sprintf(path, "units[%u].%s[%u]", i, which == 0 ? "list" : "list2", n);
                VisitElement(path, (const unsigned char*)order, &kOrderTable);
                if (!OrderPosYUnset(order))
                    VisitElement(path, (const unsigned char*)order, &kOrderPosYTable);
                order = ReadU32At(order, ORDER_NEXT);
            }
            HashByte(0);
        }
    }
    EndSection();
}

static void VisitGoal(const char* prefix, unsigned goal)
{
    char path[112];
    int cls;
    unsigned n;
    if (s_dumpNow)
        sprintf(path, "%s.class", prefix);
    if (goal == 0) {
        VisitU32(path, 0);
        return;
    }
    cls = GoalClassOf(goal);
    VisitU32(path, (unsigned)cls);
    if (cls == GOAL_UNKNOWN) {
        char msg[160];
        sprintf(msg, "testmode: unknown motion goal at %08x (vtable %08x)\n", goal, ReadU32At(goal, 0));
        Fail(msg);
    }
    switch (cls) {
    case GOAL_PATROL:
        VisitElement(prefix, (const unsigned char*)goal, &kPatrolGoalTable);
        n = ReadU32At(goal, PATROL_COUNT);
        if (n > PATROL_POINT_LIMIT)
            n = PATROL_POINT_LIMIT;
        if (s_dumpNow)
            sprintf(path, "%s.points", prefix);
        VisitRaw(path, (const unsigned char*)goal + PATROL_POINTS, n * 4);
        break;
    case GOAL_AISEARCH:
        VisitElement(prefix, (const unsigned char*)goal, &kAiSearchGoalTable);
        n = ReadU32At(goal, AISEARCH_COUNT);
        if (n > AISEARCH_POINT_LIMIT)
            n = AISEARCH_POINT_LIMIT;
        if (s_dumpNow)
            sprintf(path, "%s.points", prefix);
        VisitRaw(path, (const unsigned char*)goal + AISEARCH_POINTS, n * 4);
        break;
    default:
        VisitElement(prefix, (const unsigned char*)goal, &kPackedGoalTable);
        if (cls == GOAL_LITE)
            VisitElement(prefix, (const unsigned char*)goal, &kLiteGoalStateTable);
        break;
    }
}

static void VisitMotions(unsigned char* units, unsigned count)
{
    char path[96];
    HashByte(21);
    for (unsigned i = 0; units != NULL && i < count; i++) {
        unsigned char* u = units + i * UNIT_STRIDE;
        unsigned motion;
        if (!UnitIsLive(u))
            continue;
        motion = ReadU32At((unsigned)u, UNIT_MOTION);
        HashU32(i);
        HashByte(motion != 0);
        if (motion == 0)
            continue;
        if (s_dumpNow)
            sprintf(path, "units[%u].motion", i);
        VisitElement(path, (const unsigned char*)motion, &kUnitMotionTable);
        if (s_dumpNow)
            strcat(path, ".goal");
        VisitGoal(path, ReadU32At(motion, 0));
    }
    EndSection();
}

static void VisitFx()
{
    char path[64];
    char field[80];
    HashByte(22);
    for (unsigned id = 0; id < s_fxCountSeen; id++) {
        unsigned fx = s_fxList[id];
        int cls = FxClassOf(fx);
        unsigned flags;
        HashU32(id);
        if (s_dumpNow)
            sprintf(path, "orderFx[%u]", id);
        if (s_dumpNow)
            sprintf(field, "%s.class", path);
        VisitU32(field, (unsigned)cls);
        if (cls == FX_UNKNOWN) {
            if (s_dumpNow)
                sprintf(field, "%s.vtableDelta", path);
            VisitU32(field, ReadU32At(fx, 0) - (unsigned)g_pathOrderVtable);
            continue;
        }
        switch (cls) {
        case FX_APPROACH:
            VisitElement(path, (const unsigned char*)fx, &kApproachRadiusTable);
            break;
        case FX_RING:
            VisitElement(path, (const unsigned char*)fx, &kRingApproachTable);
            break;
        case FX_MARKER:
            VisitElement(path, (const unsigned char*)fx, &kPointMarkerTable);
            break;
        case FX_PATH:
            flags = *(const unsigned short*)(fx + PATHORDER_FLAGS);
            VisitElement(path, (const unsigned char*)fx, &kPathOrderTable);
            if (!FxPosYUnset(fx))
                VisitElement(path, (const unsigned char*)fx, &kPathOrderPosYTable);
            if (flags & 0x40)
                VisitElement(path, (const unsigned char*)fx, &kPathOrderHeadingTable);
            if (flags & 1)
                VisitElement(path, (const unsigned char*)fx, &kPathOrderPieceTable);
            if ((flags & 0x82) == 2)
                VisitElement(path, (const unsigned char*)fx, &kPathOrderFollowTable);
            break;
        case FX_AIR:
            flags = *(const unsigned short*)(fx + AIRMANEUVER_FLAGS);
            VisitElement(path, (const unsigned char*)fx, &kAirManeuverOrderTable);
            if (flags & 1)
                VisitElement(path, (const unsigned char*)fx, &kAirManeuverOrderHeadingTable);
            break;
        }
    }
    EndSection();
}

static void VisitChannel(const char* prefix, const unsigned char* channel)
{
    char path[128];
    unsigned state = *(const unsigned*)channel;
    int sp;
    unsigned live;
    if (state == 0) {
        if (s_dumpNow)
            sprintf(path, "%s.state", prefix);
        VisitU32(path, 0);
        return;
    }
    VisitElement(prefix, channel, &kScriptChannelTable);
    sp = *(const int*)(channel + 8);
    live = sp < -1 ? 0 : (unsigned)(sp + 1);
    if (live > SCRIPT_STACK_LIMIT)
        live = SCRIPT_STACK_LIMIT;
    if (s_dumpNow)
        sprintf(path, "%s.stack", prefix);
    VisitRaw(path, channel + SCRIPT_STACK, live * 4);
    if ((state & 0xff000000) == 0x2000000) {
        switch (state & 0xf00000) {
        case 0x100000:
        case 0x200000:
            VisitElement(prefix, channel, &kScriptChannelPieceTable);
            break;
        case 0x400000:
            VisitElement(prefix, channel, &kScriptChannelDelayTable);
            break;
        case 0x800000:
            VisitElement(prefix, channel, &kScriptChannelWaitTable);
            break;
        }
    }
}

static void VisitScript(unsigned i, const unsigned char* script)
{
    char path[112];
    unsigned table = ReadU32At((unsigned)script, SCRIPT_TABLE);
    unsigned statics = ReadU32At((unsigned)script, SCRIPT_STATICS);
    unsigned pieces = ReadU32At((unsigned)script, SCRIPT_PIECES);
    unsigned objectState = ReadU32At((unsigned)script, SCRIPT_OBJECT_STATE);
    unsigned staticCount = 0;
    unsigned pieceCount = 0;
    if (s_dumpNow)
        sprintf(path, "units[%u].script", i);
    VisitElement(path, script, &kScriptStateTable);
    if (table != 0) {
        staticCount = ReadU32At(table, TABLE_STATIC_COUNT);
        pieceCount = ReadU32At(table, TABLE_PIECE_COUNT);
    }
    if (staticCount > SCRIPT_STATIC_LIMIT || statics == 0)
        staticCount = 0;
    if (pieceCount > SCRIPT_PIECE_LIMIT)
        pieceCount = 0;
    for (unsigned c = 0; c < SCRIPT_CHANNEL_COUNT; c++) {
        if (s_dumpNow)
            sprintf(path, "units[%u].script.channels[%u]", i, c);
        VisitChannel(path, script + SCRIPT_CHANNELS + c * SCRIPT_CHANNEL_STRIDE);
    }
    if (s_dumpNow)
        sprintf(path, "units[%u].script.statics", i);
    VisitU32(path, staticCount);
    VisitRaw(path, (const unsigned char*)statics, staticCount * 4);
    if (pieces == 0)
        pieceCount = 0;
    for (unsigned p = 0; p < pieceCount; p++) {
        if (s_dumpNow)
            sprintf(path, "units[%u].script.pieces[%u]", i, p);
        VisitElement(path, (const unsigned char*)pieces + p * SCRIPT_PIECE_STRIDE, &kScriptPieceTable);
    }
    if (objectState == 0)
        pieceCount = 0;
    for (unsigned q = 0; q < pieceCount; q++) {
        if (s_dumpNow)
            sprintf(path, "units[%u].script.transforms[%u]", i, q);
        VisitElement(path, (const unsigned char*)objectState + OBJECT_STATE_PIECES + q * OBJECT_STATE_STRIDE,
                     &kPieceTransformTable);
    }
}

static void VisitScripts(unsigned char* units, unsigned count)
{
    HashByte(23);
    for (unsigned i = 0; units != NULL && i < count; i++) {
        unsigned char* u = units + i * UNIT_STRIDE;
        unsigned script;
        if (!UnitIsLive(u))
            continue;
        script = ReadU32At((unsigned)u, UNIT_SCRIPT);
        HashU32(i);
        HashByte(script != 0);
        if (script != 0)
            VisitScript(i, (const unsigned char*)script);
    }
    EndSection();
}

static void VisitUnitHeap()
{
    unsigned char* units = (unsigned char*)GameU32(GAME_UNITS);
    unsigned count = *(unsigned short*)GameAt(GAME_POOL_COUNT);
    NumberUnitHeap(units, count);
    VisitOrders(units, count);
    VisitMotions(units, count);
    VisitFx();
    VisitScripts(units, count);
}

enum TimerClass {
    TIMER_UNKNOWN,
    TIMER_SQUAD,
    TIMER_ASSAULT,
    TIMER_ESCORT,
    TIMER_SQUADSCOUT,
    TIMER_SPATIAL,
    TIMER_SCOUT,
    TIMER_BUILD
};


static int TimerClassOf(unsigned slot)
{
    static const unsigned char classes[SQUAD_TIMERS] = {
        TIMER_UNKNOWN, TIMER_BUILD, TIMER_ASSAULT, TIMER_ESCORT, TIMER_SCOUT,
        TIMER_SQUAD, TIMER_ASSAULT, TIMER_ESCORT, TIMER_SQUADSCOUT, TIMER_SPATIAL
    };
    return classes[slot];
}

int HarnessConditionClass(void* condition);

static int ConditionClassOf(unsigned condition)
{
    return HarnessConditionClass((void*)condition);
}

static unsigned VectorCount(unsigned vec, unsigned elem, const unsigned char** first)
{
    unsigned begin = ReadU32At(vec, VECTOR_FIRST);
    unsigned end = ReadU32At(vec, VECTOR_LAST);
    *first = (const unsigned char*)begin;
    if (begin == 0 || end < begin || (end - begin) % elem != 0 || (end - begin) / elem > VECTOR_LIMIT)
        return 0;
    return (end - begin) / elem;
}

static void VisitRawVector(const char* path, unsigned vec, unsigned elem)
{
    const unsigned char* first;
    unsigned n = VectorCount(vec, elem, &first);
    VisitU32(path, n);
    VisitRaw(path, first, n * elem);
}

static void VisitUnitVector(const char* path, unsigned vec)
{
    const unsigned char* first;
    char name[128];
    unsigned n = VectorCount(vec, 4, &first);
    sprintf(name, "%s.size", path);
    VisitU32(name, n);
    for (unsigned k = 0; k < n; k++) {
        sprintf(name, "%s[%u]", path, k);
        VisitPointer(name, *(const unsigned*)(first + k * 4), TT_UNIT);
    }
}

static void VisitString(const char* path, const char* text)
{
    unsigned len = 0;
    HashByte(text != NULL);
    if (text != NULL) {
        while (len < STRING_LIMIT && text[len] != '\0')
            len++;
        HashU32(len);
        HashBuf((const unsigned char*)text, len);
    }
    if (s_dumpNow) {
        DumpPrefix(path);
        if (text == NULL)
            fputs("null\n", s_dump);
        else
            fprintf(s_dump, "str:%.*s\n", (int)len, text);
    }
}

static unsigned char* PlayerAt(unsigned slot)
{
    return GameAt(GAME_PLAYERS) + slot * PLAYER_STRIDE;
}

struct VectorSpec {
    const char* name;
    unsigned offset;
    unsigned elem;
};

static void VisitPlayerAI()
{
    static const VectorSpec vectors[] = {
        { "cells", 0x4d, 8 }, { "vec_65", 0x65, 3 }, { "counts", 0x7d, 2 }, { "vec_8d", 0x8d, 1 },
        { "weights", 0x9d, 1 }, { "vec_ad", 0xad, 1 }, { "vec_bd", 0xbd, 4 }, { "values", 0xcd, 4 },
        { "locked", 0xdd, 4 }
    };
    char path[96];
    HashByte(24);
    for (unsigned i = 0; i < MAX_PLAYERS; i++) {
        unsigned ai = (unsigned)g_playerAI[i];
        HashU32(i);
        HashByte(ai != 0);
        if (ai == 0)
            continue;
        sprintf(path, "players[%u].ai", i);
        VisitElement(path, (const unsigned char*)ai, &kPlayerAITable);
        sprintf(path, "players[%u].ai.visible", i);
        VisitUnitVector(path, ai + 0x5);
        sprintf(path, "players[%u].ai.known", i);
        VisitUnitVector(path, ai + 0x15);
        sprintf(path, "players[%u].ai.factories", i);
        VisitUnitVector(path, ai + 0x25);
        for (unsigned v = 0; v < sizeof vectors / sizeof vectors[0]; v++) {
            sprintf(path, "players[%u].ai.%s", i, vectors[v].name);
            VisitRawVector(path, ai + vectors[v].offset, vectors[v].elem);
        }
    }
    EndSection();
}

static void VisitTimer(const char* path, unsigned slot, unsigned timer)
{
    char field[128];
    int cls = TimerClassOf(slot);
    sprintf(field, "%s.class", path);
    VisitU32(field, (unsigned)cls);
    switch (cls) {
    case TIMER_ASSAULT:
        VisitElement(path, (const unsigned char*)timer, &kAssaultTimerTable);
        break;
    case TIMER_ESCORT:
        VisitElement(path, (const unsigned char*)timer, &kEscortTimerTable);
        break;
    case TIMER_SPATIAL:
        VisitElement(path, (const unsigned char*)timer, &kSpatialTimerTable);
        break;
    default:
        VisitElement(path, (const unsigned char*)timer, &kSquadTimerTable);
        break;
    }
}

static void VisitSquadManagers()
{
    char path[96];
    HashByte(25);
    for (unsigned i = 0; i < PLAYER_SLOTS; i++) {
        unsigned manager = ReadU32At((unsigned)PlayerAt(i), PLAYER_MANAGER);
        HashU32(i);
        HashByte(manager != 0);
        if (manager == 0)
            continue;
        sprintf(path, "players[%u].squadManager", i);
        VisitElement(path, (const unsigned char*)manager, &kSquadManagerTable);
        for (unsigned t = 0; t < SQUAD_TIMERS; t++) {
            unsigned timer = ReadU32At(manager, MANAGER_TIMERS + t * 4);
            HashByte((unsigned char)t);
            HashByte(timer != 0);
            if (timer == 0)
                continue;
            sprintf(path, "players[%u].squadManager.timers[%u]", i, t);
            VisitTimer(path, t, timer);
        }
    }
    EndSection();
}

static void VisitSquads()
{
    char path[96];
    HashByte(26);
    for (unsigned i = 0; i < PLAYER_SLOTS; i++) {
        unsigned squads = ReadU32At((unsigned)PlayerAt(i), PLAYER_SQUADS);
        HashU32(i);
        HashByte(squads != 0);
        if (squads == 0)
            continue;
        for (unsigned s = 0; s < SQUAD_COUNT; s++) {
            unsigned squad = squads + s * SQUAD_STRIDE;
            sprintf(path, "players[%u].squads[%u]", i, s);
            VisitElement(path, (const unsigned char*)squad, &kSquadTable);
            strcat(path, ".items");
            VisitUnitVector(path, squad + SQUAD_ITEMS);
        }
    }
    EndSection();
}

static void VisitUnitResources()
{
    char path[96];
    HashByte(27);
    for (unsigned i = 0; i < PLAYER_SLOTS; i++) {
        unsigned econ = ReadU32At((unsigned)PlayerAt(i), PLAYER_ECON);
        HashU32(i);
        HashByte(econ != 0);
        if (econ == 0)
            continue;
        sprintf(path, "players[%u].econ", i);
        VisitElement(path, (const unsigned char*)econ, &kUnitResourcesTable);
    }
    EndSection();
}

static void VisitCondition(const char* path, unsigned condition)
{
    char field[128];
    int cls = ConditionClassOf(condition);
    sprintf(field, "%s.class", path);
    VisitU32(field, (unsigned)cls);
    if (cls == COND_UNKNOWN) {
        char msg[160];
        sprintf(msg, "testmode: unknown mission condition at %08x (vtable %08x) at %s\n", condition,
                ReadU32At(condition, 0), path);
        Fail(msg);
    }
    VisitElement(path, (const unsigned char*)condition, &kMissionConditionTable);
    if (cls == COND_VICTORY_KILL_MOBILE)
        VisitElement(path, (const unsigned char*)condition, &kConditionNumUnitsTable);
    else if (cls == COND_VICTORY_KILL_TYPE || cls == COND_DEFEAT_TYPE_KILLED)
        VisitElement(path, (const unsigned char*)condition, &kConditionNumLeftTable);
}

static void VisitMissionConditions()
{
    unsigned conditions = GameU32(GAME_CONDITIONS);
    char path[96];
    HashByte(28);
    HashByte(conditions != 0);
    if (conditions != 0) {
        VisitElement("conditions", (const unsigned char*)conditions, &kMissionConditionsTable);
        for (unsigned list = 0; list < 2; list++) {
            unsigned count = ReadU32At(conditions, list == 0 ? CONDITIONS_VICTORY_COUNT : CONDITIONS_DEFEAT_COUNT);
            unsigned base = list == 0 ? CONDITIONS_VICTORY : CONDITIONS_DEFEAT;
            if (count > CONDITIONS_LIMIT)
                count = 0;
            HashByte((unsigned char)(list + 1));
            for (unsigned k = 0; k < count; k++) {
                unsigned condition = ReadU32At(conditions, base + k * 4);
                HashByte(condition != 0);
                if (condition == 0)
                    continue;
                sprintf(path, "conditions.%s[%u]", list == 0 ? "victory" : "defeat", k);
                VisitCondition(path, condition);
            }
        }
    }
    EndSection();
}

static void VisitWeaponTable(const char* path, unsigned table)
{
    char field[128];
    unsigned first;
    unsigned last;
    unsigned n;
    HashByte(table != 0);
    if (table == 0)
        return;
    first = ReadU32At(table, WEAPON_TABLE_FIRST);
    last = ReadU32At(table, WEAPON_TABLE_LAST);
    n = first != 0 && last >= first && (last - first) % WEAPON_ENTRY_SIZE == 0
        ? (last - first) / WEAPON_ENTRY_SIZE : 0;
    if (n > VECTOR_LIMIT)
        n = 0;
    sprintf(field, "%s.entries", path);
    VisitU32(field, n);
    for (unsigned k = 0; k < n; k++) {
        unsigned entry = first + k * WEAPON_ENTRY_SIZE;
        sprintf(field, "%s[%u].name", path, k);
        VisitString(field, (const char*)ReadU32At(entry, 0));
        sprintf(field, "%s[%u].value", path, k);
        VisitU32(field, ReadU32At(entry, 4));
    }
}

static void VisitWeaponDefs()
{
    char path[96];
    HashByte(30);
    for (unsigned i = 0; i < WEAPON_COUNT; i++) {
        const unsigned char* w = GameAt(GAME_WEAPONS) + i * WEAPONDEF_STRIDE;
        sprintf(path, "weapons[%u]", i);
        VisitElement(path, w, &kWeaponDefDataTable);
        strcat(path, ".damage");
        VisitWeaponTable(path, ReadU32At((unsigned)w, WEAPON_TABLE));
    }
    EndSection();
}

static void VisitUnitDefs()
{
    unsigned char* defs = (unsigned char*)GameU32(GAME_DEFS);
    unsigned count = GameU32(GAME_DEF_COUNT);
    char path[96];
    HashByte(29);
    if (defs == NULL || count > DEF_LIMIT)
        count = 0;
    VisitU32("defs.count", count);
    for (unsigned i = 0; i < count; i++) {
        const unsigned char* d = defs + i * UNITDEF_STRIDE;
        unsigned yardmap = ReadU32At((unsigned)d, DEF_YARDMAP);
        unsigned ids = ReadU32At((unsigned)d, DEF_IDS);
        unsigned idCount = ReadU32At((unsigned)d, DEF_ID_COUNT);
        unsigned yardSize = (unsigned)*(const short*)(d + DEF_FOOTPRINT_X) * (unsigned)*(const short*)(d + DEF_FOOTPRINT_Z);
        sprintf(path, "defs[%u]", i);
        VisitElement(path, d, &kUnitDefDataTable);
        if (yardmap == 0 || yardSize > DEF_YARD_LIMIT)
            yardSize = 0;
        strcat(path, ".yardmap");
        VisitU32(path, yardSize);
        VisitRaw(path, (const unsigned char*)yardmap, yardSize);
        if (ids == 0 || idCount > DEF_ID_LIMIT)
            idCount = 0;
        sprintf(path, "defs[%u].ids", i);
        VisitU32(path, idCount);
        VisitRaw(path, (const unsigned char*)ids, idCount * 2);
        for (unsigned m = 0; m < 4; m++) {
            unsigned mask = ReadU32At((unsigned)d, m < 3 ? DEF_WEAPON_CATEGORIES + m * 4 : DEF_CATEGORIES);
            sprintf(path, "defs[%u].categories[%u]", i, m);
            HashByte(mask != 0);
            if (mask != 0)
                VisitRaw(path, (const unsigned char*)mask, DEF_MASK_SIZE);
        }
    }
    EndSection();
}

static void VisitMission()
{
    unsigned mission = GameU32(GAME_MISSION);
    char path[96];
    HashByte(31);
    HashByte(mission != 0);
    if (mission != 0) {
        unsigned units = ReadU32At(mission, MISSION_UNITS);
        unsigned unitCount = ReadU32At(mission, MISSION_UNIT_COUNT);
        unsigned rules = ReadU32At(mission, MISSION_RULES);
        unsigned ruleCount = ReadU32At(mission, MISSION_RULE_COUNT);
        unsigned features = ReadU32At(mission, MISSION_FEATURES);
        unsigned featureCount = ReadU32At(mission, MISSION_FEATURE_COUNT);
        VisitElement("mission", (const unsigned char*)mission, &kMissionDataTable);
        if (ReadU32At(mission, MISSION_TYPE) == MISSION_TYPE_CAMPAIGN)
            VisitElement("mission", (const unsigned char*)mission, &kMissionStartRestTable);
        if (units == 0 || unitCount > MISSION_LIMIT)
            unitCount = 0;
        if (rules == 0 || ruleCount > MISSION_LIMIT)
            ruleCount = 0;
        if (features == 0 || featureCount > MISSION_LIMIT)
            featureCount = 0;
        VisitU32("mission.units.count", unitCount);
        for (unsigned ku = 0; ku < unitCount; ku++) {
            unsigned unit = units + ku * MISSION_UNIT_SIZE;
            sprintf(path, "mission.units[%u].name", ku);
            VisitString(path, (const char*)ReadU32At(unit, 0));
            sprintf(path, "mission.units[%u].ident", ku);
            VisitString(path, (const char*)ReadU32At(unit, 4));
            sprintf(path, "mission.units[%u].initialMission", ku);
            VisitString(path, (const char*)ReadU32At(unit, 8));
            sprintf(path, "mission.units[%u]", ku);
            VisitRaw(path, (const unsigned char*)unit + 0xc, MISSION_UNIT_SIZE - 0xc);
        }
        VisitU32("mission.rules.count", ruleCount);
        VisitRaw("mission.rules", (const unsigned char*)rules, ruleCount * MISSION_RULE_SIZE);
        VisitU32("mission.features.count", featureCount);
        for (unsigned kf = 0; kf < featureCount; kf++) {
            unsigned feature = features + kf * MISSION_FEATURE_SIZE;
            sprintf(path, "mission.features[%u].name", kf);
            VisitString(path, (const char*)feature);
            sprintf(path, "mission.features[%u].pos", kf);
            VisitRaw(path, (const unsigned char*)feature + MISSION_FEATURE_POS, 8);
        }
    }
    EndSection();
}

static void VisitPlayerHeap()
{
    VisitPlayerAI();
    VisitSquadManagers();
    VisitSquads();
    VisitUnitResources();
    VisitMissionConditions();
}

static void VisitConstants()
{
    VisitUnitDefs();
    VisitWeaponDefs();
    VisitMission();
}

static void VisitWorld()
{
    VisitGameScalars();
    VisitMapCells();
    VisitVisibility();
    VisitExplored();
    VisitSpatial();
    VisitEyes();
    VisitPathfinder();
    VisitMoveClasses();
    VisitPassMaps();
    VisitUnitHeap();
    VisitPlayerHeap();
    if (s_traceTick == 1)
        VisitConstants();
}

enum {
    GAME_LOCAL_PLAYER = 0x2a42,
    GAME_PLAYER_INDEX = 0x2a43,
    GAME_ORDER_MODE = 0x2cc3,
    GAME_BUILD_TYPE = 0x2cc4,
    GAME_HOVER_UNIT = 0x2cba,
    GAME_POS = 0x2caa,
    UNIT_PLAYER = 0x96,
    UNIT_TYPE = 0xa6,
    UNIT_FLAGS = 0x110,
    UNITDEF_NAME = 0x20,
    UNIT_SELECTED = 0x10
};

void __stdcall IssueOrderToSelection(void* mouse, int mode, int kind, int* pos, int typeId, int amount);
void __stdcall IssueMobileBuildOrders(void* mouse);
int UpdatePlacementGhostValidity(void);
int __stdcall GetGroundHeight(int* pos);

static unsigned char* PoolUnit(unsigned index)
{
    return (unsigned char*)GameU32(GAME_UNITS) + index * UNIT_STRIDE;
}

static int FindDefIndex(const ScriptedOrder* o, const char* name)
{
    unsigned char* defs = (unsigned char*)GameU32(GAME_DEFS);
    unsigned count = GameU32(GAME_DEF_COUNT);
    for (unsigned i = 1; i < count; i++) {
        if (_stricmp((const char*)defs + i * UNITDEF_STRIDE + UNITDEF_NAME, name) == 0)
            return (int)i;
    }
    char detail[100];
    sprintf(detail, ", the game has no unit type \"%.40s\"", name);
    FailLine(o->lineNo, "names an unknown unit type", "order", detail);
    return 0;
}

static int UnitIsSlot(const unsigned char* u, int slot)
{
    return *(unsigned*)(u + UNIT_PLAYER) == (unsigned)(GameAt(GAME_PLAYERS) + slot * PLAYER_STRIDE);
}

static int SelectorMatches(const unsigned char* u, const UnitSelector* sel, int defIndex)
{
    if (*(unsigned short*)(u + UNIT_TYPE) == 0 || !UnitIsSlot(u, sel->slot))
        return 0;
    return sel->all || *(unsigned short*)(u + UNIT_TYPE) == defIndex;
}

static int FindNth(const ScriptedOrder* o, const UnitSelector* sel)
{
    int defIndex = sel->all ? 0 : FindDefIndex(o, sel->type);
    unsigned count = *(unsigned short*)GameAt(GAME_POOL_COUNT);
    int seen = 0;
    for (unsigned i = 0; i < count; i++) {
        if (SelectorMatches(PoolUnit(i), sel, defIndex) && ++seen == sel->count)
            return (int)i;
    }
    char detail[160];
    sprintf(detail, ", only %d live of that selector at tick %u", seen, GameU32(GAME_TICK));
    FailLine(o->lineNo, "names a unit that does not exist", "order", detail);
    return 0;
}

static void IssueScriptedOrder(const ScriptedOrder* o)
{
    unsigned count = *(unsigned short*)GameAt(GAME_POOL_COUNT);
    unsigned i;
    int defIndex = o->who.all ? 0 : FindDefIndex(o, o->who.type);
    int buildType = o->kind == ORDER_BUILD ? FindDefIndex(o, o->buildType) : 0;
    int target = 0;
    int pos[3];

    unsigned char* selected = (unsigned char*)malloc(count + 1);
    int chosen = 0;
    int seen = 0;
    for (i = 0; i < count; i++) {
        unsigned char* u = PoolUnit(i);
        selected[i] = 0;
        if (*(unsigned short*)(u + UNIT_TYPE) == 0 || !UnitIsSlot(u, o->who.slot))
            continue;
        selected[i] = (u[UNIT_FLAGS] & UNIT_SELECTED) ? 1 : 2;
        u[UNIT_FLAGS] &= ~UNIT_SELECTED;
        if (SelectorMatches(u, &o->who, defIndex) && (o->who.all || ++seen == o->who.count)) {
            u[UNIT_FLAGS] |= UNIT_SELECTED;
            chosen++;
        }
    }
    if (chosen == 0) {
        char detail[160];
        sprintf(detail, ", only %d live of that selector at tick %u", seen, GameU32(GAME_TICK));
        FailLine(o->lineNo, "names a unit that does not exist", "order", detail);
    }

    int x = o->x;
    int z = o->z;
    if (o->hasUnitTarget) {
        target = FindNth(o, &o->targetUnit);
        unsigned char* t = PoolUnit(target);
        x = *(int*)(t + 0x6a) >> 16;
        z = *(int*)(t + 0x72) >> 16;
    }
    if (x >= (int)GameU32(0x1422b) || z >= (int)GameU32(0x1422f)) {
        FailLine(o->lineNo, "has a position outside the map", "order", "");
    }
    pos[0] = x << 16;
    pos[2] = z << 16;
    pos[1] = GetGroundHeight(pos);
    if (pos[1] < *GameAt(0x1427f))
        pos[1] = *GameAt(0x1427f);
    pos[1] <<= 16;

    unsigned char savedLocal = *GameAt(GAME_LOCAL_PLAYER);
    unsigned char savedIndex = *GameAt(GAME_PLAYER_INDEX);
    unsigned short savedHover = *(unsigned short*)GameAt(GAME_HOVER_UNIT);
    unsigned char mouse[32];
    memset(mouse, 0, sizeof mouse);
    *GameAt(GAME_LOCAL_PLAYER) = (unsigned char)o->who.slot;
    *GameAt(GAME_PLAYER_INDEX) = (unsigned char)o->who.slot;
    *(unsigned short*)GameAt(GAME_HOVER_UNIT) = (unsigned short)target;
    memcpy(GameAt(GAME_POS), pos, sizeof pos);
    *GameAt(GAME_ORDER_MODE) = s_orderModes[o->kind].mode;
    if (o->kind == ORDER_BUILD) {
        *(unsigned short*)GameAt(GAME_BUILD_TYPE) = (unsigned short)buildType;
        if (!UpdatePlacementGhostValidity()) {
            char detail[100];
            sprintf(detail, ", %.40s cannot be built at %d %d", o->buildType, x, z);
            FailLine(o->lineNo, "gives a bad build site", "order", detail);
        }
        IssueMobileBuildOrders(mouse);
    } else {
        IssueOrderToSelection(GameAt(0x2c76), s_orderModes[o->kind].mode, 0, (int*)GameAt(GAME_POS), 0, 0);
    }
    *GameAt(GAME_ORDER_MODE) = 1;
    *(unsigned short*)GameAt(GAME_HOVER_UNIT) = savedHover;
    *GameAt(GAME_LOCAL_PLAYER) = savedLocal;
    *GameAt(GAME_PLAYER_INDEX) = savedIndex;

    for (i = 0; i < count; i++) {
        unsigned char* u = PoolUnit(i);
        if (selected[i] == 0)
            continue;
        u[UNIT_FLAGS] &= ~UNIT_SELECTED;
        if (selected[i] == 1)
            u[UNIT_FLAGS] |= UNIT_SELECTED;
    }
    free(selected);
}

void __stdcall ApplyUnitDeath(void* cmd, int local);

static void ReachQuirkGuard()
{
    if (s_quirkReach == 1) {
        unsigned char cmd[PACKET_UNIT_DEATH_SIZE];
        memset(cmd, 0, sizeof cmd);
        cmd[0] = PACKET_UNIT_DEATH;
        ApplyUnitDeath(cmd, 0);
        return;
    }
    if (s_quirkReach == 2) {
        for (int slot = 0; slot < MAX_PLAYERS; slot++) {
            if (!s_players[slot].named || s_players[slot].human)
                continue;
            unsigned manager = ReadU32At((unsigned)PlayerAt(slot), PLAYER_MANAGER);
            unsigned char* ai = g_playerAI[slot];
            unsigned count = *(unsigned short*)GameAt(GAME_POOL_COUNT);
            if (manager == 0 || ai == 0)
                Fail("quirks_reach: the AI player has no manager\n");
            for (unsigned i = 0; i < count; i++) {
                unsigned char* u = PoolUnit(i);
                if (*(unsigned short*)(u + UNIT_TYPE) == 0 || !UnitIsSlot(u, slot))
                    continue;
                g_seams.ai->SetUnitSquad((Unit*)u, SQUAD_SCOUT);
                memset(ai + AI_CENTRE, 0, AI_CENTRE_SIZE);
                void* timer = (void*)ReadU32At(manager, MANAGER_TIMERS + MANAGER_SCOUT_TIMER * 4);
                void (__thiscall* onTimer)(void*) = (void (__thiscall*)(void*))(*(void***)timer)[0];
                unsigned char* allied = PlayerAt(slot) + PLAYER_ALLIED;
                unsigned char saved[PLAYER_SLOTS];
                memcpy(saved, allied, sizeof saved);
                memset(allied, 1, sizeof saved);
                onTimer(timer);
                memcpy(allied, saved, sizeof saved);
                return;
            }
            Fail("quirks_reach: no AI player with a unit at the reach tick\n");
        }
        Fail("quirks_reach: no AI player at the reach tick\n");
        return;
    }
    for (unsigned i = 0, count = *(unsigned short*)GameAt(GAME_POOL_COUNT); i < count; i++) {
        unsigned char* u = PoolUnit(i);
        if (*(unsigned short*)(u + UNIT_TYPE) == 0 || !UnitIsSlot(u, 0))
            continue;
        void* script = (void*)ReadU32At((unsigned)u, UNIT_SCRIPT);
        if (script == NULL)
            Fail("quirks_reach: the unit has no script\n");
        void (__thiscall* setUnitValue)(void*, int, int) = (void (__thiscall*)(void*, int, int))(*(void***)script)[SCRIPT_SET_UNIT_VALUE_SLOT];
        setUnitValue(script, SCRIPT_VALUE_YARD_OPEN, 1);
        return;
    }
    Fail("quirks_reach: no unit of player 0 at the reach tick\n");
}

void HarnessBeforeStep()
{
    if (!s_testMode)
        return;
    unsigned tick = GameU32(GAME_TICK);
    if (s_quirkTamper && tick == s_quirkTamperTick)
        QuirksFlipFirstSim(&g_quirks);
    if (s_quirkReach && tick == s_quirkReachTick)
        ReachQuirkGuard();
    if (QuirksHash(g_quirks) != s_quirkHash) {
        char msg[96];
        sprintf(msg, "testmode: quirk flags changed at tick %u\n", tick);
        Fail(msg);
    }
    for (int i = 0; i < s_orderCount; i++) {
        if (s_orders[i].tick == tick)
            IssueScriptedOrder(&s_orders[i]);
    }
}

void HarnessPhase(int phase)
{
    if (!s_testMode || s_trace == NULL)
        return;
    unsigned tick = GameU32(GAME_TICK);
    unsigned holdrand = GameHoldrand(RAND_STREAM_MAIN);
    unsigned parkMiller = g_randomSeed;

    s_traceTick = tick;
    s_tracePhase = s_phaseNames[phase];
    s_dumpNow = s_dumpEnabled && tick == s_dumpTick;
    s_hashLo = 0x84222325;
    s_hashHi = 0xcbf29ce4;

    VisitU32("gameTick", tick);
    VisitU32("rng.parkMiller", parkMiller);
    VisitU32("rng.crtHoldrand", holdrand);
    VisitUnits();
    VisitProjectiles();
    VisitExplosions();
    VisitPlayers();
    VisitFeatures();
    VisitWind();
    VisitMeteors();

    fprintf(s_trace, "%u %s %08x%08x\n", tick, s_phaseNames[phase], s_hashHi, s_hashLo);
    if (phase == 7 || s_dumpNow) {
        s_hashLo = 0x84222325;
        s_hashHi = 0xcbf29ce4;
        VisitWorld();
        if (phase == 7)
            fprintf(s_trace, "%u %s %08x%08x\n", tick, s_phaseNames[8], s_hashHi, s_hashLo);
    }
    if (phase == 7) {
        fflush(s_trace);
        if (s_dumpNow)
            fflush(s_dump);
    }
}

static void OpenTrace(unsigned controlWord)
{
    char commit[64];
    char exePath[MAX_PATH + 16];
    strcpy(commit, "unknown");
    DWORD len = host_file_exe_path(exePath, MAX_PATH);
    while (len > 0 && exePath[len - 1] != '\\' && exePath[len - 1] != '/')
        len--;
    strcpy(exePath + len, "commit.txt");
    FILE* c = fopen(exePath, "rb");
    if (c != NULL) {
        char line[64];
        if (fgets(line, sizeof line, c) != NULL) {
            int ok = 1;
            for (int k = 0; k < 40; k++) {
                if (!((line[k] >= '0' && line[k] <= '9') || (line[k] >= 'a' && line[k] <= 'f')))
                    ok = 0;
            }
            if (ok && (line[40] == '\0' || isspace((unsigned char)line[40]))) {
                memcpy(commit, line, 40);
                commit[40] = '\0';
            }
        }
        fclose(c);
    }

    strcpy(s_commit, commit);
    s_trace = fopen("trace.txt", "wb");
    if (s_trace == NULL)
        Fail("testmode: cannot write trace.txt\n");
    fprintf(s_trace, "bt-trace %d\ncommit %s\nscenario %s\ngame_seed %u\ncrt_seed %u\nview_seed %u\n"
                     "fpu_control_word 0x%05X\n",
            TRACE_FORMAT, commit, s_scenarioPath, s_gameSeed, s_crtSeed, s_viewSeed, controlWord);
    if (!QuirksAllOff(g_quirks))
        fprintf(s_trace, "quirks %08x %s\n", s_quirkHash, s_quirkSpec);
    if (s_dumpEnabled) {
        s_dump = fopen("dump.txt", "wb");
        if (s_dump == NULL)
            Fail("testmode: cannot write dump.txt\n");
        fprintf(s_dump, "bt-dump %d\n", TRACE_FORMAT);
    }
}

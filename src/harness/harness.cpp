#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <time.h>

#define Mission MissionView
#define TdfRecord TdfRecordView
#define TdfFile TdfFileView
#include "../../include/ta_types.h"
#undef Mission
#undef TdfRecord
#undef TdfFile
#include "../map/mission.h"

extern Game* g_game;

void __stdcall SetMissionType(int owner);
void __stdcall ResetPlayerSlots(void);
void ApplySlotsToGamePlayers();
void InitMissionStatus();

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
static int s_started = 0;
static int s_dumpEnabled = 0;
static char s_scenarioPath[512];
static FILE* s_trace = NULL;
static FILE* s_dump = NULL;
static unsigned s_dumpTick = 0;

static void Fail(const char* text)
{
    DWORD written;
    OutputDebugStringA(text);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), text, (DWORD)strlen(text), &written, NULL);
    ExitProcess(2);
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
    else if (strcmp(key, "ticks") == 0) {
        s_ticks = ParseNumber(lineNo, key, value);
        if (s_ticks == 0)
            FailLine(lineNo, "must be at least 1", key, "");
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
    g_game->field_2a3c = (unsigned short)active;
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

void HarnessInit(char* cmdLine)
{
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
}

int HarnessTestMode()
{
    return s_testMode;
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
    if (!s_testMode || (unsigned)g_game->ticks < s_ticks)
        return;
    char msg[64];
    DWORD written;
    if (s_trace != NULL)
        fflush(s_trace);
    if (s_dump != NULL)
        fflush(s_dump);
    sprintf(msg, "testmode: reached tick %u\n", s_ticks);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, (DWORD)strlen(msg), &written, NULL);
    ExitProcess(0);
}

#include "trace_schema.h"

#ifdef BX_ROUTE
extern "C" unsigned __cdecl bx_game_holdrand(void);
#else
extern "C" void* __cdecl _getptd(void);
#endif
extern unsigned int g_randomSeed;

enum {
    GAME_PLAYERS = 0x1b63,
    GAME_WEAPONS = 0x2cf3,
    GAME_PROJECTILE_COUNT = 0x141f3,
    GAME_PROJECTILES = 0x141f7,
    GAME_SPOT_ENTRIES = 0x1420b,
    GAME_SPOT_USED_HEAD = 0x14213,
    GAME_SPOT_REST_HEAD = 0x14217,
    GAME_FEATURE_COUNT = 0x14253,
    GAME_FEATURES = 0x1426f,
    GAME_POOL_COUNT = 0x14351,
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
    EXPLODED_SLOTS = 100
};

extern unsigned char* g_explodedPieces[EXPLODED_SLOTS];

static const char* const s_phaseNames[8] = { "net", "units", "projectiles", "explosions", "players", "features",
                                             "wind", "meteors" };
static const char* const s_targetNames[] = { "", "heap", "unit", "player", "unitdef", "weapondef", "projectile",
                                             "feature", "piece" };

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

static void HashBuf(const unsigned char* p, unsigned n)
{
    unsigned lo = s_hashLo;
    unsigned hi = s_hashHi;
    while (n--) {
        lo ^= *p++;
        unsigned pc = (lo & 0xffff) * 435;
        unsigned pa = (lo >> 16) * 435;
        unsigned plo = pc + (pa << 16);
        unsigned carry = plo < pc;
        hi = hi * 435 + (pa >> 16) + carry + (lo << 8);
        lo = plo;
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

static void Resolve(unsigned p, int target, unsigned* tag, unsigned* index)
{
    unsigned base = 0;
    unsigned stride = 1;
    unsigned count = 0;
    *tag = 0;
    *index = 0;
    if (p == 0)
        return;
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
        count = MAX_PLAYERS;
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
    Resolve(p, target, &tag, &index);
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
        Resolve(*(const unsigned*)p, f->target, &tag, &index);
        if (tag == 0)
            fputs("null", s_dump);
        else if (tag == TT_HEAP)
            fputs("heap", s_dump);
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
    for (unsigned i = 0; i < MAX_PLAYERS; i++) {
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

static void VisitWind()
{
    HashByte(8);
    for (unsigned k = 0; k < kWindFieldCount; k++) {
        const TraceField* f = &kWindFields[k];
        char path[64];
        HashField(GameAt(0), f);
        if (s_dumpNow) {
            sprintf(path, "wind.%s", f->name);
            DumpPrefix(path);
            DumpValue(GameAt(f->offset), f);
        }
    }
    EndSection();
}

static void VisitMeteors()
{
    HashByte(9);
    for (unsigned k = 0; k < kMeteorGlobalCount; k++) {
        const TraceGlobal* g = &kMeteorGlobals[k];
        TraceField f;
        char path[64];
        f.name = g->name;
        f.offset = 0;
        f.size = g->size;
        f.kind = g->kind;
        f.target = g->target;
        f.mask = 0;
        if (g->kind == TF_POINTER)
            HashPointer(*(const unsigned*)g->address, g->target);
        else
            HashBuf((const unsigned char*)g->address, g->size);
        if (s_dumpNow) {
            sprintf(path, "meteors.%s", g->name);
            DumpPrefix(path);
            DumpValue((const unsigned char*)g->address, &f);
        }
    }
    EndSection();
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
void __stdcall UpdateObjectState(void* unit);

void HarnessPoseObject(void* unit)
{
    unsigned char* u = (unsigned char*)unit;
    unsigned char* state = *(unsigned char**)(u + 0x9e);
    if (state == NULL)
        return;
    memcpy(state + 0x18, u + 0x64, 6);
    *(int*)(state + 8) = 1;
    *(short*)(*(unsigned char**)(state + 0x1e) + 0x26) = 0;
    UpdateObjectState(unit);
}

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

void HarnessBeforeStep()
{
    if (!s_testMode)
        return;
    unsigned tick = GameU32(GAME_TICK);
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
#ifdef BX_ROUTE
    unsigned holdrand = bx_game_holdrand();
#else
    unsigned holdrand = *(unsigned*)((char*)_getptd() + 0x14);
#endif
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
    DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
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

    s_trace = fopen("trace.txt", "wb");
    if (s_trace == NULL)
        Fail("testmode: cannot write trace.txt\n");
    fprintf(s_trace, "bt-trace 1\ncommit %s\nscenario %s\ngame_seed %u\ncrt_seed %u\nview_seed %u\n"
                     "fpu_control_word 0x%05X\n",
            commit, s_scenarioPath, s_gameSeed, s_crtSeed, s_viewSeed, controlWord);
    if (s_dumpEnabled) {
        s_dump = fopen("dump.txt", "wb");
        if (s_dump == NULL)
            Fail("testmode: cannot write dump.txt\n");
        fputs("bt-dump 1\n", s_dump);
    }
}

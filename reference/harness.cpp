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
    } else if (strncmp(key, "player", 6) == 0 && isdigit((unsigned char)key[6]) && key[7] == '.')
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
    sprintf(msg, "testmode: reached tick %u\n", s_ticks);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, (DWORD)strlen(msg), &written, NULL);
    ExitProcess(0);
}

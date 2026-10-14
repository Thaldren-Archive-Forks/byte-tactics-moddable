#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>

static int s_testMode = 0;
static unsigned s_gameSeed = 1;
static unsigned s_crtSeed = 1;
static unsigned s_fpuControlWord = 0x9001F;

static void Fail(const char* text)
{
    DWORD written;
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), text, (DWORD)strlen(text), &written, NULL);
    ExitProcess(2);
}

static void ReadScenario(const char* path)
{
    char line[512];
    FILE* f = fopen(path, "r");
    if (f == NULL) {
        char msg[700];
        sprintf(msg, "testmode: cannot read scenario file %s\n", path);
        Fail(msg);
    }
    while (fgets(line, sizeof line, f) != NULL) {
        char* key = line;
        while (*key == ' ' || *key == '\t')
            key++;
        if (*key == '#' || *key == '\n' || *key == '\r' || *key == '\0')
            continue;
        char* end = key;
        while (*end != '\0' && !isspace((unsigned char)*end))
            end++;
        if (*end == '\0')
            continue;
        *end++ = '\0';
        while (*end == ' ' || *end == '\t')
            end++;
        unsigned value = strtoul(end, NULL, 0);
        if (strcmp(key, "game_seed") == 0)
            s_gameSeed = value;
        else if (strcmp(key, "crt_seed") == 0)
            s_crtSeed = value;
        else if (strcmp(key, "fpu_control_word") == 0)
            s_fpuControlWord = value;
    }
    fclose(f);
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
    if (flag == NULL)
        return;

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

    unsigned cw = _control87(0, 0);
    if (cw != s_fpuControlWord) {
        char msg[128];
        sprintf(msg, "testmode: FPU control word (_control87) is 0x%05X, scenario expects 0x%05X\n",
                cw, s_fpuControlWord);
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

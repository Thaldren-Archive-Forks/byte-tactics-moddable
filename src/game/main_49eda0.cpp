// Decompiled by Claude Opus 5.5. Names are provisional.
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include "../quirks/quirks.h"
#include "../../interfaces/host.h"
#include "../state/sim_globals.h"

#pragma pack(push, 1)
struct Game_0049ee30 {
    char unknown_0[0x2c74];
    unsigned short lockFlags;                // +0x2c74
    char unknown_2c76[0x37f31 - 0x2c76];
    int lobbyTimeoutSecs;                    // +0x37f31
    int netSendDropPct;                      // +0x37f35
    char unknown_37f39[0x39245 - 0x37f39];
    int cmdlineSkipIntroMovie;               // +0x39245
};
#pragma pack(pop)

extern Game_0049ee30* g_game;
extern int g_cmdlineDisplaySeed;
extern char g_preferredLanguage[];
bool __cdecl IsDebugKeyword(const char* arg);
void __stdcall Onl_LoadConfigFile(char* file);
void __stdcall SetBypassDriveScan(int val);
void __stdcall SetCommandLineUnusedFlagL(int val);
void __stdcall SetHostGameName(int flag, char* text);
void __stdcall SetDirectConnectAddress(char* address);
void __stdcall SetDirectPlayProvider(int provider);
void __stdcall SetPacketRate(int rate);
char* __stdcall Translate(char* text);
void SetNoDirectSound();
void SetUseWindowsSound();

// Parses the command line. Returns 0 when the game should quit at once
// (after -r registered it with DirectPlay), 1 otherwise.
// FUNCTION: 0x49ee30
int __stdcall ParseCommandLine(char* cmdLine, char* appName)
{
    char* copy = (char*)_alloca(strlen(cmdLine) + 1);
    strcpy(copy, cmdLine);
    g_game->lobbyTimeoutSecs = 30;
    g_game->netSendDropPct = 0;
    g_game->cmdlineSkipIntroMovie = 0;

    char* p = strtok(copy, " \t");
    while (p) {
        if (*p != '-' && *p != '/') {
            if (g_quirks.boundLanguageCopy) {
                strncpy(g_preferredLanguage, p, 0x3f);
                g_preferredLanguage[0x3f] = 0;
            } else {
                strcpy(g_preferredLanguage, p);
            }
        } else if (!IsDebugKeyword(p)) {
            switch (p[1]) {
            case 'B':
            case 'b':
                if (!*p)
                    p = strtok(NULL, " \t");
                if (p && *p) {
                    if (!_strcmpi(p, "lock"))
                        g_game->lockFlags |= 1;
                    else if (!_strcmpi(p, "deathends"))
                        ;
                    else if (!_strcmpi(p, "deathplays"))
                        ;
                    else if (!_strcmpi(p, "deathmatch"))
                        ;
                    else if (!_strcmpi(p, "fixedloc"))
                        ;
                    else if (!_strcmpi(p, "mapping"))
                        ;
                    else if (!_strcmpi(p, "circlos"))
                        ;
                    else if (!_strcmpi(p, "truelos"))
                        ;
                    else if (!_strcmpi(p, "permlos"))
                        ;
                    else if (!_strcmpi(p, "cheating"))
                        ;
                    else if (!_strcmpi(p, "watching"))
                        ;
                }
                break;
            case 'C':
            case 'c':
                p += 2;
                if (!*p)
                    p = strtok(NULL, " \t");
                if (p && *p) {
                    try {
                        Onl_LoadConfigFile(p);
                    } catch (...) {
                    }
                }
                g_game->cmdlineSkipIntroMovie = 1;
                break;
            case 'D':
            case 'd':
            {
                char mode = p[2];
                g_cmdlineDisplaySeed = 2;
                if (mode != 'f' && mode != 'F')
                    g_cmdlineDisplaySeed = 3;
                break;
            }
            case 'E':
            case 'e': {
                p += 2;
                if (!*p)
                    p = strtok(NULL, " \t");
                if (p && *p) {
                    int v = -1;
                    if (*p != '-')
                        v = atoi(p);
                    if (v < 0 || v > 100)
                        v = 0;
                    g_game->netSendDropPct = v;
                }
                break;
            }
            case 'F':
            case 'f':
                SetBypassDriveScan(1);
                break;
            case 'H':
            case 'h':
                p += 2;
                if (*p) {
                    SetHostGameName(1, p);
                } else {
                    char* q = strtok(NULL, " \t");
                    if (q && *q && *q != '-')
                        SetHostGameName(1, q);
                }
                break;
            case 'L':
            case 'l':
                SetCommandLineUnusedFlagL(0);
                break;
            case 'N':
            case 'n':
                p += 2;
                if (!*p)
                    p = strtok(NULL, " \t");
                if (p && *p && *p != '-') {
                    char* colon = strchr(p, ':');
                    int n = atoi(p);
                    if (n == 1 && colon)
                        SetDirectConnectAddress(colon + 1);
                    SetDirectPlayProvider(n);
                }
                g_game->cmdlineSkipIntroMovie = 1;
                break;
            case 'P':
            case 'p':
                p += 2;
                if (!*p)
                    p = strtok(NULL, " \t");
                if (p && *p) {
                    int v = -1;
                    if (*p != '-')
                        v = atoi(p);
                    SetPacketRate(v);
                }
                break;
            case 'R':
            case 'r': {
                // Register with DirectPlay as a lobbyable application and quit.
                char* args = cmdLine + (p - copy) + 2;
                while (isspace(*args))
                    args++;
                char moduleName[_MAX_PATH];
                unsigned int len = host_file_exe_path(moduleName, _MAX_PATH);
                int registered = 0;
                if (len > 0) {
                    char drive[_MAX_DRIVE];
                    char ext[_MAX_EXT];
                    char folder[_MAX_DIR + 2];
                    char fname[_MAX_FNAME + _MAX_EXT];
                    char* lastChar;
                    fname[0] = 0;
                    ext[0] = 0;
                    _splitpath(moduleName, drive, folder + 2, fname, ext);
                    strcat(fname, ext);
                    folder[0] = drive[0];
                    folder[1] = drive[1];
                    lastChar = &folder[strlen(folder) - 1];
                    if (strchr("/\\", *lastChar))
                        *lastChar = 0;
                    registered = host_net_register_application(appName, fname, args, folder);
                }
                if (!registered) {
                    host_alert();
                    p = Translate("DirectPlay registration failed.");
                    host_message(NULL, appName, p, HOST_MESSAGE_ERROR);
                }
                return 0;
            }
            case 'S':
            case 's':
                SetNoDirectSound();
                break;
            case 'T':
            case 't':
                p += 2;
                if (!*p)
                    p = strtok(NULL, " \t");
                if (p && *p) {
                    int v = -1;
                    if (*p != '-')
                        v = atoi(p);
                    if (v < 30 || v > 300)
                        v = 30;
                    g_game->lobbyTimeoutSecs = v;
                }
                break;
            case 'W':
            case 'w':
                SetUseWindowsSound();
                break;
            }
        }
        p = strtok(NULL, " \t");
    }
    return 1;
}

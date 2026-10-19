// Decompiled by DeepSeek V4.1 Flash, finished by Space Bunny Free. Names are provisional.
// Stays out of multi.cpp: merged, the session list pointer lands in ebp (docs/split-modules.md).
// Resolves the connection index (searching the enumerated DirectPlay
// connections for the provider GUID selected in DAT_00512c80 when the
// incoming index is negative), then records that connection's GUID and
// copies its data block into a freshly allocated buffer.
#include <stdio.h>
#include <string.h>
extern "C" __declspec(dllimport) char* __stdcall lstrcpynA(char*, const char*, int);
struct Guid_443ff0 { unsigned long data1; unsigned short data2; unsigned short data3; unsigned char data4[8]; };
#pragma pack(push,1)
#include "../game/game.h"
#pragma pack(pop)
extern Game* g_game;
extern int DAT_00512c80;
extern char g_onlineLobbyPassword;
extern Guid_443ff0 g_dpspGuidTcpip;
extern Guid_443ff0 g_dpspGuidIpx;
extern Guid_443ff0 g_dpspGuidModem;
extern Guid_443ff0 g_dpspGuidSerial;
void* __cdecl GameAllocIgnoreTag(const char* name, unsigned int size);
int IsOnlineConfigLoaded();
// FUNCTION: 0x443ff0
int __stdcall SelectConnection(int index)
{
    if (index < 0) {
        Guid_443ff0* guid = 0;
        switch (DAT_00512c80) {
        case 0: break;
        case 1: guid = &g_dpspGuidTcpip; break;
        case 2: guid = &g_dpspGuidIpx; break;
        case 3: guid = &g_dpspGuidModem; break;
        case 4: guid = &g_dpspGuidSerial; break;
        }
        DAT_00512c80 = 0;
        if (guid != 0) {
            int i = 0;
            for (; i < g_game->count; i++) {
                if (memcmp(&((Guid_443ff0 *)g_game->guids)[i], guid, sizeof(Guid_443ff0)) == 0)
                    break;
            }
            if (i < g_game->count)
                index = i;
        }
    }
    if (index >= 0) {
        (*(Guid_443ff0*)&g_game->info.guid) = ((Guid_443ff0 *)g_game->sessions)[index];
        g_game->info.conn = g_game->conns[index];
        int size = g_game->conns[index].size;
        g_game->info.conn.data = GameAllocIgnoreTag("DPLAY CONNECTION INFO", size);
        if (g_game->info.conn.data != 0) {
            memcpy(g_game->info.conn.data, g_game->conns[index].data, size);
        } else {
            g_game->info.conn.size = 0;
        }
        if (IsOnlineConfigLoaded() && g_onlineLobbyPassword != 0) {
            lstrcpynA(g_game->name_2be3, &g_onlineLobbyPassword, 0xb);
        }
        return 1;
    }
    return 0;
}

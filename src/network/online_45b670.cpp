// Decompiled by Claude Opus 5.5. Names are provisional.
// The online.dll glue (0x45b250, 0x45b490, 0x45b670 share these inline
// helpers).
#include <string.h>
#include "../../interfaces/host.h"

extern int DAT_00512c80[0x54];      // the online configuration, 0x150 bytes
extern int g_onlineConfigLoaded;

// FUNCTION: 0x45b670
int __stdcall Onl_LoadConfigFile(char* file)
{
    g_onlineConfigLoaded = 0;
    memset(DAT_00512c80, 0, sizeof(DAT_00512c80));
    try {
        g_onlineConfigLoaded = host_net_online_load_config(file, DAT_00512c80, sizeof(DAT_00512c80));
    } catch (...) {
        g_onlineConfigLoaded = 0;
    }
    return g_onlineConfigLoaded;
}

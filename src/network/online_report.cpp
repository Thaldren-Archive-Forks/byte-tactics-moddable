#include <string.h>

#include "../../interfaces/host.h"

struct Net_4c97b0;

extern int g_reporterReport;
extern int g_reporterNotify;

char* __stdcall Translate(char* text);

// FUNCTION: 0x4c9750
char* GetEnumSessionsErrorText()
{
    const char* s = strstr(host_net_error_string(host_net_enum_sessions_result()), " - ");
    if (s == 0)
        return (char*)host_net_error_string(0x80004005);
    return Translate((char*)s + 3);
}

// FUNCTION: 0x4ca9d0
int __stdcall GetCreatedLobbyInterface(Net_4c97b0* net)
{
    return *(int*)((char*)net + 0x4c9);
}

// FUNCTION: 0x4ca9e0
int __stdcall GetSessionGuidInstance(int net)
{
    return net + 0x465;
}

// FUNCTION: 0x4ca9f0
int __stdcall QueryOnlineFlags(int)
{
    return 0;
}

// FUNCTION: 0x4caa00
int __stdcall RIReportGameChat(int arg1, int arg2)
{
    return 1;
}

// FUNCTION: 0x4caa10
int __stdcall RIReport(int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10)
{
    return 1;
}

// FUNCTION: 0x4caa20
void __stdcall RISetCallbacks(int report, int notify)
{
    g_reporterReport = report;
    g_reporterNotify = notify;
}

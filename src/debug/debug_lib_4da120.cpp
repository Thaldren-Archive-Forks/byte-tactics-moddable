// Decompiled by Claude Opus 5.5. Names are provisional.
// FLAGS: /Od /Gy
//
// Cavedog's debug helpers, compiled without optimisation: every value goes
// through its local, and the linker pads after each function with int3.
#include <stdio.h>
#include <stdlib.h>
#include "../../interfaces/host.h"

char* __cdecl FindCommandLineSwitch(const char* option);
void AbortProgram(void);
void SetFpuFussy(void);

extern unsigned char g_debugSupportInited;  // InitDebugSupport has run

// FUNCTION: 0x4da1d0
void __cdecl InitDebugSupport(unsigned int flags)
{
    if (g_debugSupportInited)
        return;
    g_debugSupportInited = 1;
    char* option = FindCommandLineSwitch("-debughelper");
    if (option) {
        int value = 0;
        sscanf(option, "=%d", &value);
        host_debug_helper(value);
        AbortProgram();
    }
    getenv("windir");
    if (!(flags & 2))
        host_crash_install();
    if (!(flags & 4))
        SetFpuFussy();
}

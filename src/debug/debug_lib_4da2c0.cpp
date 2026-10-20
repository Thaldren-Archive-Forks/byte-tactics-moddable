// Decompiled by Claude Opus 5.5. Names are provisional.
// FLAGS: /Od /Gy
//
// The debug thread InitDebugSupport starts.
#include <windows.h>
#include "../../interfaces/host.h"

void StartMemoryStatus(void);
void StartPerformanceStatus(void);
void __cdecl InitDebugSupport(unsigned int flags);

extern unsigned char g_debugThreadRunning;  // the debug thread is running

void __cdecl RunDebugThread(void* param);

// FUNCTION: 0x4da2c0
DWORD __stdcall DebugThreadProc(void* param)
{
    RunDebugThread(param);
    return 0;
}

// Sets up the debug windows at raised priority, says it is running, then
// pumps this thread's messages until WM_QUIT.
// FUNCTION: 0x4da2e0
void __cdecl RunDebugThread(void* param)
{
    // Frame layout follows these local names (hThread, ret, message, priority), not declaration order.
    HANDLE hThread = GetCurrentThread();
    int priority = GetThreadPriority(hThread);
    SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);
    StartMemoryStatus();
    StartPerformanceStatus();
    SetThreadPriority(hThread, priority);
    g_debugThreadRunning = 1;
    while (host_pump_events(1)) {
    }
}

// FUNCTION: 0x4da3e0
void __cdecl InitFullDebugSupport(void)
{
    InitDebugSupport(0);
}

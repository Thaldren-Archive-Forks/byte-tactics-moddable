#include <new>
#include <stdlib.h>

#include "block.h"
#include "sim_globals.h"

SimGlobals* g_simGlobals;

inline PacketManager::~PacketManager() { }

static void __cdecl DestroySimGlobals()
{
    if (g_simGlobals != 0)
        g_simGlobals->~SimGlobals();
}

SimGlobals& SimCreate()
{
    static bool registered = false;
    void* mem = SimAlloc(sizeof(SimGlobals));
    g_simGlobals = static_cast<SimGlobals*>(mem);
    SimBlockTrack(reinterpret_cast<void**>(&g_simGlobals));
    new (mem) SimGlobals();
    if (!registered) {
        registered = true;
        atexit(DestroySimGlobals);
    }
    return *g_simGlobals;
}

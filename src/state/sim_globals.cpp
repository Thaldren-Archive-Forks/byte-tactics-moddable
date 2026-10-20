#include <new>
#include <stdlib.h>

#include "block.h"
#include "sim_globals.h"

SimGlobals* g_simGlobals;

inline PacketManager::~PacketManager() { }

static void __cdecl DestroySimGlobals()
{
    g_simGlobals->~SimGlobals();
}

SimGlobals& SimCreate()
{
    void* mem = SimAlloc(sizeof(SimGlobals));
    g_simGlobals = static_cast<SimGlobals*>(mem);
    new (mem) SimGlobals();
    atexit(DestroySimGlobals);
    return *g_simGlobals;
}

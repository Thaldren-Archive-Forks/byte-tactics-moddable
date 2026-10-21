// Decompiled by Opus and Haiku. Names are provisional, except the operators.

#include <stddef.h>

#include "../state/block.h"

void* __cdecl GameAllocThunk(size_t size);
void __cdecl GameFreeIndirectThunk(void* p);

// Cavedog replaced the global operator new and delete, scalar and array,
// with wrappers around the game's own allocator.
// FUNCTION: 0x4b4f10
__declspec(noinline) void* __cdecl operator new(size_t size)
{
    AllocSite site(_ReturnAddress(), 0);
    return GameAllocThunk(size);
}

// FUNCTION: 0x4b4f20
void __cdecl operator delete(void* p)
{
    if (SimOwns(p)) {
        SimFree(p);
        return;
    }
    GameFreeIndirectThunk(p);
}

// FUNCTION: 0x4b4f30 ??_U@YAPAXI@Z
__declspec(noinline) void* __cdecl operator new[](size_t size)
{
    AllocSite site(_ReturnAddress(), 0);
    return GameAllocThunk(size);
}

// FUNCTION: 0x4b4f40 ??_V@YAXPAX@Z
void __cdecl operator delete[](void* p)
{
    if (SimOwns(p)) {
        SimFree(p);
        return;
    }
    GameFreeIndirectThunk(p);
}

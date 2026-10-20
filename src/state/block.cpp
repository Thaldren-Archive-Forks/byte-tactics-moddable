#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"
#include "../../lib/bitexact/bitexact.h"

class CritSec_004da780;
CritSec_004da780* GetAllocLock();
extern void (*g_outOfMemoryHandler)();

const SimHeap kSim = SimHeap();

namespace {

struct BlockHeader {
    unsigned int ready;
    unsigned int freeHead;
    unsigned int inUse;
    unsigned int highWater;
    bx_rand_state streams[2];
    unsigned int reserved[10];
};

static_assert(sizeof(BlockHeader) == 64, "the block header is 64 bytes");
static_assert(offsetof(BlockHeader, streams) == 0x10, "the rand streams sit at 0x10 and 0x14");

struct Chunk {
    unsigned int size;
    unsigned int aux;
};

const unsigned int kFreeBit = 1;
const unsigned int kChunkHeader = sizeof(Chunk);
const unsigned int kMinChunk = 16;
const unsigned int kFirstChunk = sizeof(BlockHeader);

char* g_base;
unsigned int g_insideThread;
void** g_tracked[8];
unsigned int g_trackedCount;

void Fatal(const char* what)
{
    fprintf(stderr, "sim block: %s (kSimBlockBytes = %u)\n", what, kSimBlockBytes);
    abort();
}

void OutOfBlock()
{
    Fatal("out of memory, raise kSimBlockBytes in src/state/block.h");
}

void Reserve()
{
    // TODO(#172): reserve the block through the host interface
    void* p = VirtualAlloc(0, kSimBlockBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (p == 0)
        OutOfBlock();
    g_base = (char*)p;
}

BlockHeader* Head()
{
    return (BlockHeader*)g_base;
}

Chunk* At(unsigned int off)
{
    return (Chunk*)(g_base + off);
}

void Prepare()
{
    if (g_base == 0)
        Reserve();
    BlockHeader* h = Head();
    if (h->ready == 0) {
        h->ready = 1;
        h->streams[0].holdrand = 1;
        h->streams[1].holdrand = 1;
        h->freeHead = kFirstChunk;
        Chunk* first = At(kFirstChunk);
        first->size = ((kSimBlockBytes - kFirstChunk) & ~7u) | kFreeBit;
        first->aux = 0;
    }
}

struct Guard {
    CRITICAL_SECTION* cs;
    Guard()
    {
        cs = (CRITICAL_SECTION*)GetAllocLock();
        EnterCriticalSection(cs);
        if (g_insideThread != 0)
            Fatal("re-entered the allocator");
        g_insideThread = GetCurrentThreadId();
        Prepare();
    }
    ~Guard()
    {
        g_insideThread = 0;
        LeaveCriticalSection(cs);
    }
};

unsigned int ChunkSizeFor(unsigned int size)
{
    if (size > kSimBlockBytes)
        return kSimBlockBytes + kMinChunk;
    unsigned int need = ((size + 7) & ~7u) + kChunkHeader;
    return need < kMinChunk ? kMinChunk : need;
}

void CountUse(unsigned int bytes)
{
    BlockHeader* h = Head();
    h->inUse += bytes;
    if (h->inUse > h->highWater)
        h->highWater = h->inUse;
}

Chunk* Lookup(void* p)
{
    unsigned int off = (unsigned int)((char*)p - g_base) - kChunkHeader;
    if (g_base == 0 || off >= kSimBlockBytes || off < kFirstChunk || (off & 7) != 0)
        Fatal("pointer is not a sim block");
    Chunk* c = At(off);
    if ((c->size & kFreeBit) != 0)
        Fatal("pointer is already free");
    return c;
}

void* AllocLocked(unsigned int size)
{
    unsigned int need = ChunkSizeFor(size);
    for (int attempt = 0;; attempt++) {
        unsigned int prev = 0;
        unsigned int off = Head()->freeHead;
        while (off != 0) {
            Chunk* c = At(off);
            unsigned int sz = c->size & ~kFreeBit;
            if (sz >= need) {
                unsigned int next = c->aux;
                unsigned int link = next;
                if (sz - need >= kMinChunk) {
                    unsigned int restOff = off + need;
                    Chunk* rest = At(restOff);
                    rest->size = (sz - need) | kFreeBit;
                    rest->aux = next;
                    link = restOff;
                } else {
                    need = sz;
                }
                if (prev != 0)
                    At(prev)->aux = link;
                else
                    Head()->freeHead = link;
                c->size = need;
                c->aux = size;
                memset((char*)c + kChunkHeader, 0, need - kChunkHeader);
                CountUse(need);
                return (char*)c + kChunkHeader;
            }
            prev = off;
            off = c->aux;
        }
        if (attempt != 0 || g_outOfMemoryHandler == 0)
            OutOfBlock();
        unsigned int owner = g_insideThread;
        g_insideThread = 0;
        g_outOfMemoryHandler();
        g_insideThread = owner;
    }
    return 0;
}

void FreeLocked(void* p)
{
    Chunk* c = Lookup(p);
    unsigned int off = (unsigned int)((char*)c - g_base);
    unsigned int sz = c->size;
    Head()->inUse -= sz;
    unsigned int prev = 0;
    unsigned int next = Head()->freeHead;
    while (next != 0 && next < off) {
        prev = next;
        next = At(next)->aux;
    }
    unsigned int link = next;
    if (next != 0 && off + sz == next) {
        sz += At(next)->size & ~kFreeBit;
        link = At(next)->aux;
    }
    c->size = sz | kFreeBit;
    c->aux = link;
    if (prev == 0) {
        Head()->freeHead = off;
        return;
    }
    Chunk* before = At(prev);
    unsigned int beforeSize = before->size & ~kFreeBit;
    if (prev + beforeSize == off) {
        before->size = (beforeSize + sz) | kFreeBit;
        before->aux = link;
    } else {
        before->aux = off;
    }
}

void* ReallocLocked(void* p, unsigned int size)
{
    if (p == 0)
        return AllocLocked(size);
    if (size == 0) {
        FreeLocked(p);
        return 0;
    }
    Chunk* c = Lookup(p);
    unsigned int off = (unsigned int)((char*)c - g_base);
    unsigned int sz = c->size;
    unsigned int old = c->aux;
    unsigned int need = ChunkSizeFor(size);
    char* payload = (char*)c + kChunkHeader;
    if (need <= sz) {
        if (sz - need >= kMinChunk) {
            Chunk* rest = At(off + need);
            rest->size = sz - need;
            rest->aux = 0;
            c->size = need;
            FreeLocked((char*)rest + kChunkHeader);
        }
        if (size > old)
            memset(payload + old, 0, size - old);
        c->aux = size;
        return p;
    }
    unsigned int nextOff = off + sz;
    if (nextOff < kSimBlockBytes && (At(nextOff)->size & kFreeBit) != 0) {
        Chunk* n = At(nextOff);
        unsigned int combined = sz + (n->size & ~kFreeBit);
        if (combined >= need) {
            unsigned int prev = 0;
            unsigned int walk = Head()->freeHead;
            while (walk != nextOff) {
                prev = walk;
                walk = At(walk)->aux;
            }
            unsigned int link = n->aux;
            unsigned int total = combined;
            if (combined - need >= kMinChunk) {
                Chunk* rest = At(off + need);
                rest->size = (combined - need) | kFreeBit;
                rest->aux = n->aux;
                link = off + need;
                total = need;
            }
            if (prev != 0)
                At(prev)->aux = link;
            else
                Head()->freeHead = link;
            c->size = total;
            CountUse(total - sz);
            memset(payload + old, 0, total - kChunkHeader - old);
            c->aux = size;
            return p;
        }
    }
    void* moved = AllocLocked(size);
    memcpy(moved, p, old);
    FreeLocked(p);
    return moved;
}

}

void* __cdecl SimAlloc(unsigned int size)
{
    Guard guard;
    return AllocLocked(size);
}

void* __cdecl SimRealloc(void* p, unsigned int size)
{
    Guard guard;
    return ReallocLocked(p, size);
}

void __cdecl SimFree(void* p)
{
    if (p == 0)
        return;
    Guard guard;
    FreeLocked(p);
}

bool __cdecl SimOwns(const void* p)
{
    return g_base != 0 && (unsigned int)((const char*)p - g_base) < kSimBlockBytes;
}

unsigned int __cdecl SimHighWater()
{
    Guard guard;
    return Head()->highWater;
}

void __cdecl SimBlockInit()
{
    Guard guard;
}

unsigned int* __cdecl SimRandStream(int stream)
{
    if (g_base == 0) {
        Guard guard;
    }
    return &Head()->streams[stream].holdrand;
}

void* __cdecl SimBlockBase()
{
    return g_base;
}

void __cdecl SimBlockAttach(void* base)
{
    for (unsigned int i = 0; i < g_trackedCount; i++) {
        unsigned int off = (unsigned int)((char*)*g_tracked[i] - g_base);
        if (g_base != 0 && *g_tracked[i] != 0 && off < kSimBlockBytes)
            *g_tracked[i] = (char*)base + off;
    }
    g_base = (char*)base;
}

void __cdecl SimBlockReset()
{
    for (unsigned int i = 0; i < g_trackedCount; i++)
        *g_tracked[i] = 0;
    if (g_base != 0)
        VirtualFree(g_base, 0, MEM_RELEASE);
    g_base = 0;
}

void __cdecl SimBlockTrack(void** slot)
{
    for (unsigned int i = 0; i < g_trackedCount; i++)
        if (g_tracked[i] == slot)
            return;
    if (g_trackedCount == sizeof(g_tracked) / sizeof(g_tracked[0]))
        Fatal("too many tracked pointers");
    g_tracked[g_trackedCount++] = slot;
}

void* __cdecl operator new(size_t size, const SimHeap&)
{
    return SimAlloc(size);
}

void* __cdecl operator new[](size_t size, const SimHeap&)
{
    return SimAlloc(size);
}

void __cdecl operator delete(void* p, const SimHeap&)
{
    SimFree(p);
}

void __cdecl operator delete[](void* p, const SimHeap&)
{
    SimFree(p);
}

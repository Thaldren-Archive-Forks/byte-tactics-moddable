#include <stdio.h>
#include <string.h>

#include "../interfaces/host.h"
#include "../src/state/block.h"
#include "../src/harness/block_spans.h"

void* GetAllocLock()
{
    static void* lock;
    if (lock == 0)
        lock = host_mutex_create();
    return lock;
}

void (*g_outOfMemoryHandler)() = 0;

static int g_failures;

static void Check(bool ok, const char* what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        g_failures++;
    }
}

static bool AllZero(const void* p, unsigned int n)
{
    const unsigned char* b = (const unsigned char*)p;
    for (unsigned int i = 0; i < n; i++)
        if (b[i] != 0)
            return false;
    return true;
}

struct Seen {
    void* payload[16];
    unsigned int size[16];
    int count;
};

static void __cdecl Collect(void* payload, unsigned int size, void* ctx)
{
    Seen* seen = (Seen*)ctx;
    if (seen->count < 16) {
        seen->payload[seen->count] = payload;
        seen->size[seen->count] = size;
    }
    seen->count++;
}

static int HookAdds;
static int HookRemoves;
static void* HookLast;
static void* HookSite;

static void __cdecl HookEnter(void* site, const char*)
{
    if (HookSite == 0)
        HookSite = site;
}
static void __cdecl HookLeave() {}
static void __cdecl HookAdded(void* p, unsigned int)
{
    HookAdds++;
    HookLast = p;
}
static void __cdecl HookRemoved(void*)
{
    HookRemoves++;
}

static const int kSlots = 24;

static unsigned int g_offsets[512];
static int g_count;

static void Record(void* p)
{
    g_offsets[g_count++] = p ? (unsigned int)((char*)p - (char*)SimBlockBase()) : 0xffffffffu;
}

static void Scribble(void* p, unsigned int n)
{
    if (p)
        memset(p, 0xa5, n);
}

static void RunSequence(int rounds, bool* zeroOk)
{
    void* slot[kSlots];
    unsigned int size[kSlots];
    memset(slot, 0, sizeof(slot));
    memset(size, 0, sizeof(size));
    unsigned int seed = 12345;
    for (int i = 0; i < rounds; i++) {
        seed = seed * 1664525u + 1013904223u;
        int k = (seed >> 8) % kSlots;
        unsigned int n = (seed >> 16) % 700;
        switch ((seed >> 4) % 3) {
        case 0:
            if (slot[k]) {
                SimFree(slot[k]);
                slot[k] = 0;
                size[k] = 0;
                Record(0);
            }
            else {
                slot[k] = SimAlloc(n);
                size[k] = n;
                *zeroOk = *zeroOk && AllZero(slot[k], n);
                Scribble(slot[k], n);
                Record(slot[k]);
            }
            break;
        case 1: {
            void* q = SimRealloc(slot[k], n);
            if (n > size[k] && q)
                *zeroOk = *zeroOk && AllZero((char*)q + size[k], n - size[k]);
            slot[k] = q;
            size[k] = q ? n : 0;
            Scribble(slot[k], n);
            Record(q);
            break;
        }
        default:
            slot[k] = slot[k] ? slot[k] : 0;
            if (!slot[k]) {
                slot[k] = SimAlloc(n);
                size[k] = n;
                *zeroOk = *zeroOk && AllZero(slot[k], n);
                Scribble(slot[k], n);
            }
            Record(slot[k]);
            break;
        }
    }
    for (int k = 0; k < kSlots; k++)
        SimFree(slot[k]);
}

struct Run {
    unsigned int offsets[512];
    int count;
    unsigned int highWater;
    bool zeroOk;
    unsigned int blockSum;
};

static unsigned int Sum(const void* p, unsigned int n)
{
    const unsigned char* b = (const unsigned char*)p;
    unsigned int h = 2166136261u;
    for (unsigned int i = 0; i < n; i++)
        h = (h ^ b[i]) * 16777619u;
    return h;
}

static void Fresh(Run* r, int rounds)
{
    SimBlockReset();
    SimBlockInit();
    g_count = 0;
    r->zeroOk = true;
    RunSequence(rounds, &r->zeroOk);
    memcpy(r->offsets, g_offsets, sizeof(g_offsets));
    r->count = g_count;
    r->highWater = SimHighWater();
    r->blockSum = Sum(SimBlockBase(), kSimBlockBytes);
}

static unsigned int Stream(int i)
{
    return *SimRandStream(i);
}

static unsigned int Next(unsigned int h)
{
    return h * 214013u + 2531011u;
}

static void RunStreamTests()
{
    SimBlockReset();
    SimBlockInit();
    Check(Stream(0) == 1 && Stream(1) == 1, "both streams start at holdrand 1");
    Check((char*)SimRandStream(0) == (char*)SimBlockBase() + 0x10, "the main stream is at 0x10");
    Check((char*)SimRandStream(1) == (char*)SimBlockBase() + 0x14, "the load stream is at 0x14");

    *SimRandStream(0) = Next(*SimRandStream(0));
    Check(Stream(0) == 2531011u + 214013u && Stream(1) == 1, "drawing from main leaves load alone");
    *SimRandStream(1) = 0x1234567u;
    Check(Stream(1) == 0x1234567u && Stream(0) == 2531011u + 214013u, "seeding load leaves main alone");

    void* copy = host_mem_commit(host_mem_reserve(kSimBlockBytes), kSimBlockBytes);
    void* base = SimBlockBase();
    memcpy(copy, base, kSimBlockBytes);
    *SimRandStream(0) = 77;
    *SimRandStream(1) = 88;
    SimBlockAttach(copy);
    Check(Stream(0) == 2531011u + 214013u && Stream(1) == 0x1234567u, "an attached copy carries its stream values");
    SimBlockAttach(base);
    Check(Stream(0) == 77 && Stream(1) == 88, "the first block keeps its own stream values");
    host_mem_release(copy);

    SimBlockReset();
    Check(Stream(0) == 1 && Stream(1) == 1, "both streams are 1 again after SimBlockReset");
}

static void* g_tracked;

static void RunTrackTests()
{
    SimBlockTrack(&g_tracked);
    SimBlockTrack(&g_tracked);
    SimBlockReset();
    SimBlockInit();
    g_tracked = SimAlloc(64);
    memset(g_tracked, 0x5a, 64);
    unsigned int off = (unsigned int)((char*)g_tracked - (char*)SimBlockBase());

    void* copy = host_mem_commit(host_mem_reserve(kSimBlockBytes), kSimBlockBytes);
    void* base = SimBlockBase();
    memcpy(copy, base, kSimBlockBytes);
    SimBlockAttach(copy);
    Check(g_tracked == (char*)copy + off, "a tracked pointer follows an attached copy at the same offset");
    Check(SimOwns(g_tracked) && ((unsigned char*)g_tracked)[63] == 0x5a, "the tracked pointer reaches the copied bytes");
    SimBlockAttach(base);
    Check(g_tracked == (char*)base + off, "a tracked pointer follows the block back");

    int onStack = 0;
    void* outside = &onStack;
    g_tracked = outside;
    SimBlockAttach(copy);
    Check(g_tracked == outside, "a tracked pointer outside the block is left alone");
    SimBlockAttach(base);
    g_tracked = 0;
    SimBlockAttach(copy);
    Check(g_tracked == 0, "a null tracked pointer stays null");
    SimBlockAttach(base);
    host_mem_release(copy);

    g_tracked = SimAlloc(16);
    SimBlockReset();
    Check(g_tracked == 0, "SimBlockReset nulls a tracked pointer");
    g_tracked = SimAlloc(16);
    Check(AllZero(g_tracked, 16) && Stream(0) == 1, "the next use after a reset is a fresh zeroed block");
    SimBlockReset();
}

static void RunSpanTests()
{
    const Span spans[3] = { { 0x1000, 0x1100, 0 }, { 0x1100, 0x1180, 1 }, { 0x3000, 0x3040, 2 } };
    unsigned off = 99;
    const Span* s = LocateSpan(spans, 3, 0x1000, &off);
    Check(s == &spans[0] && off == 0, "a word at the start of the first span is at offset 0");
    s = LocateSpan(spans, 3, 0x1080, &off);
    Check(s == &spans[0] && off == 0x80, "a word inside a span is at its offset");
    s = LocateSpan(spans, 3, 0x1100, &off);
    Check(s == &spans[0] && off == 0x100, "a word at the end of a span belongs to that span, not the next");
    s = LocateSpan(spans, 3, 0x1101, &off);
    Check(s == &spans[1] && off == 1, "a word past the start of the next span is inside it");
    s = LocateSpan(spans, 3, 0x1180, &off);
    Check(s == &spans[1] && off == 0x80, "a word at the end of the second span belongs to it");
    s = LocateSpan(spans, 3, 0x3040, &off);
    Check(s == &spans[2] && off == 0x40, "a word at the end of the last span belongs to it");
    s = LocateSpan(spans, 3, 0x3000, &off);
    Check(s == &spans[2] && off == 0, "a word at the start of a span after a gap is at offset 0");
    Check(LocateSpan(spans, 3, 0x0fff, &off) == 0, "a word before the first span has no span");
    Check(LocateSpan(spans, 3, 0x1181, &off) == 0, "a word past a span and before the next has no span");
    Check(LocateSpan(spans, 3, 0x3041, &off) == 0, "a word past the last span has no span");
    Check(LocateSpan(spans, 0, 0x1000, &off) == 0, "no spans, no match");
}

int main()
{
    RunSpanTests();
    RunStreamTests();
    RunTrackTests();
    Run a, b;
    Fresh(&a, 400);
    Fresh(&b, 400);
    Check(a.count == b.count && memcmp(a.offsets, b.offsets, sizeof(a.offsets)) == 0, "same calls give same offsets");
    Check(a.blockSum == b.blockSum, "same calls give identical block bytes");
    Check(a.highWater == b.highWater && a.highWater > 0, "same high water");
    Check(a.zeroOk && b.zeroOk, "every returned byte is zero");

    SimBlockReset();
    SimBlockInit();
    g_count = 0;
    bool zeroOk = true;
    RunSequence(150, &zeroOk);
    void* keep = SimAlloc(100);
    memset(keep, 7, 100);
    void* copy = host_mem_commit(host_mem_reserve(kSimBlockBytes), kSimBlockBytes);
    memcpy(copy, SimBlockBase(), kSimBlockBytes);
    void* base = SimBlockBase();
    g_count = 0;
    RunSequence(200, &zeroOk);
    unsigned int tail[512];
    int tailCount = g_count;
    memcpy(tail, g_offsets, sizeof(tail));
    unsigned int original = Sum(base, kSimBlockBytes);

    SimBlockAttach(copy);
    Check(SimOwns((char*)copy + 5000) && !SimOwns(base), "ownership follows the attached block");
    g_count = 0;
    RunSequence(200, &zeroOk);
    Check(tailCount == g_count && memcmp(tail, g_offsets, sizeof(tail)) == 0, "a copy continues with the same offsets");
    Check(Sum(copy, kSimBlockBytes) == original, "a copy continues to identical bytes");
    Check(zeroOk, "bytes stay zero across the copy");

    SimBlockAttach(base);
    Check(SimOwns(keep), "SimOwns true inside the block");
    int onStack = 0;
    Check(!SimOwns(&onStack) && !SimOwns(0), "SimOwns false outside the block");
    SimFree(keep);

    void* x = SimAlloc(0);
    Check(x != 0, "SimAlloc(0) returns a block");
    Check(SimRealloc(x, 0) == 0, "SimRealloc(p, 0) frees");
    Check(((unsigned int)SimAlloc(3) & 7) == 0, "payloads are 8-aligned");

    SimBlockReset();
    SimBlockInit();
    Seen seen;
    memset(&seen, 0, sizeof(seen));
    SimBlockForEach(Collect, &seen);
    Check(seen.count == 0, "SimBlockForEach sees nothing in a fresh block");
    void* pa = SimAlloc(10);
    void* pb = SimAlloc(100);
    void* pc = SimAlloc(0);
    void* pd = SimAlloc(33);
    SimFree(pb);
    void* pe = SimAlloc(8);
    memset(&seen, 0, sizeof(seen));
    SimBlockForEach(Collect, &seen);
    Check(seen.count == 4, "SimBlockForEach visits each live allocation once");
    bool found[4] = { false, false, false, false };
    void* want[4] = { pa, pc, pd, pe };
    unsigned int wantSize[4] = { 10, 0, 33, 8 };
    for (int i = 0; i < seen.count && i < 16; i++)
        for (int k = 0; k < 4; k++)
            if (seen.payload[i] == want[k] && seen.size[i] == wantSize[k])
                found[k] = true;
    Check(found[0] && found[1] && found[2] && found[3], "SimBlockForEach reports payloads and requested sizes");
    SimFree(pe);
    SimFree(pd);
    SimFree(pc);
    SimFree(pa);
    memset(&seen, 0, sizeof(seen));
    SimBlockForEach(Collect, &seen);
    Check(seen.count == 0, "SimBlockForEach skips freed allocations");

    static const AllocHooks hooks = { HookEnter, HookLeave, HookAdded, HookRemoved };
    g_allocHooks = &hooks;
    void* h = SimAlloc(16);
    Check(HookAdds == 1 && HookLast == h && HookSite != 0, "the allocation hook sees SimAlloc");
    h = SimRealloc(h, 64);
    Check(HookAdds == 2 && HookRemoves == 1 && HookLast == h, "SimRealloc removes the old block and adds the new");
    SimFree(h);
    Check(HookRemoves == 2, "SimFree calls the removal hook");
    g_allocHooks = 0;

    printf(g_failures ? "FAILED\n" : "block_test: all passed\n");
    return g_failures != 0;
}

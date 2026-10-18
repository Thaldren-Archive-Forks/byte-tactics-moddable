#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "../src/state/block.h"

class CritSec_004da780 {
public:
    CRITICAL_SECTION cs;
    CritSec_004da780() { InitializeCriticalSection(&cs); }
};

CritSec_004da780* GetAllocLock()
{
    static CritSec_004da780 lock;
    return &lock;
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

int main()
{
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
    void* copy = VirtualAlloc(0, kSimBlockBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
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

    printf(g_failures ? "FAILED\n" : "block_test: all passed\n");
    return g_failures != 0;
}

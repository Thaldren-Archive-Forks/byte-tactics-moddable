#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block_scan.h"
#include "../../interfaces/host.h"
#include "../state/block.h"
#include "../state/sim_globals.h"

class CritSec_004da780;
CritSec_004da780* GetAllocLock();

namespace {

const unsigned kRegistryBits = 19;
const unsigned kRegistrySize = 1u << kRegistryBits;
const unsigned kRowBits = 21;
const unsigned kRowSize = 1u << kRowBits;

struct Entry {
    unsigned addr;
    unsigned size;
    unsigned site;
    const char* tag;
};

struct Span {
    unsigned start;
    unsigned end;
    unsigned index;
};

struct Row {
    unsigned holder;
    const char* holderTag;
    unsigned offset;
    unsigned target;
    const char* targetTag;
    unsigned ticks;
    unsigned lastTick;
    unsigned targetMin;
    unsigned targetMax;
    unsigned used;
    unsigned globals;
};

Entry* g_registry;
unsigned g_registryCount;
Span* g_spans;
unsigned g_spanCount;
unsigned g_spanMin;
unsigned g_spanMax;
bool g_dirty = true;
Row* g_rows;
unsigned g_rowCount;
unsigned g_scanned;
unsigned g_mainThread;
unsigned g_depth;
unsigned g_curSite;
const char* g_curTag;
unsigned long long g_scanTotal;
unsigned long long g_scanMax;
unsigned g_words;
unsigned g_hits;

void Die(const char* text)
{
    host_log(text);
    host_exit(2);
}

void* AllocZeroed(unsigned bytes)
{
    void* p = VirtualAlloc(0, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (p == 0)
        Die("blockscan: cannot allocate the scan tables\n");
    return p;
}

unsigned Slot(unsigned addr)
{
    return ((addr >> 3) * 2654435761u) >> (32 - kRegistryBits);
}

void __cdecl OnEnter(void* site, const char* tag)
{
    if (host_thread_id() != g_mainThread)
        return;
    if (g_depth++ == 0) {
        g_curSite = (unsigned)site;
        g_curTag = tag;
    } else if (g_curTag == 0) {
        g_curTag = tag;
    }
}

void __cdecl OnLeave()
{
    if (host_thread_id() != g_mainThread)
        return;
    if (g_depth > 0)
        g_depth--;
}

bool Inside(unsigned addr)
{
    return SimOwns((const void*)addr);
}

void __cdecl OnAdded(void* p, unsigned size)
{
    unsigned addr = (unsigned)p;
    unsigned i = Slot(addr);
    while (g_registry[i].addr != 0 && g_registry[i].addr != addr)
        i = (i + 1) & (kRegistrySize - 1);
    if (g_registry[i].addr == 0) {
        if (g_registryCount >= kRegistrySize - kRegistrySize / 8)
            Die("blockscan: allocation registry is full, raise kRegistryBits in src/harness/block_scan.cpp\n");
        g_registryCount++;
    }
    bool main = host_thread_id() == g_mainThread && g_depth > 0;
    g_registry[i].addr = addr;
    g_registry[i].size = size;
    g_registry[i].site = main ? g_curSite : 0;
    g_registry[i].tag = main ? g_curTag : 0;
    if (!Inside(addr))
        g_dirty = true;
}

void __cdecl OnRemoved(void* p)
{
    unsigned addr = (unsigned)p;
    unsigned i = Slot(addr);
    while (g_registry[i].addr != 0 && g_registry[i].addr != addr)
        i = (i + 1) & (kRegistrySize - 1);
    if (g_registry[i].addr == 0)
        return;
    if (!Inside(addr))
        g_dirty = true;
    unsigned hole = i;
    unsigned j = i;
    for (;;) {
        j = (j + 1) & (kRegistrySize - 1);
        if (g_registry[j].addr == 0)
            break;
        unsigned home = Slot(g_registry[j].addr);
        bool movable = hole <= j ? (home <= hole || home > j) : (home <= hole && home > j);
        if (movable) {
            g_registry[hole] = g_registry[j];
            hole = j;
        }
    }
    g_registry[hole].addr = 0;
    g_registryCount--;
}

const AllocHooks g_hooks = { OnEnter, OnLeave, OnAdded, OnRemoved };

const Entry* Find(unsigned addr)
{
    unsigned i = Slot(addr);
    while (g_registry[i].addr != 0) {
        if (g_registry[i].addr == addr)
            return &g_registry[i];
        i = (i + 1) & (kRegistrySize - 1);
    }
    return 0;
}

int __cdecl CompareSpans(const void* a, const void* b)
{
    unsigned x = ((const Span*)a)->start;
    unsigned y = ((const Span*)b)->start;
    return x < y ? -1 : x > y ? 1 : 0;
}

void BuildSpans()
{
    g_spanCount = 0;
    g_spanMin = 0xffffffffu;
    g_spanMax = 0;
    for (unsigned i = 0; i < kRegistrySize; i++) {
        const Entry& e = g_registry[i];
        if (e.addr == 0 || e.size == 0 || Inside(e.addr))
            continue;
        g_spans[g_spanCount].start = e.addr;
        g_spans[g_spanCount].end = e.addr + e.size;
        g_spans[g_spanCount].index = i;
        g_spanCount++;
        if (e.addr < g_spanMin)
            g_spanMin = e.addr;
        if (e.addr + e.size > g_spanMax)
            g_spanMax = e.addr + e.size;
    }
    qsort(g_spans, g_spanCount, sizeof(Span), CompareSpans);
    g_dirty = false;
}

const Span* FindSpan(unsigned v)
{
    unsigned lo = 0;
    unsigned hi = g_spanCount;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (g_spans[mid].start <= v)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == 0)
        return 0;
    const Span* s = &g_spans[lo - 1];
    return v < s->end ? s : 0;
}

unsigned HashRow(const Row& r)
{
    unsigned h = r.holder * 2654435761u;
    h = (h ^ (unsigned)r.holderTag) * 2246822519u;
    h = (h ^ r.offset) * 3266489917u;
    h = (h ^ r.target) * 668265263u;
    h = (h ^ (unsigned)r.targetTag) * 374761393u;
    return (h ^ (h >> 15)) >> (32 - kRowBits);
}

struct HolderCount {
    unsigned holder;
    const char* tag;
    unsigned rows;
};

void CleanTag(const char* tag, char* out);

void ReportTopHolders()
{
    const unsigned kSlots = 4096;
    const unsigned kTop = 10;
    HolderCount* counts = (HolderCount*)calloc(kSlots, sizeof(HolderCount));
    if (counts == 0)
        return;
    for (unsigned i = 0; i < kRowSize; i++) {
        const Row& r = g_rows[i];
        if (!r.used)
            continue;
        unsigned h = ((r.holder * 2654435761u) ^ (unsigned)r.holderTag) * 2246822519u;
        unsigned j = (h ^ (h >> 15)) & (kSlots - 1);
        for (unsigned probes = 0; probes < kSlots; probes++) {
            HolderCount& c = counts[j];
            if (c.rows == 0) {
                c.holder = r.holder;
                c.tag = r.holderTag;
                c.rows = 1;
                break;
            }
            if (c.holder == r.holder && c.tag == r.holderTag) {
                c.rows++;
                break;
            }
            j = (j + 1) & (kSlots - 1);
        }
    }
    host_log("blockscan: holders with the most rows:\n");
    for (unsigned k = 0; k < kTop; k++) {
        unsigned best = 0;
        for (unsigned j = 1; j < kSlots; j++)
            if (counts[j].rows > counts[best].rows)
                best = j;
        if (counts[best].rows == 0)
            break;
        char tag[96];
        char msg[200];
        CleanTag(counts[best].tag, tag);
        sprintf(msg, "blockscan:   0x%08x %s: %u rows\n", counts[best].holder, tag, counts[best].rows);
        host_log(msg);
        counts[best].rows = 0;
    }
    free(counts);
}

void Note(const Row& key, unsigned targetOffset, unsigned tick)
{
    unsigned i = HashRow(key);
    for (;;) {
        Row& r = g_rows[i];
        if (!r.used) {
            if (g_rowCount >= kRowSize - kRowSize / 4) {
                ReportTopHolders();
                Die("blockscan: row table is full, raise kRowBits in src/harness/block_scan.cpp\n");
            }
            r = key;
            r.used = 1;
            r.ticks = 1;
            r.lastTick = tick;
            r.targetMin = targetOffset;
            r.targetMax = targetOffset;
            g_rowCount++;
            return;
        }
        if (r.holder == key.holder && r.holderTag == key.holderTag && r.offset == key.offset && r.target == key.target
            && r.targetTag == key.targetTag && r.globals == key.globals) {
            if (r.lastTick != tick) {
                r.lastTick = tick;
                r.ticks++;
            }
            if (targetOffset < r.targetMin)
                r.targetMin = targetOffset;
            if (targetOffset > r.targetMax)
                r.targetMax = targetOffset;
            return;
        }
        i = (i + 1) & (kRowSize - 1);
    }
}

struct WalkCtx {
    unsigned tick;
};

void __cdecl Visit(void* payload, unsigned size, void* ctx)
{
    unsigned tick = ((WalkCtx*)ctx)->tick;
    const Entry* holder = Find((unsigned)payload);
    Row key;
    memset(&key, 0, sizeof key);
    key.globals = payload == (void*)g_simGlobals;
    key.holder = holder != 0 ? holder->site : 0;
    key.holderTag = holder != 0 ? holder->tag : 0;
    const unsigned* words = (const unsigned*)payload;
    for (unsigned off = 0; off + 4 <= size; off += 4) {
        unsigned v = words[off / 4];
        g_words++;
        if (v < g_spanMin || v >= g_spanMax || Inside(v))
            continue;
        const Span* s = FindSpan(v);
        if (s == 0)
            continue;
        g_hits++;
        const Entry& t = g_registry[s->index];
        key.offset = off;
        key.target = t.site;
        key.targetTag = t.tag;
        Note(key, v - t.addr, tick);
    }
}

void CleanTag(const char* tag, char* out)
{
    if (tag == 0 || IsBadReadPtr(tag, 1)) {
        strcpy(out, "-");
        return;
    }
    unsigned n = 0;
    while (n < 80 && !IsBadReadPtr(tag + n, 1) && tag[n] != '\0') {
        unsigned char c = (unsigned char)tag[n];
        out[n] = c < 32 || c > 126 ? ' ' : (char)c;
        n++;
    }
    out[n] = '\0';
    if (n == 0)
        strcpy(out, "-");
}

struct OutRow {
    char holder[16];
    char holderTag[96];
    unsigned offset;
    char target[16];
    char targetTag[96];
    unsigned ticks;
    unsigned targetMin;
    unsigned targetMax;
};

int __cdecl CompareOut(const void* a, const void* b)
{
    const OutRow* x = (const OutRow*)a;
    const OutRow* y = (const OutRow*)b;
    int c = strcmp(x->holder, y->holder);
    if (c == 0)
        c = strcmp(x->holderTag, y->holderTag);
    if (c == 0)
        c = x->offset < y->offset ? -1 : x->offset > y->offset ? 1 : 0;
    if (c == 0)
        c = strcmp(x->target, y->target);
    if (c == 0)
        c = strcmp(x->targetTag, y->targetTag);
    return c;
}

}

void BlockScanEnable()
{
    g_registry = (Entry*)AllocZeroed(sizeof(Entry) * kRegistrySize);
    g_spans = (Span*)AllocZeroed(sizeof(Span) * kRegistrySize);
    g_rows = (Row*)AllocZeroed(sizeof(Row) * kRowSize);
    g_mainThread = host_thread_id();
    g_allocHooks = &g_hooks;
}

void BlockScanTick(unsigned tick)
{
    if (g_allocHooks == 0)
        return;
    unsigned long long t0 = host_time_counter();
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)GetAllocLock();
    EnterCriticalSection(cs);
    if (g_dirty)
        BuildSpans();
    WalkCtx ctx;
    ctx.tick = tick;
    SimBlockForEach(Visit, &ctx);
    LeaveCriticalSection(cs);
    unsigned long long dt = host_time_counter() - t0;
    g_scanTotal += dt;
    if (dt > g_scanMax)
        g_scanMax = dt;
    g_scanned++;
}

void BlockScanWrite(const char* commit)
{
    if (g_allocHooks == 0)
        return;
    OutRow* out = (OutRow*)malloc(sizeof(OutRow) * (g_rowCount + 1));
    if (out == 0)
        Die("blockscan: out of memory\n");
    unsigned n = 0;
    for (unsigned i = 0; i < kRowSize; i++) {
        const Row& r = g_rows[i];
        if (!r.used)
            continue;
        OutRow& o = out[n++];
        if (r.globals)
            strcpy(o.holder, "SimGlobals");
        else
            sprintf(o.holder, "0x%08x", r.holder);
        CleanTag(r.holderTag, o.holderTag);
        o.offset = r.offset;
        sprintf(o.target, "0x%08x", r.target);
        CleanTag(r.targetTag, o.targetTag);
        o.ticks = r.ticks;
        o.targetMin = r.targetMin;
        o.targetMax = r.targetMax;
    }
    qsort(out, n, sizeof(OutRow), CompareOut);
    unsigned m = 0;
    for (unsigned i = 0; i < n; i++) {
        if (m > 0 && CompareOut(&out[m - 1], &out[i]) == 0) {
            if (out[i].ticks > out[m - 1].ticks)
                out[m - 1].ticks = out[i].ticks;
            if (out[i].targetMin < out[m - 1].targetMin)
                out[m - 1].targetMin = out[i].targetMin;
            if (out[i].targetMax > out[m - 1].targetMax)
                out[m - 1].targetMax = out[i].targetMax;
        } else {
            out[m++] = out[i];
        }
    }
    n = m;
    FILE* f = fopen("blockptrs.txt", "wb");
    if (f == 0)
        Die("blockscan: cannot write blockptrs.txt\n");
    fprintf(f, "bt-blockptrs 1\ncommit\t%s\nticks\t%u\n", commit, g_scanned);
    fprintf(f, "holder\tholder_tag\toffset\ttarget\ttarget_tag\tticks\ttarget_offset_min\ttarget_offset_max\n");
    for (unsigned i = 0; i < n; i++)
        fprintf(f, "%s\t%s\t0x%x\t%s\t%s\t%u\t0x%x\t0x%x\n", out[i].holder, out[i].holderTag, out[i].offset, out[i].target,
                out[i].targetTag, out[i].ticks, out[i].targetMin, out[i].targetMax);
    fclose(f);
    free(out);
    char msg[256];
    sprintf(msg, "blockscan: %u ticks, %u rows, %u table rows, scan counter total %I64u max %I64u, %u words, %u hits, %u live allocations\n",
            g_scanned, n, g_rowCount, g_scanTotal, g_scanMax, g_words, g_hits, g_registryCount);
    host_log(msg);
}

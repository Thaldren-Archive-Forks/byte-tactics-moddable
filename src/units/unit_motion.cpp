// Decompiled by DeepSeek V4.1 Flash, Opus, Haiku, GPT-6-Luna, Space Bunny Free, deepseek-v4.1, Fledge Alpha Free, GPT-6 and claude-opus-5-5. Names are provisional.
// The memory cache: an arena of chunks {owner handle, size} that hands out
// bitmaps, and the object pictures built in it.
// Needed for AllocHandle's operand orders.
#include "win_types.h"
#include <string.h>
#include "../state/block.h"

void __cdecl GameFreeThunk(int* param_1);
void* __cdecl GameAllocIgnoreTag(char* name, unsigned int size);

#include "cob_script.h"

#include "../map/cell.h"

struct Point16_437840 {
    short x;
    short y;
};

#pragma pack(push, 1)
struct UnitType_437840 {
    char unknown_0[0x1ce];
    float extractsMetal;               // +0x1ce
    float windGenerator;               // +0x1d2
};

#include "unit.h"

#include "../game/game.h"
#pragma pack(pop)

union Fixed_437840 {
    int value;
    struct {
        unsigned short fraction;
        short whole;
    } parts;
};

struct Chunk_00437a30 {
    void** owner;                      // +0x0
    int size;                          // +0x4
};

#include "../graphics/gaf_frame.h"

#include "../graphics/model_types.h"
#include "../graphics/memory_cache.h"

void __stdcall CutOutFrame(GafFrame* dst, GafFrame* src, int x, int y);
int __stdcall CompressFrame(unsigned char* dest, GafFrame* img);

#include "../../interfaces/seams.h"

extern Game* g_game;

// Wind/metal picker for a metal extractor: when the unit type extracts metal
// (+0x1ce > 0), sums the metal byte of every map cell under the unit's
// footprint and stores the resulting rate at +0x58, then tells the script.
//
// The sum is accumulated in 16.16 fixed point: the unity's footprint is a
// Point16 at +0x7e, the cell is at +0x76, the type pointer at +0x92 and the
// script at +0x9a. The accumulator keeps the count in the high half of a
// dword so the final `(float)` conversion and the 2^-16 scale cancel out.
// FUNCTION: 0x437840
void __stdcall UpdateMetalExtraction(Unit* unit)
{
    if (((UnitType_437840*)unit->def)->extractsMetal > 0.0f) {
        Fixed_437840 total;
        total.value = 0;
        Point16_437840 fp = *(Point16_437840*)&unit->footprint;
        for (int y = unit->cell.y; y < unit->cell.y + fp.y; y++) {
            for (int x = unit->cell.x; x < unit->cell.x + fp.x; x++) {
                Cell* c = g_seams.los_radar->GetMapCell(x, y);
                if (c) {
                    total.parts.whole += c->metal + 1;
                }
            }
        }
        *(float*)&unit->extraction = ((UnitType_437840*)unit->def)->extractsMetal * 1.52587890625e-05 * (float)total.value;
        if (unit->script)
            g_seams.units->StartScriptWithArgs((CobScript*)unit->script, "SetSpeed", 0, 0, 1, total.parts.whole, 0, 0, 0);
    }
}

// For a unit whose type has a positive value at +0x1d2 (presumably the wind
// generator rating), passes the current wind direction and speed to its
// script's SetDirection and SetSpeed functions when wind is enabled.
// FUNCTION: 0x437910
void __stdcall UpdateWindGenerator(Unit* unit)
{
    if (((UnitType_437840*)unit->def)->windGenerator > 0.0f && g_game->windEnabled) {
        ((CobScript*)unit->script)->StartScriptWithArgs("SetDirection", 0, 0, 1, g_game->windDirection, 0, 0, 0);
        ((CobScript*)unit->script)->StartScriptWithArgs("SetSpeed", 0, 0, 1, g_game->windSpeed << 4, 0, 0, 0);
    }
}

// FUNCTION: 0x4379a0
CMemoryCache* CMemoryCache::ClearPointers()
{
    int zero = 0;
    base = zero;
    handle = (void*)zero;
    return this;
}

// Memory cache initialisation: frees any previous block (as 0x437a00 does),
// allocates a "CMemoryCache CCH" block of the given size and makes it one
// free chunk covering the whole block.
// FUNCTION: 0x4379b0
int CMemoryCache::InitCache(unsigned int newSize)
{
    if (base != 0) {
        GameFreeThunk((int*)base);
        base = 0;
    }
    Chunk_00437a30* chunk = (Chunk_00437a30*)GameAllocIgnoreTag("CMemoryCache CCH", newSize);
    base = (int)chunk;
    cap = newSize;
    cur = chunk;
    chunk->owner = 0;
    cur->size = cap;
    return 1;
}

int CMemoryCache::InitSimCache(unsigned int newSize)
{
    if (base != 0) {
        GameFreeThunk((int*)base);
        base = 0;
    }
    Chunk_00437a30* chunk = (Chunk_00437a30*)SimAllocTagged("CMemoryCache CCH", newSize);
    base = (int)chunk;
    cap = newSize;
    cur = chunk;
    chunk->owner = 0;
    cur->size = cap;
    return 1;
}

// FUNCTION: 0x437a00
void CMemoryCache::FreeCache()
{
    if (base != 0) {
        GameFreeThunk((int*)base);
        base = 0;
    }
}

// FUNCTION: 0x437a20
void CMemoryCache::FreeBuffer()
{
    GameFreeThunk((int*)base);
}

// Memory cache allocation: grows or reuses the handle in *p, splitting the
// block found at `cur` (or wrapping to the base) into an allocated chunk plus
// a leftover free chunk. Chunks are {owner handle, size} and the handle points
// just past the 8-byte header.
// FUNCTION: 0x437a30
int CMemoryCache::AllocHandle(void** p, int need)
{
    int total = 0;
    if (need <= 0) {
        *p = 0;
        return 0;
    }
    int size = need + 8;
    int existing = (int)*p;
    if (existing != 0)
        existing = *(int*)(existing - 4);
    if (existing >= size)
        return 1;
    if (size > cap) {
        *p = 0;
        return 0;
    }
    Chunk_00437a30* start;
    int end = base + cap;
    if ((int)cur + size > end) {
        int q = (int)cur;
        start = (Chunk_00437a30*)base;
        while (q < end) {
            void** owner = *(void***)q;
            if (owner != 0 && (int)*owner == q + 8)
                *owner = 0;
            q += *(int*)(q + 4);
            end = base + cap;
        }
    } else {
        start = cur;
    }
    int q = (int)start;
    while (total < size) {
        total += *(int*)(q + 4);
        void** owner = *(void***)q;
        if (owner != 0 && (int)*owner == q + 8)
            *owner = 0;
        q += *(int*)(q + 4);
    }
    if (total - size > 8u) {
        Chunk_00437a30* nc = (Chunk_00437a30*)((char*)start + size);
        nc->owner = 0;
        nc->size = total - size;
        cur = nc;
    } else {
        size = total;
        int e = (int)start + total;
        if (e >= base + cap)
            e = base;
        cur = (Chunk_00437a30*)e;
    }
    start->owner = p;
    start->size = size;
    *p = (void*)((char*)start + 8);
    return 1;
}

// Allocates a one-plane w*h bitmap from the arena (0x437a30): a 0x18-byte
// header, then the pixels, filled with 1. One-plane version of 0x437be0.
// The original calls this from BuildShadow (0x45a790) rather than inlining it.
#pragma auto_inline(off)
// FUNCTION: 0x437b50
int CMemoryCache::AllocBitmap(GafFrame** handle, int w, int h)
{
    int n = w * h;
    if (!AllocHandle((void**)handle, n + 0x18))
        return 0;
    GafFrame* b = *handle;
    if (!b)
        return 0;
    b->scratch = 0;
    b->xOffset = 0;
    b->yOffset = 0;
    b->layers = 0;
    b->blend = 0;
    b->reserved = 0;
    b->compressed = 0;
    b->width = w;
    b->height = h;
    b->pixelsOrLayers = (unsigned char*)(b + 1);
    b->transparency = 1;
    memset(b->pixelsOrLayers, 1, n);
    return 1;
}
#pragma auto_inline(on)

// Allocates a two-plane w*h bitmap from the arena (0x437a30): a 0x18-byte
// header, then a plane filled with 1 and a plane cleared to 0.
// FUNCTION: 0x437be0
int CMemoryCache::AllocTwoPlaneBitmap(GafFrame** handle, int w, int h)
{
    int n = w * h;
    if (!AllocHandle((void**)handle, n * 2 + 0x18))
        return 0;
    GafFrame* b = *handle;
    if (!b)
        return 0;
    // Plane pointer taken before the header stores: it keeps the zero out of eax.
    unsigned char* p = (unsigned char*)(b + 1);
    b->xOffset = 0;
    b->yOffset = 0;
    b->layers = 0;
    b->blend = 0;
    b->reserved = 0;
    b->compressed = 0;
    b->width = w;
    b->height = h;
    b->pixelsOrLayers = p;
    b->scratch = p + n;
    memset(b->scratch, 0, n);
    b->transparency = 1;
    memset(b->pixelsOrLayers, 1, n);
    return 1;
}

// Hands the whole arena (its length minus the 8-byte record header) to the
// allocator 0x437a30 as one block, storing the handle at +0xc.
// FUNCTION: 0x437c80
void CMemoryCache::FlushCache()
{
    AllocHandle(&handle, cap - 8);
}

// Walks a buffer of variable-sized records {owner, size, ...} and clears the owner
// of every record whose owner is `id`.
// FUNCTION: 0x437c90
void CMemoryCache::ReleaseHandle(int id)
{
    Chunk_00437a30* r = (Chunk_00437a30*)base;
    for (int offset = 0; offset < cap; ) {
        offset += r->size;
        if ((int)r->owner == id) {
            r->owner = 0;
        }
        r = (Chunk_00437a30*)((char*)r + r->size);
    }
}

// FUNCTION: 0x437b30
int __stdcall GetHandleSize(int param_1)
{
    if (param_1 == 0) {
        return 0;
    }
    return *(int*)(param_1 - 4);
}

// FUNCTION: 0x458810
void CMemoryCache::DrawObjectState(Model_459200* list, void* result)
{
    int rebuild = 0;
    int x = g_game->scrollX << 16;
    int z = g_game->scrollY << 16;
    int visible;
    Unit_459200* owner = list->owner;
    if (owner->flags & 0x20000000) {
        int t = (unsigned char)~owner->activateFlags;
        if (t & 1)
            visible = 1;
        else
            visible = 0;
    } else {
        visible = *(int*)((char*)owner->motion + 0x20) == 0;
    }
    if (list->cacheDrawCount == 0)
        rebuild = 1;
    if (list->bitmap == 0)
        list->shadow = 0;
    GafFrame* bitmap = list->bitmap;
    if ((owner->flags & 0x20000000) != 0) {
        // The bitmap-null case is its own `rebuild = 1` statement.
        if (list->bitmap == 0)
            rebuild = 1;
        // The last conjunct goes through `bitmap`, the one before it through `list->bitmap`.
        else if (owner->buildLeft != 0.0f
                && (owner->flags & 0x2000) != 0
                && list->bitmap->scratch == 0
                && bitmap->scratch == 0)
            rebuild = 1;
    }
    if (list->bitmap == 0 && (owner->flags & 0x20000000) != 0)
        rebuild = 1;
    if ((owner->zBufferFlag & 1) != 0 && list->bitmap == 0)
        rebuild = 1;
    if (rebuild) {
        list->shadow = 0;
        ((UnitTable*)this)->BuildObjectPicture(list, 0, 1);
    }
    Vec3 coords;
    coords.x = x;
    coords.z = z;
    DrawObjectPieces((int)result, list, coords, visible);
    list->cacheDrawCount++;
}

// Copies an image (size, three header fields, pixels and optional mask) into
// the object's own image, whose buffers are already allocated.
// FUNCTION: 0x459170
void CMemoryCache::CopyPicture(GafFrame* source)
{
    image->width = source->width;
    image->height = source->height;
    image->xOffset = source->xOffset;
    image->yOffset = source->yOffset;
    image->transparency = source->transparency;
    memcpy(image->pixelsOrLayers, source->pixelsOrLayers, source->width * source->height);
    if (source->scratch)
        memcpy(image->scratch, source->scratch, source->width * source->height);
}

// Builds the picture of a piece: MeasureShadow measures the piece bounding box
// of the state, the scratch image at this->image is cleared to its key
// colour and the pieces are drawn into it (DrawShadowShape), the background image
// is blitted over it, and the run length compressed result becomes the state's
// sprite (CompressFrame, then a one plane bitmap from the arena).
// FUNCTION: 0x45a790
void CMemoryCache::BuildShadow(Model_459200* obj, GafFrame* dest)
{
    int w, h, x, y;
    MeasureShadow(&w, &h, &x, &y, obj);
    image->width = (unsigned short)w;
    image->height = (unsigned short)h;
    image->xOffset = (unsigned short)x;
    image->yOffset = (unsigned short)y;
    memset(image->pixelsOrLayers, image->transparency, h * w);
    memset(image->scratch, 0, h * w);
    DrawShadowShape(image, obj);
    CutOutFrame(dest, image, 5, 0);
    int size = CompressFrame(image->scratch, image);
    GafFrame** slot = &obj->shadow;
    AllocBitmap(slot, size, 1);
    GafFrame* bmp = *slot;
    memcpy(bmp->pixelsOrLayers, image->scratch, size);
    bmp->width = image->width;
    bmp->height = image->height;
    bmp->xOffset = image->xOffset;
    bmp->yOffset = image->yOffset;
    bmp->compressed = 1;
}

// Decompiled by Claude Opus 5.5, Sonnet, Haiku, edited by deepseek-v4.1, finished by deepseek-v4.1-flash, finished by Claude Opus 5.5. Names are provisional.
// FLAGS: /Gi
// std::vector<MapCacheEntry>::insert(iterator, size_type, const T&), for the
// 8-byte {string handle, int} element of the static vector at 0x5122c0 (see
// map_list.cpp); its only caller, 0x4373a0, does a push_back. The element's
// copy constructor (0x437820) and operator= (0x437800) are out-of-line calls.
#include <vector>

#include "../util/string_ref.h"

// Declares its own copy constructor and operator=: implicit ones get inlined into insert.
class MapCacheEntry {
public:
    StringRef handle;                  // +0x0
    int tntChecksum;                   // +0x4

    MapCacheEntry(const MapCacheEntry& other);
    MapCacheEntry& operator=(const MapCacheEntry& other);
};

typedef std::vector<MapCacheEntry> Vec_00437580;
typedef void (Vec_00437580::*InsertFn_00437580)(
    Vec_00437580::iterator, Vec_00437580::size_type, const MapCacheEntry&);

// FUNCTION: 0x437580 ?insert@?$vector@VMapCacheEntry@@V?$allocator@VMapCacheEntry@@@std@@@std@@QAEXPAVMapCacheEntry@@IABV3@@Z
InsertFn_00437580 g_insert_00437580 = &Vec_00437580::insert;

// Out of line, as insert calls them: with inlining they are expanded into it.
#pragma auto_inline(off)
// FUNCTION: 0x437800
MapCacheEntry& MapCacheEntry::operator=(const MapCacheEntry& other)
{
    ((StringRef*)this)->Assign(&other.handle);
    tntChecksum = other.tntChecksum;
    return *this;
}

// FUNCTION: 0x437820
MapCacheEntry::MapCacheEntry(const MapCacheEntry& other)
    : handle(other.handle), tntChecksum(other.tntChecksum)
{
}
#pragma auto_inline()

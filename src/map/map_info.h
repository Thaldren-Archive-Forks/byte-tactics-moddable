#ifndef MAP_INFO_H
#define MAP_INFO_H

struct Grid;

#pragma pack(push, 1)
struct MapInfo {
    char unknown_0[36];
    Grid* grid;
    char unknown_28[16];
    int width;
    int height;
    char unknown_40[56];
    unsigned short* visibilityMask;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(MapInfo) == 124, "MapInfo");
#endif

#endif

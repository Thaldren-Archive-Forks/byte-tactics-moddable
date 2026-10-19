#ifndef MAP_GRID_H
#define MAP_GRID_H

#pragma pack(push, 1)
struct MapGrid {
    int* buf;
    int** cursor;
    unsigned short* count;
    char unknown_c[44];
    int width;
    int height;
    char unknown_40[16];
    int stride;
    int rows;
    char unknown_58[52];
    int tiles;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(MapGrid) == 144, "MapGrid");
#endif

#endif

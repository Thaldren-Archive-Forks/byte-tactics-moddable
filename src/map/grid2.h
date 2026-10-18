#ifndef GRID2_H
#define GRID2_H

struct FogCell;

#pragma pack(push, 1)
struct Grid2 {
    FogCell* cells;
    unsigned int width;
    unsigned int height;
    int count;
    int widthFixed;
    int heightFixed;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Grid2) == 24, "Grid2");
#endif

#endif

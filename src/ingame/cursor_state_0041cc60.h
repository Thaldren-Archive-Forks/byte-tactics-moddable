#ifndef CURSOR_STATE_0041CC60_H
#define CURSOR_STATE_0041CC60_H

#include "arg_00419670.h"
#include "point_0041cd50.h"

#pragma pack(push, 1)
struct CursorState_0041cc60 {
    Arg_00419670 rect;
    int flag;
    Point_0041cd50 center;
    Point_0041cd50 savedScroll;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(CursorState_0041cc60) == 44, "CursorState_0041cc60");
#endif

#endif

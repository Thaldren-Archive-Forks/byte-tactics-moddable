// Decompiled by deepseek-v4.1-flash, finished by Claude Opus 5.5, verified by GPT-6. Names are provisional.
// FLAGS: /Gi
// Stays in its own file: it needs /Gi, and /Gi moves 0x44e3c0's registers.
// std::vector<Point_0044eec0>::insert(iterator, size_type, const _Ty&), stock
// MSVC 5 <vector> on the 4-byte point (two shorts) that the only caller,
// 0x44da00, builds and appends with insert(end(), 1, p).
#include <vector>

#include "../util/vec3.h"

typedef std::vector<Point16> Vec_0044ec30;
typedef void (Vec_0044ec30::*InsertFn_0044ec30)(
    Vec_0044ec30::iterator, Vec_0044ec30::size_type, const Point16&);

// A growth step on this vector type, standing in for the original TU's own;
// the insert's bytes need the TU to instantiate reserve (or the copy
// constructor).
void __stdcall Grow_0044ec30(Vec_0044ec30* v, int extra)
{
    v->reserve(extra + v->size());
}

// FUNCTION: 0x44ec30 ?insert@?$vector@UPoint16@@V?$allocator@UPoint16@@@std@@@std@@QAEXPAUPoint16@@IABU3@@Z
// Taking the member's address makes the compiler emit it out of line.
InsertFn_0044ec30 g_insert_0044ec30 = &Vec_0044ec30::insert;

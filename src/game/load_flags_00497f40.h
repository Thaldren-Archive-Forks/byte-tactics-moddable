#ifndef LOAD_FLAGS_00497F40_H
#define LOAD_FLAGS_00497F40_H

#pragma pack(push, 1)
union LoadFlags_00497f40 {
    union { unsigned short value; struct { unsigned short started : 1; unsigned short loaded : 1; unsigned short b2 : 1; unsigned short b3 : 1; unsigned short rest : 12; } bits; };
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(LoadFlags_00497f40) == 2, "LoadFlags_00497f40");
#endif

#endif

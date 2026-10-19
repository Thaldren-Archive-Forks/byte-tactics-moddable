#ifndef VIEW_FLAGS_497180_H
#define VIEW_FLAGS_497180_H

#pragma pack(push, 1)
struct ViewFlags_497180 {
    unsigned short bit0 : 1;
    unsigned short bit1 : 1;
    unsigned short bit2 : 1;
    unsigned short rest : 13;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(ViewFlags_497180) == 2, "ViewFlags_497180");
#endif

#endif

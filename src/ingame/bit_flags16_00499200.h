#ifndef BIT_FLAGS16_00499200_H
#define BIT_FLAGS16_00499200_H

#pragma pack(push, 1)
struct BitFlags16_00499200 {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short b2 : 1;
    unsigned short b3 : 1;
    unsigned short b4 : 1;
    unsigned short b5 : 1;
    unsigned short b6 : 1;
    unsigned short b7 : 1;
    unsigned short b8 : 1;
    unsigned short b9 : 1;
    unsigned short b10 : 1;
    unsigned short b11 : 1;
    unsigned short b12 : 1;
    unsigned short b13 : 1;
    unsigned short b14 : 1;
    unsigned short b15 : 1;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(BitFlags16_00499200) == 2, "BitFlags16_00499200");
#endif

#endif

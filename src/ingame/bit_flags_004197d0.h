#ifndef BIT_FLAGS_004197D0_H
#define BIT_FLAGS_004197D0_H

#pragma pack(push, 1)
struct BitFlags_004197d0 {
    unsigned char b0 : 1;
    unsigned char b1 : 1;
    unsigned char b2 : 1;
    unsigned char b3 : 1;
    unsigned char b4 : 1;
    unsigned char b5 : 1;
    unsigned char b6 : 1;
    unsigned char b7 : 1;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(BitFlags_004197d0) == 1, "BitFlags_004197d0");
#endif

#endif

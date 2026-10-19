#ifndef FLAGS_004197D0_H
#define FLAGS_004197D0_H

#include "bit_flags_004197d0.h"

#pragma pack(push, 1)
union Flags_004197d0 {
    union { unsigned char value; BitFlags_004197d0 bits; };
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Flags_004197d0) == 1, "Flags_004197d0");
#endif

#endif

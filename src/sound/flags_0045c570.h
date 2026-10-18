#ifndef FLAGS_0045C570_H
#define FLAGS_0045C570_H

#include "bits_0045c570.h"

#pragma pack(push, 1)
union Flags_0045c570 {
    union { unsigned short word; Bits_0045c570 bits; };
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Flags_0045c570) == 2, "Flags_0045c570");
#endif

#endif

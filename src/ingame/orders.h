#ifndef ORDERS_H
#define ORDERS_H

#include "orders_bits.h"

#pragma pack(push, 1)
union Orders {
    unsigned short raw;
    unsigned char lo;
    OrdersBits bits;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Orders) == 6, "Orders");
#endif

#endif

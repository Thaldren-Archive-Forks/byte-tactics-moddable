#ifndef POOL_H
#define POOL_H

struct Spot;

#pragma pack(push, 1)
struct Pool {
    Spot* entries;
    void* featureUnit;
    int usedHead;
    int restHead;
    int freeHead;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Pool) == 20, "Pool");
#endif

#endif

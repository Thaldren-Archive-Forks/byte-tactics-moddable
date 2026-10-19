#ifndef CONN_443FF0_H
#define CONN_443FF0_H

#pragma pack(push, 1)
struct Conn_443ff0 {
    void* data;
    int size;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Conn_443ff0) == 8, "Conn_443ff0");
#endif

#endif

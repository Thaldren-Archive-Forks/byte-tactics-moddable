#ifndef SUB_00460F60_H
#define SUB_00460F60_H

struct Sub_00460f60 {
    void** items;
    unsigned int count;
    char pad[0x18];
    void* q;
    char pad2[0x1020];
    ~Sub_00460f60();
};

#endif

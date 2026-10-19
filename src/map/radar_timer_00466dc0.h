#ifndef RADAR_TIMER_00466DC0_H
#define RADAR_TIMER_00466DC0_H

#pragma pack(push, 1)
union RadarTimer_00466dc0 {
    struct {
        short blinkTimer;
        struct {
            unsigned short blinkOn : 1;
            unsigned short pending : 1;
            unsigned short mapChanged : 1;
            unsigned short :13;
        } flags;
    } word;
    struct {
        unsigned char unknown_142ef;
        union {
            unsigned short all;
            struct {
                unsigned char lo;
                unsigned char hi;
            } b;
            struct {
                unsigned short :8;
                unsigned short bit0 : 1;
                unsigned short bit1 : 1;
                unsigned short bit2 : 1;
                unsigned short :5;
            } bits;
        } field_142f0;
    } byte;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(RadarTimer_00466dc0) == 4, "RadarTimer_00466dc0");
#endif

#endif

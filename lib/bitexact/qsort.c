#include "bitexact.h"

#define CUTOFF 8
#define STKSIZ 30

static void swap_elements(char *a, char *b, size_t width)
{
    if (a != b) {
        while (width--) {
            char t = *a;
            *a++ = *b;
            *b++ = t;
        }
    }
}

static void shortsort(char *lo, char *hi, size_t width, int (*cmp)(const void *, const void *))
{
    char *p, *max;

    while (hi > lo) {
        max = lo;
        for (p = lo + width; p <= hi; p += width)
            if (cmp(p, max) > 0)
                max = p;
        swap_elements(max, hi, width);
        hi -= width;
    }
}

void bx_qsort(void *base, size_t num, size_t width, int (*cmp)(const void *, const void *))
{
    char *lo, *hi, *mid, *loguy, *higuy;
    size_t size;
    char *lostk[STKSIZ], *histk[STKSIZ];
    int stkptr;

    if (num < 2 || width == 0)
        return;
    stkptr = 0;
    lo = (char *)base;
    hi = (char *)base + width * (num - 1);

recurse:
    size = (size_t)(hi - lo) / width + 1;
    if (size <= CUTOFF) {
        shortsort(lo, hi, width, cmp);
    } else {
        mid = lo + (size / 2) * width;
        swap_elements(mid, lo, width);

        loguy = lo;
        higuy = hi + width;
        for (;;) {
            do {
                loguy += width;
            } while (loguy <= hi && cmp(loguy, lo) <= 0);
            do {
                higuy -= width;
            } while (higuy > lo && cmp(higuy, lo) >= 0);
            if (higuy < loguy)
                break;
            swap_elements(loguy, higuy, width);
        }
        swap_elements(lo, higuy, width);

        if (higuy - 1 - lo >= hi - loguy) {
            if (lo + width < higuy) {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - width;
                ++stkptr;
            }
            if (loguy < hi) {
                lo = loguy;
                goto recurse;
            }
        } else {
            if (loguy < hi) {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;
            }
            if (lo + width < higuy) {
                hi = higuy - width;
                goto recurse;
            }
        }
    }

    --stkptr;
    if (stkptr >= 0) {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;
    }
}

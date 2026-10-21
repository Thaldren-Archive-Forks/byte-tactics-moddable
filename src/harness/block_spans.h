#ifndef HARNESS_BLOCK_SPANS_H
#define HARNESS_BLOCK_SPANS_H

struct Span {
    unsigned start;
    unsigned end;
    unsigned index;
};

inline const Span* LocateSpan(const Span* spans, unsigned count, unsigned v, unsigned* offset)
{
    unsigned lo = 0;
    unsigned hi = count;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (spans[mid].start < v)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo > 0 && v <= spans[lo - 1].end) {
        *offset = v - spans[lo - 1].start;
        return &spans[lo - 1];
    }
    if (lo < count && spans[lo].start == v) {
        *offset = 0;
        return &spans[lo];
    }
    return 0;
}

#endif

// Decompiled by space-bunny-free, deepseek-v4.1-flash, deepseek-v4.1, mimo-v2.6-pro, claude-sonnet-5-5 and Haiku. Names are provisional.
// std::vector<TdfField>::insert(iterator, const Elem&). It is the out-of-line
// instantiation the 0x4c54f0 map code calls.
#include <stddef.h>
#include <vector>

#include "string_ref.h"
#include "tdf.h"

namespace std {
inline void _Destroy(TdfField* p)
{
    ((StringRef*)&p->value)->ReleaseRef();
    ((StringRef*)&p->key)->ReleaseRef();
}
}

class TdfFieldVector : public std::vector<TdfField> {
public:
    int GetPairCount(void);

    iterator Insert(iterator p, const TdfField& x);
};

// FUNCTION: 0x4c59d0
TdfField* TdfFieldVector::Insert(iterator p, const TdfField& x)
{
    return insert(p, x);
}
// The original calls this from 0x4c59d0 rather than inlining it.
#pragma auto_inline(off)
// FUNCTION: 0x4c5ba0
int TdfFieldVector::GetPairCount(void)
{
    return size();
}
#pragma auto_inline(on)

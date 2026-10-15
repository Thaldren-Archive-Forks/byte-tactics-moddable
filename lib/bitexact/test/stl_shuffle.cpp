#include <algorithm>
#include <stdlib.h>

extern "C" void __cdecl orig_random_shuffle_int(int *first, int *last)
{
    std::random_shuffle(first, last);
}

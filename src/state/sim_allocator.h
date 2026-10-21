#ifndef STATE_SIM_ALLOCATOR_H
#define STATE_SIM_ALLOCATOR_H

#include <stddef.h>
#include <new>
#include <vector>

#include "block.h"

template <class T>
class SimAllocator {
public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    pointer address(reference x) const { return &x; }
    const_pointer address(const_reference x) const { return &x; }
    pointer allocate(size_type n, const void*) { return (pointer)SimAlloc((unsigned int)(n * sizeof(T))); }
    char* _Charalloc(size_type n) { return (char*)SimAlloc((unsigned int)n); }
    void deallocate(void* p, size_type) { SimFree(p); }
    void construct(pointer p, const T& v) { new ((void*)p) T(v); }
    void destroy(pointer p) { p->~T(); }
    size_t max_size() const
    {
        size_t n = (size_t)(-1) / sizeof(T);
        return 0 < n ? n : 1;
    }
};

template <class T, class U>
inline bool operator==(const SimAllocator<T>&, const SimAllocator<U>&)
{
    return true;
}

template <class T, class U>
inline bool operator!=(const SimAllocator<T>&, const SimAllocator<U>&)
{
    return false;
}

template <class T>
using SimVector = std::vector<T, SimAllocator<T> >;

#endif

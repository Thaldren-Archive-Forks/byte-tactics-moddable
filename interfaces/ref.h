#ifndef INTERFACES_REF_H
#define INTERFACES_REF_H

// Major version of the Ref interface, changed when the stored representation changes.
#define REF_VERSION_MAJOR 1
// Minor version of the Ref interface, changed when a member is added.
#define REF_VERSION_MINOR 0

#if !defined(__wasm32__)
#include "host.h"

// True when p lies inside the sim block.
bool __cdecl SimOwns(const void* p);
// The first byte of the sim block, or null before it is reserved.
void* __cdecl SimBlockBase();
// Nonzero when the harness runs in test mode.
int HarnessTestMode();

// Reports a pointer outside the state block named by the signature of the converting function.
inline void RefOutsideBlock(const char* signature)
{
    char text[256];
    const char tail[] = " pointer outside the state block";
    unsigned int n = 0;
    for (; signature[n] != 0 && n < sizeof(text) - sizeof(tail); n++)
        text[n] = signature[n];
    for (unsigned int i = 0; i < sizeof(tail); i++)
        text[n + i] = tail[i];
    host_fatal(0, "Ref<T>", text);
}

#if defined(__clang__)
#define REF_SIGNATURE __PRETTY_FUNCTION__
#else
#define REF_SIGNATURE "Ref<T>"
#endif
#endif

// A reference to a T in the state block: a 4-byte offset from the block base (0 is null) natively, a plain pointer on wasm32.
template <class T>
struct Ref {
#if !defined(__wasm32__)
    unsigned int offset;

    // Leaves the offset unset, so a Ref in zeroed or aggregate-initialised memory stays trivial.
    Ref() = default;
    // Stores p as an offset; in test mode a non-null p outside the block is fatal.
    Ref(T* p)
    {
        if (p != 0 && !SimOwns(p) && HarnessTestMode())
            RefOutsideBlock(REF_SIGNATURE);
        offset = p == 0 ? 0u : (unsigned int)((const char*)p - (const char*)SimBlockBase());
    }

    // The referenced object, or null.
    T* get() const { return offset == 0 ? (T*)0 : (T*)((char*)SimBlockBase() + offset); }
    // True unless null.
    explicit operator bool() const { return offset != 0; }
    // True when both refer to the same object.
    friend bool operator==(const Ref& a, const Ref& b) { return a.offset == b.offset; }
    // True when they refer to different objects.
    friend bool operator!=(const Ref& a, const Ref& b) { return a.offset != b.offset; }
#else
    T* ptr;

    // Leaves the pointer unset, so a Ref in zeroed or aggregate-initialised memory stays trivial.
    Ref() = default;
    // Stores p.
    Ref(T* p) : ptr(p) {}

    // The referenced object, or null.
    T* get() const { return ptr; }
    // True unless null.
    explicit operator bool() const { return ptr != 0; }
    // True when both refer to the same object.
    friend bool operator==(const Ref& a, const Ref& b) { return a.ptr == b.ptr; }
    // True when they refer to different objects.
    friend bool operator!=(const Ref& a, const Ref& b) { return a.ptr != b.ptr; }
#endif

    // The referenced object as a plain pointer, or null.
    operator T*() const { return get(); }
    // Member access on the referenced object.
    T* operator->() const { return get(); }
    // The referenced object.
    T& operator*() const { return *get(); }

    // True when a refers to b.
    friend bool operator==(const Ref& a, T* b) { return a.get() == b; }
    // True when a does not refer to b.
    friend bool operator!=(const Ref& a, T* b) { return a.get() != b; }
    // True when b refers to a.
    friend bool operator==(T* a, const Ref& b) { return a == b.get(); }
    // True when b does not refer to a.
    friend bool operator!=(T* a, const Ref& b) { return a != b.get(); }
};

#if !defined(__wasm32__)
static_assert(sizeof(Ref<int>) == 4, "a Ref is one 4-byte offset");
#endif

#endif

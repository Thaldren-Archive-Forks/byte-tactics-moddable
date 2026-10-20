#ifndef STATE_BLOCK_H
#define STATE_BLOCK_H

#include <stddef.h>

// The size of the block every sim allocation comes from, in bytes.
const unsigned int kSimBlockBytes = 32u << 20;

// Allocates size zeroed bytes from the sim block, 8-byte aligned.
void* __cdecl SimAlloc(unsigned int size);

// Allocates like SimAlloc; the tag names the site for the analysis tools and the block ignores it.
inline void* __cdecl SimAllocTagged(const char*, unsigned int size) { return SimAlloc(size); }

// Resizes a sim block like realloc, zeroing the bytes beyond the old size.
void* __cdecl SimRealloc(void* p, unsigned int size);

// Returns a sim block to the free list; null is ignored.
void __cdecl SimFree(void* p);

// True when p lies inside the sim block.
bool __cdecl SimOwns(const void* p);

// The peak bytes of the block in use since start-up, headers included.
unsigned int __cdecl SimHighWater();

// Reserves the sim block if it is not reserved yet.
void __cdecl SimBlockInit();

// The first byte of the sim block, or null before it is reserved.
void* __cdecl SimBlockBase();

// The holdrand of a rand stream of the block (0 is the main thread's, 1 the load thread's), reserving the block first if need be.
unsigned int* __cdecl SimRandStream(int stream);

// Makes a copy of the block (kSimBlockBytes bytes) the live sim block.
void __cdecl SimBlockAttach(void* base);

// Releases the block so that the next use reserves a fresh zeroed one; a tracked pointer becomes null.
void __cdecl SimBlockReset();

// Tracks a pointer into the block: SimBlockReset nulls *slot and SimBlockAttach moves it into the new block.
void __cdecl SimBlockTrack(void** slot);

// The tag that selects the sim block in a placement new.
struct SimHeap {};
extern const SimHeap kSim;

// Allocates a zeroed object from the sim block: new (kSim) T(...).
void* __cdecl operator new(size_t size, const SimHeap&);

// Allocates a zeroed array from the sim block: new (kSim) T[n].
void* __cdecl operator new[](size_t size, const SimHeap&);

// Frees a sim object whose constructor threw.
void __cdecl operator delete(void* p, const SimHeap&);

// Frees a sim array whose constructor threw.
void __cdecl operator delete[](void* p, const SimHeap&);

#endif

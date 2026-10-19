// Decompiled by space-bunny-free. Names are provisional.
// Grows the arena ObjectPool (vtable 0x4fd580, see 0x470a90.cpp and
// 0x470ae0.cpp) to newCapacity slots of newSlotSize bytes. The table of slot
// pointers is reallocated with GameReallocIgnoreTag, the raw memory for the new
// slots comes from GameAllocShared, and the base of that block is pushed on
// the vector of blocks that the destructor frees one by one (0x470e50).
#include <stddef.h>
#include <vector>

void* __cdecl GameAllocShared(int size);
void* __cdecl GameReallocIgnoreTag(void* table, int size);
void* __cdecl operator new(unsigned int size);
void __cdecl operator delete(void* p);

struct Elem_00470f00 {
    void* p;                            // +0x0, the base of one memory block
};

class ObjectPool {
public:
    int vtable;                                // +0x0, the vtable pointer
    std::vector<Elem_00470f00> items;
    Elem_00470f00** slots;                     // +0x14, the slot table
    int slotSize;                              // +0x18, the slot size
    int capacity;                              // +0x1c, the slot count
    int used;                                  // +0x20, slots handed out

    int Grow(int newCapacity, int newSlotSize);
};

// FUNCTION: 0x470c10
int ObjectPool::Grow(int newCapacity, int newSlotSize)
{
    int result = 0;
    if (newCapacity > capacity) {
        Elem_00470f00** table = (Elem_00470f00**)GameReallocIgnoreTag(slots, newCapacity * 4);
        if (table != 0) {
            Elem_00470f00 block;
            block.p = GameAllocShared((newCapacity - capacity) * newSlotSize);
            slots = table;
            if (block.p != 0) {
                int i = capacity;
                if (i < newCapacity) {
                    int offset = 0;
                    do {
                        slots[i] = (Elem_00470f00*)((char*)block.p + offset);
                        offset += newSlotSize;
                        i++;
                    } while (i < newCapacity);
                }
                items.push_back(block);
                capacity = newCapacity;
                slotSize = newSlotSize;
                result = 1;
            }
        }
    }
    else {
        result = 1;
    }
    return result;
}

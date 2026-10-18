// Decompiled by deepseek-v4.1-flash. Names are provisional.
// Loads a .TDF document from an in-memory buffer: throws away the tree parsed
// from the previous load and parses the buffer again. The buffer is copied
// into a fresh "TDF file" block that is NUL terminated, comments are blanked
// out in that copy (TdfFile::StripComments), and the text is parsed
// into a new root section named "root" (TdfRecord::TdfRecord). The class holding the
// tree is the same one 0x4c2f60 loads files into.
#include <vector>
#include <string.h>

// The reference-counted string handle: a pointer to the characters with the
// reference count in the int just before them.
class StringRef {
public:
    char* p;                           // count in the dword before p

    StringRef();
    StringRef(const StringRef& other);
    StringRef(const char* text);
    ~StringRef() { ReleaseRef(); }
    void ReleaseRef();
    StringRef* Assign(const StringRef* other);
    StringRef* AssignText(const char* text);
    StringRef* MakeLower();
    StringRef* MakeUpper();
};

struct TdfField {
    StringRef a;                // +0x0 key
    StringRef b;                // +0x4 value
};

void __cdecl GameFreeThunk(int* param_1);
void* __cdecl operator new(unsigned int size);
void __cdecl operator delete(void* p);
void* __cdecl GameAllocIgnoreTag(char* name, int size);

#include <new>
#include "tdf.h"

// Own file: needs its own view of TdfRecord (see the note in TdfFile, tdf.cpp).
// FUNCTION: 0x4c3120
void TdfFile::LoadBuffer(char* data, int size, int flag, char* path)
{
    delete root;
    root = 0;
    current = 0;
    flags = flag;
    char* text = (char*)GameAllocIgnoreTag("TDF file", size + 1);
    memcpy(text, data, size);
    text[size] = 0;
    this->StripComments(text);
    TdfRecord* node = (TdfRecord*)operator new(0x29);
    root = node ? new (node) TdfRecord("root", text, 0, path) : 0;
    GameFreeThunk((int*)text);
}

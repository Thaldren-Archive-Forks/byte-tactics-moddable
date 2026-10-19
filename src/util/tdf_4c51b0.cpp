// Decompiled by Opus. Names are provisional.
// The compiler-generated scalar deleting destructor of the TDF entry (a key
// and a value, both reference-counted string handles; the vector<entry>
// _Ucopy is 0x4c5bc0). It is called with flag 0 in the destroy loop of the
// entry vector inside TdfFile's destructor (0x4c2eb0), which `delete`s the
// root section.
#include <vector>

#include "string_ref.h"

void __cdecl GameFreeThunk(int* param_1);

#include <new>
#include "tdf.h"

// Own file: needs its own view of TdfRecord (see the note in TdfFile, tdf.cpp).
// FUNCTION: 0x4c2eb0
// FUNCTION: 0x4c51b0 ??_GTdfField@@QAEPAXI@Z
TdfFile::~TdfFile()
{
    delete root;
    root = 0;
    current = 0;
    flags = 0;
}

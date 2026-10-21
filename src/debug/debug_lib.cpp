// Decompiled by Haiku, Opus, Sonnet, DeepSeek V4.1 Flash, deepseek-v4.1, Claude Opus 5.5, claude-opus-5-5, Claude Sonnet 5.5, claude-sonnet-5-5, Sonnet 5.5, deepseek-v4.1-flash, space-bunny-free, Space Bunny Free, muse-spark-1.3-free, mimo-v2.6-pro, GPT-6 and GPT-6.1-sol. Names are provisional.
//
// The debug library (0x4d80b0 to 0x4e42a0) in address order: the
// memfussy, gonzo and setmemory command-line switches, the game's
// allocator and page protection, the debug fill pattern, the
// allocator statistics, the free-block and file-record red-black
// trees, the ErrorLog.txt crash reports with their stack traces and
// symbol lookup (imagehlp, psapi), the unhandled exception filter and
// debug thread, the memory status dialog, the performance status
// dialog and its GDPERF counters, the Cavedog registry key and the
// name table.
//
// The files the gathers left in their own places stay there: 0x4d8310,
// 0x4d8870, 0x4d8d70, 0x4d9ab0, 0x4da120, 0x4da2c0, 0x4e16b0, 0x4e1e50
// (with 0x4e20a0) and 0x4e35b0 are gap regions; 0x4dacf0, 0x4db610,
// 0x4db7d0, 0x4dfd10, 0x4dfd50, 0x4e1990, 0x4e21f0, 0x4e2580 and
// 0x4e2620 each match only in their own file (see the notes where the
// rest of their part's functions sit).
//
// The join left four more functions in files of their own, because in
// this file's symbol context their register allocation lands differently
// (docs/c2-regalloc.md): 0x4da3f0 (debug_lib_4da3f0.cpp), 0x4db1c0
// (free_block_map.cpp), 0x4e0b90 (memory_status_dialog.cpp) and
// 0x4e3750 (debug_lib_4e3750.cpp).
#define NOMINMAX
// The debug library's allocator and page protection: the memfussy, gonzo and
// setmemory command-line switches, the debug fill pattern, and the game's
// malloc wrappers around them.
// Renames the allocator's _Charalloc so the call carries the symbol name Allocate.
#define _Charalloc Allocate
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include "../state/block.h"
#include "../../interfaces/host.h"
#include "command_line.h"

extern char g_memFussyQueried;
extern char g_memFussyDefault;


// The original calls this out of line from GetBlockSize, the allocator and
// the page protection.
#pragma auto_inline(off)
// FUNCTION: 0x4d80d0
char IsMemFussy()
{
    g_memFussyQueried = 1;
    static CommandLineSwitch memFussy("memfussy", 1, g_memFussyDefault, "-memfussy", "-memnofussy", "-memfrontalign", 0);
    return memFussy.on;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8130
void MemFussyAtexitHandler(void)
{
}

// The "gonzo" command-line switch, held in a function-local static
// (see 0x4d80d0 for the "memfussy" twin).
// The original calls this out of line from ProtectPages.
#pragma auto_inline(off)
// FUNCTION: 0x4d8140
char IsGonzo()
{
    static CommandLineSwitch gonzo("gonzo", 1, 1, 0, "-gonzo", 0, 0);
    return gonzo.on;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8190
void GonzoAtexitHandler(void)
{
}

char IsMemSet();
int GetMemSetValue();


// The "setmemory" command-line switch, held in a function-local static.
#pragma auto_inline(off)
// FUNCTION: 0x4d8200
char IsMemSet()
{
    static CommandLineSwitch memSet("setmemory", 1, 0, "-memset", "-memnoset", 0, 0);
    return memSet.on;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8250
void MemSetAtexitHandler(void)
{
}

// The "-memset" fill value, a hex or decimal command-line setting that
// defaults to 0xdeadbeef (see 0x4d8200 for the "setmemory" switch).
class CommandLineNumber {
public:
    int value;                         // +0x0
    CommandLineNumber(char* name, int a, int def, char* sw);
    ~CommandLineNumber() {}
};

#pragma auto_inline(off)
// FUNCTION: 0x4d8260
int GetMemSetValue()
{
    static CommandLineNumber setValue("setvalue", 0xdeadbeef, 0xdeadbeef, "-memset");
    return setValue.value;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d82b0
void SetValueAtexitHandler(void)
{
}

// Fills n bytes at p with a repeating 4-byte pattern (a debug-heap fill; its
// checking counterpart follows at 0x4d8310).
#pragma auto_inline(off)
// FUNCTION: 0x4d82c0
void __cdecl FillPattern(void* p, unsigned int pattern, unsigned int n)
{
    unsigned int* d = (unsigned int*)p;
    while (n >= 4) {
        *d++ = pattern;
        n -= 4;
    }
    if (n > 0)
        memcpy(d, &pattern, n);
}
#pragma auto_inline(on)

// CheckFillPattern (0x4d8310) is gap code and stays in
// src/debug/debug_lib_4d8310.cpp: tools/gapcheck.py sizes a region's functions
// by the next annotation in the file, so a file that also holds functions
// outside the region cannot be its source.

unsigned int __cdecl LookupBlockSize(unsigned int key);

// The original calls this out of line from GameRealloc, GameFree and
// ProtectBlock.
#pragma auto_inline(off)
// FUNCTION: 0x4d8360
size_t __cdecl GetBlockSize(void* p)
{
    if (p == 0)
        return 0;

    if (IsMemFussy())
        return LookupBlockSize((unsigned int)p);

    return _msize(p);
}
#pragma auto_inline(on)

// The original calls this out of line from AbortProgram.
#pragma auto_inline(off)
// FUNCTION: 0x4d8390
void NopAbortHook(void)
{
}
#pragma auto_inline(on)

// FUNCTION: 0x4d83a0
void __cdecl AllocNotifyNop(int)
{
}

void* __cdecl GameAlloc(unsigned int size);

// FUNCTION: 0x4d83b0
void* __cdecl GameAllocIgnoreTag(const char* name, unsigned int size)
{
    AllocSite site(_ReturnAddress(), name);
    return GameAlloc(size);
}

void* GetAllocLock();
void* __cdecl AllocDebugBlock(unsigned int size, int flags);
void __cdecl CountAlloc(unsigned int size);
extern void (*g_outOfMemoryHandler)();

// The original calls this out of line from GameAllocIgnoreTag, GameAllocShared,
// GameCalloc, GameStrdup and GameAllocThunk.
#pragma auto_inline(off)
// FUNCTION: 0x4d83c0
void* __cdecl GameAlloc(unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    void* cs = GetAllocLock();
    host_mutex_lock(cs);
    void* p;
    do {
        if (IsMemFussy()) {
            p = AllocDebugBlock(size, 0);
        } else {
            p = malloc(size);
            if (p != 0)
                CountAlloc(size);
        }
        if (p == 0 && g_outOfMemoryHandler != 0)
            g_outOfMemoryHandler();
    } while (p == 0 && g_outOfMemoryHandler != 0);
    if (p != 0) {
        if (IsMemSet())
            FillPattern(p, GetMemSetValue(), size);
    }
    if (p != 0 && g_allocHooks != 0)
        g_allocHooks->added(p, size);
    host_mutex_unlock(cs);
    return p;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8450
void* __cdecl GameAllocShared(unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    return GameAlloc(size);
}

// calloc-style allocation: count * size bytes from GameAlloc, zeroed.
// FUNCTION: 0x4d8460
void* __cdecl GameCalloc(unsigned int count, unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    unsigned int total = size * count;
    void* p = GameAlloc(total);
    if (p != 0) {
        memset(p, 0, total);
    }
    return p;
}

void* __cdecl GameRealloc(void* block, unsigned int size);

// The original calls this out of line from GameReallocIgnoreTag.
#pragma auto_inline(off)
// FUNCTION: 0x4d84a0
void* __cdecl GameReallocTagged(void* block, int unused_param_2, unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    return GameRealloc(block, size);
}
#pragma auto_inline(on)

void* __cdecl ReallocDebugBlock(void* p, unsigned int size, int flags);
void __cdecl CountFree(int size);
int HarnessTestMode();

// The original calls this out of line from GameReallocTagged.
#pragma auto_inline(off)
// FUNCTION: 0x4d84c0
void* __cdecl GameRealloc(void* block, unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    if (SimOwns(block))
        return SimRealloc(block, size);
    void* cs = GetAllocLock();
    host_mutex_lock(cs);
    size_t grownFrom = 0;
    if (HarnessTestMode())
        grownFrom = GetBlockSize(block);
    void* p;
    do {
        if (IsMemFussy()) {
            p = ReallocDebugBlock(block, size, 0);
        } else {
            size_t old = GetBlockSize(block);
            if (block == 0 && size == 0)
                p = 0;
            else
                p = realloc(block, size);
            if (p != 0 || size == 0) {
                if (block != 0)
                    CountFree(old);
                if (size != 0)
                    CountAlloc(size);
            }
        }
        if (p == 0 && size != 0 && g_outOfMemoryHandler != 0)
            g_outOfMemoryHandler();
    } while (p == 0 && size != 0 && g_outOfMemoryHandler != 0);
    if (HarnessTestMode() && p != 0 && IsMemSet() && size > grownFrom)
        FillPattern((char*)p + grownFrom, GetMemSetValue(), size - grownFrom);
    if (g_allocHooks != 0 && (p != 0 || size == 0)) {
        if (block != 0)
            g_allocHooks->removed(block);
        if (p != 0)
            g_allocHooks->added(p, size);
    }
    host_mutex_unlock(cs);
    return p;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8580
void* __cdecl GameReallocIgnoreTag(int* block, int size)
{
    AllocSite site(_ReturnAddress(), 0);
    return GameReallocTagged(block, 0, size);
}

void __cdecl GameFree(void* p);

// The original calls this out of line from GameFreeIndirectThunk.
#pragma auto_inline(off)
// FUNCTION: 0x4d85a0
void __cdecl GameFreeThunk(int* p)
{
    GameFree(p);
}
#pragma auto_inline(on)

void __cdecl FreeDebugBlock(void* p, int flags);

// The game's free(): under the allocator lock, either hands the block to the
// "memfussy" debug heap or updates the allocation counters and frees it.
// The original calls this out of line from GameFreeThunk.
#pragma auto_inline(off)
// FUNCTION: 0x4d85b0
void __cdecl GameFree(void* p)
{
    if (SimOwns(p)) {
        SimFree(p);
        return;
    }
    if (p) {
        void* cs = GetAllocLock();
        host_mutex_lock(cs);
        if (g_allocHooks != 0)
            g_allocHooks->removed(p);
        if (IsMemFussy()) {
            FreeDebugBlock(p, 0);
        } else {
            CountFree(GetBlockSize(p));
            free(p);
        }
        host_mutex_unlock(cs);
    }
}
#pragma auto_inline(on)

// strdup() through the game's allocator.
// FUNCTION: 0x4d8610
char* __cdecl GameStrdup(char* s)
{
    if (!s)
        return 0;
    AllocSite site(_ReturnAddress(), 0);
    char* p = (char*)GameAlloc(strlen(s) + 1);
    if (p)
        strcpy(p, s);
    return p;
}

// FUNCTION: 0x4d8660
void* __cdecl GameAllocThunk(unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    return GameAlloc(size);
}

// FUNCTION: 0x4d8670
void __cdecl GameFreeIndirectThunk(int* p)
{
    GameFreeThunk(p);
}

// The original calls this through a cast function pointer from ReportException.
#pragma auto_inline(off)
// FUNCTION: 0x4d8680
char __cdecl AvWriteDetectStub(unsigned long)
{
    return 0;
}
#pragma auto_inline(on)

void __cdecl ProtectPages(int addr, int size, int protect);


// Changes the protection of the whole pages inside [addr, addr + size),
// only when IsGonzo (the "gonzo" switch) is on.
// The original calls this out of line from the page protection wrappers.
#pragma auto_inline(off)
// FUNCTION: 0x4d86b0
void __cdecl ProtectPages(int addr, int size, int protect)
{
    if (IsGonzo()) {
        unsigned int start = (addr + 0xfff) & 0xfffff000;
        unsigned int end = (addr + size) & 0xfffff000;
        if (start < end)
            host_mem_protect((void*)start, end - start, protect);
    }
}
#pragma auto_inline(on)


void __cdecl ProtectBlock(void* p, int protect);

// FUNCTION: 0x4d8710
void __cdecl ProtectBlockReadOnly(void* p)
{
    ProtectBlock(p, HOST_MEM_READONLY);
}

void __cdecl RoundRangeToPages(unsigned int* start, unsigned int* end);

// Changes the protection of a heap block, widened to whole pages when the
// "memfussy" switch (IsMemFussy) is on.
// The original calls this out of line from ProtectBlockReadOnly and
// ProtectBlockReadWrite.
#pragma auto_inline(off)
// FUNCTION: 0x4d8720
void __cdecl ProtectBlock(void* p, int protect)
{
    unsigned int start = (unsigned int)p;
    size_t size = GetBlockSize(p);
    unsigned int end = start + size;
    if (IsMemFussy())
        RoundRangeToPages(&start, &end);
    ProtectPages(start, end - start, protect);
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8780
void __cdecl ProtectBlockReadWrite(void* p)
{
    ProtectBlock(p, HOST_MEM_READWRITE);
}

class BlockInfo {
public:
    unsigned int address;              // +0x00
    long size;                         // +0x04
    int allocNumber;                   // +0x08
    char name[0x20];                   // +0x0c
    unsigned int tag;                  // +0x2c

    BlockInfo(void);
    BlockInfo(int p1, int p2, int p3, int p4, const char* p5);
    void SetName(char* newName);
};



// The original calls this out of line from the block-map lookups and the tree inserts.
#pragma auto_inline(off)
// FUNCTION: 0x4d8820 ??0BlockInfo@@QAE@HHHHPBD@Z
BlockInfo::BlockInfo(int p1, int p2, int p3, int p4, const char* p5)
{
    address = p1;
    size = p2;
    allocNumber = p3;
    SetName((char*)p5);
    tag = p4;
}
#pragma auto_inline(on)

// The original calls this out of line from the BlockInfo constructors.
#pragma auto_inline(off)
// FUNCTION: 0x4d8850
void BlockInfo::SetName(char* newName)
{
    if (newName != 0) {
        int i = 0;
        for (; i < 0x1f && newName[i] != 0; i++)
            name[i] = newName[i];
        name[i] = 0;
    } else {
        name[0] = 0;
    }
}
#pragma auto_inline(on)

// The debug library's crash reporting and command-line settings: the
// ErrorLog.txt reports and stack traces, the fatal error handler, the
// unhandled exception filter, the debug thread, DebugHelper.dll, the dialog
// templates, the allocator statistics and the file-record map singleton.
// The gap functions (0x4d8d70, 0x4d9ab0, 0x4da120, 0x4da2c0) stay in their
// own files.
#define NOMINMAX
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <math.h>
#include <yvals.h>
#include <memory>
// Nothing here uses <io.h>: its symbols put NormalizeLineEndings in the
// symbol-id window it matches in (docs/c2-regalloc.md).
#include <io.h>





// The fields are called min and max.













// The out-of-memory handler, seen as a function pointer here and at 0x4da8d0.
extern void (*g_outOfMemoryHandler)();

// FUNCTION: 0x4d8e50
int __cdecl SetOutOfMemoryHandler(int handler)
{
    int temp = (int)g_outOfMemoryHandler;
    g_outOfMemoryHandler = (void (*)())handler;
    return temp;
}

// FatalError (0x4d9ab0) stays in src/debug/debug_lib_4d9ab0.cpp: it is the
// source of a gap region (its __try/__except and `int 3` have no FPO record).

// A DllMain-shaped entry point: on DLL_PROCESS_ATTACH it saves the module
// handle (read back by 0x4d9f50). `sub eax, 0; je; dec eax; jne` is a switch
// with cases 0 and 1.

extern int g_debugLibInstance;


extern int g_debugLibInstance;

#pragma auto_inline(off)
// FUNCTION: 0x4d9f50
int GetDebugLibInstance(void)
{
    return g_debugLibInstance;
}
#pragma auto_inline(on)

// Finds a switch on the command line (case-insensitively) that ends at the
// end of the line, a space or '=', and returns the text just after it, or 0.

// The original calls this out of line from the CommandLineSwitch and
// CommandLineNumber constructors.
#pragma auto_inline(off)
// FUNCTION: 0x4d9f60
char* __cdecl FindCommandLineSwitch(char* name)
{
    if (!name) return 0;
    char* cmd = (char*)host_command_line();
    if (!cmd) return 0;
    size_t len = strlen(name);
    if (len == 0) return 0;
    while (*cmd) {
        if (_strnicmp(cmd, name, len) == 0) {
            char c = cmd[len];
            if (c == 0 || isspace(c) || c == '=')
                return cmd + len;
        }
        cmd++;
    }
    return 0;
}
#pragma auto_inline(on)

// A command-line switch: `on` is set from the default, then forced on by
// either on-switch and off by either off-switch (off wins).

char* __cdecl FindCommandLineSwitch(char* name);

// The original calls this out of line from IsMemFussy, IsGonzo and 0x4df160.
#pragma auto_inline(off)
// FUNCTION: 0x4d9fe0
CommandLineSwitch::CommandLineSwitch(char* name, int a, char def, char* onSwitch,
                               char* offSwitch, char* onSwitch2, char* offSwitch2)
{
    on = def;
    if (FindCommandLineSwitch(onSwitch) || FindCommandLineSwitch(onSwitch2))
        on = 1;
    if (FindCommandLineSwitch(offSwitch) || FindCommandLineSwitch(offSwitch2))
        on = 0;
}
#pragma auto_inline(on)

// An integer command-line setting (e.g. "setvalue" / "-memset" in 0x4d8260):
// starts at the default, then parses the text after the switch as hex
// ("0x...") or decimal.

char* __cdecl FindCommandLineSwitch(char* name);

// The original calls this out of line from the setmemory switch and the fpufussy twin.
#pragma auto_inline(off)
// FUNCTION: 0x4da040
CommandLineNumber::CommandLineNumber(char* name, int a, int def, char* sw)
{
    value = def;
    char* p = FindCommandLineSwitch(sw);
    if (p) {
        while (*p == ' ' || *p == '\t' || *p == '=')
            p++;
        if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
            sscanf(p, "%x", &value);
            return;
        }
        sscanf(p, "%d", &value);
    }
}
#pragma auto_inline(on)


// Same shape as the C runtime's abort(): report, raise SIGABRT, exit code 3.

void NopAbortHook(void);

// FUNCTION: 0x4da0c0
void AbortProgram(void)
{
    NopAbortHook();
    raise(SIGABRT);
    _exit(3);
}

// Returns whether arg starts (case-insensitively) with one of the 19 option
// names in the table at 0x50c908 ("memfussy" ... "saveresources").

extern char* g_debugKeywords[19];

// FUNCTION: 0x4da0e0
bool __cdecl IsDebugKeyword(const char* arg)
{
    for (char** p = g_debugKeywords; p < g_debugKeywords + 19; p++) {
        if (_strnicmp(*p, arg, strlen(*p)) == 0)
            return true;
    }
    return false;
}

// The debug helpers ShowDebugMessage, CallDebugHelperDll and InitDebugSupport
// (0x4da120) stay in src/debug/debug_lib_4da120.cpp: they are the source of a
// gap region and are compiled with /Od (see the file's `// FLAGS:` line).



int GetDebugLibInstance(void);


int GetDebugLibInstance(void);


extern char* __cdecl strrchr(const char*, int);


// Opens a URL in the browser. It tries ShellExecuteA on the URL first; when
// that fails (result <= 31) it looks the file association for the extension up
// under HKEY_CLASSES_ROOT, reads the key's default value into buf1, builds
// "<default>\shell\open\command" in the shared `sub` buffer, reads that
// key's default value into buf2, trims the program path out of the command
// line and finally runs that program with the URL as its parameter. If any of
// that fails, the message box at the end reports the last ShellExecute result.
// Callers pass a http:// URL and the extension to look up (".htm").


// The original calls this out of line from the allocator, the block map and the trees.
#pragma auto_inline(off)
// FUNCTION: 0x4da780
void* GetAllocLock()
{
    static void* lock;
    if (lock == 0)
        lock = host_mutex_create();
    return lock;
}
#pragma auto_inline(on)

// FUNCTION: 0x4da7c0
void AllocLockAtexitHandler(void)
{
}

extern unsigned int g_allocSerial;
extern unsigned int g_liveAllocCount;
extern unsigned int g_liveAllocPeak;
extern unsigned int g_bytesRequestedLo;
extern unsigned int g_bytesRequestedHi;
extern unsigned int g_currentBytes;
extern unsigned int g_currentBytesPeak;
extern unsigned int g_committedBytesPeak;
extern unsigned int g_committedBytes;
extern unsigned int g_freeBlockWraps;

// The original calls this out of line from the allocator and the page protection.
#pragma auto_inline(off)
// FUNCTION: 0x4da7d0
void __cdecl CountAlloc(unsigned int size)
{
    g_allocSerial++;
    g_liveAllocCount++;
    if (g_liveAllocCount > g_liveAllocPeak)
        g_liveAllocPeak = g_liveAllocCount;
    g_bytesRequestedLo += size;
    if (g_bytesRequestedLo >= 1000000000) {
        g_bytesRequestedLo -= 1000000000;
        g_bytesRequestedHi++;
    }
    g_currentBytes += size;
    if (g_currentBytes > g_currentBytesPeak)
        g_currentBytesPeak = g_currentBytes;
}
#pragma auto_inline(on)

// The original calls this out of line from the allocator and the page protection.
#pragma auto_inline(off)
// FUNCTION: 0x4da840
void __cdecl CountFree(int size) {
    g_liveAllocCount--;
    g_currentBytes -= size;
}
#pragma auto_inline(on)


// FUNCTION: 0x4da8a0
int __cdecl RoundUpToDoublePage(int size)
{
    return (size + 0xfff & ~0xfff) * 2;
}

// FUNCTION: 0x4da8c0
unsigned int __cdecl RoundUpToPage(int size)
{
    return (size + 0xfff) & 0xfffff000;
}

// Lazily built singleton for the game's file-record map (the std::_Tree whose
// insert is 0x4dc680 and whose erase is 0x4dc910): allocate the 0x10-byte tree
// object, make the _Nil node g_blockMapNil (black, self-null children) and the
// head node (red, parent _Nil, both children itself), both carved from the
// pooled free list at g_blockMapFreeList.

extern void* g_blockMapFreeList;       // free list of 0x40-byte nodes
extern void (*g_outOfMemoryHandler)();  // out-of-memory handler
extern int g_blockMapLiveCount;        // bumped on every node carved here
extern void* g_blockMap;               // the tree singleton

struct Node_004da8d0 {
    Node_004da8d0* left;               // +0x0
    Node_004da8d0* parent;             // +0x4
    Node_004da8d0* right;              // +0x8
    char value[0x30];                  // +0xc
    int color;                         // +0x3c (0 = red)
};

extern void* g_blockMapNil;            // the tree's _Nil node

// The pool allocator that sits at +0 of the tree; its method ignores `this`.
class BlockMapAllocator {
public:
    void* Allocate(unsigned int n);
};

class Tree_004da8d0 {
public:
    char allocator;                    // +0x0
    char key_compare;                  // +0x1
    Node_004da8d0* head;               // +0x4
    unsigned char multi;               // +0x8
    int size;                          // +0xc
    void* operator new(unsigned int n) { return host_mem_alloc(n); }
    Tree_004da8d0(const char& a, const char& b);
    void Init();
};

static inline Node_004da8d0* AllocNode()
{
    if (g_blockMapFreeList == 0) {
        Node_004da8d0* block;
        do {
            block = (Node_004da8d0*)host_mem_alloc(0x2000);
            if (block == 0 && g_outOfMemoryHandler != 0) {
                g_outOfMemoryHandler();
            }
        } while (block == 0 && g_outOfMemoryHandler != 0);
        if (block == 0) {
            return 0;
        }
        Node_004da8d0* head = (Node_004da8d0*)g_blockMapFreeList;
        for (int i = 0; i < 0x80; i++) {
            block->left = head;
            head = block;
            block++;
        }
        g_blockMapFreeList = head;
    }
    Node_004da8d0* node = (Node_004da8d0*)g_blockMapFreeList;
    g_blockMapFreeList = node->left;
    return node;
}

void Tree_004da8d0::Init()
{
    std::_Lockit lock;
    if (g_blockMapNil == 0) {
        // The allocator is the tree's +0 member, which this view types as char.
        Node_004da8d0* nil =
            (Node_004da8d0*)((BlockMapAllocator*)this)->Allocate(0x40);
        nil->parent = 0;
        nil->color = 1;
        g_blockMapNil = nil;
        nil->left = 0;
        // Reads the global back: nil->right would reuse the register.
        ((Node_004da8d0*)g_blockMapNil)->right = 0;
    }
    Node_004da8d0* nil = (Node_004da8d0*)g_blockMapNil;
    g_blockMapLiveCount++;
    Node_004da8d0* node = AllocNode();
    node->color = 0;
    node->parent = nil;
    head = node;
    size = 0;
    node->left = node;
    head->right = head;
}

inline Tree_004da8d0::Tree_004da8d0(const char& a, const char& b)
{
    allocator = a;
    key_compare = b;
    multi = 0;
    Init();
}

// The original calls this out of line from the block-map lookups and the trees.
#pragma auto_inline(off)
// FUNCTION: 0x4da8d0
void* GetBlockMap()
{
    if (g_blockMap == 0) {
        char s0, s1;
        g_blockMap = new Tree_004da8d0(s0, s1);
    }
    return g_blockMap;
}
#pragma auto_inline(on)

// A lazily created singleton: a vector-like container (an empty allocator,
// copied from an uninitialised default-argument temporary, then four zeroed
// dwords) allocated with a class operator new that calls GlobalAlloc.

template <class T, class A = std::allocator<T> >
class Container_004da9f0 {
public:
    explicit Container_004da9f0(const A& al = A())
        : allocator(al), field_4(0), field_8(0), field_c(0), field_10(0) {}
    static void* operator new(size_t size) { return host_mem_alloc(size); }

    A allocator;       // +0x0
    int field_4;       // +0x4
    int field_8;       // +0x8
    int field_c;       // +0xc
    int field_10;      // +0x10
};

extern Container_004da9f0<int>* g_freedBlockRing;

#pragma auto_inline(off)
// FUNCTION: 0x4da9f0
Container_004da9f0<int>* GetFreedBlockRing()
{
    if (g_freedBlockRing == 0) {
        g_freedBlockRing = new Container_004da9f0<int>;
    }
    return g_freedBlockRing;
}
#pragma auto_inline(on)
// The debug library's allocators: the free-block set (std::map<unsigned int,
// unsigned int> as hand-walked red-black trees, GetFreeBlockSet 0x4db610)
// and the game's file-record map (0x4dc680 insert, 0x4dc910 erase, 0x4dce00
// find_, GetBlockMap 0x4da8d0), plus the single copies of the tree helpers
// the linker folded together, the memfussy command-line switches, the debug
// fill pattern and the block descriptions. AllocDebugBlock (0x4dacf0),
// GetFreeBlockSet (0x4db610) and FreeDebugBlock (0x4db7d0) stay in their own
// files: each inlines a tree helper (or constructs the std::map member) from
// that file's own view of the types, which cannot agree with the gathered
// file's one view of each class.
#include <string.h>
#include <set>
#include <yvals.h>
#include <map>
#include <list>
#include <time.h>
// The real <vector> header must supply the body; a hand-rolled copy does not match.
#include <vector>
#include <algorithm>
// ---- the types the gathered files share, unified ---------------------------

// The debug allocator keeps its free blocks and the game its 0x30-byte file
// records in hand-walked red-black trees. Each matched file named the nodes
// and iterators in its own view; a name a symbol pins (data/progress.csv) is
// kept, and the rest of a tree's views share the fullest one.

class FreeBlockMap;

extern void* DAT_00528a54;             // the free-block tree's _Nil node
extern void* g_blockMapNil;            // the file-record tree's _Nil node
extern void* g_freeBlockFreeList;      // the free-block tree's node free list
extern void* g_blockMapFreeList;       // the file-record tree's node free list
extern void (*g_outOfMemoryHandler)();  // out-of-memory handler
extern unsigned int g_lastAllocOffset;  // offset the last block was handed out at
extern unsigned int g_freeBlockWraps;  // how often the search wrapped around
extern unsigned int g_committedBytes;  // bytes committed
extern unsigned int g_committedBytesPeak;      // high-water mark of g_committedBytes
extern unsigned int g_allocSerial;
extern FreeBlockMap* g_freeBlockSet;   // the free-block set singleton

// The file-record tree's node, the fullest of its views.
struct Node_004daa30 {
    Node_004daa30* left;           // +0x0
    Node_004daa30* parent;         // +0x4
    Node_004daa30* right;          // +0x8
    BlockInfo value;              // +0xc
    unsigned int color;            // +0x3c
};

// size; each carves nodes from its own free list.
class FreeBlockAllocator {
public:
    void* Allocate(unsigned int n);
};

struct Pair_004db000 {
    unsigned int offset;               // +0x0
    unsigned int length;               // +0x4
    Pair_004db000() {}
    Pair_004db000(unsigned int o, unsigned int l) : offset(o), length(l) {}
};

struct Node_004db000 {
    Node_004db000* left;               // +0x0
    Node_004db000* parent;             // +0x4
    Node_004db000* right;              // +0x8
    Pair_004db000 value;               // +0xc
    int color;                         // +0x14
};

// The out-of-line _Min, taken on its own opaque node type.
struct Node_004dd1b0 {
    Node_004dd1b0* left;               // +0x0
};

Node_004dd1b0* __cdecl FindLeftmost(Node_004dd1b0* p);

// The other out-of-line moves, on the shared node type.
static inline Node_004db000* Max_004dd2a0(Node_004db000* p)
{
    std::_Lockit lock;
    while (p->right != DAT_00528a54) {
        p = p->right;
    }
    return p;
}

static inline Node_004db000* Min(Node_004db000* p)
{
    std::_Lockit lock;
    while (p->left != DAT_00528a54)
        p = p->left;
    return p;
}

static inline Node_004db000* Min_004dbd80(Node_004db000* p)
{
    std::_Lockit lock;
    while (p->left != DAT_00528a54) {
        p = p->left;
    }
    return p;
}

static inline Node_004db000* Max_004dbd80(Node_004db000* p)
{
    std::_Lockit lock;
    while (p->right != DAT_00528a54) {
        p = p->right;
    }
    return p;
}

// The free-block tree's iterator: one pointer. PrevNode is its _Dec(); Next
// and Previous are its out-of-line operator++(int) and operator--(int).
class FreeBlockIter {
public:
    Node_004db000* ptr;

    FreeBlockIter() {}
    FreeBlockIter(Node_004db000* q) : ptr(q) {}
    bool operator==(const FreeBlockIter& o) const { return ptr == o.ptr; }
    bool operator!=(const FreeBlockIter& o) const { return !(*this == o); }
    Pair_004db000& operator*() const { return ptr->value; }
    Pair_004db000* operator->() const { return &ptr->value; }
    FreeBlockIter& operator++() { NextNode(); return *this; }
    FreeBlockIter operator++(int) { FreeBlockIter tmp = *this; ++*this; return tmp; }
    FreeBlockIter& operator--() { PrevNode(); return *this; }
    FreeBlockIter operator--(int) { FreeBlockIter tmp = *this; --*this; return tmp; }
    Node_004db000* Mynode() const { return ptr; }
    Node_004db000* _Mynode() const { return ptr; }
    void PrevNode();
    void NextNode();                   // _Inc, out of line at 0x4dd340
    FreeBlockIter Previous(int); // operator--(int)
    FreeBlockIter Next(int); // operator++(int)

    void Inc()
    {
        std::_Lockit lock;
        if (ptr->right != DAT_00528a54) {
            ptr = Min_004dbd80(ptr->right);
        } else {
            Node_004db000* p;
            while (ptr == (p = ptr->parent)->right) {
                ptr = p;
            }
            if (ptr->right != p) {
                ptr = p;
            }
        }
    }

    void Dec()
    {
        std::_Lockit lock;
        if (ptr->color == 0 && ptr->parent->parent == ptr) {
            ptr = ptr->right;
        } else if (ptr->left != DAT_00528a54) {
            ptr = Max_004dbd80(ptr->left);
        } else {
            Node_004db000* p;
            while (ptr == (p = ptr->parent)->left) {
                ptr = p;
            }
            ptr = p;
        }
    }
};

// The file-record tree's iterator: one node pointer. PrevNode is its _Dec and
// NextNode its _Inc.
class BlockMapIter {
public:
    Node_004daa30* ptr;                // +0x0

    BlockMapIter() {}
    BlockMapIter(Node_004daa30* q) : ptr(q) {}
    bool operator==(const BlockMapIter& o) const { return ptr == o.ptr; }
    BlockInfo& operator*() const { return ptr->value; }
    BlockInfo* operator->() const { return &ptr->value; }
    // Calls NextNode (the _Inc at 0x4dde70) by that exact name.
    BlockMapIter operator++(int)
    {
        BlockMapIter _Tmp = *this;
        NextNode();
        return _Tmp;
    }
    Node_004daa30* _Mynode() const { return ptr; }
    void PrevNode();
    void NextNode();
};

// The (iterator, inserted) pair the trees' insert functions return: an
// iterator then a byte. 0x4ddbe0 is its out-of-line constructor.
class MapInsertResult {
public:
    FreeBlockIter first;              // +0x0
    unsigned char second;             // +0x4

    MapInsertResult() {}
    MapInsertResult(const FreeBlockIter& i, const unsigned char& b) : first(i), second(b) {}
    MapInsertResult(const BlockMapIter& i, unsigned char b) : first((Node_004db000*)i.ptr), second(b) {}
    // The byte is copied before the iterator on purpose: 0x4dbbc0 needs the
    // byte in cl and the dword in edx at every return.
    inline MapInsertResult(const MapInsertResult& o)
    {
        second = o.second;
        first = o.first;
    }
    MapInsertResult* Assign(int* iter, unsigned char* inserted);
    MapInsertResult* Assign(const FreeBlockIter& first, unsigned char& second);
    MapInsertResult* Assign(const FreeBlockIter& first, const bool& second);
};

static inline Node_004daa30* Max_004dd820(Node_004daa30* p)
{
    std::_Lockit lock;
    while (p->right != g_blockMapNil) {
        p = p->right;
    }
    return p;
}

// std::_Tree<...>::upper_bound returns this; the out-of-line copy is 0x4dbd20.
class Iter_004dbd20 {
public:
    Node_004db000* ptr;
    Iter_004dbd20() : ptr(0) {}
    Iter_004dbd20(Node_004db000* p) : ptr(p) {}
};

class Iter_004dbd00 {
public:
    Node_004db000* ptr;                // +0x0

    Iter_004dbd00() {}
    Iter_004dbd00(Node_004db000* p) : ptr(p) {}
};

class ConstIter_004dbd00 : public Iter_004dbd00 {
public:
    ConstIter_004dbd00() {}
    ConstIter_004dbd00(Node_004db000* p) : Iter_004dbd00(p) {}
    ConstIter_004dbd00(const Iter_004dbd00& x) : Iter_004dbd00(x) {}
};

// The game's file-record tree: a std::map keyed on the block address, as
// hand-walked red-black tree code. The nodes are 0x40 bytes, carved from the
// pool at g_blockMapFreeList; g_blockMapNil is its _Nil node.
struct Less_004dd3d0 {
    bool operator()(const unsigned int& a, const unsigned int& b) const { return a < b; }
};

class BlockMap {
public:
    BlockMapAllocator allocator;       // +0x0
    Less_004dd3d0 key_compare;         // +0x1
    Node_004daa30* head;               // +0x4
    unsigned char multi;             // +0x8
    int size;                          // +0xc

    BlockMapIter Begin() { return BlockMapIter(head->left); }
    BlockMapIter End() { return BlockMapIter(head); }
    BlockMapIter end() { return BlockMapIter(head); }
    BlockMapIter find(const unsigned int& key)
    {
        BlockMapIter it = lower_bound(key);
        return (it == End() || key_compare(key, it.ptr->value.address)) ? End() : it;
    }

    Node_004daa30* Lbound(const unsigned int& key) const
    {
        std::_Lockit lock;
        Node_004daa30* x = head->parent;
        Node_004daa30* y = head;
        while (x != g_blockMapNil) {
            if (key_compare(x->value.address, key))
                x = x->right;
            else
                y = x, x = x->left;
        }
        return y;
    }

    Node_004daa30*& _Root() { return head->parent; }
    Node_004daa30*& _Lmost() { return head->left; }
    Node_004daa30*& _Rmost() { return head->right; }

    static Node_004daa30* _Min(Node_004daa30* _P)
        { std::_Lockit _Lk;
          while (_P->left != g_blockMapNil)
              _P = _P->left;
          return _P; }

    static Node_004daa30* _Max(Node_004daa30* _P)
        { std::_Lockit _Lk;
          while (_P->right != g_blockMapNil)
              _P = _P->right;
          return _P; }

    void _Lrotate(Node_004daa30* _X)
        { std::_Lockit _Lk;
          Node_004daa30* _Y = _X->right;
          _X->right = _Y->left;
          if (_Y->left != g_blockMapNil)
              _Y->left->parent = _X;
          _Y->parent = _X->parent;
          if (_X == _Root())
              _Root() = _Y;
          else if (_X == _X->parent->left)
              _X->parent->left = _Y;
          else
              _X->parent->right = _Y;
          _Y->left = _X;
          _X->parent = _Y; }

    void _Rrotate(Node_004daa30* _X)
        { std::_Lockit _Lk;
          Node_004daa30* _Y = _X->left;
          _X->left = _Y->right;
          if (_Y->right != g_blockMapNil)
              _Y->right->parent = _X;
          _Y->parent = _X->parent;
          if (_X == _Root())
              _Root() = _Y;
          else if (_X == _X->parent->right)
              _X->parent->right = _Y;
          else
              _X->parent->left = _Y;
          _Y->right = _X;
          _X->parent = _Y; }

    void Lrotate(Node_004daa30* x)
    {
        std::_Lockit lock;
        Node_004daa30* y = x->right;
        x->right = y->left;
        if (y->left != g_blockMapNil)
            y->left->parent = x;
        y->parent = x->parent;
        if (x == head->parent)
            head->parent = y;
        else if (x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;
        y->left = x;
        x->parent = y;
    }

    void Rrotate(Node_004daa30* x)
    {
        std::_Lockit lock;
        Node_004daa30* y = x->left;
        x->left = y->right;
        if (y->right != g_blockMapNil)
            y->right->parent = x;
        y->parent = x->parent;
        if (x == head->parent)
            head->parent = y;
        else if (x == x->parent->right)
            x->parent->right = y;
        else
            x->parent->left = y;
        y->right = x;
        x->parent = y;
    }

    MapInsertResult Insert(BlockInfo* p);
    BlockMapIter InsertHinted(Node_004daa30* x, Node_004daa30* y, const BlockInfo& v);
    BlockMapIter* InsertHinted(BlockMapIter* out, Node_004daa30* x, Node_004daa30* y, const BlockInfo* v);
    BlockMapIter erase(BlockMapIter _P);
    BlockMapIter Find(const unsigned int& key);
    BlockMapIter lower_bound(const unsigned int& key);
    Node_004daa30* LowerBound(const unsigned int& key);
    Node_004daa30* Ubound(const unsigned int& key);
    Node_004daa30* CreateNode(int parent, int color);
    void RotateLeft(Node_004daa30* x);
    void RotateRight(Node_004daa30* x);
};

class Iter_004dc620 {
public:
    Node_004db000* ptr;
    Iter_004dc620() : ptr(0) {}
    Iter_004dc620(Node_004db000* p) : ptr(p) {}
};

struct Less_004db000 {
    bool operator()(const unsigned int& a, const unsigned int& b) const { return a < b; }
};

struct Pair_004db450 {
    unsigned int offset;               // +0x0
    int length;                        // +0x4
};

struct InsertResult_004db450 { FreeBlockIter first; int second; };
// The helper's one frame object: the pair, then the value_type.

struct Frame_004db450 { InsertResult_004db450 r; Pair_004db450 p; };

struct Kfn_004db000 {
    const unsigned int& operator()(const Pair_004db000& x) const { return x.offset; }
};

class FreeBlockMap {
public:
    enum Redbl { _Red, _Black };

    FreeBlockAllocator allocator;      // +0x0
    Less_004db000 key_compare;         // +0x1
    Node_004db000* head;               // +0x4
    bool multi;                        // +0x8
    unsigned int count;                // +0xc
    unsigned int total;                // +0x10

    static Node_004db000*& Left(Node_004db000* p) { return p->left; }
    static Node_004db000*& Parent(Node_004db000* p) { return p->parent; }
    static Node_004db000*& Right(Node_004db000* p) { return p->right; }
    static int& Color(Node_004db000* p) { return p->color; }
    static unsigned int* _Value(Node_004db000* p) { return (unsigned int*)&p->value; }
    static const unsigned int& Key(Node_004db000* p) { return Kfn_004db000()(p->value); }

    Node_004db000*& Root() { return Parent(head); }
    Node_004db000*& Lmost() { return Left(head); }
    Node_004db000*& Rmost() { return Right(head); }

    static Node_004db000* Min(Node_004db000* p)
    {
        std::_Lockit _Lk;
        while (Left(p) != DAT_00528a54)
            p = Left(p);
        return p;
    }

    static Node_004db000* Max(Node_004db000* p)
    {
        std::_Lockit _Lk;
        while (Right(p) != DAT_00528a54)
            p = Right(p);
        return p;
    }

    void Lrotate(Node_004db000* x)
    {
        std::_Lockit _Lk;
        Node_004db000* y = Right(x);
        Right(x) = Left(y);
        if (Left(y) != DAT_00528a54)
            Parent(Left(y)) = x;
        Parent(y) = Parent(x);
        if (x == Root())
            Root() = y;
        else if (x == Left(Parent(x)))
            Left(Parent(x)) = y;
        else
            Right(Parent(x)) = y;
        Left(y) = x;
        Parent(x) = y;
    }

    void Rrotate(Node_004db000* x)
    {
        std::_Lockit _Lk;
        Node_004db000* y = Left(x);
        Left(x) = Right(y);
        if (Right(y) != DAT_00528a54)
            Parent(Right(y)) = x;
        Parent(y) = Parent(x);
        if (x == Root())
            Root() = y;
        else if (x == Right(Parent(x)))
            Right(Parent(x)) = y;
        else
            Left(Parent(x)) = y;
        Right(y) = x;
        Parent(x) = y;
    }

    static void Destval(unsigned int*) { }

    static void Freenode(Node_004db000* p)
    {
        if (p != 0) {
            *(void**)p = g_freeBlockFreeList;
            g_freeBlockFreeList = p;
        }
    }

    // The constructor (0x4db610) stays in its own file: it is the one function
    // that constructs the std::map member, whose _Init allocates the two
    // sentinel nodes; the walkers below use the same bytes as raw fields.
    FreeBlockIter begin() { return FreeBlockIter(head->left); }
    FreeBlockIter end() { return FreeBlockIter(head); }
    unsigned int size() const { return count; }
    // The original tests this as a value (sete; neg; sbb; inc; test), which
    // MSVC 5 only does for a `!` applied to a bool-returning member.
    bool Neq(FreeBlockIter a, FreeBlockIter b) { return !(a == b); }
    FreeBlockIter upper_bound(const Pair_004db000& k)
    {
        return FreeBlockIter(LowerBound(k));
    }

    // Inlined std::_Tree<...>::_Ubound(const _K&): its own lock scope.
    Node_004db000* Ubound(const unsigned int& kv)
    {
        std::_Lockit lock;
        Node_004db000* x = head->parent;
        Node_004db000* y = head;
        while (x != DAT_00528a54)
            if (key_compare(kv, x->value.offset))
                y = x, x = x->left;
            else
                x = x->right;
        return y;
    }

    // The original tests the iterators as a value (sete; neg; sbb; inc; test),
    // which MSVC 5 only does for a `!` applied to a bool-returning member.
    inline void Tail(unsigned base, unsigned len, FreeBlockIter& it) {
    total += len;
    FreeBlockIter n;
    // One frame object (pair, then value_type): separate locals move the stack slots.
    Frame_004db450 f;
    f.p.offset = base;
    f.p.length = len;
    lower_bound(&n, f.p.offset);
    it = n;
    Begin((int*)&f.r);
    if (it == f.r.first)
        it.ptr = head;
    else
        it.PrevNode();
    if (Neq(n, FreeBlockIter(head))) {
        if (n.ptr->value.offset == f.p.offset + f.p.length) {
            f.p.length = f.p.length + n.ptr->value.length;
            Erase(&f.r.first, n);
        }
    }
    if (Neq(it, FreeBlockIter(head))) {
        if (it.ptr->value.offset + it.ptr->value.length == f.p.offset) {
            f.p.length = f.p.length + it.ptr->value.length;
            f.p.offset = it.ptr->value.offset;
            Erase(&f.r.first, it);
        }
    }
    InsertOrFindInline(&f.r, &f.p);
    }

    void AddFreeBlock(Pair_004db000 p);
    unsigned int TakeFreeBlock(unsigned int bytes);
    bool GrowReservation(unsigned int size);

    // 0x4db450: reserve more address space and add it to the free blocks.
    bool Grow(unsigned int size)
    {
        // len declared before base: the other order swaps the operands of base + len.
        unsigned int len = 0x10000000;
        unsigned int base;

        if (2 * size > len && size < 0x40000000u)
            len = ((size + 0x1fff) & 0xffffe000) * 2;
        if (size > len)
            len = (size + 0x1fff) & 0xffffe000;
        base = (unsigned int)host_mem_reserve(len + 0x2000);
        for (;;) {
            if (base != 0 && base + len <= 0x80000000u)
                break;
            if (base != 0)
                host_mem_release((void*)base);
            len = (len >> 1) & 0x7fffe000;
            if (len < 0x10000 || len < size)
                return false;
            base = (unsigned int)host_mem_reserve(len + 0x2000);
        }
        total += len;
        AddFreeBlock(Pair_004db000(base, len));
        return true;
    }

    FreeBlockIter Begin() { return FreeBlockIter(head->left); }
    int* Begin(int* out);
    // A method that ignores `this`: its caller (0x4dbec0, an inlined tree insert
    // after its std::_Lockit) sets ecx to the tree. Shaped like
    // std::_Tree<...>::_Buynode(parent, colour) with a pooled allocator.
    Node_004db000* CreateNode(int parent, int color);
    MapInsertResult TreeInsert(const Pair_004db000& V);
    MapInsertResult InsertOrFind(const Pair_004db000& V);
    MapInsertResult InsertOrFindInline(Pair_004db000* p);
    void InsertOrFindInline(InsertResult_004db450* it, Pair_004db450* p);
    FreeBlockIter Insert(Node_004db000* x, Node_004db000* y, const Pair_004db000* v);
    FreeBlockIter Erase(FreeBlockIter _P);
    void Erase(FreeBlockIter* out, FreeBlockIter it);
    Iter_004dbd00 Erase(Iter_004dbd00 it);
    ConstIter_004dbd00 EraseCopyIter(ConstIter_004dbd00 it);
    Iter_004dbd20 UpperBound(const unsigned int& kv);
    Iter_004dc620 lower_bound(const unsigned int& kv);
    void lower_bound(FreeBlockIter* out, const unsigned int& kv);
    // Takes the pair; the out-of-line function reads only its first dword.
    Node_004db000* LowerBound(const Pair_004db000& k);
    Node_004db000* LowerBound(const unsigned int& kv);
    void RotateLeft(Node_004db000* x);
    void RotateRight(Node_004db000* x);
};

FreeBlockMap* GetFreeBlockSet();

// The <vector> the freed-block ring is (the real header supplies insert).
struct Elem_004dd8c0 {
    unsigned int w[0xc];               // +0x0, 0x30 bytes
};

class Alloc_004dd8c0 {
public:
    typedef unsigned int size_type;
    typedef int difference_type;
    typedef Elem_004dd8c0* pointer;
    typedef const Elem_004dd8c0* const_pointer;
    typedef Elem_004dd8c0& reference;
    typedef const Elem_004dd8c0& const_reference;
    typedef Elem_004dd8c0 value_type;

    pointer allocate(size_type _N, const void* = 0)
    {
        pointer _P;
        do {
            _P = (pointer)host_mem_alloc(_N * sizeof(value_type));
            if (_P == 0 && g_outOfMemoryHandler != 0)
                g_outOfMemoryHandler();
        } while (_P == 0 && g_outOfMemoryHandler != 0);
        return _P;
    }
    void deallocate(pointer _P, size_type)
    {
        // Must test the pointer before freeing.
        if (_P != 0)
            host_mem_free(_P);
    }
    void construct(pointer _P, const value_type& _V)
    {
        std::_Construct(_P, _V);
    }
    void destroy(pointer) {}
    size_type max_size() const { return (size_type)(-1) / sizeof(value_type); }
};

typedef std::vector<Elem_004dd8c0, Alloc_004dd8c0> Vec_004dd8c0;
typedef void (Vec_004dd8c0::*InsertFn_004dd8c0)(
    Vec_004dd8c0::iterator, Vec_004dd8c0::size_type, const Elem_004dd8c0&);

// The free functions the gathered functions call.
// AllocDebugBlock (0x4dacf0), GetFreeBlockSet (0x4db610) and FreeDebugBlock
// (0x4db7d0) stay in their own files: each inlines a tree helper written
// against that file's own view of the classes (FreeBlockIter's begin/end in
// 0x4dacf0, the std::map member's _Init in 0x4db610, the FreeBlockMap
// upper_bound stub in 0x4db7d0), and the gathered file holds only one view
// of each class.
char IsMemFussy();
char IsBackAlign();
int GetDebugFillPattern();
void* GetAllocLock();
void* GetBlockMap();
Container_004da9f0<int>* GetFreedBlockRing();
unsigned int __cdecl RoundUpToPage(unsigned int size);
unsigned int __cdecl RoundUpToDoublePage(unsigned int size);
void __cdecl CountAlloc(unsigned int size);
void __cdecl CountFree(int size);
void __cdecl FillPattern(void* at, int value, unsigned int count);
void __cdecl CheckFillPattern(void* at, int value, unsigned int count);
unsigned int __cdecl LookupBlockSize(unsigned int key);
void __cdecl RoundRangeToPages(unsigned int* lo, unsigned int* hi);
void* __cdecl AllocDebugBlock(unsigned int size, int flags);
void __cdecl FreeDebugBlock(void* p, int flags);
void* __cdecl ReallocDebugBlock(void* p, unsigned int size, int flags);
size_t __cdecl GetBlockSize(void* p);

// ---- from src/debug/debug_lib_4daa30.cpp --------------------


// ---- from src/debug/debug_lib_4dab10.cpp --------------------


// ---- from src/debug/debug_lib_4dabb0.cpp --------------------


// ---- from src/debug/free_block_map.cpp --------------------

// FUNCTION: 0x4db000
void FreeBlockMap::AddFreeBlock(Pair_004db000 p)
{
    FreeBlockIter it;
    FreeBlockIter it2;
    // One function-scope object for all three calls: lets their identical endings merge.
    MapInsertResult result;
    unsigned char inserted;

    FreeBlockIter n(LowerBound(p));
    it = n;
    if (n == begin()) {
        it = end();
    } else {
        it.PrevNode();
    }
    if (Neq(n, end())) {
        if (n->offset == p.length + p.offset) {
            p.length = p.length + n->length;
            Erase(n);
        }
    }
    if (Neq(it, end())) {
        Node_004db000* m = it.ptr;
        if (m->value.length + m->value.offset == p.offset) {
            p.length = p.length + m->value.length;
            p.offset = m->value.offset;
            Erase(it);
        }
    }

    Node_004db000* y = head;
    bool less = true;
    Node_004db000* x = y->parent;
    {
        std::_Lockit lock;
        while (x != DAT_00528a54) {
            y = x;
            less = p.offset < x->value.offset;
            x = less ? x->left : x->right;
        }
    }

    if (multi) {
        FreeBlockIter t(Insert(x, y, &p));
        return;
    }
    it2.ptr = y;
    if (less) {
        if (FreeBlockIter(y) == begin()) {
            inserted = 1;
            // Insert call passed straight in, not via a local: fixes the argument push order.
            result.Assign(Insert(x, y, &p), inserted);
            goto done;
        }
        it2.PrevNode();
    }
    if (key_compare(it2->offset, p.offset)) {
        inserted = 1;
        result.Assign(Insert(x, y, &p), inserted);
    } else {
        inserted = 0;
        result.Assign(it2, inserted);
    }
done: ;
}

// The allocator's alloc(): look for a free block of `bytes` in the free-block
// set, preferring the block the last allocation came from (g_lastAllocOffset),
// take it out, hand back the leftovers on either side as new free blocks, and
// if two passes over the set find nothing, reserve more address space (Grow,
// the out-of-line copy of which is 0x4db450) and try again. g_freeBlockWraps
// counts the wraps around the set and DAT_00528a54 is the tree's _Nil node.
// 0x4db1c0 FreeBlockMap::TakeFreeBlock stays in src/debug/free_block_map.cpp: joined into
// this file its register allocation lands differently
// (docs/c2-regalloc.md).

// ---- from src/debug/debug_lib_4db450.cpp --------------------

// FUNCTION: 0x4db450
bool FreeBlockMap::GrowReservation(unsigned int size)
{
    unsigned int len = 0x10000000;
    unsigned int base;
    // Once the reservation loop is done the parameter is dead, and the original
    // reuses its stack slot for the map iterator.
    FreeBlockIter& it = *(FreeBlockIter*)&size;

    if (2 * size > len && size < 0x40000000u)
        len = ((size + 0x1fff) & 0xffffe000) * 2;
    if (size > len)
        len = (size + 0x1fff) & 0xffffe000;
    base = (unsigned int)host_mem_reserve(len + 0x2000);
    for (;;) {
        if (base != 0 && base + len <= 0x80000000u)
            break;
        if (base != 0)
            host_mem_release((void*)base);
        len = (len >> 1) & 0x7fffe000;
        if (len < 0x10000 || len < size)
            return false;
        base = (unsigned int)host_mem_reserve(len + 0x2000);
    }
    Tail(base, len, *(FreeBlockIter*)&size);
    return true;
}
// ---- from src/debug/debug_lib_4db760.cpp --------------------

// FUNCTION: 0x4db760
char IsBackAlign(void)
{
    static CommandLineSwitch g_backAlign("backalign", 1, 1, 0, "-memfrontalign", 0, 0);
    return g_backAlign.on;
}

// ---- from src/debug/debug_lib_4db7b0.cpp --------------------

// FUNCTION: 0x4db7b0
void BackAlignAtexitHandler(void)
{
}

// ---- from src/debug/debug_lib_4db7c0.cpp --------------------

// FUNCTION: 0x4db7c0
int GetDebugFillPattern(void)
{
    return 0xcccccccc;
}

// ---- from src/debug/debug_lib_4dba40.cpp --------------------

// The original calls this out of line from GameRealloc.
#pragma auto_inline(off)
// FUNCTION: 0x4dba40
void* __cdecl ReallocDebugBlock(void* p, unsigned int size, int flags)
{
    void* cs = GetAllocLock();
    host_mutex_lock(cs);
    void* q = 0;
    size_t old = 0;
    if (p)
        old = GetBlockSize(p);
    if (size > 0) {
        q = AllocDebugBlock(size, flags);
        if (!q) {
            host_mutex_unlock(cs);
            return 0;
        }
        unsigned int n = old >= size ? size : old;
        if (n > 0)
            memcpy(q, p, n);
    }
    if (p)
        FreeDebugBlock(p, flags);
    host_mutex_unlock(cs);
    return q;
}
#pragma auto_inline(on)

// ---- from src/debug/debug_lib_4dbae0.cpp --------------------

// The original calls this out of line from GetBlockSize.
#pragma auto_inline(off)
// FUNCTION: 0x4dbae0
unsigned int __cdecl LookupBlockSize(unsigned int key)
{
    void* cs = GetAllocLock();
    host_mutex_lock(cs);
    // Named local fetched before rec is built: GetBlockMap must be called first.
    BlockMap* tree = (BlockMap*)GetBlockMap();
    BlockInfo rec(key, 0, 0, 0, 0);
    BlockMapIter it = tree->find(rec.address);
    if (it == ((BlockMap*)GetBlockMap())->End()) {
        host_mutex_unlock(cs);
        return 0;
    }
    unsigned int value = it.ptr->value.size;
    host_mutex_unlock(cs);
    return value;
}
#pragma auto_inline(on)

// ---- from src/debug/debug_lib_4dbb90.cpp --------------------

// The original calls this out of line from ProtectBlock.
#pragma auto_inline(off)
// FUNCTION: 0x4dbb90
void __cdecl RoundRangeToPages(unsigned int* start, unsigned int* end)
{
    *start &= 0xfffff000;
    *end = (*end + 0xfff) & 0xfffff000;
}
#pragma auto_inline(on)

// ---- from src/debug/debug_lib_4dbbc0.cpp --------------------

inline MapInsertResult FreeBlockMap::TreeInsert(const Pair_004db000& V)
{
    Node_004db000* X = Root();
    Node_004db000* Y = head;
    bool Ans = true;
    {
        std::_Lockit Lk;
        while (X != DAT_00528a54) {
            Y = X;
            Ans = key_compare(Kfn_004db000()(V), Key(X));
            X = Ans ? Left(X) : Right(X);
        }
    }
    if (multi)
        return MapInsertResult(Insert(X, Y, &V), true);
    FreeBlockIter P = FreeBlockIter(Y);
    if (!Ans)
        ;
    else if (P == begin())
        return MapInsertResult(Insert(X, Y, &V), true);
    else
        P.PrevNode();
    if (key_compare(Key(P.Mynode()), Kfn_004db000()(V)))
        return MapInsertResult(Insert(X, Y, &V), true);
    MapInsertResult res;
    res.Assign(P, false);
    return res;
}

// FUNCTION: 0x4dbbc0
MapInsertResult FreeBlockMap::InsertOrFind(const Pair_004db000& V)
{
    MapInsertResult ans = TreeInsert(V);
    return MapInsertResult(ans.first, ans.second);
}

// The out-of-line tree insert for the allocator's free-block map, shaped like
// std::_Tree<...>::_Insert from MSVC 5's <xtree> but written out by hand.
//
// Under the outer std::_Lockit a 0x18-byte node is carved from the pool
// (0x4ddd70), the pair is placement-new'd into it, the map's size is bumped and
// the node is linked in. Then the red-black fixup walks up from the new node
// with a cursor z, colouring and rotating until z's parent is black or the root
// is reached, and finally blackens the root.
// FUNCTION: 0x4dce60 ?Insert@FreeBlockMap@@QAE?AVFreeBlockIter@@PAUNode_004db000@@0PBUPair_004db000@@@Z
FreeBlockIter FreeBlockMap::Insert(Node_004db000* x, Node_004db000* y,
                                         const Pair_004db000* v)
{
    std::_Lockit lock;
    Node_004db000* p = (Node_004db000*)allocator.Allocate(0x18);
    p->parent = y;
    p->color = 0;                      // red
    p->left = (Node_004db000*)DAT_00528a54;
    p->right = (Node_004db000*)DAT_00528a54;
    new (&p->value) Pair_004db000(*v);
    ++count;

    // A positive disjunction through the bool comparator: keeps the left-child block as the then-part.
    if (y == head || x != DAT_00528a54 || key_compare(v->offset, y->value.offset)) {
        y->left = p;
        // The empty-tree case comes first and updates head->right, not head->left.
        if (y == head) {
            head->parent = p;          // the root
            head->right = p;           // the rightmost node
        } else if (y == head->left) {
            head->left = p;            // the leftmost node
        }
    } else {
        y->right = p;
        if (y == head->right)
            head->right = p;
    }

    Node_004db000* z = p;
    // Explicit break, and z->parent->... re-read from the cursor: parent/grandparent locals cost a register.
    while (z != head->parent) {
        if (z->parent->color != 0)
            break;

        if (z->parent == z->parent->parent->left) {
            Node_004db000* u = z->parent->parent->right;
            if (u->color == 0) {
                // Red uncle: recolour and carry on two levels up.
                z->parent->color = 1;
                u->color = 1;
                z->parent->parent->color = 0;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    Lrotate(z);
                }
                z->parent->color = 1;
                z->parent->parent->color = 0;
                Rrotate(z->parent->parent);
            }
        } else {
            Node_004db000* u = z->parent->parent->left;
            if (u->color == 0) {
                z->parent->color = 1;
                u->color = 1;
                z->parent->parent->color = 0;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    Rrotate(z);
                }
                z->parent->color = 1;
                z->parent->parent->color = 0;
                Lrotate(z->parent->parent);
            }
        }
    }
    head->parent->color = 1;
    return FreeBlockIter(p);
}

// ---- from src/debug/debug_lib_4dbd00.cpp --------------------

// FUNCTION: 0x4dbd00
ConstIter_004dbd00 FreeBlockMap::EraseCopyIter(ConstIter_004dbd00 it)
{
    return Erase((Iter_004dbd00&)it);
}

// ---- from src/debug/debug_lib_4dbd20.cpp --------------------

// FUNCTION: 0x4dbd20
Iter_004dbd20 FreeBlockMap::UpperBound(const unsigned int& kv)
{
    return Iter_004dbd20(Ubound(kv));
}

// ---- from src/debug/debug_lib_4dbd80.cpp --------------------

// FUNCTION: 0x4dbd80
FreeBlockIter FreeBlockIter::Next(int)
{
    FreeBlockIter tmp = *this;
    Inc();
    return tmp;
}

// Tree iterator operator--(int): copy the iterator, step it to the in-order
// predecessor and return the copy. DAT_00528a54 is the tree's _Nil node.
// FUNCTION: 0x4dbe10
FreeBlockIter FreeBlockIter::Previous(int)
{
    FreeBlockIter tmp = *this;
    Dec();
    return tmp;
}

// ---- from src/debug/debug_lib_4dbeb0.cpp --------------------

// 0x4db450's inlined Tail calls this out of line in the original.
#pragma auto_inline(off)
// FUNCTION: 0x4dbeb0
int* FreeBlockMap::Begin(int* out)
{
    *out = (int)head->left;
    return out;
}
#pragma auto_inline(on)

// ---- from src/debug/debug_lib_4dbec0.cpp --------------------

// FUNCTION: 0x4dbec0
MapInsertResult FreeBlockMap::InsertOrFindInline(Pair_004db000* p)
{
    // The pair is returned by value (no out parameter) and _Insert returns its
    // iterator by value too; a &p result slot would make p address-taken.
    Node_004db000* y = head;
    bool less = true;
    Node_004db000* x = y->parent;
    FreeBlockIter it2;
    FreeBlockIter it;
    {
        std::_Lockit lock;
        while (x != DAT_00528a54) {
            y = x;
            less = p->offset < x->value.offset;
            x = less ? x->left : x->right;
        }
    }
    if (multi) {
        {
            std::_Lockit lock;
            it = FreeBlockIter(CreateNode((int)y, 0));
            Node_004db000* z = it.ptr;
            z->left = (Node_004db000*)DAT_00528a54;
            z->right = (Node_004db000*)DAT_00528a54;
            new (&z->value) Pair_004db000(*p);
            count++;
            if (y == head || x != DAT_00528a54 || key_compare(p->offset, y->value.offset)) {
                y->left = z;
                if (y == head) {
                    head->parent = z;
                    head->right = z;
                } else if (y == head->left) {
                    head->left = z;
                }
            } else {
                y->right = z;
                if (y == head->right) {
                    head->right = z;
                }
            }
            for (Node_004db000* q = z; q != head->parent && q->parent->color == 0; ) {
                if (q->parent == q->parent->parent->left) {
                    Node_004db000* w = q->parent->parent->right;
                    if (w->color == 0) {
                        q->parent->color = 1;
                        w->color = 1;
                        q->parent->parent->color = 0;
                        q = q->parent->parent;
                    } else {
                        if (q == q->parent->right) {
                            q = q->parent;
                            RotateLeft(q);
                        }
                        q->parent->color = 1;
                        q->parent->parent->color = 0;
                        RotateRight(q->parent->parent);
                    }
                } else {
                    Node_004db000* w = q->parent->parent->left;
                    if (w->color == 0) {
                        q->parent->color = 1;
                        w->color = 1;
                        q->parent->parent->color = 0;
                        q = q->parent->parent;
                    } else {
                        if (q == q->parent->left) {
                            q = q->parent;
                            RotateRight(q);
                        }
                        q->parent->color = 1;
                        q->parent->parent->color = 0;
                        RotateLeft(q->parent->parent);
                    }
                }
            }
            head->parent->color = 1;
        }
        return MapInsertResult(it, 1);
    }
    it2 = FreeBlockIter(y);
    if (less) {
        if (FreeBlockIter(y) == Begin())
            return MapInsertResult(Insert(x, y, p), 1);
        it2.PrevNode();
    }
    if (key_compare(it2.ptr->value.offset, p->offset))
        return MapInsertResult(Insert(x, y, p), 1);
    return MapInsertResult(it2, 0);
}

// ---- from src/debug/debug_lib_4dc130.cpp --------------------

// FUNCTION: 0x4dc130 ?Erase@FreeBlockMap@@QAE?AVFreeBlockIter@@V2@@Z
FreeBlockIter FreeBlockMap::Erase(FreeBlockIter _P)
{
    Node_004db000* _X;
    Node_004db000* _Y = (_P++)._Mynode();
    Node_004db000* _Z = _Y;
    std::_Lockit _Lk;
    if (Left(_Y) == DAT_00528a54)
        _X = Right(_Y);
    else if (Right(_Y) == DAT_00528a54)
        _X = Left(_Y);
    else
        _Y = Min(Right(_Y)), _X = Right(_Y);
    if (_Y != _Z) {
        Parent(Left(_Z)) = _Y;
        Left(_Y) = Left(_Z);
        if (_Y == Right(_Z))
            Parent(_X) = _Y;
        else {
            Parent(_X) = Parent(_Y);
            Left(Parent(_Y)) = _X;
            Right(_Y) = Right(_Z);
            Parent(Right(_Z)) = _Y;
        }
        if (Root() == _Z)
            Root() = _Y;
        else if (Left(Parent(_Z)) == _Z)
            Left(Parent(_Z)) = _Y;
        else
            Right(Parent(_Z)) = _Y;
        Parent(_Y) = Parent(_Z);
        std::swap(Color(_Y), Color(_Z));
        _Y = _Z;
    } else {
        Parent(_X) = Parent(_Y);
        if (Root() == _Z)
            Root() = _X;
        else if (Left(Parent(_Z)) == _Z)
            Left(Parent(_Z)) = _X;
        else
            Right(Parent(_Z)) = _X;
        if (Lmost() != _Z)
            ;
        else if (Right(_Z) == DAT_00528a54)
            Lmost() = Parent(_Z);
        else
            Lmost() = Min(_X);
        if (Rmost() != _Z)
            ;
        else if (Left(_Z) == DAT_00528a54)
            Rmost() = Parent(_Z);
        else
            Rmost() = Max(_X);
    }
    if (Color(_Y) == _Black) {
        while (_X != Root() && Color(_X) == _Black)
            if (_X == Left(Parent(_X))) {
                Node_004db000* _W = Right(Parent(_X));
                if (Color(_W) == _Red) {
                    Color(_W) = _Black;
                    Color(Parent(_X)) = _Red;
                    Lrotate(Parent(_X));
                    _W = Right(Parent(_X));
                }
                if (Color(Left(_W)) == _Black && Color(Right(_W)) == _Black) {
                    Color(_W) = _Red;
                    _X = Parent(_X);
                } else {
                    if (Color(Right(_W)) == _Black) {
                        Color(Left(_W)) = _Black;
                        Color(_W) = _Red;
                        Rrotate(_W);
                        _W = Right(Parent(_X));
                    }
                    Color(_W) = Color(Parent(_X));
                    Color(Parent(_X)) = _Black;
                    Color(Right(_W)) = _Black;
                    Lrotate(Parent(_X));
                    break;
                }
            } else {
                Node_004db000* _W = Left(Parent(_X));
                if (Color(_W) == _Red) {
                    Color(_W) = _Black;
                    Color(Parent(_X)) = _Red;
                    Rrotate(Parent(_X));
                    _W = Left(Parent(_X));
                }
                if (Color(Right(_W)) == _Black && Color(Left(_W)) == _Black) {
                    Color(_W) = _Red;
                    _X = Parent(_X);
                } else {
                    if (Color(Left(_W)) == _Black) {
                        Color(Right(_W)) = _Black;
                        Color(_W) = _Red;
                        Lrotate(_W);
                        _W = Left(Parent(_X));
                    }
                    Color(_W) = Color(Parent(_X));
                    Color(Parent(_X)) = _Black;
                    Color(Left(_W)) = _Black;
                    Rrotate(Parent(_X));
                    break;
                }
            }
        Color(_X) = _Black;
    }
    Destval(_Value(_Y));
    Freenode(_Y);
    --count;
    return (_P);
}

// ---- from src/debug/debug_lib_4dc620.cpp --------------------

// FUNCTION: 0x4dc620
Iter_004dc620 FreeBlockMap::lower_bound(const unsigned int& kv)
{
    Iter_004dc620 y;
    {
        std::_Lockit lock;
        Node_004db000* x = head->parent;
        y.ptr = head;
        while (x != DAT_00528a54)
            if (key_compare(kv, x->value.offset))
                y.ptr = x, x = x->left;
            else
                x = x->right;
    }
    return y;
}

// ---- from src/debug/debug_lib_4dc680.cpp --------------------

// FUNCTION: 0x4dc680 ?Insert@BlockMap@@QAE?AVMapInsertResult@@PAVBlockInfo@@@Z
MapInsertResult BlockMap::Insert(BlockInfo* p)
{
    Node_004daa30* y = head;
    bool less = true;
    // Read before the first _Lockit.
    Node_004daa30* x = y->parent;
    BlockMapIter it2;
    BlockMapIter it;
    {
        std::_Lockit lock;
        while (x != g_blockMapNil) {
            y = x;
            less = p->address < x->value.address;
            x = less ? x->left : x->right;
        }
    }
    if (multi) {
        {
            std::_Lockit lock;
            it = BlockMapIter(CreateNode((int)y, 0));
            Node_004daa30* z = it.ptr;
            z->left = (Node_004daa30*)g_blockMapNil;
            z->right = (Node_004daa30*)g_blockMapNil;
            new (&z->value) BlockInfo(*p);
            size++;
            if (y == head || x != g_blockMapNil || key_compare(p->address, y->value.address)) {
                y->left = z;
                if (y == head) {
                    head->parent = z;
                    head->right = z;
                } else if (y == head->left) {
                    head->left = z;
                }
            } else {
                y->right = z;
                if (y == head->right) {
                    head->right = z;
                }
            }
            for (Node_004daa30* q = z; q != head->parent && q->parent->color == 0; ) {
                if (q->parent == q->parent->parent->left) {
                    Node_004daa30* w = q->parent->parent->right;
                    if (w->color == 0) {
                        q->parent->color = 1;
                        w->color = 1;
                        q->parent->parent->color = 0;
                        q = q->parent->parent;
                    } else {
                        if (q == q->parent->right) {
                            q = q->parent;
                            RotateLeft(q);
                        }
                        q->parent->color = 1;
                        q->parent->parent->color = 0;
                        RotateRight(q->parent->parent);
                    }
                } else {
                    Node_004daa30* w = q->parent->parent->left;
                    if (w->color == 0) {
                        q->parent->color = 1;
                        w->color = 1;
                        q->parent->parent->color = 0;
                        q = q->parent->parent;
                    } else {
                        if (q == q->parent->left) {
                            q = q->parent;
                            RotateRight(q);
                        }
                        q->parent->color = 1;
                        q->parent->parent->color = 0;
                        RotateLeft(q->parent->parent);
                    }
                }
            }
            head->parent->color = 1;
        }
        // Outside the _Lockit scope: the pair is built after ~_Lockit.
        return MapInsertResult(it, 1);
    }
    it2 = BlockMapIter(y);
    if (less) {
        if (BlockMapIter(y) == Begin())
            return MapInsertResult(*InsertHinted((BlockMapIter*)&p, x, y, p), 1);
        it2.PrevNode();
    }
    if (key_compare(it2.ptr->value.address, p->address))
        return MapInsertResult(*InsertHinted((BlockMapIter*)&p, x, y, p), 1);
    return MapInsertResult(it2, 0);
}

// ---- from src/debug/debug_lib_4dc910.cpp --------------------

// FUNCTION: 0x4dc910
BlockMapIter BlockMap::erase(BlockMapIter _P)
{
    Node_004daa30* _X;
    Node_004daa30* _Y = (_P++)._Mynode();
    Node_004daa30* _Z = _Y;
    std::_Lockit _Lk;
    if (_Y->left == g_blockMapNil)
        _X = _Y->right;
    else if (_Y->right == g_blockMapNil)
        _X = _Y->left;
    else
        _Y = _Min(_Y->right), _X = _Y->right;
    if (_Y != _Z)
        {
        _Z->left->parent = _Y;
        _Y->left = _Z->left;
        if (_Y == _Z->right)
            _X->parent = _Y;
        else
            {
            _X->parent = _Y->parent;
            _Y->parent->left = _X;
            _Y->right = _Z->right;
            _Z->right->parent = _Y;
            }
        if (_Root() == _Z)
            _Root() = _Y;
        else if (_Z->parent->left == _Z)
            _Z->parent->left = _Y;
        else
            _Z->parent->right = _Y;
        _Y->parent = _Z->parent;
        std::swap(_Y->color, _Z->color);
        _Y = _Z;
        }
    else
        {
        _X->parent = _Y->parent;
        if (_Root() == _Z)
            _Root() = _X;
        else if (_Z->parent->left == _Z)
            _Z->parent->left = _X;
        else
            _Z->parent->right = _X;
        if (_Lmost() != _Z)
            ;
        else if (_Z->right == g_blockMapNil)
            _Lmost() = _Z->parent;
        else
            _Lmost() = _Min(_X);
        if (_Rmost() != _Z)
            ;
        else if (_Z->left == g_blockMapNil)
            _Rmost() = _Z->parent;
        else
            _Rmost() = _Max(_X);
        }
    if (_Y->color == 1)
        {
        while (_X != _Root() && _X->color == 1)
            if (_X == _X->parent->left)
                {
                Node_004daa30* _W = _X->parent->right;
                if (_W->color == 0)
                    {
                    _W->color = 1;
                    _X->parent->color = 0;
                    _Lrotate(_X->parent);
                    _W = _X->parent->right;
                    }
                if (_W->left->color == 1 && _W->right->color == 1)
                    {
                    _W->color = 0;
                    _X = _X->parent;
                    }
                else
                    {
                    if (_W->right->color == 1)
                        {
                        _W->left->color = 1;
                        _W->color = 0;
                        _Rrotate(_W);
                        _W = _X->parent->right;
                        }
                    _W->color = _X->parent->color;
                    _X->parent->color = 1;
                    _W->right->color = 1;
                    _Lrotate(_X->parent);
                    break;
                    }
                }
            else
                {
                Node_004daa30* _W = _X->parent->left;
                if (_W->color == 0)
                    {
                    _W->color = 1;
                    _X->parent->color = 0;
                    _Rrotate(_X->parent);
                    _W = _X->parent->left;
                    }
                if (_W->right->color == 1 && _W->left->color == 1)
                    {
                    _W->color = 0;
                    _X = _X->parent;
                    }
                else
                    {
                    if (_W->left->color == 1)
                        {
                        _W->right->color = 1;
                        _W->color = 0;
                        _Lrotate(_W);
                        _W = _X->parent->left;
                        }
                    _W->color = _X->parent->color;
                    _X->parent->color = 1;
                    _W->left->color = 1;
                    _Rrotate(_X->parent);
                    break;
                    }
                }
        _X->color = 1;
        }
    if (_Y != 0)
        {
        *(void**)_Y = g_blockMapFreeList;
        g_blockMapFreeList = _Y;
        }
    --size;
    return (_P);
}

// ---- from src/debug/debug_lib_4dce00.cpp --------------------

// FUNCTION: 0x4dce00
BlockMapIter BlockMap::Find(const unsigned int& key)
{
    BlockMapIter p = BlockMapIter(LowerBound(key));
    return (p == End() || key_compare(key, p.ptr->value.address)) ? End() : p;
}

// ---- from src/debug/debug_lib_4dd150.cpp --------------------

// FUNCTION: 0x4dd150
void FreeBlockMap::RotateLeft(Node_004db000* x)
{
    std::_Lockit lock;
    Node_004db000* y = x->right;
    x->right = y->left;
    if (y->left != DAT_00528a54)
        y->left->parent = x;
    y->parent = x->parent;
    if (x == head->parent)
        head->parent = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

// ---- from src/debug/debug_lib_4dd1b0.cpp --------------------

// 0x4db1c0 calls this out of line in the original (Inc repeats the loop
// inline only where the original did).
#pragma auto_inline(off)
// FUNCTION: 0x4dd1b0
Node_004dd1b0* __cdecl FindLeftmost(Node_004dd1b0* p)
{
    std::_Lockit lock;
    while (p->left != DAT_00528a54)
        p = p->left;
    return p;
}
#pragma auto_inline(on)

// ---- from src/debug/debug_lib_4dd1f0.cpp --------------------

// FUNCTION: 0x4dd1f0
void FreeBlockMap::RotateRight(Node_004db000* x)
{
    std::_Lockit lock;
    Node_004db000* y = x->left;
    x->left = y->right;
    if (y->right != DAT_00528a54)
        y->right->parent = x;
    y->parent = x->parent;
    if (x == head->parent)
        head->parent = y;
    else if (x == x->parent->right)
        x->parent->right = y;
    else
        x->parent->left = y;
    y->right = x;
    x->parent = y;
}

// ---- from src/debug/debug_lib_4dd250.cpp --------------------

// FUNCTION: 0x4dd250
Node_004db000* FreeBlockMap::LowerBound(const unsigned int& kv)
{
    std::_Lockit lock;
    Node_004db000* x = head->parent;
    Node_004db000* y = head;
    while (x != DAT_00528a54)
        if (key_compare(kv, x->value.offset))
            y = x, x = x->left;
        else
            x = x->right;
    return y;
}

// ---- from src/debug/debug_lib_4dd2a0.cpp --------------------

// The original calls this out of line from 0x4daa30 and 0x4db1c0;
// their files had no definition to inline.
#pragma auto_inline(off)
// FUNCTION: 0x4dd2a0
void FreeBlockIter::PrevNode()
{
    std::_Lockit lock;
    if (ptr->color == 0 && ptr->parent->parent == ptr) {
        ptr = ptr->right;
    } else if (ptr->left != DAT_00528a54) {
        ptr = Max_004dd2a0(ptr->left);
    } else {
        Node_004db000* p;
        while (ptr == (p = ptr->parent)->left) {
            ptr = p;
        }
        ptr = p;
    }
}
#pragma auto_inline(on)

// ---- from src/debug/debug_lib_4dd340.cpp --------------------

// FUNCTION: 0x4dd340
void FreeBlockIter::NextNode()
{
    std::_Lockit lock;
    if (ptr->right != DAT_00528a54)
        ptr = Min(ptr->right);
    else {
        Node_004db000* p;
        while (ptr == (p = ptr->parent)->right)
            ptr = p;
        if (ptr->right != p)
            ptr = p;
    }
}

// ---- from src/debug/debug_lib_4dd3d0.cpp --------------------

// 0x4dbae0's inlined find calls this out of line in the original; its
// file had no definition to inline.
#pragma auto_inline(off)
// FUNCTION: 0x4dd3d0
BlockMapIter BlockMap::lower_bound(const unsigned int& key)
{
    return BlockMapIter(Lbound(key));
}
#pragma auto_inline(on)

// ---- from src/debug/debug_lib_4dd430.cpp --------------------

// FUNCTION: 0x4dd430
BlockMapIter BlockMap::InsertHinted(Node_004daa30* x, Node_004daa30* y,
                                    const BlockInfo& v)
{
    std::_Lockit lock;
    Node_004daa30* z = (Node_004daa30*)allocator.Allocate(0x40);
    z->parent = y;
    z->color = 0;
    z->left = (Node_004daa30*)g_blockMapNil;
    z->right = (Node_004daa30*)g_blockMapNil;
    new (&z->value) BlockInfo(v);
    size++;
    if (y == head || x != g_blockMapNil || key_compare(v.address, y->value.address)) {
        y->left = z;
        if (y == head) {
            head->parent = z;
            head->right = z;
        } else if (y == head->left) {
            head->left = z;
        }
    } else {
        y->right = z;
        if (y == head->right) {
            head->right = z;
        }
    }
    for (x = z; x != head->parent && x->parent->color == 0; ) {
        if (x->parent == x->parent->parent->left) {
            Node_004daa30* w = x->parent->parent->right;
            if (w->color == 0) {
                x->parent->color = 1;
                w->color = 1;
                x->parent->parent->color = 0;
                x = x->parent->parent;
            } else {
                if (x == x->parent->right) {
                    x = x->parent;
                    Lrotate(x);
                }
                x->parent->color = 1;
                x->parent->parent->color = 0;
                Rrotate(x->parent->parent);
            }
        } else {
            Node_004daa30* w = x->parent->parent->left;
            if (w->color == 0) {
                x->parent->color = 1;
                w->color = 1;
                x->parent->parent->color = 0;
                x = x->parent->parent;
            } else {
                if (x == x->parent->left) {
                    x = x->parent;
                    Rrotate(x);
                }
                x->parent->color = 1;
                x->parent->parent->color = 0;
                Lrotate(x->parent->parent);
            }
        }
    }
    head->parent->color = 1;
    return BlockMapIter(z);
}

// ---- from src/debug/debug_lib_4dd710.cpp --------------------

// FUNCTION: 0x4dd710
void BlockMap::RotateLeft(Node_004daa30* x)
{
    std::_Lockit lock;
    Node_004daa30* y = x->right;
    x->right = y->left;
    if (y->left != g_blockMapNil)
        y->left->parent = x;
    y->parent = x->parent;
    if (x == head->parent)
        head->parent = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

// ---- from src/debug/debug_lib_4dd770.cpp --------------------

// FUNCTION: 0x4dd770
void BlockMap::RotateRight(Node_004daa30* x)
{
    std::_Lockit lock;
    Node_004daa30* y = x->left;
    x->left = y->right;
    if (y->right != g_blockMapNil)
        y->right->parent = x;
    y->parent = x->parent;
    if (x == head->parent)
        head->parent = y;
    else if (x == x->parent->right)
        x->parent->right = y;
    else
        x->parent->left = y;
    y->right = x;
    x->parent = y;
}

// ---- from src/debug/debug_lib_4dd7d0.cpp --------------------


// ---- from src/debug/debug_lib_4dd820.cpp --------------------

// FUNCTION: 0x4dd820
void BlockMapIter::PrevNode()
{
    std::_Lockit lock;
    if (ptr->color == 0 && ptr->parent->parent == ptr) {
        ptr = ptr->right;
    } else if (ptr->left != g_blockMapNil) {
        ptr = Max_004dd820(ptr->left);
    } else {
        Node_004daa30* p;
        while (ptr == (p = ptr->parent)->left) {
            ptr = p;
        }
        ptr = p;
    }
}

// ---- from src/debug/debug_lib_4dd8c0.cpp --------------------

// FUNCTION: 0x4dd8c0 ?insert@?$vector@UElem_004dd8c0@@VAlloc_004dd8c0@@@std@@QAEXPAUElem_004dd8c0@@IABU3@@Z
InsertFn_004dd8c0 g_insert_004dd8c0 = &Vec_004dd8c0::insert;

// ---- from src/debug/debug_lib_4ddbe0.cpp --------------------

// FUNCTION: 0x4ddbe0
MapInsertResult* MapInsertResult::Assign(int* iter, unsigned char* inserted)
{
    MapInsertResult* eax = this;
    int* ecx = iter;
    int edx = *ecx;
    unsigned char* ecx2 = (unsigned char*)inserted;
    eax->first.ptr = (Node_004db000*)edx;
    unsigned char dl = *ecx2;
    eax->second = dl;
    return eax;
}
// The debug library's image and symbol handling, and the performance status
// dialog: the pooled tree-node allocators, the loaded-image reader (the FPO
// records and the imagehlp symbol handler), the stack walkers and the
// call-stack formatter, the system information dump, and the performance
// window's settings and dialog procedure. The files of the module's fourth
// part, gathered in address order.
// Included only for its symbol count: the functions below match at this count.
#include <math.h>
#include <yvals.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>
#include "../state/sim_globals.h"

// A 0x18-byte pooled node, allocated 0x155 at a time (one 0x2000 block).
extern void* g_freeBlockFreeList;      // free list
extern void (*g_outOfMemoryHandler)();  // out-of-memory handler

static inline Node_004db000* AllocNode_004db000()
{
    if (g_freeBlockFreeList == 0) {
        Node_004db000* block;
        do {
            block = (Node_004db000*)host_mem_alloc(0x2000);
            if (block == 0 && g_outOfMemoryHandler != 0) {
                g_outOfMemoryHandler();
            }
        } while (block == 0 && g_outOfMemoryHandler != 0);
        if (block == 0) {
            return 0;
        }
        Node_004db000* head = (Node_004db000*)g_freeBlockFreeList;
        for (int i = 0; i < 0x155; i++) {
            block->left = head;
            head = block;
            block++;
        }
        g_freeBlockFreeList = head;
    }
    Node_004db000* node = (Node_004db000*)g_freeBlockFreeList;
    g_freeBlockFreeList = node->left;
    return node;
}

// A method that ignores `this`: its caller (0x4dbec0, an inlined tree insert
// after its std::_Lockit) sets ecx to the tree. Shaped like
// std::_Tree<...>::_Buynode(parent, colour) with a pooled allocator.

// The original calls this out of line from the file-record tree insert.
#pragma auto_inline(off)
// FUNCTION: 0x4ddc00
Node_004db000* FreeBlockMap::CreateNode(int parent, int color)
{
    Node_004db000* node = AllocNode_004db000();
    node->parent = (Node_004db000*)parent;
    node->color = color;
    return node;
}
#pragma auto_inline(on)

// std::_Tree<unsigned int, ...>::_Lbound(const key&) from MSVC 5's <xtree>,
// written out by hand: g_blockMapNil is the tree's _Nil node, keys are
// unsigned ints compared with less<>.

extern void* g_blockMapNil;            // the tree's _Nil node

// The original calls this out of line from BlockMap::Find.
#pragma auto_inline(off)
// FUNCTION: 0x4ddc90
Node_004daa30* BlockMap::LowerBound(const unsigned int& key)
{
    std::_Lockit lock;
    Node_004daa30* x = head->parent;
    Node_004daa30* y = head;
    while (x != g_blockMapNil) {
        if (key_compare(x->value.address, key))
            x = x->right;
        else
            y = x, x = x->left;
    }
    return y;
}
#pragma auto_inline(on)

// A 0x40-byte pooled node, allocated 0x80 at a time.
extern void* g_blockMapFreeList;       // free list

static inline Node_004daa30* AllocNode_004daa30()
{
    if (g_blockMapFreeList == 0) {
        Node_004daa30* block;
        do {
            block = (Node_004daa30*)host_mem_alloc(0x2000);
            if (block == 0 && g_outOfMemoryHandler != 0) {
                g_outOfMemoryHandler();
            }
        } while (block == 0 && g_outOfMemoryHandler != 0);
        if (block == 0) {
            return 0;
        }
        Node_004daa30* head = (Node_004daa30*)g_blockMapFreeList;
        for (int i = 0; i < 0x80; i++) {
            block->left = head;
            head = block;
            block++;
        }
        g_blockMapFreeList = head;
    }
    Node_004daa30* node = (Node_004daa30*)g_blockMapFreeList;
    g_blockMapFreeList = node->left;
    return node;
}

// A method that ignores `this`: its caller (0x4dc680, an inlined tree insert
// after its std::_Lockit) sets ecx to the tree. Shaped like
// std::_Tree<...>::_Buynode(parent, colour) with a pooled allocator.

// The original calls this out of line from the file-record tree insert.
#pragma auto_inline(off)
// FUNCTION: 0x4ddce0
Node_004daa30* BlockMap::CreateNode(int parent, int color)
{
    Node_004daa30* node = AllocNode_004daa30();
    node->parent = (Node_004daa30*)parent;
    node->color = color;
    return node;
}
#pragma auto_inline(on)

// Pool allocator for the g_freeBlockFreeList free list: refills it 0x2000 bytes at a
// time (GlobalAlloc, retrying through the out-of-memory handler) by carving
// n-byte pieces, then pops one piece. Identical to 0x4dddf0 apart from the
// free list.

// A method that ignores `this`: its callers (0x4db610, 0x4dce60) set ecx
// to the tree whose nodes it allocates (the allocator sits at +0),
// pushing the node size (0x18).
// The original calls this out of line from the free-block tree.
#pragma auto_inline(off)
// FUNCTION: 0x4ddd70
void* FreeBlockAllocator::Allocate(unsigned int n)
{
    if (g_freeBlockFreeList == 0) {
        unsigned int rem = 0x2000;
        char* block;
        do {
            block = (char*)host_mem_alloc(0x2000);
            if (block == 0 && g_outOfMemoryHandler != 0) {
                g_outOfMemoryHandler();
            }
        } while (block == 0 && g_outOfMemoryHandler != 0);
        if (block == 0) {
            return 0;
        }
        for (; rem >= n; rem -= n) {
            *(void**)block = g_freeBlockFreeList;
            g_freeBlockFreeList = block;
            block += n;
        }
    }
    void* p = g_freeBlockFreeList;
    g_freeBlockFreeList = *(void**)g_freeBlockFreeList;
    return p;
}
#pragma auto_inline(on)

// Pool allocator for the g_blockMapFreeList free list: refills it 0x2000 bytes at a
// time (GlobalAlloc, retrying through the out-of-memory handler) by carving
// n-byte pieces, then pops one piece. Same shape as 0x4e2b60; 0x4ddce0 has
// an inlined copy for 0x40-byte nodes.

// A method that ignores `this`: its callers (0x4da8d0, 0x4dd430) set ecx
// to the tree whose nodes it allocates (the allocator sits at +0),
// pushing the node size (0x40).
// The original calls this out of line from the file-record tree.
#pragma auto_inline(off)
// FUNCTION: 0x4dddf0
void* BlockMapAllocator::Allocate(unsigned int n)
{
    if (g_blockMapFreeList == 0) {
        unsigned int rem = 0x2000;
        char* block;
        do {
            block = (char*)host_mem_alloc(0x2000);
            if (block == 0 && g_outOfMemoryHandler != 0) {
                g_outOfMemoryHandler();
            }
        } while (block == 0 && g_outOfMemoryHandler != 0);
        if (block == 0) {
            return 0;
        }
        for (; rem >= n; rem -= n) {
            *(void**)block = g_blockMapFreeList;
            g_blockMapFreeList = block;
            block += n;
        }
    }
    void* p = g_blockMapFreeList;
    g_blockMapFreeList = *(void**)g_blockMapFreeList;
    return p;
}
#pragma auto_inline(on)

// Shaped like std::_Tree<...>::const_iterator::_Inc() from MSVC 5's <xtree>
// (step an iterator to the in-order successor) under a lock object;
// g_blockMapNil is the tree's _Nil node. Compare 0x4dd710 and 0x4ddc90.

static inline Node_004daa30* Min_004dde70(Node_004daa30* p)
{
    std::_Lockit lock;
    while (p->left != g_blockMapNil)
        p = p->left;
    return p;
}

// The original calls this out of line from the const_iterator increment.
#pragma auto_inline(off)
// FUNCTION: 0x4dde70
void BlockMapIter::NextNode()
{
    std::_Lockit lock;
    if (ptr->right != g_blockMapNil)
        ptr = Min_004dde70(ptr->right);
    else {
        Node_004daa30* p;
        while (ptr == (p = ptr->parent)->right)
            ptr = p;
        if (ptr->right != p)
            ptr = p;
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x4df160
void SetFpuFussy()
{
    static CommandLineSwitch fussy("fpufussy", 1, 0, "-fpufussy", "-fpunofussy", 0, 0);
    if (fussy.on)
        _controlfp(0, _EM_ZERODIVIDE | _EM_INVALID);
    else
        _controlfp(_EM_ZERODIVIDE | _EM_INVALID, _EM_ZERODIVIDE | _EM_INVALID);
}

// FUNCTION: 0x4df1d0
void FpuFussyAtexitHandler(void)
{
}

// FUNCTION: 0x4e3e10
int StringMaxSize(void)
{
    return 0xfffffffd;
}

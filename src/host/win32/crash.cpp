#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../../interfaces/host.h"
#include "../../debug/command_line.h"


// This thread's stack, as the last check saw it (the image's .tls section
// holds them at +4, +8 and +0xc, after the runtime's __tls_start; 0x4d8df0.cpp
// and 0x4d8e20.cpp read the first two). This object defines them, the only
// thread-local data the original has. They are separate variables: as fields
// of one struct, the compiler would not read the flag before storing
// stackLow.
__declspec(thread) char* g_stackLow;       // the stack pointer at the last check
__declspec(thread) char* g_stackHigh;      // the end of the stack's memory region
__declspec(thread) char g_stackKnown;      // g_stackHigh has been looked up

// Whether [p, p + size) lies outside this thread's live stack: 0 when it is
// between the current stack pointer and the end of the stack's region.
// FUNCTION: 0x4d8d70
char __cdecl IsOutsideStack(char* p, int size)
{
    MEMORY_BASIC_INFORMATION info;
    char* top;
    __asm mov top, esp
    g_stackLow = top;
    if (!g_stackKnown) {
        g_stackKnown = 1;
        if (VirtualQuery(top, &info, sizeof info) != sizeof info)
            return 1;
        g_stackHigh = (char*)info.BaseAddress + info.RegionSize;
    }
    if (p < g_stackLow || p + size > g_stackHigh)
        return 1;
    return 0;
}


// The original calls this out of line from FormatSystemInfo.
#pragma auto_inline(off)
// FUNCTION: 0x4d8df0
int __cdecl GetStackLow(void)
{
    IsOutsideStack(0, 0);
    return (int)g_stackLow;
}
#pragma auto_inline(on)

// The original calls this out of line from FormatSystemInfo.
#pragma auto_inline(off)
// FUNCTION: 0x4d8e20
void* GetStackHigh()
{
    IsOutsideStack(0, 0);
    return g_stackHigh;
}
#pragma auto_inline(on)

// Suspected original bugs: the `i % 3 == 3` test in the parameters loop can never
// be true, so the per-three newline is dead code; CreateFileA's result is tested
// against 0 rather than INVALID_HANDLE_VALUE, so a failed open passes the check;
// and the lstrcpynA dump buffer at obj+0x2084 is copied with
// room = 0x7358 - strlen(log) - 0x3e8 regardless of its own length.
// The EAX register-dump format string really reads "EFLGS" in the exe.

class StackTrace {
public:
    unsigned long ret[0x1e];           // +0x00
    int count;                         // +0x78
    int stack[0x800];                  // +0x7c
    int copied;                        // +0x207c
    unsigned long* pc;                 // +0x2080
    char dump_text[0xa44c];            // +0x2084

    void CaptureStack(int* frame, int* esp, int eip, int skip);
    void FormatStackReport();
};

extern int g_errorLogWritten;

char __cdecl AvWriteDetectStub();
char* __cdecl GetExceptionName(unsigned long code);
void __cdecl NormalizeLineEndings(char* buf, int size);
void __cdecl FormatSystemInfo(char* dst, int size);
void UnloadImageHelp();

// The game's structured-exception reporter: builds a text dump in a 0x7358-byte
// stack buffer and appends it to ErrorLog.txt next to the executable.
// FUNCTION: 0x4d8e60
int __cdecl ReportException(EXCEPTION_POINTERS* ep, char* handlerName)
{
    HANDLE file;
    char* dot, * base, name[1000], path[1000], log[0x7358], exe[1000];
    StackTrace obj;
    DWORD written;
    char* reason;

    if (g_errorLogWritten)
        return 0;
    g_errorLogWritten = 1;
    CONTEXT* ctx = ep->ContextRecord;
    EXCEPTION_RECORD* rec = ep->ExceptionRecord;
    obj.CaptureStack((int*)ctx->Ebp, (int*)ctx->Esp, ctx->Eip, 0);
    obj.FormatStackReport();

    // Each sprintf site keeps its destination form (`d`, `L` or inline strlen): it sets push order.
    // find the executable's directory and open the log there
    char* slash;
    if (0 == host_file_exe_path(path, 1000) || !(((slash = strrchr(path, '\\')) != 0) != 0)) { strcpy(path, "C:\\"); } else { slash[1] = 0; }
    strcat(path, "ErrorLog.txt");
    file = CreateFileA(path, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file) { SetFilePointer(file, 0, 0, FILE_END); } else {
    }
    log[0] = 0;
    reason = GetExceptionName(rec->ExceptionCode);

    // the "what crashed" header, written out early
    if (((0 < host_file_exe_path(exe, 1000)) != 0)) {
        base = strrchr(exe, '\\');
        base = base != 0 ? 1 + base : exe;
        strcpy(name, base);
        dot = strrchr(name, '.');
        if (((int)(0 != dot)) != 0) { *dot = 0; }
        sprintf(log + strlen(log), "%s caused an %s in\n", name, reason);
        { char* d = log + strlen(log); sprintf(d, "module %s at %04x:%08lx.\n", base, ctx->SegCs, ctx->Eip); }
        if (file) {
            WriteFile(file, log, strlen(log), &written, 0);
        }
        // Stays inside this block.
        log[0] = 0;
    }

    // the handler name, the module walk and the code/data address the fault
    // happened at
    { size_t L = strlen(log); sprintf(log + L, "Exception handler called in %s. ", handlerName); }
    { char* d = log + strlen(log); FormatSystemInfo(d, 0x7358 - strlen(log)); }
    { char* d = log + strlen(log); sprintf(d, "Instruction pointer is %08lX\n", ctx->Eip); }
    { char* d = log + strlen(log); sprintf(d, "ExceptionCode = %08lX", rec->ExceptionCode); }
    sprintf(log + strlen(log), " - %s\n", reason);
    if (0xc0000005 == rec->ExceptionCode) {
        if (rec->NumberParameters >= 2) {
            if (0 != ((char(__cdecl*)(unsigned long))AvWriteDetectStub)(rec->ExceptionInformation[1])) {
                char* d = log + strlen(log);
                sprintf(d, "Error: Write to read only memory attempted\n");
            }
            const char* rw = 0 != rec->ExceptionInformation[0] ? "write" : "read";
            char* d = log + strlen(log);
            sprintf(d, "Access violation: Illegal %s, data address 0x%08lX\n",
                    rw, rec->ExceptionInformation[1]);
        }
    }

    // the exception record, then the general registers
    { size_t L = strlen(log); sprintf(log + L, "ExceptionFlags = %08lX\t", rec->ExceptionFlags); }
    { char* d = log + strlen(log); sprintf(d, "ExceptionAddress = %08lX\n", rec->ExceptionAddress); }
    if (rec->NumberParameters > 0) {
        { size_t L = strlen(log); sprintf(log + L, "Parameters = "); }
        // Index ExceptionInformation[i] directly; `c` is its own local before `d`.
        for (unsigned int i = 0; i < rec->NumberParameters; i++) {
            char c = ((((int)i) == rec->NumberParameters - 1) || i % 3 == 3) ? '\n' : '\t';
            char* d = log + strlen(log);
            sprintf(d, "%08lX%c", rec->ExceptionInformation[i], c);
        }
    }
    { size_t L = strlen(log); sprintf(log + L, "\n"); }
    { size_t L = strlen(log); sprintf(log + L, "Registers:\n"); }
    { char* d = log + strlen(log);
    sprintf(d, "EAX=%08lX CS=%04lX EIP=%08lX EFLGS=%08lX\n",
            ctx->Eax, ctx->SegCs, ctx->Eip, ctx->EFlags); }
    { char* d = log + strlen(log);
    sprintf(d, "EBX=%08lX SS=%04lX ESP=%08lX EBP=%08lX\n",
            ctx->Ebx, ctx->SegSs, ctx->Esp, ctx->Ebp); }
    { char* d = log + strlen(log);
    sprintf(d, "ECX=%08lX DS=%04lX ESI=%08lX FS=%08lX\n",
            ctx->Ecx, ctx->SegDs, ctx->Esi, ctx->SegFs); }
    { char* d = log + strlen(log);
    sprintf(d, "EDX=%08lX ES=%04lX EDI=%08lX GS=%08lX\n",
            ctx->Edx, ctx->SegEs, ctx->Edi, ctx->SegGs); }

    // the 16 bytes of code at the faulting address
    { size_t L = strlen(log); sprintf(log + L, "\n"); }
    { size_t L = strlen(log); sprintf(L + log, "Bytes at CS:EIP:\n"); }
    unsigned char* code = *((unsigned char**)&ctx->Eip);
    int i = 0;
    if (i < 0x10) {
        do {
            char* d = log + strlen(log);
            sprintf(d, "%02x%c", code[i], i == 0xf ? '\n' : ' ');
            i = 1 + i;
        } while (0x10 > i);
    }
    { size_t L = strlen(log); sprintf(L + log, "\n"); }

    // the disassembler's own dump, the debug registers and the saved FPU state
    int room = 0x7358 - (int)strlen(log);
    room = room - 0x3e8;
    // The whole tail of the dump stays inside this if.
    if (room > 0) {
        lstrcpynA(log + strlen(log), obj.dump_text, room);
        { size_t L = strlen(log); sprintf(log + L, "\n"); }
        { char* d = log + strlen(log); sprintf(d, "Dr0 = %08lX\t", ctx->Dr0); }
        { char* d = log + strlen(log); sprintf(d, "Dr1 = %08lX\t", ctx->Dr1); }
        { char* d = log + strlen(log); sprintf(d, "Dr2 = %08lX\n", ctx->Dr2); }
        { char* d = log + strlen(log); sprintf(d, "Dr3 = %08lX\t", ctx->Dr3); }
        { char* d = log + strlen(log); sprintf(d, "Dr6 = %08lX\t", ctx->Dr6); }
        { char* d = log + strlen(log); sprintf(d, "Dr7 = %08lX\n", ctx->Dr7); }
        { size_t L = strlen(log); sprintf(L + log, "\n"); }
        { char* d = log + strlen(log); sprintf(d, "ContextFlags = %08lX\n", ctx->ContextFlags); }
        { char* d = log + strlen(log); sprintf(d, "Control Word = %08lX\t\t", ctx->FloatSave.ControlWord); }
        { char* d = log + strlen(log); sprintf(d, "StatusWord = %08lX\n", ctx->FloatSave.StatusWord); }
        { char* d = log + strlen(log); sprintf(d, "TagWord = %08lX\t\t", ctx->FloatSave.TagWord); }
        { char* d = log + strlen(log); sprintf(d, "ErrorOffset = %08lX\n", ctx->FloatSave.ErrorOffset); }
        { char* d = log + strlen(log); sprintf(d, "ErrorSelector = %08lX\t", ctx->FloatSave.ErrorSelector); }
        { char* d = log + strlen(log); sprintf(d, "DataOffset = %08lX\n", ctx->FloatSave.DataOffset); }
        { char* d = log + strlen(log); sprintf(d, "DataSelector = %08lX\t\t", ctx->FloatSave.DataSelector); }
        { char* d = log + strlen(log); sprintf(d, "Cr0NpxState = %08lX\n", ctx->FloatSave.Cr0NpxState); }
        { size_t L = strlen(log); sprintf(log + L, "\n\n\n\n\n"); }
    }

    // flush the buffer, close the log and exit the handler
    NormalizeLineEndings(log, 0x7358);
    if (0 != file) {
        WriteFile(file, log, strlen(log), &written, 0);
        CloseHandle(file);
    }
    UnloadImageHelp();
    return 0;
}

// Turns an exception status code into a printable name. A 24 entry table of
// (code, name) pairs is built on the stack and scanned from the top; the first
// pair whose code matches is returned, and anything else returns "Unknown
// exception type". The codes are the Win32 exception status values from
// windows.h, in ascending order.
struct ExceptionName {
    unsigned long code;
    char* name;
};

// FUNCTION: 0x4d98c0
char* __cdecl GetExceptionName(unsigned long code)
{
    ExceptionName table[24] = {
        {0x40010005, "Control-C"},
        {0x40010008, "Control-Break"},
        {0x80000002, "Datatype Misalignment"},
        {0x80000003, "Breakpoint"},
        {0xc0000005, "Access Violation"},
        {0xc0000006, "In Page Error"},
        {0xc0000017, "No Memory"},
        {0xc000001d, "Illegal Instruction"},
        {0xc0000025, "Noncontinuable Exception"},
        {0xc0000026, "Invalid Disposition"},
        {0xc000008c, "Array Bounds Exceeded"},
        {0xc000008d, "Float Denormal Operand"},
        {0xc000008e, "Float Divide by Zero"},
        {0xc000008f, "Float Inexact Result"},
        {0xc0000090, "Float Invalid Operation"},
        {0xc0000091, "Float Overflow"},
        {0xc0000092, "Float Stack Check"},
        {0xc0000093, "Float Underflow"},
        {0xc0000094, "Integer Divide by Zero"},
        {0xc0000095, "Integer Overflow"},
        {0xc0000096, "Privileged Instruction"},
        {0xc00000fd, "Stack Overflow"},
        {0xc0000142, "DLL Initialization Failed"},
        {0xe06d7363, "Microsoft C++ Exception"},
    };
    unsigned int i;
    for (i = 0; i < 24; i++) {
        if (code == table[i].code) {
            return table[i].name;
        }
    }
    return "Unknown exception type";
}

void __cdecl WalkFrameChain(int* frame, int* stack, int eip, int skip,
                           void* this_, int flag, int* field_78, int* field_7c,
                           int size, int* field_207c);

// The original calls this out of line from ReportException.
#pragma auto_inline(off)
// FUNCTION: 0x4d9c60
void StackTrace::CaptureStack(int* frame, int* esp, int eip, int skip)
{
    pc = (unsigned long*)esp;
    WalkFrameChain(frame, esp, eip, skip, this, 0x1e, &count, stack, 0x800, &copied);
}
#pragma auto_inline(on)

// Formats the saved call stack and stack dump into buf.

// FUNCTION: 0x4d9ca0
void StackTrace::FormatStackReport()
{
    int m = copied;
    unsigned long* q0 = pc;
    int n = count;
    // One len for both phases, not a second copy.
    unsigned int len = 0xa44c;
    char* p = dump_text;
    int i;

    if (n > 0) {
        sprintf(p, "Call stack:\n");
        len -= strlen(p);
        p += strlen(p);
        // Both loops index the arrays; walking pointers changes the loop setup.
        for (i = 0; i < n; i++) {
            if (len <= 0x1e) break;
            sprintf(p, "%08lX", ret[i]);
            // A named local set by this if/else, not a ternary: fixes the register split.
            const char* sep;
            if (i == n - 1) {
                sep = "\n";
            } else {
                sep = " ";
                if (i % 8 == 7) sep = "\n";
            }
            strcat(p, sep);
            len -= strlen(p);
            p += strlen(p);
        }
    } else {
        *p = 0;
    }
    if (m > 0 && len > 0x1e) {
        sprintf(p, "Stack dump:\n");
        len -= strlen(p);
        p += strlen(p);
        for (i = 0; i < m; i++) {
            if (len <= 0x1e) break;
            if (i % 8 == 0) {
                sprintf(p, "%08lX: ", (unsigned long)&q0[i]);
                len -= strlen(p);
                p += strlen(p);
            }
            sprintf(p, "%08lX", stack[i]);
            strcat(p, (i == m - 1 || i % 8 == 7) ? "\n" : " ");
            len -= strlen(p);
            p += strlen(p);
        }
    }
}

int __cdecl ReportException(EXCEPTION_POINTERS*, char*);
extern const char g_exceptionHandlerName[];

// FUNCTION: 0x4da2a0
void __stdcall UnhandledExceptionHandler(int ep)
{
    ReportException((EXCEPTION_POINTERS*)ep, (char*)g_exceptionHandlerName);
}

class MappedFile {
public:
    HANDLE hFile;     // +0x0
    HANDLE hMapping;  // +0x4
    void* view;       // +0x8
    int size;         // +0xc
    int state;        // +0x10

    MappedFile(const char* fileName);
    void OpenMappedFile(const char* fileName);
    void CloseMappedFile();
};

// One FPO_DATA record (see 0x4de020).
struct Fpo_004de020 {
    unsigned int offStart;             // +0x0
    unsigned int procSize;             // +0x4
    unsigned int locals;               // +0x8
    unsigned short params;             // +0xc
    unsigned short flags;              // +0xe
};

class LoadedImage : public MappedFile {
public:
    HMODULE module;                        // +0x14
    unsigned int imageBase;                // +0x18
    IMAGE_DOS_HEADER* dosHeader;           // +0x1c
    IMAGE_NT_HEADERS* ntHeaders;           // +0x20
    IMAGE_DEBUG_DIRECTORY* debugDirs;      // +0x24
    int numDebugDirs;                      // +0x28

    LoadedImage(HMODULE m);
    ~LoadedImage();
    Fpo_004de020* GetFpoRecords();
    unsigned int GetFpoRecordCount();
    Fpo_004de020* FindFpoRecord(unsigned int address);
};

// The loaded-image reader of the debug library; 0x4de0a0's function-local
// static builds the one at 0x528a78 lazily.
extern LoadedImage g_loadedImage;

// FUNCTION: 0x4ddf00
LoadedImage::LoadedImage(HMODULE m) : MappedFile(0)
{
    char path[1000];
    module = m;
    imageBase = (unsigned int)m;
    dosHeader = (IMAGE_DOS_HEADER*)m;
    if (host_file_exe_path(path, sizeof(path)))
        path[sizeof(path) - 1] = 0;
    else
        path[0] = 0;
    OpenMappedFile(path);
    ntHeaders = (IMAGE_NT_HEADERS*)((char*)dosHeader + dosHeader->e_lfanew);
    numDebugDirs = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size / sizeof(IMAGE_DEBUG_DIRECTORY);
    // Count stored before debugDirs is cleared: ntHeaders is reloaded for the sum.
    debugDirs = 0;
    if (numDebugDirs)
    {
        // base local and the second test pick the registers of the sum.
        char* base = (char*)imageBase;
        if (numDebugDirs)
            debugDirs = (IMAGE_DEBUG_DIRECTORY*)(base + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress);
    }
}

// Returns the image's FPO records (debug directory type 3,
// IMAGE_DEBUG_TYPE_FPO), or 0 if it has none.

// The original calls this out of line from 0x4de020.
#pragma auto_inline(off)
// FUNCTION: 0x4ddfa0
Fpo_004de020* LoadedImage::GetFpoRecords()
{
    if (debugDirs == 0)
        return 0;
    for (int i = 0; i < numDebugDirs; i++) {
        if (debugDirs[i].Type == IMAGE_DEBUG_TYPE_FPO)
            return (Fpo_004de020*)((char*)view + debugDirs[i].PointerToRawData);
    }
    return 0;
}
#pragma auto_inline(on)

// The original calls this out of line from 0x4de020.
#pragma auto_inline(off)
// FUNCTION: 0x4ddfe0
unsigned int LoadedImage::GetFpoRecordCount()
{
    if (debugDirs == 0)
        return 0;
    for (int i = 0; i < numDebugDirs; i++) {
        if (debugDirs[i].Type == 3)
            return debugDirs[i].SizeOfData / sizeof(Fpo_004de020);
    }
    return 0;
}
#pragma auto_inline(on)

// FUNCTION: 0x4de020
Fpo_004de020* LoadedImage::FindFpoRecord(unsigned int address)
{
    Fpo_004de020* first = GetFpoRecords();
    if (first == 0)
        return 0;
    int n = GetFpoRecordCount();
    if (n == 0)
        return 0;
    Fpo_004de020* last = first + n;
    Fpo_004de020* mid = first + n / 2;
    unsigned int rva = address - imageBase;
    while (first + 1 != last) {
        if (rva < mid->offStart)
            last = mid;
        else
            first = mid;
        mid = first + (last - first) / 2;
    }
    if (rva >= first->offStart && rva < first->offStart + first->procSize)
        return first;
    return 0;
}

// FUNCTION: 0x4de0a0
void __stdcall FunctionTableAccess(int unused, unsigned int address)
{
    static LoadedImage imageTable(GetModuleHandleA(0));
    imageTable.FindFpoRecord(address);
}

// FUNCTION: 0x4de0f0
void CloseLoadedImage()
{
    g_loadedImage.CloseMappedFile();
}

extern int GetDebugLibInstance();

// FUNCTION: 0x4de100
void __stdcall GetModuleBase(int arg1, int arg2)
{
    GetDebugLibInstance();
}

// The imagehlp entry points, resolved by LoadImageHelp (0x4de180): the
// loader, the stack walker and the call-stack formatter each use a different
// signature of the same pointer.
typedef DWORD (__stdcall *SymSetOptions_004de180)(DWORD);
typedef BOOL (__stdcall *SymInitialize_004de180)(HANDLE, char*, DWORD);
typedef void (__stdcall *SymProc_004de180)(void);
typedef BOOL (__stdcall *StackWalk_004de700)(DWORD, HANDLE, HANDLE, void*, void*, void*, void*, void*, void*);

extern char g_imageHelpLoaded;
extern char g_imageHelpInited;
extern HMODULE g_imageHelpModule;
extern SymSetOptions_004de180 g_pfnSymSetOptions;
extern SymInitialize_004de180 g_pfnSymInitialize;
extern BOOL (__stdcall* g_pfnSymCleanup)(HANDLE);
extern StackWalk_004de700 g_pfnStackWalk;
extern void* g_pfnSymFunctionTableAccess;
extern void* g_pfnSymGetModuleBase;
extern SymProc_004de180 g_pfnSymGetSymFromAddr;
extern SymProc_004de180 g_pfnSymGetLineFromAddr;
extern SymProc_004de180 g_pfnUnDecorateSymbolName;

// The original calls this out of line from 0x4de180.
#pragma auto_inline(off)
// FUNCTION: 0x4de110
void UnloadImageHelp()
{
    g_imageHelpLoaded = 0;
    if (g_imageHelpModule) {
        g_pfnSymCleanup(GetCurrentProcess());
        FreeLibrary(g_imageHelpModule);
    }
    g_imageHelpModule = 0;
    g_pfnSymSetOptions = 0;
    g_pfnSymInitialize = 0;
    g_pfnSymCleanup = 0;
    g_pfnStackWalk = 0;
    g_pfnSymFunctionTableAccess = 0;
    g_pfnSymGetModuleBase = 0;
    g_pfnSymGetSymFromAddr = 0;
    g_pfnSymGetLineFromAddr = 0;
    g_pfnUnDecorateSymbolName = 0;
}
#pragma auto_inline(on)

// FUNCTION: 0x4de180
char __cdecl LoadImageHelp(char param)
{
    static CommandLineSwitch imagehlp("imagehlp", 0, 1, "-enableimagehlp",
                                   "-disableimagehlp", 0, 0);
    char path[0x100];
    char symPath[0x3e8];
    char* searchPath;
    char* windir;
    char* slash;
    typedef int (__stdcall *GetProcAddress_004de180)(HMODULE, char*);
    GetProcAddress_004de180 getProcAddress = (GetProcAddress_004de180)GetProcAddress;
    HANDLE (__stdcall *getCurrentProcess)(void) = GetCurrentProcess;

    if (!param && !imagehlp.on)
        return 0;
    if (g_imageHelpLoaded)
        return g_imageHelpInited;
    g_imageHelpLoaded = 1;
    if (!(g_imageHelpModule = LoadLibraryA("IMAGEHLP.DLL")))
        return 0;
    if (!(g_pfnSymSetOptions = (SymSetOptions_004de180)getProcAddress(g_imageHelpModule, "SymSetOptions")))
        return 0;
    if (!(g_pfnSymInitialize = (SymInitialize_004de180)getProcAddress(g_imageHelpModule, "SymInitialize")))
        return 0;
    if (!(g_pfnSymCleanup = (BOOL (__stdcall*)(HANDLE))getProcAddress(g_imageHelpModule, "SymCleanup")))
        return 0;
    if (!(g_pfnStackWalk = (StackWalk_004de700)getProcAddress(g_imageHelpModule, "StackWalk")))
        return 0;
    if (!(g_pfnSymFunctionTableAccess = (SymProc_004de180)getProcAddress(g_imageHelpModule, "SymFunctionTableAccess")))
        return 0;
    if (!(g_pfnSymGetModuleBase = (SymProc_004de180)getProcAddress(g_imageHelpModule, "SymGetModuleBase")))
        return 0;
    g_pfnSymGetSymFromAddr = (SymProc_004de180)getProcAddress(g_imageHelpModule, "SymGetSymFromAddr");
    g_pfnSymGetLineFromAddr = (SymProc_004de180)getProcAddress(g_imageHelpModule, "SymGetLineFromAddr");
    g_pfnUnDecorateSymbolName = (SymProc_004de180)getProcAddress(g_imageHelpModule, "UnDecorateSymbolName");
    DWORD symOpts = 4;
    if (g_pfnSymGetLineFromAddr)
        symOpts = 0x14;
    g_pfnSymSetOptions(symOpts);
    searchPath = 0;
    if (host_file_exe_path(path, sizeof(path))) {
        windir = getenv("windir");
        if (windir) {
            if (strlen(path) + strlen(windir) < 0x3e8) {
                strcpy(symPath, windir);
                slash = strrchr(path, '\\');
                if (slash) {
                    *slash = 0;
                    strcat(symPath, ";");
                    strcat(symPath, path);
                }
                searchPath = symPath;
            }
        }
    }
    // The second SymInitialize passes this result as its last argument, not a literal.
    BOOL inited = g_pfnSymInitialize(getCurrentProcess(), searchPath, 1);
    if (!inited) {
        g_pfnSymFunctionTableAccess = (SymProc_004de180)FunctionTableAccess;
        g_pfnSymGetModuleBase = (SymProc_004de180)GetModuleBase;
        g_pfnSymGetSymFromAddr = 0;
        g_pfnSymGetLineFromAddr = 0;
        if (!g_pfnSymInitialize(getCurrentProcess(), searchPath, inited)) {
            GetLastError();
            UnloadImageHelp();
            g_imageHelpLoaded = 1;
            return 0;
        }
    }
    g_imageHelpInited = 1;
    return 1;
}

// FUNCTION: 0x4de4c0
void ImageHlpAtexitHandler(void)
{
}







// A chain of stack frames (each frame holds the next frame pointer and a
// return address), walked without imagehlp when a stack symbol lookup is
// unavailable.
void __cdecl WalkStack(int param1, int param2, int param3, int param4,
                       int* param5, int param6, int* param7);

// FUNCTION: 0x4de600
void __cdecl WalkFrameChain(int* frame, int* stack, int eip, int skip, int* out, int max, int* count,
                          int* copy, int copyMax, int* copied)
{
    *count = 0;
    if (skip == 0) {
        out[*count] = eip;
        (*count)++;
    } else {
        skip--;
    }
    if (LoadImageHelp(0)) {
        WalkStack((int)frame, (int)stack, eip, skip + 1, out, max, count);
    } else {
        for (int* fp = frame; *count < max; fp = (int*)*fp) {
            if (IsOutsideStack((char*)fp, 8))
                break;
            if (fp < stack)
                break;
            if (*fp <= (int)fp)
                break;
            if (fp[1] == 0)
                break;
            if (skip == 0) {
                out[*count] = fp[1];
                (*count)++;
            } else {
                skip--;
            }
        }
    }
    int i = 0;
    if (stack != 0) {
        for (i = 0; i < copyMax; i++) {
            if (IsOutsideStack((char*)&stack[i], 4))
                break;
            copy[i] = stack[i];
        }
    }
    *copied = i;
    for (; i < copyMax; i++)
        copy[i] = 0;
}

struct Frame_004de700 {
    int addrPc;
    int unknown_04;
    int flags0;                                // +0x08
    char unknown_0c[0x0c];
    int addrFrame;
    char unknown_1c[4];
    int flags1;                                // +0x20
    int addrStack;
    char unknown_28[4];
    int flags2;                                // +0x2c
    char unknown_30[0x68];
};
static_assert(sizeof(Frame_004de700) == 0x98, "StackWalk writes the 0x98-byte STACKFRAME");

extern char g_stackProcessHandleRead;         // "process handle read" flag
extern HANDLE g_stackProcessHandle;           // cached process handle

// FUNCTION: 0x4de700
void __cdecl WalkStack(int param1, int param2, int param3, int param4, int* param5, int param6, int* param7)
{
    Frame_004de700 frame;
    memset(&frame, 0, sizeof(frame));
    frame.addrPc = param3;
    frame.flags0 = 3;
    frame.flags2 = 3;
    frame.flags1 = 3;
    frame.addrStack = param2;
    frame.addrFrame = param1;
    if (!(g_stackProcessHandleRead & 1)) {
        g_stackProcessHandleRead |= 1;
        g_stackProcessHandle = GetCurrentProcess();
    }
    HANDLE thread = GetCurrentThread();
    while (*param7 < param6) {
        if (!g_pfnStackWalk(0x14c, g_stackProcessHandle, thread, &frame, 0, 0, g_pfnSymFunctionTableAccess, g_pfnSymGetModuleBase, 0)) {
            if (*param7 == 0) {
                *param7 = 1;
                param5[0] = param3;
            }
            return;
        }
        if (frame.addrFrame == 0)
            return;
        if (frame.addrPc == 0)
            return;
        if (param4 == 0) {
            param5[*param7] = frame.addrPc;
            ++(*param7);
        } else {
            --param4;
        }
    }
}







extern char* g_libraryDateStamp;
int __cdecl GetStackLow(void);
void* GetStackHigh(void);

// FUNCTION: 0x4ded60
void __cdecl FormatSystemInfo(char* dest, int destLen)
{
    WORD fatDate;
    time_t now;
    WORD fatTime;
    DWORD userNameSize;
    FILETIME ft;
    MEMORYSTATUS memStatus;
    SYSTEM_INFO sysInfo;
    struct tm gmTimeCopy;
    struct tm localTimeCopy;
    char userName[300];
    char buf[2000];
    char exeName[1000];
    char* p;

    buf[0] = 0;
    memset(buf + 1, 0, 1999);

    now = time(NULL);
    localTimeCopy = *localtime(&now);
    p = buf + strlen(buf);
    sprintf(p, "Time: %s", asctime(&localTimeCopy));

    userNameSize = 300;
    if (GetUserNameA(userName, &userNameSize) == 0) {
        strcpy(userName, "unknown user");
    }

    char* machine = getenv("computername");
    if (machine == NULL) {
        machine = "unknown machine";
    }

    if (host_file_exe_path(exeName, 1000) == 0) {
        strcpy(exeName, "Unknown");
    }

    sprintf(buf + strlen(buf), "%s, run by %s on %s\n", exeName, userName, machine);

    HANDLE hFile = CreateFileA(exeName, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != 0) {
        DWORD fileSize = GetFileSize(hFile, NULL);
        if (GetFileTime(hFile, NULL, NULL, &ft)) {
            if (FileTimeToLocalFileTime(&ft, &ft)) {
                if (FileTimeToDosDateTime(&ft, &fatDate, &fatTime)) {
                    p = buf + strlen(buf);
                    sprintf(p,
                        "Executable is %d bytes long and dated %d/%d/%d %02d:%02d:%02d\n",
                        fileSize, (fatDate >> 5) & 0xf, fatDate & 0x1f, (fatDate >> 9) + 1980,
                        fatTime >> 11, (fatTime >> 5) & 0x3f, (fatTime & 0x1f) * 2);
                }
            }
        }
        CloseHandle(hFile);
    }

    HANDLE hMod = GetModuleHandleA(NULL);
    // NT header named and used twice: forces it into a register.
    IMAGE_NT_HEADERS* pNT = (IMAGE_NT_HEADERS*)((char*)hMod + ((IMAGE_DOS_HEADER*)hMod)->e_lfanew);
    gmTimeCopy = *gmtime((time_t*)&pNT->FileHeader.TimeDateStamp);
    p = buf + strlen(buf);
    sprintf(p, "UTC link time: %08lx - %s", pNT->FileHeader.TimeDateStamp, asctime(&gmTimeCopy));

    p = buf + strlen(buf);
    sprintf(p, "Library version %d. Library date %s\n",
            996, g_libraryDateStamp + strlen("Library date stamp: "));

    GetSystemInfo(&sysInfo);
    if (sysInfo.dwNumberOfProcessors > 1) {
        // Then arm keeps the p temporary, else arm calls sprintf directly: not interchangeable.
        p = buf + strlen(buf);
    sprintf(p, "%d processors\n", sysInfo.dwNumberOfProcessors);
    } else {
        sprintf(buf + strlen(buf), "1 processor\n");
    }

    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatus(&memStatus);
    p = buf + strlen(buf);
    sprintf(p, "%d MBytes physical memory\n",
            (memStatus.dwTotalPhys + 900000) >> 20);

    p = buf + strlen(buf);
    sprintf(p, "Stack goes from %08lX to %08lX\n",
            GetStackLow(), GetStackHigh());

    strncpy(dest, buf, destLen);
    dest[destLen - 1] = '\0';
}

#pragma auto_inline(off)
// FUNCTION: 0x4e1560
MappedFile::MappedFile(const char* fileName)
{
    hFile = (void*)-1;
    hMapping = 0;
    view = 0;
    size = 0;
    state = 0;
    if (fileName != 0)
        OpenMappedFile(fileName);
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4e1590
void MappedFile::OpenMappedFile(const char* fileName)
{
    hFile = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        state = 1;
        return;
    }
    size = GetFileSize(hFile, NULL);
    hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapping == NULL) {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
        state = 2;
        return;
    }
    view = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (view == NULL) {
        CloseHandle(hMapping);
        hMapping = NULL;
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
        state = 3;
    }
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4e1650
void MappedFile::CloseMappedFile()
{
    if (view != 0) {
        UnmapViewOfFile(view);
    }
    if (hMapping != 0) {
        CloseHandle(hMapping);
    }
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
}
#pragma auto_inline(on)

// Normalizes the line endings in `text` in place: counts the '\n's, moves the
// string to the tail of the buffer, then rewrites it from the front converting
// lone '\r' and lone '\n' into "\r\n" and collapsing existing "\r\n" pairs.
// Needed: keeps the destination as `text + (size - len) - 1`.

// FUNCTION: 0x4da3f0
void __cdecl NormalizeLineEndings(char *text, int size)
{
    int len = strlen(text);
    char *dst = text + (size - len) - 1;
    int room = dst - text;

    // Index text[i]; a pointer copy is set up after the loop guard.
    for (int i = 0; text[i] != 0; i++)
        if (text[i] == '\n')
            room--;
    if (room <= 0)
        return;

    // memmove, not memcpy: the callee is 0x4e84e0.
    memmove(dst, text, len + 1);

    while (*dst != '\0') {
        char c = *dst;
        if (c == '\r') {
            *text++ = c;
            *text++ = '\n';
            if (dst[1] == '\n')
                dst++;
        } else if (c == '\n') {
            *text++ = '\r';
            *text++ = '\n';
        } else {
            *text++ = c;
        }
        dst++;
    }
    *text = '\0';
}

extern "C" void HOST_CALL host_crash_report(void* exception, const char* handler)
{
    ReportException((EXCEPTION_POINTERS*)exception, (char*)handler);
}

extern "C" void HOST_CALL host_crash_install(void)
{
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)UnhandledExceptionHandler);
}

// FUNCTION: 0x4da120
static void __cdecl ShowDebugMessage(const char* message)
{
    OutputDebugStringA(message);
    MessageBoxA(0, message, "Cavedog", 0);
}

// FUNCTION: 0x4da150
extern "C" int HOST_CALL host_debug_helper(int value)
{
    HMODULE library = LoadLibraryA("DebugHelper.dll");
    if (library) {
        int (__cdecl* func)(int) = (int (__cdecl*)(int))GetProcAddress(library, "DebugFunc1");
        if (func)
            value = func(value);
        else
            ShowDebugMessage("Couldn't find function 'DebugFunc1'\n");
        FreeLibrary(library);
    } else
        ShowDebugMessage("Couldn't find library 'debughelper.dll'\n");
    return value;
}

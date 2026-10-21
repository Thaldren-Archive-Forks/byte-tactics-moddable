#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../../interfaces/host.h"
#include <ctype.h>

__declspec(thread) char* g_stackLow;
__declspec(thread) char* g_stackHigh;
__declspec(thread) char g_stackKnown;

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

#pragma auto_inline(off)
// FUNCTION: 0x4d8df0
int __cdecl GetStackLow(void)
{
    IsOutsideStack(0, 0);
    return (int)g_stackLow;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4d8e20
void* GetStackHigh()
{
    IsOutsideStack(0, 0);
    return g_stackHigh;
}
#pragma auto_inline(on)

class StackTrace {
public:
    unsigned long ret[0x1e];
    int count;
    int stack[0x800];
    int copied;
    unsigned long* pc;
    char dump_text[0xa44c];

    void CaptureStack(int* frame, int* esp, int eip, int skip);
    void FormatStackReport();
};

static int g_errorLogWritten;

char* __cdecl GetExceptionName(unsigned long code);
void __cdecl NormalizeLineEndings(char* buf, int size);
void __cdecl FormatSystemInfo(char* dst, int size);
void UnloadImageHelp();

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

    char* slash;
    if (0 == host_file_exe_path(path, 1000) || !(((slash = strrchr(path, '\\')) != 0) != 0)) { strcpy(path, "C:\\"); } else { slash[1] = 0; }
    strcat(path, "ErrorLog.txt");
    file = CreateFileA(path, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file) { SetFilePointer(file, 0, 0, FILE_END); } else {
    }
    log[0] = 0;
    reason = GetExceptionName(rec->ExceptionCode);

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
        log[0] = 0;
    }

    { size_t L = strlen(log); sprintf(log + L, "Exception handler called in %s. ", handlerName); }
    { char* d = log + strlen(log); FormatSystemInfo(d, 0x7358 - strlen(log)); }
    { char* d = log + strlen(log); sprintf(d, "Instruction pointer is %08lX\n", ctx->Eip); }
    { char* d = log + strlen(log); sprintf(d, "ExceptionCode = %08lX", rec->ExceptionCode); }
    sprintf(log + strlen(log), " - %s\n", reason);
    if (0xc0000005 == rec->ExceptionCode) {
        if (rec->NumberParameters >= 2) {
            const char* rw = 0 != rec->ExceptionInformation[0] ? "write" : "read";
            char* d = log + strlen(log);
            sprintf(d, "Access violation: Illegal %s, data address 0x%08lX\n",
                    rw, rec->ExceptionInformation[1]);
        }
    }

    { size_t L = strlen(log); sprintf(log + L, "ExceptionFlags = %08lX\t", rec->ExceptionFlags); }
    { char* d = log + strlen(log); sprintf(d, "ExceptionAddress = %08lX\n", rec->ExceptionAddress); }
    if (rec->NumberParameters > 0) {
        { size_t L = strlen(log); sprintf(log + L, "Parameters = "); }
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

    int room = 0x7358 - (int)strlen(log);
    room = room - 0x3e8;
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

    NormalizeLineEndings(log, 0x7358);
    if (0 != file) {
        WriteFile(file, log, strlen(log), &written, 0);
        CloseHandle(file);
    }
    UnloadImageHelp();
    return 0;
}

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
                           int* this_, int flag, int* field_78, int* field_7c,
                           int size, int* field_207c);

#pragma auto_inline(off)
// FUNCTION: 0x4d9c60
void StackTrace::CaptureStack(int* frame, int* esp, int eip, int skip)
{
    pc = (unsigned long*)esp;
    WalkFrameChain(frame, esp, eip, skip, (int*)this, 0x1e, &count, stack, 0x800, &copied);
}
#pragma auto_inline(on)

// FUNCTION: 0x4d9ca0
void StackTrace::FormatStackReport()
{
    int m = copied;
    unsigned long* q0 = pc;
    int n = count;
    unsigned int len = 0xa44c;
    char* p = dump_text;
    int i;

    if (n > 0) {
        sprintf(p, "Call stack:\n");
        len -= strlen(p);
        p += strlen(p);
        for (i = 0; i < n; i++) {
            if (len <= 0x1e) break;
            sprintf(p, "%08lX", ret[i]);
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
static const char g_exceptionHandlerName[28] = "Global Exception Handler";

// FUNCTION: 0x4da2a0
void __stdcall UnhandledExceptionHandler(int ep)
{
    ReportException((EXCEPTION_POINTERS*)ep, (char*)g_exceptionHandlerName);
}

class MappedFile {
public:
    HANDLE hFile;
    HANDLE hMapping;
    void* view;
    int size;
    int state;

    MappedFile(const char* fileName);
    void OpenMappedFile(const char* fileName);
    void CloseMappedFile();
};

struct Fpo_004de020 {
    unsigned int offStart;
    unsigned int procSize;
    unsigned int locals;
    unsigned short params;
    unsigned short flags;
};

class LoadedImage : public MappedFile {
public:
    HMODULE module;
    unsigned int imageBase;
    IMAGE_DOS_HEADER* dosHeader;
    IMAGE_NT_HEADERS* ntHeaders;
    IMAGE_DEBUG_DIRECTORY* debugDirs;
    int numDebugDirs;

    LoadedImage(HMODULE m);
    Fpo_004de020* GetFpoRecords();
    unsigned int GetFpoRecordCount();
    Fpo_004de020* FindFpoRecord(unsigned int address);
};

static unsigned char g_loadedImage[sizeof(LoadedImage)];

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
    debugDirs = 0;
    if (numDebugDirs)
    {
        char* base = (char*)imageBase;
        if (numDebugDirs)
            debugDirs = (IMAGE_DEBUG_DIRECTORY*)(base + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress);
    }
}

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
    ((LoadedImage*)g_loadedImage)->CloseMappedFile();
}

// FUNCTION: 0x4de100
void __stdcall GetModuleBase(int arg1, int arg2)
{
}

typedef DWORD (__stdcall *SymSetOptions_004de180)(DWORD);
typedef BOOL (__stdcall *SymInitialize_004de180)(HANDLE, char*, DWORD);
typedef void (__stdcall *SymProc_004de180)(void);
typedef BOOL (__stdcall *StackWalk_004de700)(DWORD, HANDLE, HANDLE, void*, void*, void*, void*, void*, void*);

static char g_imageHelpLoaded;
static char g_imageHelpInited;
static HMODULE g_imageHelpModule;
static SymSetOptions_004de180 g_pfnSymSetOptions;
static SymInitialize_004de180 g_pfnSymInitialize;
static BOOL (__stdcall* g_pfnSymCleanup)(HANDLE);
static StackWalk_004de700 g_pfnStackWalk;
static void* g_pfnSymFunctionTableAccess;
static void* g_pfnSymGetModuleBase;
static SymProc_004de180 g_pfnSymGetSymFromAddr;
static SymProc_004de180 g_pfnSymGetLineFromAddr;
static SymProc_004de180 g_pfnUnDecorateSymbolName;

static bool HasCommandLineSwitch(const char* name)
{
    const char* cmd = GetCommandLineA();
    size_t len = strlen(name);
    if (!cmd)
        return false;
    for (; *cmd; cmd++) {
        if (_strnicmp(cmd, name, len) == 0) {
            char c = cmd[len];
            if (c == 0 || isspace((unsigned char)c) || c == '=')
                return true;
        }
    }
    return false;
}

static char ImageHelpEnabled()
{
    char on = 1;
    if (HasCommandLineSwitch("-enableimagehlp"))
        on = 1;
    if (HasCommandLineSwitch("-disableimagehlp"))
        on = 0;
    return on;
}

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
    static char imagehlpOn = ImageHelpEnabled();
    char path[0x100];
    char symPath[0x3e8];
    char* searchPath;
    char* windir;
    char* slash;
    typedef int (__stdcall *GetProcAddress_004de180)(HMODULE, char*);
    GetProcAddress_004de180 getProcAddress = (GetProcAddress_004de180)GetProcAddress;
    HANDLE (__stdcall *getCurrentProcess)(void) = GetCurrentProcess;

    if (!param && !imagehlpOn)
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
    int flags0;
    char unknown_0c[0x0c];
    int addrFrame;
    char unknown_1c[4];
    int flags1;
    int addrStack;
    char unknown_28[4];
    int flags2;
    char unknown_30[0x68];
};
static_assert(sizeof(Frame_004de700) == 0x98, "StackWalk writes the 0x98-byte STACKFRAME");

static char g_stackProcessHandleRead;
static HANDLE g_stackProcessHandle;

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

static char* g_libraryDateStamp = (char*)"Library date stamp: Jun 30 1998 16:08:53";
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
    IMAGE_NT_HEADERS* pNT = (IMAGE_NT_HEADERS*)((char*)hMod + ((IMAGE_DOS_HEADER*)hMod)->e_lfanew);
    gmTimeCopy = *gmtime((time_t*)&pNT->FileHeader.TimeDateStamp);
    p = buf + strlen(buf);
    sprintf(p, "UTC link time: %08lx - %s", pNT->FileHeader.TimeDateStamp, asctime(&gmTimeCopy));

    p = buf + strlen(buf);
    sprintf(p, "Library version %d. Library date %s\n",
            996, g_libraryDateStamp + strlen("Library date stamp: "));

    GetSystemInfo(&sysInfo);
    if (sysInfo.dwNumberOfProcessors > 1) {
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

// FUNCTION: 0x4da3f0
void __cdecl NormalizeLineEndings(char *text, int size)
{
    int len = strlen(text);
    char *dst = text + (size - len) - 1;
    int room = dst - text;

    for (int i = 0; text[i] != 0; i++)
        if (text[i] == '\n')
            room--;
    if (room <= 0)
        return;

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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../interfaces/host.h"

extern "C" void HOST_CALL host_message(void* owner, const char* title, const char* text, unsigned int style)
{
    UINT flags = 0;
    if (style & HOST_MESSAGE_ERROR) {
        flags |= MB_ICONHAND;
    }
    if (style & HOST_MESSAGE_SYSTEM_MODAL) {
        flags |= MB_SYSTEMMODAL;
    }
    if (style & HOST_MESSAGE_TOPMOST) {
        flags |= MB_TOPMOST;
    }
    MessageBoxA((HWND)owner, text, title, flags);
}

extern "C" void HOST_CALL host_fatal(void* owner, const char* title, const char* text)
{
    if (text) {
        host_message(owner, title, text, HOST_MESSAGE_ERROR | HOST_MESSAGE_SYSTEM_MODAL);
    }
    exit(1);
}

extern "C" void HOST_CALL host_exit(int code)
{
    TerminateProcess(GetCurrentProcess(), (UINT)code);
}

extern "C" void HOST_CALL host_alert(void)
{
    MessageBeep(MB_ICONHAND);
}

extern "C" void HOST_CALL host_log(const char* text)
{
    DWORD written;
    OutputDebugStringA(text);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), text, (DWORD)strlen(text), &written, NULL);
}

int __stdcall GameEntry(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                        LPSTR lpCmdLine, int nCmdShow);

extern "C" int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                                 LPSTR lpCmdLine, int nCmdShow)
{
    return GameEntry(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

extern "C" int HOST_CALL host_instance_running(const char* name)
{
    HANDLE existing = OpenSemaphoreA(0x1f0003, FALSE, name);
    if (existing != NULL) {
        CloseHandle(existing);
        return 1;
    }
    CreateSemaphoreA(NULL, 1, 1, name);
    return 0;
}

extern "C" unsigned int HOST_CALL host_user_name(char* buf, unsigned int size)
{
    DWORD length = size;
    if (!GetUserNameA(buf, &length)) {
        return 0;
    }
    return length;
}

extern "C" void HOST_CALL host_debug_break(void)
{
    DebugBreak();
}

extern "C" void HOST_CALL host_memory_status(struct host_memory* status)
{
    MEMORYSTATUS mem;
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatus(&mem);
    status->total = mem.dwTotalPhys;
    status->available = mem.dwAvailPhys;
}

extern "C" const char* HOST_CALL host_command_line(void)
{
    return GetCommandLineA();
}

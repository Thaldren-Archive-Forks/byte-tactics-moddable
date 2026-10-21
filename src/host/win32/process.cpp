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

extern "C" const char* HOST_CALL host_command_line(void)
{
    return GetCommandLineA();
}

#include <windows.h>
#include <string.h>

#include "../../../interfaces/host.h"
#include "headless.h"

namespace {

const unsigned int kQueueSize = 256;

HWND g_hwnd;
DWORD g_thread;
host_event g_queue[kQueueSize];
unsigned int g_head;
unsigned int g_count;
UINT g_savedMouseTrails;
bool g_mouseTrailsSaved;

void RestoreMouseTrails()
{
    if (g_mouseTrailsSaved) {
        SystemParametersInfoA(SPI_SETMOUSETRAILS, g_savedMouseTrails, 0, TRUE);
        g_mouseTrailsSaved = false;
    }
}

bool Droppable(unsigned int type, int level)
{
    if (level == 0) {
        return type == HOST_EVENT_MOUSE_MOVE;
    }
    if (type == HOST_EVENT_CLOSE || type == HOST_EVENT_DESTROY) {
        return false;
    }
    if (level == 1 && (type == HOST_EVENT_KEY_UP || type == HOST_EVENT_SYS_KEY_UP)) {
        return false;
    }
    return true;
}

void Push(const host_event& ev)
{
    if (g_count == kQueueSize) {
        for (int level = 0; level < 3; level++) {
            bool dropped = false;
            for (unsigned int i = 0; i < g_count && !dropped; i++) {
                if (Droppable(g_queue[(g_head + i) % kQueueSize].type, level)) {
                    for (unsigned int j = i; j + 1 < g_count; j++) {
                        g_queue[(g_head + j) % kQueueSize] = g_queue[(g_head + j + 1) % kQueueSize];
                    }
                    g_count--;
                    dropped = true;
                }
            }
            if (dropped) {
                break;
            }
        }
        if (g_count == kQueueSize) {
            g_head = (g_head + 1) % kQueueSize;
            g_count--;
        }
    }
    g_queue[(g_head + g_count) % kQueueSize] = ev;
    g_count++;
}

void Queue(unsigned int type, unsigned int a, unsigned int b)
{
    host_event ev;
    ev.type = type;
    ev.a = a;
    ev.b = b;
    ev.x = 0;
    ev.y = 0;
    ev.buttons = 0;
    ev.time_ms = 0;
    Push(ev);
}

void QueueMouse(unsigned int type, unsigned int a, unsigned int buttons, unsigned int lparam)
{
    host_event ev;
    ev.type = type;
    ev.a = a;
    ev.b = 0;
    ev.x = (int)(lparam & 0xffff);
    ev.y = (int)((lparam >> 16) & 0xffff);
    ev.buttons = buttons;
    ev.time_ms = host_time_ms();
    Push(ev);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_CREATE:
        return 0;
    case WM_DESTROY:
        RestoreMouseTrails();
        Queue(HOST_EVENT_DESTROY, 0, 0);
        return 0;
    case WM_ACTIVATE:
        Queue(HOST_EVENT_FOCUS, LOWORD(wparam) != 0, 0);
        return 0;
    case WM_CLOSE:
        Queue(HOST_EVENT_CLOSE, 0, 0);
        return 0;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
        Queue(msg, wparam, 0);
        return 0;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        Queue(msg, wparam, 0);
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    case WM_SYSCOMMAND:
        if (wparam == 0xf100) {
            return 0;
        }
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
        QueueMouse(msg, 0, (unsigned int)wparam, (unsigned int)lparam);
        return 0;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        QueueMouse(msg, 0, (unsigned int)wparam, (unsigned int)lparam);
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    case 0x20a:
        QueueMouse(HOST_EVENT_WHEEL, (unsigned int)(int)(short)HIWORD(wparam), LOWORD(wparam), (unsigned int)lparam);
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    case 0x219:
    case 0x3b9:
        Queue(msg, (unsigned int)wparam, (unsigned int)lparam);
        return 1;
    case 0x30f:
        Queue(HOST_EVENT_PALETTE_QUERY, 0, 0);
        return 1;
    case 0x311:
        Queue(HOST_EVENT_PALETTE_CHANGED, (HWND)wparam == hwnd, 0);
        return 0;
    default:
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    }
}

}

extern "C" void* HOST_CALL host_window_open(const char* title, int width, int height, int fullscreen)
{
    if (HostHeadless()) return HeadlessWindowOpen(title, width, height, fullscreen);
    HINSTANCE instance = GetModuleHandleA(NULL);
    WNDCLASSA wc;
    wc.style = 8;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIconA(instance, IDI_APPLICATION);
    wc.hCursor = LoadCursorA(instance, (LPCSTR)103);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "Total Annihilation Class";
    if (RegisterClassA(&wc) == 0) {
        return NULL;
    }
    g_thread = GetCurrentThreadId();
    SystemParametersInfoA(SPI_GETMOUSETRAILS, 0, &g_savedMouseTrails, TRUE);
    g_mouseTrailsSaved = true;
    SystemParametersInfoA(SPI_SETMOUSETRAILS, 0, 0, TRUE);
    g_hwnd = CreateWindowExA(WS_EX_APPWINDOW, wc.lpszClassName, title,
                             WS_POPUP | WS_VISIBLE | WS_SYSMENU,
                             fullscreen ? 0 : CW_USEDEFAULT, fullscreen ? 0 : CW_USEDEFAULT, width, height,
                             NULL, NULL, instance, NULL);
    if (g_hwnd != NULL) {
        STARTUPINFOA startup;
        GetStartupInfoA(&startup);
        int show = SW_SHOWDEFAULT;
        if (startup.dwFlags & STARTF_USESHOWWINDOW) {
            show = startup.wShowWindow;
        }
        ShowWindow(g_hwnd, show);
        UpdateWindow(g_hwnd);
    }
    return g_hwnd;
}

extern "C" void HOST_CALL host_window_close(void)
{
    if (HostHeadless()) { HeadlessWindowClose(); return; }
    DestroyWindow(g_hwnd);
    RestoreMouseTrails();
}

extern "C" int HOST_CALL host_window_has_focus(void)
{
    if (HostHeadless()) return HeadlessWindowHasFocus();
    return GetFocus() == g_hwnd;
}

extern "C" void HOST_CALL host_window_post(unsigned int type)
{
    if (HostHeadless()) { HeadlessWindowPost(type); return; }
    PostMessageA(g_hwnd, type, 0, 0);
}

extern "C" void HOST_CALL host_window_place(int x, int y, int width, int height)
{
    if (HostHeadless() || g_hwnd == NULL) return;
    SetWindowPos(g_hwnd, NULL, x, y, width, height, SWP_NOZORDER);
}

extern "C" void HOST_CALL host_window_resize(int width, int height)
{
    if (HostHeadless() || g_hwnd == NULL) return;
    SetWindowPos(g_hwnd, NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE);
}

extern "C" void HOST_CALL host_window_set_topmost(int on)
{
    if (HostHeadless() || g_hwnd == NULL) return;
    SetWindowPos(g_hwnd, on ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

// FUNCTION: 0x4da380
static BOOL HandleDialogMessage(MSG* msg)
{
    HWND root;
    HWND parent = msg->hwnd;
    char cls[8];
    root = parent;
    while (parent) {
        root = parent;
        parent = GetParent(parent);
    }
    if (root && GetClassNameA(root, cls, sizeof cls) && strcmp(cls, "#32770") == 0 && IsDialogMessageA(root, msg))
        return 1;
    return 0;
}

static void DispatchOne(MSG* msg)
{
    if (!HandleDialogMessage(msg)) {
        TranslateMessage(msg);
        DispatchMessageA(msg);
    }
}

extern "C" int HOST_CALL host_pump_events(int wait)
{
    if (HostHeadless()) return HeadlessPumpEvents(wait);
    MSG msg;
    if (wait) {
        int ret = GetMessageA(&msg, NULL, 0, 0);
        if (ret == 0) {
            return 0;
        }
        if (ret != -1) {
            DispatchOne(&msg);
        }
    }
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return 0;
        }
        DispatchOne(&msg);
    }
    return 1;
}

extern "C" void HOST_CALL host_post_quit(void)
{
    if (HostHeadless()) { HeadlessPostQuit(); return; }
    PostQuitMessage(0);
}

extern "C" int HOST_CALL host_event_next(host_event* event)
{
    if (HostHeadless()) return HeadlessEventNext(event);
    if (GetCurrentThreadId() != g_thread || g_count == 0) {
        return 0;
    }
    *event = g_queue[g_head];
    g_head = (g_head + 1) % kQueueSize;
    g_count--;
    return 1;
}

extern "C" int HOST_CALL host_key_state(int vk)
{
    if (HostHeadless()) return HeadlessKeyState(vk);
    return (unsigned short)GetAsyncKeyState(vk);
}

extern "C" void HOST_CALL host_cursor_get(int* x, int* y)
{
    if (HostHeadless()) { HeadlessCursorGet(x, y); return; }
    POINT pt;
    GetCursorPos(&pt);
    *x = pt.x;
    *y = pt.y;
}

extern "C" void HOST_CALL host_cursor_set(int x, int y)
{
    if (HostHeadless()) { HeadlessCursorSet(x, y); return; }
    SetCursorPos(x, y);
}

HWND host_win32_window(void)
{
    return g_hwnd;
}

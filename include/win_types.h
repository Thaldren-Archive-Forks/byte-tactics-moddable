#ifndef WIN_TYPES_H
#define WIN_TYPES_H

#ifndef _WINDEF_
typedef unsigned long DWORD;
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef long LONG;
typedef unsigned int UINT;
typedef const char* LPCSTR;
typedef void* HANDLE;
typedef void* PVOID;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef char* LPSTR;
typedef unsigned char* LPBYTE;
typedef unsigned int WPARAM;
typedef long LPARAM;
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef NOMINMAX
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#endif
typedef long (__stdcall* WNDPROC)(void*, unsigned int, unsigned int, long);
#define MAX_PATH 260
typedef HANDLE HWND;
typedef HANDLE HINSTANCE;
typedef HANDLE HDC;
typedef HANDLE HPALETTE;
typedef HANDLE HICON;
typedef HANDLE HBRUSH;
typedef HINSTANCE HMODULE;
typedef HICON HCURSOR;
typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT;
typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT;
typedef RECT* LPRECT;
#endif

#ifndef _WINNT_
typedef wchar_t WCHAR;
typedef struct _RTL_CRITICAL_SECTION {
    void* DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    DWORD Reserved;
} RTL_CRITICAL_SECTION;
#endif

struct _EXCEPTION_POINTERS;
typedef struct _EXCEPTION_POINTERS EXCEPTION_POINTERS;
struct _IMAGE_DOS_HEADER;
struct _IMAGE_NT_HEADERS;
struct _IMAGE_DEBUG_DIRECTORY;

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;
#endif

#ifndef _WINGDI_
typedef struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} PALETTEENTRY;
typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;
typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER;
#endif

#ifndef _WINUSER_
#define VK_CONTROL 0x11
#define VK_PAUSE 0x13
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONDOWN 0x0204
#define WM_RBUTTONDBLCLK 0x0206
#pragma pack(push, 2)
typedef struct {
    DWORD style;
    DWORD dwExtendedStyle;
    WORD cdit;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATE;
typedef struct {
    DWORD style;
    DWORD dwExtendedStyle;
    short x;
    short y;
    short cx;
    short cy;
    WORD id;
} DLGITEMTEMPLATE;
#pragma pack(pop)
typedef struct tagWNDCLASSA {
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCSTR lpszMenuName;
    LPCSTR lpszClassName;
} WNDCLASSA;
typedef struct tagWINDOWPLACEMENT {
    UINT length;
    UINT flags;
    UINT showCmd;
    POINT ptMinPosition;
    POINT ptMaxPosition;
    RECT rcNormalPosition;
} WINDOWPLACEMENT;
#endif

static_assert(sizeof(DWORD) == 4 && sizeof(BOOL) == 4 && sizeof(BYTE) == 1 && sizeof(WORD) == 2, "scalar sizes");
static_assert(sizeof(LONG) == 4 && sizeof(UINT) == 4 && sizeof(HWND) == 4 && sizeof(HANDLE) == 4, "scalar sizes");
static_assert(sizeof(RTL_CRITICAL_SECTION) == 24 && sizeof(WCHAR) == 2, "critical section and WCHAR sizes");
static_assert(sizeof(DLGTEMPLATE) == 18 && sizeof(DLGITEMTEMPLATE) == 18, "dialog template sizes");
static_assert(sizeof(RECT) == 16 && sizeof(POINT) == 8 && sizeof(GUID) == 16, "RECT, POINT, GUID sizes");
static_assert(sizeof(PALETTEENTRY) == 4 && sizeof(RGBQUAD) == 4 && sizeof(BITMAPINFOHEADER) == 40, "GDI sizes");
static_assert(sizeof(WNDCLASSA) == 40 && sizeof(WINDOWPLACEMENT) == 44, "USER sizes");

#endif

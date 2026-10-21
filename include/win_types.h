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
typedef long (__stdcall* WNDPROC)(void*, unsigned int, unsigned int, long);
#define MAX_PATH 260
#define DECLARE_WIN_HANDLE(name) \
    struct name##__;             \
    typedef name##__* name
DECLARE_WIN_HANDLE(HWND);
DECLARE_WIN_HANDLE(HINSTANCE);
DECLARE_WIN_HANDLE(HDC);
DECLARE_WIN_HANDLE(HPALETTE);
DECLARE_WIN_HANDLE(HICON);
DECLARE_WIN_HANDLE(HBRUSH);
typedef HINSTANCE HMODULE;
typedef HICON HCURSOR;
#undef DECLARE_WIN_HANDLE
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
#endif

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
static_assert(sizeof(RECT) == 16 && sizeof(POINT) == 8 && sizeof(GUID) == 16, "RECT, POINT, GUID sizes");
static_assert(sizeof(PALETTEENTRY) == 4 && sizeof(RGBQUAD) == 4 && sizeof(BITMAPINFOHEADER) == 40, "GDI sizes");
static_assert(sizeof(WNDCLASSA) == 40 && sizeof(WINDOWPLACEMENT) == 44, "USER sizes");

#endif

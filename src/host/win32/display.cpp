#include <windows.h>
#include <ddraw.h>
#include <stdlib.h>
#include <string.h>

#include "../../../interfaces/host.h"
#include "headless.h"

HWND host_win32_window(void);

namespace {

const HRESULT kSurfaceLost = (HRESULT)0x887601c2;

IDirectDraw* g_ddraw;
IDirectDrawSurface* g_primary;
IDirectDrawSurface* g_back;
IDirectDrawClipper* g_clipper;
IDirectDrawPalette* g_palette;
int g_width;
int g_height;
HDC g_windowedDC;
HBITMAP g_windowedDib;
HPALETTE g_windowedPalette;
void* g_windowedBits;
int g_windowedWidth;
int g_windowedHeight;
unsigned char g_windowedEntries[256 * 4];

IDirectDrawSurface* Frame(int which)
{
    return which == HOST_FRAME_FRONT ? g_primary : g_back;
}

int Result(HRESULT hr)
{
    if (hr == DD_OK) {
        return HOST_FRAME_OK;
    }
    return hr == kSurfaceLost ? HOST_FRAME_LOST : HOST_FRAME_FAILED;
}

void FreeWindowed()
{
    if (g_windowedDC) {
        DeleteDC(g_windowedDC);
        g_windowedDC = 0;
    }
    if (g_windowedPalette) {
        DeleteObject(g_windowedPalette);
        g_windowedPalette = 0;
    }
    if (g_windowedDib) {
        DeleteObject(g_windowedDib);
        g_windowedDib = 0;
    }
    g_windowedBits = 0;
}

void ReleaseDirectDraw()
{
    if (g_ddraw) {
        if (g_palette) {
            g_palette->Release();
            g_palette = 0;
        }
        if (g_back) {
            g_back->Release();
            g_back = 0;
        }
        if (g_primary) {
            g_primary->Release();
            g_primary = 0;
        }
        if (g_clipper) {
            g_clipper->Release();
            g_clipper = 0;
        }
        g_ddraw->Release();
        g_ddraw = 0;
    }
}

bool WindowedPaletteActive()
{
    return g_windowedDC && !g_ddraw;
}

bool WindowedBackActive()
{
    return g_windowedDC && !g_back;
}

int WindowedPaletteCreate(const unsigned char* entries)
{
    struct {
        WORD version;
        WORD count;
        PALETTEENTRY entries[256];
    } logical;
    unsigned char quads[256 * 4];

    if (g_windowedPalette) {
        DeleteObject(g_windowedPalette);
        g_windowedPalette = 0;
    }
    logical.version = 0x300;
    logical.count = 0x100;
    for (int i = 0; i < 256; i++) {
        logical.entries[i].peRed = entries[i * 4];
        logical.entries[i].peGreen = entries[i * 4 + 1];
        logical.entries[i].peBlue = entries[i * 4 + 2];
        logical.entries[i].peFlags = entries[i * 4 + 3];
        quads[i * 4 + 2] = entries[i * 4];
        quads[i * 4 + 1] = entries[i * 4 + 1];
        quads[i * 4 + 0] = entries[i * 4 + 2];
        quads[i * 4 + 3] = 0;
    }
    if (entries != g_windowedEntries) {
        memcpy(g_windowedEntries, entries, sizeof(g_windowedEntries));
    }
    g_windowedPalette = CreatePalette((LOGPALETTE*)&logical);
    if (!g_windowedPalette) {
        return 0;
    }
    SetDIBColorTable(g_windowedDC, 0, 0x100, (RGBQUAD*)quads);
    return 1;
}

int WindowedPresent(int x, int y, int width, int height)
{
    HWND hwnd = host_win32_window();
    HDC dc;
    BOOL copied;

    if (!g_windowedDC) {
        return HOST_FRAME_FAILED;
    }
    dc = GetDC(hwnd);
    if (!dc) {
        return HOST_FRAME_FAILED;
    }
    if (g_windowedPalette) {
        SelectPalette(dc, g_windowedPalette, FALSE);
        RealizePalette(dc);
    }
    copied = BitBlt(dc, x, y, width, height, g_windowedDC, x, y, SRCCOPY);
    ReleaseDC(hwnd, dc);
    return copied ? HOST_FRAME_OK : HOST_FRAME_FAILED;
}

int WindowedSetUp(int width, int height, const unsigned char* palette)
{
    struct {
        BITMAPINFOHEADER header;
        RGBQUAD colours[256];
    } bmi;
    HDC window = GetDC(host_win32_window());

    if (!window) {
        return HOST_DISPLAY_FAILED;
    }
    g_windowedDC = CreateCompatibleDC(window);
    ReleaseDC(host_win32_window(), window);
    if (!g_windowedDC) {
        return HOST_DISPLAY_FAILED;
    }
    memset(&bmi, 0, sizeof(bmi));
    bmi.header.biSize = sizeof(BITMAPINFOHEADER);
    bmi.header.biWidth = width;
    bmi.header.biHeight = -height;
    bmi.header.biPlanes = 1;
    bmi.header.biBitCount = 8;
    bmi.header.biCompression = BI_RGB;
    g_windowedDib = CreateDIBSection(g_windowedDC, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, &g_windowedBits, NULL, 0);
    if (!g_windowedDib) {
        FreeWindowed();
        return HOST_DISPLAY_FAILED;
    }
    SelectObject(g_windowedDC, g_windowedDib);
    g_windowedWidth = width;
    g_windowedHeight = height;
    if (palette) {
        WindowedPaletteCreate(palette);
    }
    return HOST_DISPLAY_OK;
}

struct ModeCollector {
    host_display_mode* modes;
    int max;
    int count;
};

HRESULT __stdcall CollectMode(DDSURFACEDESC* desc, void* context)
{
    ModeCollector* collector = (ModeCollector*)context;
    if (collector->count < collector->max) {
        host_display_mode* mode = &collector->modes[collector->count++];
        mode->width = desc->dwWidth;
        mode->height = desc->dwHeight;
        mode->bits = desc->ddpfPixelFormat.dwRGBBitCount;
        mode->refresh = desc->dwRefreshRate;
    }
    return DDENUMRET_OK;
}

}

extern "C" int HOST_CALL host_display_mode_set(int width, int height, int flags, const unsigned char* palette)
{
    if (HostHeadless()) return HeadlessDisplayModeSet(width, height, flags, palette);
    HWND hwnd = host_win32_window();
    DDSURFACEDESC ddsd;
    HRESULT hr;

    if (!(flags & HOST_DISPLAY_PRIMARY_ONLY)) {
        FreeWindowed();
    }
    if (flags & HOST_DISPLAY_WINDOWED) {
        ReleaseDirectDraw();
        if (width <= 0 || height <= 0) {
            return HOST_DISPLAY_FAILED;
        }
        return WindowedSetUp(width, height, palette);
    }

    g_width = width;
    g_height = height;
    hr = DirectDrawCreate(0, &g_ddraw, 0);
    if (hr != DD_OK) {
        return HOST_DISPLAY_FAILED;
    }

    if (flags & HOST_DISPLAY_PRIMARY_ONLY) {
        if (g_ddraw->SetCooperativeLevel(hwnd, 8) != 0) {
            g_ddraw->Release();
            g_ddraw = 0;
            return HOST_DISPLAY_FAILED;
        }
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        if (g_ddraw->CreateSurface(&ddsd, &g_primary, 0) != 0) {
            return HOST_DISPLAY_NO_FRAME;
        }
        return HOST_DISPLAY_OK;
    }

    hr = g_ddraw->SetCooperativeLevel(hwnd, 0x53);
    if (hr != DD_OK) {
        return HOST_DISPLAY_FAILED;
    }
    hr = g_ddraw->SetDisplayMode(width, height, 8);
    if (hr != DD_OK) {
        return HOST_DISPLAY_FAILED;
    }

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
    ddsd.dwBackBufferCount = 1;
    hr = g_ddraw->CreateSurface(&ddsd, &g_primary, NULL);
    if (hr != DD_OK) {
        return HOST_DISPLAY_FAILED;
    }

    DDSCAPS caps;
    caps.dwCaps = DDSCAPS_BACKBUFFER;
    hr = g_primary->GetAttachedSurface(&caps, &g_back);
    if (hr != DD_OK) {
        return HOST_DISPLAY_FAILED;
    }

    hr = g_ddraw->CreateClipper(0, &g_clipper, NULL);
    if (hr != DD_OK) {
        return HOST_DISPLAY_FAILED;
    }
    hr = g_clipper->SetHWnd(0, hwnd);
    if (hr != DD_OK) {
        return HOST_DISPLAY_FAILED;
    }
    hr = g_primary->SetClipper(g_clipper);
    if (hr != DD_OK) {
        return HOST_DISPLAY_FAILED;
    }

    if (palette) {
        hr = g_ddraw->CreatePalette(4, (LPPALETTEENTRY)palette, &g_palette, NULL);
        if (hr == DD_OK) {
            hr = g_primary->SetPalette(g_palette);
            if (hr != DD_OK) {
                return HOST_DISPLAY_FAILED;
            }
        }
    }
    return HOST_DISPLAY_OK;
}

extern "C" void HOST_CALL host_display_close(void)
{
    if (HostHeadless()) { HeadlessDisplayClose(); return; }
    if (g_ddraw) {
        ReleaseDirectDraw();
        if (g_windowedDC) {
            return;
        }
    }
    FreeWindowed();
}

extern "C" int HOST_CALL host_display_is_open(void)
{
    if (HostHeadless()) return HeadlessDisplayIsOpen();
    return g_ddraw != 0;
}

extern "C" int HOST_CALL host_display_has_back_buffer(void)
{
    if (HostHeadless()) return HeadlessDisplayHasBackBuffer();
    return g_back != 0;
}

extern "C" int HOST_CALL host_display_mode_list(host_display_mode* modes, int max, int* count)
{
    if (HostHeadless()) return HeadlessDisplayModeList(modes, max, count);
    ModeCollector collector;
    LPDIRECTDRAW2 ddraw2 = 0;
    int result = 1;
    HRESULT hr;

    collector.modes = modes;
    collector.max = max;
    collector.count = 0;
    if (!g_ddraw) {
        *count = 0;
        return 0;
    }
    hr = g_ddraw->QueryInterface(IID_IDirectDraw2, (LPVOID*)&ddraw2);
    if (hr == 0) {
        hr = ddraw2->EnumDisplayModes(0, 0, &collector, (LPDDENUMMODESCALLBACK)CollectMode);
        if (hr != 0) {
            result = 0;
        }
    } else {
        result = 0;
    }
    if (ddraw2) {
        ddraw2->Release();
    }
    *count = collector.count;
    return result;
}

extern "C" void HOST_CALL host_display_screen_size(int* width, int* height)
{
    if (HostHeadless()) { HeadlessDisplayScreenSize(width, height); return; }
    *width = GetSystemMetrics(SM_CXSCREEN);
    *height = GetSystemMetrics(SM_CYSCREEN);
}

extern "C" int HOST_CALL host_frame_lock(int which, host_frame* frame)
{
    if (HostHeadless()) return HeadlessFrameLock(which, frame);
    if (which == HOST_FRAME_BACK && WindowedBackActive()) {
        frame->pixels = (unsigned char*)g_windowedBits;
        frame->pitch = (g_windowedWidth + 3) & ~3;
        frame->width = g_windowedWidth;
        frame->height = g_windowedHeight;
        return HOST_FRAME_OK;
    }
    IDirectDrawSurface* surface = Frame(which);
    DDSURFACEDESC desc;
    int result;

    if (!surface) {
        return HOST_FRAME_FAILED;
    }
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    result = Result(surface->Lock(0, &desc, which == HOST_FRAME_FRONT ? DDLOCK_WAIT : 0x801, 0));
    if (result == HOST_FRAME_OK) {
        frame->pixels = (unsigned char*)desc.lpSurface;
        frame->pitch = desc.lPitch;
        frame->width = g_width;
        frame->height = g_height;
    }
    return result;
}

extern "C" int HOST_CALL host_frame_unlock(int which)
{
    if (HostHeadless()) return HeadlessFrameUnlock(which);
    if (which == HOST_FRAME_BACK && WindowedBackActive()) {
        return 1;
    }
    IDirectDrawSurface* surface = Frame(which);
    if (!surface) {
        return 0;
    }
    surface->Unlock(0);
    return 1;
}

extern "C" int HOST_CALL host_frame_present(int flip)
{
    if (HostHeadless()) return HeadlessFramePresent(flip);
    if (WindowedBackActive()) {
        return WindowedPresent(0, 0, g_windowedWidth, g_windowedHeight);
    }
    if (flip) {
        return Result(g_primary->Flip(0, 1));
    }
    RECT rect;
    POINT pt;
    HWND hwnd = host_win32_window();
    GetClientRect(hwnd, &rect);
    pt.x = 0;
    pt.y = 0;
    RECT src = rect;
    ClientToScreen(hwnd, &pt);
    OffsetRect(&rect, pt.x, pt.y);
    return Result(g_primary->Blt(&rect, g_back, &src, 0x1000000, 0));
}

extern "C" int HOST_CALL host_frame_present_rect(int x, int y, int width, int height)
{
    if (HostHeadless()) return HeadlessFramePresentRect(x, y, width, height);
    return WindowedPresent(x, y, width, height);
}

extern "C" int HOST_CALL host_frame_is_lost(void)
{
    if (HostHeadless()) return HeadlessFrameIsLost();
    return g_primary && g_primary->IsLost() == kSurfaceLost;
}

extern "C" int HOST_CALL host_frame_restore(void)
{
    if (HostHeadless()) return HeadlessFrameRestore();
    return g_primary && g_back && g_primary->Restore() == 0 && g_back->Restore() == 0;
}

extern "C" int HOST_CALL host_frame_fill(int which, unsigned int colour)
{
    if (HostHeadless()) return HeadlessFrameFill(which, colour);
    IDirectDrawSurface* surface = Frame(which);
    DDBLTFX fx;

    if (!surface) {
        return 0;
    }
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(DDBLTFX);
    fx.dwFillColor = colour;
    return surface->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &fx) == 0;
}

extern "C" int HOST_CALL host_frame_pixel_format(host_pixel_format* format)
{
    if (HostHeadless()) return HeadlessFramePixelFormat(format);
    DDPIXELFORMAT ddpf;

    memset(&ddpf, 0, sizeof(ddpf));
    ddpf.dwSize = sizeof(ddpf);
    ddpf.dwFlags = DDPF_RGB;
    if (!g_primary || g_primary->GetPixelFormat(&ddpf) != 0) {
        return 0;
    }
    format->bits = ddpf.dwRGBBitCount;
    format->red = ddpf.dwRBitMask;
    format->green = ddpf.dwGBitMask;
    format->blue = ddpf.dwBBitMask;
    return 1;
}

extern "C" int HOST_CALL host_frame_text(int which, void* font, int x, int y, const char* text, int length, unsigned int rgb)
{
    if (HostHeadless()) return HeadlessFrameText(which, font, x, y, text, length, rgb);
    IDirectDrawSurface* surface = Frame(which);
    HDC dc;

    if (!surface || surface->GetDC(&dc) != 0) {
        return 0;
    }
    SetBkMode(dc, TRANSPARENT);
    HGDIOBJ oldFont = SelectObject(dc, (HGDIOBJ)font);
    SetTextColor(dc, RGB((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff));
    TextOut(dc, x, y, text, length);
    SelectObject(dc, oldFont);
    surface->ReleaseDC(dc);
    return 1;
}

extern "C" int HOST_CALL host_frame_read(unsigned char* pixels, int pitch, unsigned char* palette)
{
    if (HostHeadless()) return HeadlessFrameRead(pixels, pitch, palette);
    host_frame frame;

    if (host_frame_lock(HOST_FRAME_BACK, &frame) != HOST_FRAME_OK) {
        return 0;
    }
    for (int row = 0; row < frame.height; row++) {
        memcpy(pixels + row * pitch, frame.pixels + row * frame.pitch, frame.width);
    }
    host_frame_unlock(HOST_FRAME_BACK);
    memset(palette, 0, 256 * 4);
    return host_palette_get(palette);
}

extern "C" int HOST_CALL host_palette_create(const unsigned char* entries)
{
    if (HostHeadless()) return HeadlessPaletteCreate(entries);
    if (WindowedPaletteActive()) {
        return WindowedPaletteCreate(entries);
    }
    if (!g_ddraw || !g_primary) {
        return 0;
    }
    if (g_ddraw->CreatePalette(4, (LPPALETTEENTRY)entries, &g_palette, 0) != 0) {
        return 0;
    }
    return g_primary->SetPalette(g_palette) == 0;
}

extern "C" int HOST_CALL host_palette_set(int start, int count, const unsigned char* entries)
{
    if (HostHeadless()) return HeadlessPaletteSet(start, count, entries);
    if (WindowedPaletteActive()) {
        unsigned char merged[256 * 4];
        if (start < 0 || count < 0 || start + count > 256) {
            return 0;
        }
        memcpy(merged, g_windowedEntries, sizeof(merged));
        memcpy(merged + start * 4, entries, count * 4);
        return WindowedPaletteCreate(merged);
    }
    return g_palette && g_palette->SetEntries(0, start, count, (LPPALETTEENTRY)entries) == 0;
}

extern "C" int HOST_CALL host_palette_get(unsigned char* entries)
{
    if (HostHeadless()) return HeadlessPaletteGet(entries);
    if (WindowedPaletteActive()) {
        return g_windowedPalette && GetPaletteEntries(g_windowedPalette, 0, 256, (LPPALETTEENTRY)entries) == 256;
    }
    return g_palette && g_palette->GetEntries(0, 0, 0x100, (LPPALETTEENTRY)entries) == 0;
}

extern "C" int HOST_CALL host_palette_system(unsigned char* entries)
{
    if (HostHeadless()) return 0;
    HWND hwnd = host_win32_window();
    HDC dc = GetDC(hwnd);
    if (dc == NULL) return 0;
    UINT count = GetSystemPaletteEntries(dc, 0, 256, (LPPALETTEENTRY)entries);
    ReleaseDC(hwnd, dc);
    return count == 256;
}

extern "C" int HOST_CALL host_palette_apply(void)
{
    if (HostHeadless()) return HeadlessPaletteApply();
    if (WindowedPaletteActive()) {
        HWND hwnd = host_win32_window();
        HDC dc;
        if (!g_windowedPalette) {
            return 0;
        }
        dc = GetDC(hwnd);
        if (!dc) {
            return 0;
        }
        SelectPalette(dc, g_windowedPalette, FALSE);
        RealizePalette(dc);
        ReleaseDC(hwnd, dc);
        return 1;
    }
    return g_primary && g_palette && g_primary->SetPalette(g_palette) == DD_OK;
}

extern "C" int HOST_CALL host_media_version_get(host_media_version* version)
{
    typedef int (__stdcall* SetupGetVersion)(DWORD* major, DWORD* minor);
    HMODULE lib = LoadLibraryA("dsetup.dll");
    if (lib) {
        FARPROC proc = GetProcAddress(lib, "DirectXSetupGetVersion");
        if (proc) {
            DWORD dwMaj = 0;
            DWORD dwMin = 0;
            DWORD status = ((SetupGetVersion)proc)(&dwMaj, &dwMin);
            if (status) {
                version->major_hi = HIWORD(dwMaj);
                version->minor_hi = HIWORD(dwMin);
                version->major_lo = LOWORD(dwMaj);
                version->minor_lo = LOWORD(dwMin);
                version->major_only = 0;
                FreeLibrary(lib);
                return 1;
            }
        }
        FreeLibrary(lib);
    }
    memset(version, 0, sizeof(*version));
    OSVERSIONINFOA osvi;
    bool isNT = false;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (GetVersionExA(&osvi)) {
        isNT = osvi.dwPlatformId == 2;
    }
    if (isNT) {
        DWORD installed = 0;
        unsigned int intSize = 4;
        if (host_setting_get(HOST_SCOPE_SYSTEM_READ, "Software\\Microsoft\\DirectX", "InstalledVersion",
                             HOST_SETTING_INT, &installed, &intSize) != 1) {
            return 0;
        }
        version->major_lo = installed & 0xff;
        version->major_only = 1;
        return 1;
    }
    char text[30];
    unsigned int len = sizeof(text);
    if (host_setting_get(HOST_SCOPE_SYSTEM_READ, "Software\\Microsoft\\DirectX", "Version",
                         HOST_SETTING_STRING, text, &len) != 1) {
        return 0;
    }
    version->major_hi = atoi(strtok(text, "."));
    version->major_lo = atoi(strtok(0, "."));
    version->minor_hi = atoi(strtok(0, "."));
    version->minor_lo = atoi(strtok(0, "."));
    return 1;
}

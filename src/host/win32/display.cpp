#include <windows.h>
#include <ddraw.h>
#include <string.h>

#include "../../../interfaces/host.h"

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
    HWND hwnd = host_win32_window();
    DDSURFACEDESC ddsd;
    HRESULT hr;

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

extern "C" int HOST_CALL host_display_is_open(void)
{
    return g_ddraw != 0;
}

extern "C" int HOST_CALL host_display_has_back_buffer(void)
{
    return g_back != 0;
}

extern "C" int HOST_CALL host_display_mode_list(host_display_mode* modes, int max, int* count)
{
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

extern "C" int HOST_CALL host_frame_lock(int which, host_frame* frame)
{
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
    IDirectDrawSurface* surface = Frame(which);
    if (!surface) {
        return 0;
    }
    surface->Unlock(0);
    return 1;
}

extern "C" int HOST_CALL host_frame_present(int flip)
{
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

extern "C" int HOST_CALL host_frame_is_lost(void)
{
    return g_primary && g_primary->IsLost() == kSurfaceLost;
}

extern "C" int HOST_CALL host_frame_restore(void)
{
    return g_primary && g_back && g_primary->Restore() == 0 && g_back->Restore() == 0;
}

extern "C" int HOST_CALL host_frame_fill(int which, unsigned int colour)
{
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

extern "C" void* HOST_CALL host_frame_context_get(void* window)
{
    return GetDC((HWND)window);
}

extern "C" void HOST_CALL host_frame_context_release(void* window, void* context)
{
    ReleaseDC((HWND)window, (HDC)context);
}

extern "C" void HOST_CALL host_frame_context_palette(void* context, void* palette)
{
    SelectPalette((HDC)context, (HPALETTE)palette, FALSE);
    RealizePalette((HDC)context);
}

extern "C" int HOST_CALL host_palette_create(const unsigned char* entries)
{
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
    return g_palette && g_palette->SetEntries(0, start, count, (LPPALETTEENTRY)entries) == 0;
}

extern "C" int HOST_CALL host_palette_get(unsigned char* entries)
{
    return g_palette && g_palette->GetEntries(0, 0, 0x100, (LPPALETTEENTRY)entries) == 0;
}

extern "C" int HOST_CALL host_palette_apply(void)
{
    return g_primary && g_palette && g_primary->SetPalette(g_palette) == DD_OK;
}

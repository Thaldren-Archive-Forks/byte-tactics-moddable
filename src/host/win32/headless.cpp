#include <windows.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "headless.h"

namespace {

bool ScanCommandLine()
{
    const wchar_t* cmd = GetCommandLineW();
    if (cmd == NULL) {
        return false;
    }
    const wchar_t* p = cmd;
    if (*p == L'"') {
        p++;
        while (*p != L'\0' && *p != L'"') {
            p++;
        }
        if (*p == L'"') {
            p++;
        }
    } else {
        while (*p != L'\0' && *p != L' ' && *p != L'\t') {
            p++;
        }
    }
    while (*p != L'\0') {
        if ((p[0] == L'-' || p[0] == L'/') && _wcsnicmp(p + 1, L"testmode", 8) == 0
            && (p[9] == L'\0' || p[9] == L' ' || p[9] == L'\t') && (p == cmd || p[-1] == L' ' || p[-1] == L'\t')) {
            return true;
        }
        p++;
    }
    return false;
}

LONG g_headlessState;

bool Scan()
{
    LONG state = *(volatile LONG*)&g_headlessState;
    if (state == 0) {
        bool headless = ScanCommandLine();
        InterlockedExchange(&g_headlessState, headless ? 2 : 1);
        return headless;
    }
    return state == 2;
}

const unsigned int kQueueSize = 256;
const int kScreenWidth = 640;
const int kScreenHeight = 480;

void* const kWindow = (void*)0x48454144;
void* const kSoundDevice = (void*)0x48535344;

struct PostRing {
    unsigned int items[kQueueSize];
    unsigned int head;
    unsigned int count;
};

struct EventRing {
    host_event items[kQueueSize];
    unsigned int head;
    unsigned int count;
};

bool g_windowOpen;
DWORD g_windowThread;
bool g_quitPending;
DWORD g_quitThread;
PostRing g_posted;
CRITICAL_SECTION g_postLock;

LONG g_postLockClaims;
LONG g_postLockReady;

void LockPosts()
{
    if (!*(volatile LONG*)&g_postLockReady) {
        if (InterlockedIncrement(&g_postLockClaims) == 1) {
            InitializeCriticalSection(&g_postLock);
            InterlockedIncrement(&g_postLockReady);
        } else {
            while (!*(volatile LONG*)&g_postLockReady) {
                Sleep(0);
            }
        }
    }
    EnterCriticalSection(&g_postLock);
}

void UnlockPosts()
{
    LeaveCriticalSection(&g_postLock);
}

EventRing g_events;
int g_cursorX = kScreenWidth / 2;
int g_cursorY = kScreenHeight / 2;

void PushEvent(unsigned int type, unsigned int a, unsigned int b, int x, int y, unsigned int buttons, unsigned int time)
{
    if (g_events.count == kQueueSize) {
        g_events.head = (g_events.head + 1) % kQueueSize;
        g_events.count--;
    }
    host_event& ev = g_events.items[(g_events.head + g_events.count) % kQueueSize];
    ev.type = type;
    ev.a = a;
    ev.b = b;
    ev.x = x;
    ev.y = y;
    ev.buttons = buttons;
    ev.time_ms = time;
    g_events.count++;
}

void DispatchPosted(unsigned int type)
{
    switch (type) {
    case HOST_EVENT_DESTROY:
    case HOST_EVENT_CLOSE:
    case HOST_EVENT_KEY_DOWN:
    case HOST_EVENT_KEY_UP:
    case HOST_EVENT_CHAR:
    case HOST_EVENT_SYS_KEY_DOWN:
    case HOST_EVENT_SYS_KEY_UP:
    case HOST_EVENT_DEVICE_CHANGE:
    case HOST_EVENT_PALETTE_QUERY:
    case HOST_EVENT_PALETTE_CHANGED:
    case HOST_EVENT_MEDIA_NOTIFY:
    case HOST_EVENT_FOCUS:
        PushEvent(type, 0, 0, 0, 0, 0, 0);
        break;
    case HOST_EVENT_MOUSE_MOVE:
    case HOST_EVENT_LBUTTON_DOWN:
    case HOST_EVENT_LBUTTON_UP:
    case HOST_EVENT_LBUTTON_DOUBLE:
    case HOST_EVENT_RBUTTON_DOWN:
    case HOST_EVENT_RBUTTON_UP:
    case HOST_EVENT_RBUTTON_DOUBLE:
    case HOST_EVENT_MBUTTON_DOWN:
    case HOST_EVENT_MBUTTON_UP:
    case HOST_EVENT_WHEEL:
        PushEvent(type, 0, 0, 0, 0, 0, host_time_ms());
        break;
    default:
        break;
    }
}

bool g_displayOpen;
bool g_hasPrimary;
bool g_hasBack;
bool g_hasPalette;
int g_width;
int g_height;
unsigned char* g_front;
unsigned char* g_back;
unsigned char* g_windowed;
int g_windowedWidth;
int g_windowedHeight;
bool g_windowedPalette;

void FreeWindowed()
{
    free(g_windowed);
    g_windowed = NULL;
    g_windowedPalette = false;
}

bool WindowedPaletteActive()
{
    return g_windowed != NULL && !g_displayOpen;
}

void FreeFrames()
{
    free(g_front);
    free(g_back);
    g_front = NULL;
    g_back = NULL;
    g_hasPrimary = false;
    g_hasBack = false;
    g_hasPalette = false;
}

unsigned char* Frame(int which)
{
    if (!g_displayOpen) {
        return NULL;
    }
    return which == HOST_FRAME_FRONT ? g_front : g_back;
}

struct SoundData {
    int refs;
    unsigned int bytes;
    unsigned char* bytesPtr;
};

struct SoundBuffer {
    SoundData* data;
    int rate;
    int bits;
    int channels;
    unsigned int flags;
    int volume;
    unsigned int position;
};

bool g_soundOpen;

SoundBuffer* MakeBuffer(unsigned int bytes, int rate, int bits, int channels, unsigned int flags)
{
    SoundBuffer* buf = (SoundBuffer*)calloc(1, sizeof(SoundBuffer));
    SoundData* data = (SoundData*)calloc(1, sizeof(SoundData));
    unsigned char* ptr = (unsigned char*)calloc(bytes != 0 ? bytes : 1, 1);
    if (buf == NULL || data == NULL || ptr == NULL) {
        free(buf);
        free(data);
        free(ptr);
        return NULL;
    }
    data->refs = 1;
    data->bytes = bytes;
    data->bytesPtr = ptr;
    buf->data = data;
    buf->rate = rate;
    buf->bits = bits;
    buf->channels = channels;
    buf->flags = flags;
    return buf;
}

}

bool HostHeadless()
{
    return Scan();
}

void* HeadlessWindowOpen(const char* title, int width, int height, int fullscreen)
{
    (void)title;
    (void)width;
    (void)height;
    (void)fullscreen;
    g_windowThread = GetCurrentThreadId();
    g_windowOpen = true;
    PushEvent(HOST_EVENT_FOCUS, 1, 0, 0, 0, 0, 0);
    return kWindow;
}

void HeadlessWindowClose()
{
    if (g_windowOpen) {
        g_windowOpen = false;
        PushEvent(HOST_EVENT_DESTROY, 0, 0, 0, 0, 0, 0);
    }
}

int HeadlessWindowHasFocus()
{
    return g_windowOpen ? 1 : 0;
}

void HeadlessWindowPost(unsigned int type)
{
    if (!g_windowOpen) {
        return;
    }
    LockPosts();
    if (g_posted.count == kQueueSize) {
        g_posted.head = (g_posted.head + 1) % kQueueSize;
        g_posted.count--;
    }
    g_posted.items[(g_posted.head + g_posted.count) % kQueueSize] = type;
    g_posted.count++;
    UnlockPosts();
}

int HeadlessPumpEvents(int wait)
{
    DWORD self = GetCurrentThreadId();
    bool onWindowThread = self == g_windowThread;
    bool quitHere = g_quitPending && g_quitThread == self;
    if (wait) {
        LockPosts();
        bool empty = g_posted.count == 0;
        UnlockPosts();
        if (empty && !g_quitPending) {
            host_sleep_ms(1);
        }
    }
    while (onWindowThread) {
        LockPosts();
        if (g_posted.count == 0) {
            UnlockPosts();
            break;
        }
        unsigned int type = g_posted.items[g_posted.head];
        g_posted.head = (g_posted.head + 1) % kQueueSize;
        g_posted.count--;
        UnlockPosts();
        if (type == WM_QUIT) {
            return 0;
        }
        DispatchPosted(type);
    }
    if (quitHere) {
        g_quitPending = false;
        return 0;
    }
    return 1;
}

void HeadlessPostQuit()
{
    g_quitThread = GetCurrentThreadId();
    g_quitPending = true;
}

int HeadlessEventNext(host_event* event)
{
    if (GetCurrentThreadId() != g_windowThread || g_events.count == 0) {
        return 0;
    }
    *event = g_events.items[g_events.head];
    g_events.head = (g_events.head + 1) % kQueueSize;
    g_events.count--;
    return 1;
}

int HeadlessKeyState(int vk)
{
    (void)vk;
    return 0;
}

void HeadlessCursorGet(int* x, int* y)
{
    *x = g_cursorX;
    *y = g_cursorY;
}

void HeadlessCursorSet(int x, int y)
{
    g_cursorX = x;
    g_cursorY = y;
}

int HeadlessDisplayModeSet(int width, int height, int flags, const unsigned char* palette)
{
    if (width <= 0 || height <= 0) {
        return HOST_DISPLAY_FAILED;
    }
    if (!(flags & HOST_DISPLAY_PRIMARY_ONLY)) {
        FreeWindowed();
    }
    FreeFrames();
    if (flags & HOST_DISPLAY_WINDOWED) {
        g_displayOpen = false;
        g_windowedWidth = width;
        g_windowedHeight = height;
        g_windowed = (unsigned char*)calloc((size_t)((width + 3) & ~3) * (size_t)height, 1);
        if (g_windowed == NULL) {
            return HOST_DISPLAY_FAILED;
        }
        g_windowedPalette = palette != NULL;
        return HOST_DISPLAY_OK;
    }
    g_width = width;
    g_height = height;
    g_displayOpen = true;
    g_front = (unsigned char*)calloc((size_t)width * (size_t)height, 1);
    if (g_front == NULL) {
        g_displayOpen = false;
        return HOST_DISPLAY_FAILED;
    }
    g_hasPrimary = true;
    if (flags & HOST_DISPLAY_PRIMARY_ONLY) {
        return HOST_DISPLAY_OK;
    }
    g_back = (unsigned char*)calloc((size_t)width * (size_t)height, 1);
    if (g_back == NULL) {
        FreeFrames();
        g_displayOpen = false;
        return HOST_DISPLAY_FAILED;
    }
    g_hasBack = true;
    g_hasPalette = palette != NULL;
    return HOST_DISPLAY_OK;
}

void HeadlessDisplayClose()
{
    if (g_displayOpen) {
        FreeFrames();
        g_displayOpen = false;
        if (g_windowed != NULL) {
            return;
        }
    }
    FreeWindowed();
}

int HeadlessDisplayIsOpen()
{
    return g_displayOpen ? 1 : 0;
}

int HeadlessDisplayHasBackBuffer()
{
    return g_hasBack ? 1 : 0;
}

int HeadlessDisplayModeList(host_display_mode* modes, int max, int* count)
{
    static const int sizes[][2] = {{640, 480}, {800, 600}, {1024, 768}};
    *count = 0;
    if (!g_displayOpen) {
        return 0;
    }
    for (int i = 0; i < 3 && *count < max; i++) {
        modes[*count].width = sizes[i][0];
        modes[*count].height = sizes[i][1];
        modes[*count].bits = 8;
        modes[*count].refresh = 0;
        (*count)++;
    }
    return 1;
}

void HeadlessDisplayScreenSize(int* width, int* height)
{
    *width = 1024;
    *height = 768;
}

int HeadlessFrameLock(int which, host_frame* frame)
{
    unsigned char* pixels = Frame(which);
    if (pixels == NULL && which == HOST_FRAME_BACK && g_windowed != NULL) {
        frame->pixels = g_windowed;
        frame->pitch = (g_windowedWidth + 3) & ~3;
        frame->width = g_windowedWidth;
        frame->height = g_windowedHeight;
        return HOST_FRAME_OK;
    }
    if (pixels == NULL) {
        return HOST_FRAME_FAILED;
    }
    memset(pixels, 0, (size_t)g_width * (size_t)g_height);
    frame->pixels = pixels;
    frame->pitch = g_width;
    frame->width = g_width;
    frame->height = g_height;
    return HOST_FRAME_OK;
}

int HeadlessFrameUnlock(int which)
{
    if (which == HOST_FRAME_BACK && Frame(which) == NULL && g_windowed != NULL) {
        return 1;
    }
    return Frame(which) != NULL ? 1 : 0;
}

int HeadlessFramePresent(int flip)
{
    (void)flip;
    return g_displayOpen || g_windowed != NULL ? HOST_FRAME_OK : HOST_FRAME_FAILED;
}

int HeadlessFramePresentRect(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    return g_windowed != NULL ? HOST_FRAME_OK : HOST_FRAME_FAILED;
}

int HeadlessFrameIsLost()
{
    return 0;
}

int HeadlessFrameRestore()
{
    return g_hasPrimary && g_hasBack ? 1 : 0;
}

int HeadlessFrameFill(int which, unsigned int colour)
{
    (void)colour;
    return Frame(which) != NULL ? 1 : 0;
}

int HeadlessFramePixelFormat(host_pixel_format* format)
{
    if (!g_hasPrimary) {
        return 0;
    }
    format->bits = 8;
    format->red = 0;
    format->green = 0;
    format->blue = 0;
    return 1;
}

int HeadlessFrameText(int which, void* font, int x, int y, const char* text, int length, unsigned int rgb)
{
    (void)font;
    (void)x;
    (void)y;
    (void)text;
    (void)length;
    (void)rgb;
    return Frame(which) != NULL ? 1 : 0;
}

int HeadlessFrameRead(unsigned char* pixels, int pitch, unsigned char* palette)
{
    if (Frame(HOST_FRAME_BACK) == NULL && g_windowed != NULL) {
        for (int row = 0; row < g_windowedHeight; row++) {
            memcpy(pixels + row * pitch, g_windowed + row * ((g_windowedWidth + 3) & ~3), (size_t)g_windowedWidth);
        }
        memset(palette, 0, 256 * 4);
        return g_windowedPalette ? 1 : 0;
    }
    if (Frame(HOST_FRAME_BACK) == NULL) {
        return 0;
    }
    for (int row = 0; row < g_height; row++) {
        memset(pixels + row * pitch, 0, (size_t)g_width);
    }
    memset(palette, 0, 256 * 4);
    return 1;
}

void* HeadlessFrameContextGet(void* window)
{
    (void)window;
    return NULL;
}

void HeadlessFrameContextRelease(void* window, void* context)
{
    (void)window;
    (void)context;
}

void HeadlessFrameContextPalette(void* context, void* palette)
{
    (void)context;
    (void)palette;
}

int HeadlessPaletteCreate(const unsigned char* entries)
{
    (void)entries;
    if (WindowedPaletteActive()) {
        g_windowedPalette = true;
        return 1;
    }
    if (!g_displayOpen || !g_hasPrimary) {
        return 0;
    }
    g_hasPalette = true;
    return 1;
}

int HeadlessPaletteSet(int start, int count, const unsigned char* entries)
{
    (void)start;
    (void)count;
    (void)entries;
    if (WindowedPaletteActive()) {
        return g_windowedPalette ? 1 : 0;
    }
    return g_hasPalette ? 1 : 0;
}

int HeadlessPaletteGet(unsigned char* entries)
{
    if (WindowedPaletteActive()) {
        if (!g_windowedPalette) {
            return 0;
        }
        memset(entries, 0, 256 * 4);
        return 1;
    }
    if (!g_hasPalette) {
        return 0;
    }
    memset(entries, 0, 256 * 4);
    return 1;
}

int HeadlessPaletteApply()
{
    if (WindowedPaletteActive()) {
        return g_windowedPalette ? 1 : 0;
    }
    return g_hasPrimary && g_hasPalette ? 1 : 0;
}

void* HeadlessSoundOpen(void* window, int rate, int bits, int channels, int* error)
{
    (void)window;
    (void)rate;
    (void)bits;
    (void)channels;
    (void)error;
    g_soundOpen = true;
    return kSoundDevice;
}

void HeadlessSoundClose()
{
    g_soundOpen = false;
}

void* HeadlessSoundNative()
{
    return NULL;
}

void* HeadlessSoundBufferCreate(unsigned int bytes, int rate, int bits, int channels, unsigned int flags)
{
    if (!g_soundOpen) {
        return NULL;
    }
    return MakeBuffer(bytes, rate, bits, channels, flags);
}

void* HeadlessSoundBufferDuplicate(void* buffer)
{
    SoundBuffer* src = (SoundBuffer*)buffer;
    if (!g_soundOpen || src == NULL) {
        return NULL;
    }
    SoundBuffer* copy = (SoundBuffer*)calloc(1, sizeof(SoundBuffer));
    if (copy == NULL) {
        return NULL;
    }
    *copy = *src;
    copy->position = 0;
    copy->data->refs++;
    return copy;
}

void HeadlessSoundBufferFree(void* buffer)
{
    SoundBuffer* buf = (SoundBuffer*)buffer;
    if (buf == NULL) {
        return;
    }
    if (--buf->data->refs == 0) {
        free(buf->data->bytesPtr);
        free(buf->data);
    }
    free(buf);
}

int HeadlessSoundBufferLock(void* buffer, unsigned int offset, unsigned int bytes, void** data, unsigned int* size)
{
    SoundBuffer* buf = (SoundBuffer*)buffer;
    if (buf == NULL || offset >= buf->data->bytes || bytes > buf->data->bytes) {
        return 0;
    }
    unsigned int got = buf->data->bytes - offset;
    if (bytes < got) {
        got = bytes;
    }
    *data = buf->data->bytesPtr + offset;
    *size = got;
    return 1;
}

int HeadlessSoundBufferUnlock(void* buffer, void* data, unsigned int size)
{
    (void)data;
    (void)size;
    return buffer != NULL ? 1 : 0;
}

int HeadlessSoundBufferPlay(void* buffer, int loop)
{
    (void)loop;
    return buffer != NULL ? 1 : 0;
}

int HeadlessSoundBufferStop(void* buffer)
{
    return buffer != NULL ? 1 : 0;
}

int HeadlessSoundBufferStatus(void* buffer)
{
    return buffer != NULL ? 0 : -1;
}

int HeadlessSoundBufferPosition(void* buffer, unsigned int* play, unsigned int* write)
{
    SoundBuffer* buf = (SoundBuffer*)buffer;
    if (buf == NULL) {
        return 0;
    }
    *play = buf->position;
    *write = buf->position;
    return 1;
}

int HeadlessSoundBufferSetPosition(void* buffer, unsigned int offset)
{
    SoundBuffer* buf = (SoundBuffer*)buffer;
    if (buf == NULL || offset > buf->data->bytes) {
        return 0;
    }
    buf->position = offset;
    return 1;
}

int HeadlessSoundBufferSetVolume(void* buffer, int volume)
{
    SoundBuffer* buf = (SoundBuffer*)buffer;
    if (buf == NULL || volume > 0 || volume < -10000) {
        return 0;
    }
    buf->volume = volume;
    return 1;
}

int HeadlessSoundBufferPlace(void* buffer, const float* xyz, float min_distance, float max_distance)
{
    SoundBuffer* buf = (SoundBuffer*)buffer;
    (void)xyz;
    (void)min_distance;
    (void)max_distance;
    return buf != NULL && (buf->flags & HOST_SOUND_3D) ? 1 : 0;
}

int HeadlessSoundWavPlay(const void* wav, unsigned int flags)
{
    (void)wav;
    (void)flags;
    return 1;
}

void HeadlessSoundWavStop()
{
}

int HeadlessMixerWaveCount()
{
    return 0;
}

int HeadlessMixerCdDevice()
{
    return -1;
}

int HeadlessMixerGet(int line, int device, unsigned int* volume)
{
    (void)line;
    (void)device;
    (void)volume;
    return 0;
}

int HeadlessMixerSet(int line, int device, unsigned int volume)
{
    (void)line;
    (void)device;
    (void)volume;
    return 0;
}

int HeadlessMusicOpen()
{
    return 1;
}

void HeadlessMusicClose()
{
}

int HeadlessMusicStop()
{
    return 1;
}

int HeadlessMusicPause()
{
    return 1;
}

int HeadlessMusicPlaying()
{
    return 0;
}

int HeadlessMusicTrackCount()
{
    return 0;
}

int HeadlessMusicTrackIsAudio(int track)
{
    (void)track;
    return -1;
}

int HeadlessMusicTrackLength(int track)
{
    (void)track;
    return -1;
}

int HeadlessMusicTrackStart(int track)
{
    (void)track;
    return -1;
}

int HeadlessMusicCurrentTrack()
{
    return -1;
}

int HeadlessMusicPosition()
{
    return 0;
}

int HeadlessMusicPlayTracks(int first, int last)
{
    (void)first;
    (void)last;
    return 1;
}

int HeadlessMusicPlayRange(int from_ms, int to_ms)
{
    (void)from_ms;
    (void)to_ms;
    return 1;
}

unsigned int HeadlessMusicDiscSerial()
{
    return 0;
}

void* HeadlessMusicFindCdPlayer()
{
    return NULL;
}

void HeadlessMusicCloseCdPlayer(void* window)
{
    (void)window;
}

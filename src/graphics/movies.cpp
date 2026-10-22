// Decompiled by space-bunny-free, deepseek-v4.1-flash, Opus, Sonnet, GPT-6-Luna, Space Bunny Free and Haiku. Names are provisional.
// The Smacker movie player.
#include <stdio.h>
#include <windows.h>
#include "../../interfaces/host.h"
#include "../util/file_printf.h"

int __stdcall HandleHostEvent(host_event* ev);

// The open Smacker movie (smackw32.dll's SMK struct).
struct Smk_0047c3a0 {
    unsigned int version;               // +0x0
    unsigned int width;                 // +0x4
    unsigned int height;                // +0x8
    unsigned int frames;                // +0xc
    char unknown_10[0x68 - 0x10];
    unsigned int newPalette;            // +0x68, the palette changed
    unsigned char rgb[256][3];          // +0x6c
    char unknown_36c[0x370 - 0x36c];
    unsigned short palType;             // +0x370
    char unknown_372[0x374 - 0x372];
    int frameNum;                       // +0x374
    char unknown_378[0x380 - 0x378];
    int lastLeft;                       // +0x380
    int lastTop;                        // +0x384
    int lastWidth;                      // +0x388
    int lastHeight;                     // +0x38c
};

// smackw32.dll, imported by ordinal, so the names are ours: 14 SmackOpen,
// 15 SmackSoundEnable, 17 SmackSoundOnOff, 19 SmackDoFrame, 21
// SmackNextFrame, 23 SmackToBuffer, 27 SmackGoto, 28 SmackToBufferRect,
// 38 SmackSoundUseDirectSound.
extern "C" __declspec(dllimport) int __stdcall SmackSoundEnable(HWND hwnd);
extern "C" __declspec(dllimport) int __stdcall SmackSoundUseDirectSound(void* sound);
extern "C" __declspec(dllimport) Smk_0047c3a0* __stdcall SmackOpen(char* path, unsigned int flags, int extra);
extern "C" __declspec(dllimport) unsigned int __stdcall SmackSoundOnOff(Smk_0047c3a0* smk, unsigned int on);
extern "C" __declspec(dllimport) void __stdcall SmackGoto(Smk_0047c3a0* smk, unsigned int frame);
extern "C" __declspec(dllimport) void __stdcall SmackDoFrame(Smk_0047c3a0* smk);
extern "C" __declspec(dllimport) void __stdcall SmackNextFrame(Smk_0047c3a0* smk);
extern "C" __declspec(dllimport) void __stdcall SmackToBuffer(Smk_0047c3a0* smk, unsigned int left, unsigned int top, unsigned int width, unsigned int height, unsigned int bufferHeight, void* buffer);
extern "C" __declspec(dllimport) int __stdcall SmackToBufferRect(Smk_0047c3a0* smk, int flag);
// Ordinal 32: SmackWait, the DLL's export name.
extern "C" __declspec(dllimport) unsigned int __stdcall SmackWait(Smk_0047c3a0* smack);

// smackw32.dll ordinal 18 (SmackClose), called through its import slot.
extern "C" __declspec(dllimport) void __stdcall SmackClose(void* smack);

// The 0x54 byte statistics block SmackSummary fills in. Six of the twenty-one
// fields are never printed, and the seven printed before TotalBlitTime are
// read from further in than the DLL writes.
struct SmkStats_0047c530 {
    unsigned int totalTime;            // +0x00 divisor of the frame rate
    unsigned int unknown_04;           // +0x04
    unsigned int openTime;             // +0x08 "Time to Open File"
    unsigned int frames;               // +0x0c "Total Frames Played"
    unsigned int framesSkipped;        // +0x10 "SkippedFrames"
    unsigned int unknown_14;           // +0x14
    unsigned int timeBlit;             // +0x18 "TotalBlitTime"
    unsigned int readTime;             // +0x1c "TotalReadTime"
    unsigned int decompTime;           // +0x20 "TotalDecompTime"
    unsigned int unknown_24;           // +0x24
    unsigned int readSpeed;            // +0x28 "TotalReadSpeed"
    unsigned int slowestFrameTime;     // +0x2c "SlowestFrameTime"
    unsigned int slowest2FrameTime;    // +0x30 "Slowest2FrameTime"
    unsigned int unknown_34;           // +0x34
    unsigned int unknown_38;           // +0x38
    unsigned int averageFrameSize;     // +0x3c "AverageFrameSize"
    unsigned int highest1SecRate;      // +0x40 "Highest1SecRate"
    unsigned int unknown_44;           // +0x44
    unsigned int highestMemAmount;     // +0x48 "HighestMemAmount"
    unsigned int totalExtraMemory;     // +0x4c "TotalExtraMemory"
    unsigned int highestExtraUsed;     // +0x50 "HighestExtraUsed"
};

// smackw32.dll ordinal 20, called through its import slot.
extern "C" __declspec(dllimport) void __stdcall SmackSummary(Smk_0047c3a0* smk, SmkStats_0047c530* stats);

struct Surfaces_0047bdf0 {
    void* ddraw;
    void* primary;
    void* back;
    void* clipper;
    void* palette;
};

struct Display_0047bdf0 {
    char unknown_0[0x24];
    void* sound;                     // +0x24
    char unknown_28[0x40 - 0x28];
    HWND hwnd;                       // +0x40
    char unknown_44[0x84 - 0x44];
    Surfaces_0047bdf0 surfaces;      // +0x84
    char unknown_98[0xf0 - 0x98];
    unsigned short flags;            // +0xf0
};

// The offscreen screen the frames are drawn to.
struct Display_0047c3a0 {
    char unknown_0[8];
    unsigned int width;                 // +0x8
    unsigned int height;                // +0xc
    void* palette;
};

#pragma pack(push, 1)
#include "../game/game.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

extern Game* g_game;                    // 0x511de8

Display_0047bdf0* GetDisplay();
void __stdcall FatalError(char* message);
void __stdcall SetOffscreenSurface(Display_0047c3a0* display);
void FlipScreen();

// The object smackw32.dll ordinal 2 hands back (a SmackBuf); only the colour
// remap arguments SmackColorRemap takes from it are named.
struct Smk_0047bf70_b {
    char unknown_0[0x2c];
    DWORD paletteColors;               // +0x2c
    char unknown_30[0x3c - 0x30];
    DWORD palette;                     // +0x3c
    char unknown_40[0x43c - 0x40];
    DWORD paletteType;                 // +0x43c
};

// smackw32.dll's buffer API, imported by ordinal: ordinal 2 opens the decoder
// state (HWND, HDC, 640, 480, 0, 0), ordinal 5 takes the movie's 256 colour
// palette, and ordinal 25 is given the movie and three fields of that state.
extern "C" __declspec(dllimport) Smk_0047bf70_b *__stdcall SmackBufferOpen(HWND hwnd, HDC hdc, int width, int height, int flags, int background);
extern "C" __declspec(dllimport) void __stdcall SmackBufferNewPalette(Smk_0047bf70_b *smk, unsigned char *rgb, unsigned short flag);
extern "C" __declspec(dllimport) void __stdcall SmackColorRemap(Smk_0047c3a0 *smk, DWORD *palette, DWORD paletteColors, DWORD paletteType);

struct SmackerSurface;

class MoviePlayer {
public:
    Smk_0047c3a0* smack;               // +0x0
    unsigned int frame;                // +0x4, the last full frame seen
    int stopped;                       // +0x8, the movie is done or stopped
    HWND hwnd;                         // +0xc
    PALETTEENTRY entries[256];         // +0x10
    int paletteResult;                 // +0x410
    int hasSurfaces;                   // +0x414
    char unknown_418[0x5b4 - 0x418];

    MoviePlayer(char* path, int a, int b, int c, int d, int e);
    void ReadSystemPalette(int unused);
    void UpdatePalette(void);
    void ClearScreen();
    void OnPaint(HWND hwnd);
    void PlayFrame(HWND hwnd);
    void WriteSmackStats();
    void Play();
    void Close();
    int SetupDirectDraw();
    int GetBlitMode(SmackerSurface* unused);
};



// The constructor: opens the .smk file, hands the Smack library the game's
// DirectSound object, then paints a black frame into the surface the player
// blits through. That surface is the display's own DirectDraw surface when the
// game has one, otherwise a private one made by SetupDirectDraw.
// FUNCTION: 0x47bdf0
MoviePlayer::MoviePlayer(char* path, int a, int b, int c, int d, int e)
{
    stopped = 0;
    frame = 0;
    if (a)
        SmackSoundEnable((HWND)b);
    void* sound = host_sound_native();
    SmackSoundUseDirectSound(sound);
    smack = SmackOpen(path, 0xfe100, -1);
    if (!smack)
        FatalError("Could not open movie file, please check filename in INI.");
    SmackSoundOnOff(smack, sound != 0);

    Display_0047bdf0* disp = GetDisplay();
    hwnd = disp->hwnd;
    if ((disp->flags & 2) && host_display_is_open()) {
        hasSurfaces = 0;
    } else if (!(disp->flags & 2) && !host_display_is_open()) {
        if (!SetupDirectDraw())
            FatalError("Could not setup Direct Draw to play movie.");
        hasSurfaces = 1;
    }

    host_frame_fill(HOST_FRAME_FRONT, 0);
}

// FUNCTION: 0x47bf20
void MoviePlayer::Close()
{
    SmackClose(smack);
    if (hasSurfaces)
        host_display_close();
}

// Sets the movie player up: asks DirectDraw for full screen exclusive on the
// player window, builds the primary surface and a palette out of the system
// palette, sizes the window to the movie, and when the movie has a new palette
// hands it to the Smacker decoder. Returns 1 unless DirectDraw refuses.
// FUNCTION: 0x47bf70
int MoviePlayer::SetupDirectDraw()
{
    POINT point;
    int i;

    point.x = 0;
    point.y = 0;
    host_window_client_origin((int*)&point.x, (int*)&point.y);
    int shown = host_display_mode_set(0, 0, HOST_DISPLAY_PRIMARY_ONLY, 0);
    if (shown == HOST_DISPLAY_FAILED)
        goto fail;
    if (shown == HOST_DISPLAY_OK) {
        paletteResult = GetBlitMode(0);
        if (paletteResult == 0) {
            host_palette_system((unsigned char*)entries);
            for (i = 0; i < 10; ++i) entries[i].peFlags = 0;
            for (i = 10; i < 246; ++i) entries[i].peFlags = 4;
            for (i = 246; i < 256; ++i) entries[i].peFlags = 0;
            host_palette_create((unsigned char*)entries);
        }
        host_window_resize(smack->width, smack->height);
        host_window_raise();
    }
    if (smack->newPalette) {
        Smk_0047bf70_b *smk = SmackBufferOpen(hwnd, 0, 0x280, 0x1e0, 0, 0);
        SmackBufferNewPalette(smk, &smack->rgb[0][0], smack->palType);
        SmackColorRemap(smack, &smk->palette, smk->paletteColors, smk->paletteType);
    }
    return 1;
fail:
    return 0;
}

// Asks the movie's output surface for its pixel format and returns the blit
// mode matching it, or 0 when the format is not one of the four 16-bit RGB
// layouts Smacker can blit to.
// FUNCTION: 0x47c150
int MoviePlayer::GetBlitMode(SmackerSurface* unused)
{
    host_pixel_format format;
    host_frame_pixel_format(&format);
    if (format.bits == 8)
        return 0;
    if (format.red == 0xf800 && format.green == 0x7e0 && format.blue == 0x1f)
        return 0xc0000000;
    if (format.red == 0x7c00 && format.green == 0x3e0 && format.blue == 0x1f)
        return 0x80000000;
    if (format.red == 0xf800 && format.green == 0x7c0 && format.blue == 0x3f)
        return 0xa0000000;
    if (format.red == 0xfc00 && format.green == 0x3e0 && format.blue == 0x1f)
        return 0xe0000000;
    host_message(NULL, "Smacker Error", "Unsupported pixel format.", HOST_MESSAGE_PLAIN);
    return 0;
}

// Reads the system palette into entries[], marking the 20 static colours
// (10 at each end) as plain and the 236 in between as PC_NOCOLLAPSE.
// FUNCTION: 0x47c230
void MoviePlayer::ReadSystemPalette(int unused)
{
    host_palette_system((unsigned char*)entries);
    int i;
    for (i = 0; i < 10; i++)
        entries[i].peFlags = 0;
    for (i = 10; i < 246; i++)
        entries[i].peFlags = PC_NOCOLLAPSE;
    for (i = 246; i < 256; i++)
        entries[i].peFlags = 0;
}

// Copies the movie's palette into entries[] and the display palette.
// FUNCTION: 0x47c2a0
void MoviePlayer::UpdatePalette(void)
{
    unsigned char* src = smack->rgb[0];
    for (int i = 0; i < 256; i++) {
        entries[i].peRed = *src++;
        entries[i].peGreen = *src++;
        entries[i].peBlue = *src++;
    }
    host_palette_set(0, 256, (unsigned char*)entries);
}

// FUNCTION: 0x47c2f0
void MoviePlayer::ClearScreen()
{
    host_frame_fill(HOST_FRAME_FRONT, 0);
}

// The WM_PAINT handler: clears the video area to black, then, when a full
// frame has been seen, seeks back to it with the sound muted so the picture
// is redrawn.
// FUNCTION: 0x47c330
void MoviePlayer::OnPaint(HWND hwnd)
{
    host_window_paint_black(smack->width, smack->height);
    if (frame) {
        SmackSoundOnOff(smack, 0);
        SmackGoto(smack, frame);
        SmackSoundOnOff(smack, 1);
    }
}

// FUNCTION: 0x47c3a0
void MoviePlayer::PlayFrame(HWND hwnd)
{
    if (!host_window_has_focus())
        return;
    if (stopped)
        return;
    if (smack->newPalette) {
        unsigned char* src = smack->rgb[0];
        for (int i = 0; i < 256; i++) {
            entries[i].peRed = *src++;
            entries[i].peGreen = *src++;
            entries[i].peBlue = *src++;
        }
        host_palette_set(0, 256, (unsigned char*)entries);
    }
    SetOffscreenSurface((Display_0047c3a0 *)g_game->screen);
    SmackToBuffer(smack, 0, (0x1e0 - smack->height) >> 1, ((Display_0047c3a0 *)g_game->screen)->width, smack->height, ((Display_0047c3a0 *)g_game->screen)->height, 0);
    SmackDoFrame(smack);
    FlipScreen();
    if (SmackToBufferRect(smack, 1) && smack->lastLeft == 0 && smack->lastTop == 0
        && smack->lastWidth == smack->width && smack->lastHeight == smack->height) {
        frame = smack->frameNum + 1;
    }
    if (smack->frameNum == smack->frames - 1) {
        stopped = 1;
        return;
    }
    SmackNextFrame(smack);
}

// Dumps the playback statistics smackw32.dll collected for the open movie into
// stats.txt, one per line. The frame rate and the playback time are worked out
// in integer arithmetic from the millisecond totals the DLL recorded, and the
// playback time comes out as 1000 for every movie, the original's arithmetic.
// FUNCTION: 0x47c530
void MoviePlayer::WriteSmackStats()
{
    SmkStats_0047c530 stats;
    SmackSummary(smack, &stats);
    void* file = host_file_create("stats.txt", HOST_FILE_TEXT);
    if (file) {
        FilePrintf(file, "Frames Per Sec\t%d\n", 1000 * stats.frames / stats.totalTime);
        FilePrintf(file, "Total Playback Time\t%d\n", 1000 * stats.totalTime / stats.totalTime);
        FilePrintf(file, "Time to Open File\t%d\n", stats.openTime);
        FilePrintf(file, "Total Frames Played\t%d\n", stats.frames);
        FilePrintf(file, "SkippedFrames\t%d\n", stats.framesSkipped);
        FilePrintf(file, "TotalBlitTime\t%d\n", stats.timeBlit);
        FilePrintf(file, "TotalReadTime\t%d\n", stats.readTime);
        FilePrintf(file, "TotalDecompTime\t%d\n", stats.decompTime);
        FilePrintf(file, "TotalReadSpeed\t%d bytes/sec\n", stats.readSpeed);
        FilePrintf(file, "SlowestFrameTime\t%d\n", stats.slowestFrameTime);
        FilePrintf(file, "Slowest2FrameTime\t%d\n", stats.slowest2FrameTime);
        FilePrintf(file, "AverageFrameSize\t%d\n", stats.averageFrameSize);
        FilePrintf(file, "Highest1SecRate\t%d\n", stats.highest1SecRate);
        FilePrintf(file, "HighestMemAmount\t%d\n", stats.highestMemAmount);
        FilePrintf(file, "TotalExtraMemory\t%d\n", stats.totalExtraMemory);
        FilePrintf(file, "HighestExtraUsed\t%d\n", stats.highestExtraUsed);
        host_file_close(file);
    }
}

// Pumps Windows messages while the Smacker movie plays: every message is
// translated and dispatched, and two message codes end the movie, one of them
// with a mouse move that also quits the game. When the queue is empty the
// player is asked for the next frame, and if there is none yet the frame on
// screen is redrawn (PlayFrame).
// FUNCTION: 0x47c6c0
void MoviePlayer::Play()
{
    host_event ev;
    while (!stopped) {
        if (!host_pump_events(0))
            host_post_quit();
        if (host_event_next(&ev)) {
            do {
                if (ev.type == HOST_EVENT_SYS_KEY_DOWN && ev.a == 0x73) {
                    stopped = 1;
                    g_game->introMovieLoop = 0;
                    host_post_quit();
                    return;
                }
                if (ev.type == HOST_EVENT_CHAR) {
                    stopped = 1;
                    g_game->introMovieLoop = 0;
                }
                HandleHostEvent(&ev);
            } while (host_event_next(&ev));
        } else if (!SmackWait(smack)) {
            PlayFrame(hwnd);
        }
    }
}

// FUNCTION: 0x47c770
int GetBuildSiteMetal(void)
{
    return g_lastPlaceMetalSum;
}

// FUNCTION: 0x47c780
int GetBuildSiteHeight(void)
{
    return g_lastPlaceHeight;
}

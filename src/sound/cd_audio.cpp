// Decompiled by Haiku, deepseek-v4.1-flash, space-bunny-free, Opus, Sonnet, GPT-6.1-sol, mimo-v2.6-pro, Claude Opus 5.5, LongCat 2.5 Preview Free, GPT-6 and DeepSeek V4.1 Flash. Names are provisional.
// The CD audio player: the methods of the sound object at g_game+0x10 that
// talk to the MCI cdaudio device, in address order. The object's sample and
// streaming methods are the samples module (data/modules.csv) and live in
// samples.cpp; the Sound class and the sample-side declarations here are the
// view the gathered files shared.

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "../../interfaces/host.h"
// <stdio.h> must stay: the header state decides StartStream's multiply register.
#include <stdio.h>
#include <float.h>
// Nothing here uses <io.h>: its symbols put InitDirectSound,
// CreateSampleFromMemory and StartStream in the symbol-id windows they match
// in (docs/c2-regalloc.md).
#include <io.h>


extern HWND g_cdPlayerWindow;


extern int g_cdFadeVolume;
extern int g_cdCategorySavedTrack[];
extern int g_cdFadeTimer;
extern int g_cdNextTrackTimer;

extern int __stdcall RemoveTimer(int handle);
extern int __stdcall AddTimer(int delay, int id, void (__stdcall* callback)(void*));
extern void __stdcall OnCdFadeTimer(void* unused);

struct FileHandle;

void* __cdecl GameAllocIgnoreTag(char* name, unsigned int size);

void __cdecl GameFreeThunk(void* p);

extern int g_playBufferLooping;

struct Pos_004cf570 {
    int x, y, z;
};

int __stdcall HAPI_CloseFile(FileHandle* file);
long __stdcall HAPI_TellFile(FileHandle* file);
long __stdcall HAPI_FileLength(FileHandle* file);
int __stdcall HAPI_readfromfile(FileHandle* file, void* buf, int size);

void __stdcall OnStreamTimer(int unused1);
int __stdcall AddTimer(int delay, int param, void (__stdcall* callback)(int));

extern int g_delayedSampleVolume;
extern char g_delayedSampleName[];

int __stdcall HAPI_SeekFile(void* file, int pos);
int __stdcall HAPI_readfromfile(void* file, void* buf, int size);

// The sound system: the CD audio player, the DirectSound sample channels and
// the streamed sample, at g_game+0x10.
#include "sound.h"

// The sound object g_cdPlayer points at: the same object as g_game+0x10.
extern Sound* g_cdPlayer;

// OpenCdAudio registers these; 0x4ce1e0 defines FindCdPlayerWindow.
extern void __stdcall SetMediaNotifyCallback(void (__stdcall*)(int, int, int));
extern BOOL __stdcall FindCdPlayerWindow(HWND, LPARAM);

// FUNCTION: 0x4cd9b0
void __stdcall NopRet4(int)
{
}

// FUNCTION: 0x4cd9c0
int Sound::GetDiscSerial()
{
    return discSerial;
}

// FUNCTION: 0x4cd9d0
int Sound::SetCdCallback(void (*cb)())
{
    callback = cb;
    QueryDisc();
    if (callback != 0)
        callback();
    return 1;
}

// FUNCTION: 0x4cda00
int Sound::QueryDisc()
{
    currentTrack = 0;
    playState = 0;
    int n = host_music_track_count();
    if (n >= 0) {
        trackCount = n;
        discSerial = host_music_disc_serial();
        if (host_music_track_is_audio(1) == 1) {
            dataTrack = 0;
        } else {
            dataTrack = 1;
            if (--trackCount < 0)
                trackCount = 0;
        }
        if (trackCount != 0)
            currentTrack = 1;
        return trackCount;
    }
    return 0;
}

int HarnessViewRand();

// FUNCTION: 0x4cdb40
void Sound::PlayNextTrack()
{
    int playing;
    int r;
    int count;
    int i;
    int j;
    int zero = 0;

    if (trackCount == zero)
        return;
    if (trackCategory == 4) {
        host_music_stop();
        if (trackCount != zero)
            currentTrack = 1;
        else
            currentTrack = zero;
        playState = zero;
        step = zero;
        RemoveTimer(g_cdNextTrackTimer);
        RemoveTimer(g_cdFadeTimer);
        // Chained store, as in the original: g_cdNextTrackTimer is written first.
        g_cdNextTrackTimer = g_cdFadeTimer = -1;
        return;
    }
    if (playState == 2)
        return;
    if (trackCategory != 2 && trackCategory != 3) {
        int one = 1;
        switch (playbackOrder) {
        case 0:
            {
                int none = 0;
                if (playState == zero)
                    return;
                playState = zero;
                playing = host_music_playing() == 1;
                if (playing == none)
                    return;
                host_music_stop();
                if (trackCount != none)
                    currentTrack = 1;
                else
                    currentTrack = none;
                playState = none;
                step = none;
                RemoveTimer(g_cdNextTrackTimer);
                RemoveTimer(g_cdFadeTimer);
                g_cdNextTrackTimer = g_cdFadeTimer = -1;
                return;
            }
        case 1:
            {
            playing = host_music_playing() == 1;
            if (playing != zero)
                goto done;
            if (currentTrack < one)
                currentTrack = one;
            else
                currentTrack++;
            this->PlayCdTrack(currentTrack, trackCount - currentTrack + 1);
            if (currentTrack > trackCount)
                currentTrack = one;
            goto done;
            }
        case 2:
            {
            playing = host_music_playing() == 1;
            if (playing != zero)
                goto done;
            this->PlayCdTrack(HarnessViewRand() % trackCount + 1, one);
            goto done;
            }
        case 3:
            {
            playing = host_music_playing() == 1;
            if (playing == zero || currentTrack != lockedTrack) {
                if (lockedTrack == zero)
                    lockedTrack = one;
                this->PlayCdTrack(lockedTrack, one);
            }
            goto done;
            }
        case 4:
            break;
        default:
            goto done;
        }
    }
    r = HarnessViewRand() & 0xf;
    playing = host_music_playing() == 1;
            if (playing != 0 && arr_214[currentTrack] == trackCategory)
        goto done;
    count = (r + 1) * trackCount;
    i = currentTrack;
    while (count > 0) {
            i++;
            if (i > trackCount)
                i = 1;
            if (arr_214[i] == trackCategory) {
                if (--r <= 0) {
                    j = i;
                    while (j <= trackCount && arr_214[j] == trackCategory)
                        j++;
                    this->PlayCdTrack(i, j - i);
                    break;
                }
            }
            count--;
    }
    if (count > 0)
        goto done;
stop:
    host_music_stop();
    playState = 0;
    currentTrack = (trackCount != 0);
    step = 0;
    RemoveTimer(g_cdNextTrackTimer);
    RemoveTimer(g_cdFadeTimer);
    g_cdNextTrackTimer = g_cdFadeTimer = -1;
done:
    SetAuxVolume(cdVolume, 1);
    playState = 1;
    return;
}

// FUNCTION: 0x4ce020
int Sound::GetPlayState()
{
    return playState;
}

// FUNCTION: 0x4ce030
void __cdecl HandleCdMessage(int msg, int wParam, int lParam)
{
    switch (msg) {
    case 0x219: {
        Sound* obj = g_cdPlayer;
        host_music_stop();
        if (obj->trackCount)
            obj->currentTrack = 1;
        else
            obj->currentTrack = 0;
        obj->playState = 0;
        obj->step = 0;
        RemoveTimer(g_cdNextTrackTimer);
        RemoveTimer(g_cdFadeTimer);
        g_cdNextTrackTimer = g_cdFadeTimer = -1;
        if (wParam == 0x8000) {
            g_cdPlayer->QueryDisc();
            if (g_cdPlayer->callback)
                g_cdPlayer->callback();
        }
        break;
    }
    case 0x3b9:
        if (wParam == 1 && g_cdPlayer->playState == 1) {
            int playing = host_music_playing() == 1;
            if (!playing)
                g_cdPlayer->PlayNextTrack();
        }
        break;
    }
}

// FUNCTION: 0x4ce190
void Sound::CloseCdPlayerWindow()
{
    if (g_cdPlayerWindow) {
        SendMessageA(g_cdPlayerWindow, WM_CLOSE, 0, 0);
        SendMessageA(g_cdPlayerWindow, WM_QUIT, 0, 0);
        host_sleep_ms(500);
        g_cdPlayerWindow = 0;
    }
}

// A method of the object at g_game+0x10 (its one caller, 0x4263b0, loads ecx
// from there) that never uses `this`, like 0x4ce190.
// FUNCTION: 0x4ce1d0
bool Sound::HasCdPlayerWindow()
{
    return g_cdPlayerWindow != 0;
}

// EnumWindows callback: remembers the window whose class is the CD player's.
// FUNCTION: 0x4ce1e0
BOOL __stdcall FindCdPlayerWindow(HWND hwnd, LPARAM param)
{
    char className[200];
    GetClassNameA(hwnd, className, sizeof(className) - 1);
    if (strcmp(className, "SJE_CdPlayerClass") == 0)
        g_cdPlayerWindow = hwnd;
    return TRUE;
}

// Initialises the CD player object and opens the MCI cdaudio device.
// The store to arr_214[0] before the loop is overwritten by the loop's first
// iteration (0 % 4 + 1 == 1), so it is redundant in the original.
// FUNCTION: 0x4ce260
int Sound::OpenCdAudio()
{
    if (open != 0)
        return 1;
    g_cdPlayerWindow = 0;
    g_cdPlayer = this;
    arr_214[0] = 1;
    for (int i = 0; i < 100; i++)
        arr_214[i] = (i % 4) + 1;
    lockedTrack = 1;
    currentTrack = 0;
    discSerial = 0;
    callback = 0;
    playbackOrder = 1;
    trackCategory = 0;
    open = 0;
    trackCount = 0;
    dataTrack = 0;
    if (!host_music_open()) {
        EnumWindows((WNDENUMPROC)FindCdPlayerWindow, 0);
        if (!host_music_open())
            return 0;
    }
    playState = 0;
    currentTrack = (trackCount != 0);
    step = 0;
    RemoveTimer(g_cdNextTrackTimer);
    RemoveTimer(g_cdFadeTimer);
    g_cdFadeTimer = -1;
    g_cdNextTrackTimer = -1;
    discSerial = 0;
    trackCount = QueryDisc();
    SetMediaNotifyCallback((void (__stdcall*)(int, int, int))HandleCdMessage);
    open = 1;
    lockedTrack = 1;
    currentTrack = 0;
    callback = 0;
    playbackOrder = 1;
    trackCategory = 0;
    cdEnabled = 1;
    return 1;
}

// FUNCTION: 0x4ce3e0
void Sound::CopyTrackTypeTable(const void* src)
{
    memcpy(arr_214 + 1, src, trackCount);
}

// ReleaseDirectSound (0x4ceee0) calls this out of line; in one file /Ob2
// would inline it into its caller.
#pragma auto_inline(off)
// FUNCTION: 0x4ce410
void Sound::CloseCdAudio()
{
    if (open != 0) {
        host_music_close();
        open = 0;
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x4ce450
int Sound::GetTrackCount()
{
    return trackCount;
}

// FUNCTION: 0x4ce460
int Sound::IsFirstTrackData()
{
    return host_music_track_is_audio(1) != 1;
}

// FUNCTION: 0x4ce530
int __stdcall GetTrackLength(int track)
{
    int length = host_music_track_length(track);
    if (length < 0)
        return 0;
    return length;
}

// FUNCTION: 0x4ce580
void Sound::SetLockedTrack(int v)
{
    if (v <= trackCount) {
        lockedTrack = v;
    }
}

// FUNCTION: 0x4ce5a0
int Sound::GetLockedTrack()
{
    return this->lockedTrack;
}

// FUNCTION: 0x4ce5b0
void __stdcall OnNextTrackTimer(void*)
{
    int temp = g_cdNextTrackTimer;
    RemoveTimer(temp);
    g_cdNextTrackTimer = 0xffffffff;
    g_cdPlayer->PlayNextTrack();
}

// Timer callback: steps the level by the object's step; once it reaches zero
// the timer is killed and either a new one is started or PlayNextTrack runs.
// FUNCTION: 0x4ce5e0
void __stdcall OnCdFadeTimer(void*)
{
    g_cdFadeVolume += g_cdPlayer->step;
    if (g_cdFadeVolume <= 0) {
        RemoveTimer(g_cdFadeTimer);
        g_cdFadeTimer = -1;
        g_cdFadeVolume = 0;
        g_cdPlayer->step = 0;
        g_cdPlayer->SetAuxVolume(g_cdFadeVolume, 1);
        if (g_cdPlayer->trackCategory == 0)
            g_cdNextTrackTimer = AddTimer(0x78, 0, OnNextTrackTimer);
        else
            g_cdPlayer->PlayNextTrack();
    } else {
        g_cdPlayer->SetAuxVolume(g_cdFadeVolume, 1);
    }
}

// FUNCTION: 0x4ce680
int Sound::GetTrackCategory()
{
    return this->trackCategory;
}

// FUNCTION: 0x4ce690
void Sound::SetTrackCategory(int mode)
{
    int old = trackCategory;
    if (old == mode)
        return;
    if (old >= 0)
        g_cdCategorySavedTrack[old] = currentTrack;
    trackCategory = mode;
    if (playbackOrder == 4 || mode == 2 || mode == 3) {
        g_cdFadeVolume = cdVolume;
        if (old == 4) {
            if (g_cdFadeTimer >= 0) {
                RemoveTimer(g_cdFadeTimer);
                g_cdFadeTimer = -1;
            }
            if (g_cdNextTrackTimer >= 0) {
                RemoveTimer(g_cdNextTrackTimer);
                g_cdNextTrackTimer = -1;
            }
            SetAuxVolume(cdVolume, 0);
            PlayNextTrack();
        } else {
            if (g_cdFadeTimer >= 0) {
                RemoveTimer(g_cdFadeTimer);
                g_cdFadeTimer = -1;
                RemoveTimer(g_cdNextTrackTimer);
                g_cdNextTrackTimer = -1;
                PlayNextTrack();
            } else {
                step = cdVolume / -18;
                g_cdFadeTimer = AddTimer(2, 0, OnCdFadeTimer);
            }
        }
    }
}

// FUNCTION: 0x4ce7a0
int Sound::SetPlaybackOrder(int value)
{
    playbackOrder = value;
    return 1;
}

// FUNCTION: 0x4ce7c0
void Sound::SetCategoryOfTrack(int index, unsigned char value)
{
    arr_214[index] = value;
}

// FUNCTION: 0x4ce7e0
unsigned char Sound::GetCategoryOfTrack(int index)
{
    return arr_214[index];
}

// FUNCTION: 0x4ce7f0
int Sound::GetCurrentTrack()
{
    return currentTrack;
}

// Returns 1 when MCI reports the CD audio device as "playing". A method of
// the object at g_game+0x10 (its one caller, 0x497f40, loads ecx from
// there) that never uses `this`.
// FUNCTION: 0x4ce800
int Sound::IsCdPlaying()
{
    return host_music_playing() == 1;
}

// FUNCTION: 0x4ce880
int GetCdPosition()
{
    int position = host_music_position();
    if (position < 0)
        return 0;
    return position;
}

// FUNCTION: 0x4ce8c0
int Sound::SelectTrack(int index)
{
    if (trackCount == 0)
        return 0;
    if (index > trackCount)
        index = index % trackCount;
    if (playState == 1) {
        PlayCdTrack(index, 1);
        return currentTrack;
    }
    currentTrack = index;
    return index;
}

// FUNCTION: 0x4ce910
int Sound::PauseCdAudio(int pause)
{
    int ok;
    int track;

    if (cdEnabled == 0)
        return 1;
    if (playState == 0)
        return 1;

    if (pause == 0) {
        track = host_music_current_track();
        if (track < trackCount)
            ok = host_music_play_range(host_music_position(), host_music_track_start(track + 1));
        else
            ok = host_music_play_range(-1, -1);
        playState = 1;
    } else {
        ok = host_music_pause();
        playState = 2;
    }
    return ok;
}

// FUNCTION: 0x4ceb60
int Sound::PlayCdTrack(int index, int flag)
{
    int same;

    if (cdEnabled == 0)
        return 1;
    playState = 1;
    if (index == 0) {
        PlayNextTrack();
        return 1;
    }
    same = host_music_playing() == 1;
    if (same && index == currentTrack)
        return 1;
    currentTrack = index;
    index += dataTrack;
    SetAuxVolume(cdVolume, 1);
    return host_music_play_tracks(index, index < trackCount ? index + 1 : 0);
}

// Stops CD audio playback; returns 1 when the MCI command succeeded.
// FUNCTION: 0x4ced40
int Sound::StopCdAudio()
{
    int ok = host_music_stop();
    if (trackCount)
        currentTrack = 1;
    else
        currentTrack = 0;
    playState = 0;
    step = 0;
    RemoveTimer(g_cdNextTrackTimer);
    RemoveTimer(g_cdFadeTimer);
    g_cdNextTrackTimer = g_cdFadeTimer = -1;
    return ok ? 1 : 0;
}

// FUNCTION: 0x4cedc0
void Sound::EnableCdAudio(int on)
{
    cdEnabled = on;
    if (on == 0) {
        host_music_stop();
        if (trackCount)
            currentTrack = 1;
        else
            currentTrack = 0;
        playState = 0;
        step = 0;
        RemoveTimer(g_cdNextTrackTimer);
        RemoveTimer(g_cdFadeTimer);
        g_cdNextTrackTimer = g_cdFadeTimer = -1;
    }
}

// FUNCTION: 0x4cee40
void __stdcall NopRet4_B(int)
{
}

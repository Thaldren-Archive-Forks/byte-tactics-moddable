#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../interfaces/host.h"

extern HWND host_win32_window(void);

namespace {

#ifdef __clang__
void SeedTrackReplies(char* count, char* trackType)
{
    int files = 0;
    struct host_dir_entry found;
    void* h = host_dir_first("music\\*.mp3", &found);
    if (h != NULL) {
        do {
            files++;
        } while (host_dir_next(h, &found));
        host_dir_close(h);
    }
    if (count != NULL) {
        wsprintfA(count, "%d", files);
    }
    if (trackType != NULL) {
        strcpy(trackType, "other");
    }
}
#endif

int Query(const char* command, char* reply, UINT size)
{
    return mciSendStringA(command, reply, size, 0) == 0;
}

int QueryNumber(const char* command)
{
    char reply[0x40];

    if (!Query(command, reply, sizeof(reply))) {
        return -1;
    }
    return atoi(reply);
}

}

extern "C" int HOST_CALL host_music_open(void)
{
    MCIERROR hr = mciSendStringA("open cdaudio", 0, 0, 0);
    if (hr != 0) {
        return 0;
    }
    mciSendStringA("stop cdaudio", 0, 0, 0);
    hr = mciSendStringA("set cdaudio time format milliseconds", 0, 0, 0);
    if (hr != 0) {
        return -1;
    }
    return 1;
}

extern "C" void HOST_CALL host_music_close(void)
{
    mciSendStringA("stop cdaudio", 0, 0, 0);
    mciSendStringA("close cdaudio", 0, 0, 0);
}

extern "C" int HOST_CALL host_music_stop(void)
{
    return mciSendStringA("stop cdaudio", 0, 0, 0) == 0;
}

extern "C" int HOST_CALL host_music_pause(void)
{
    return mciSendStringA("pause cdaudio", 0, 0, host_win32_window()) == 0;
}

extern "C" int HOST_CALL host_music_playing(void)
{
    char buf[0x40];

    if (!Query("status cdaudio mode", buf, sizeof(buf))) {
        return -1;
    }
    return strcmp(buf, "playing") == 0;
}

extern "C" int HOST_CALL host_music_track_count(void)
{
    char buf[0x20];

#ifdef __clang__
    SeedTrackReplies(buf, NULL);
#endif
    if (!Query("status cdaudio number of tracks", buf, sizeof(buf))) {
        return -1;
    }
    return atoi(buf);
}

extern "C" int HOST_CALL host_music_track_is_audio(int track)
{
    char type[0x20];
    char cmd[0x40];
    int ok;

    mciSendStringA("set cdaudio time format tmsf", 0, 0, 0);
    sprintf(cmd, "status cdaudio type track %i", track);
#ifdef __clang__
    SeedTrackReplies(NULL, type);
#endif
    ok = Query(cmd, type, sizeof(type));
    mciSendStringA("set cdaudio time format milliseconds", 0, 0, 0);
    if (!ok) {
        return -1;
    }
    return strcmp(type, "audio") == 0;
}

extern "C" int HOST_CALL host_music_track_length(int track)
{
    char cmd[0x40];

    sprintf(cmd, "status cdaudio length track %i", track);
    return QueryNumber(cmd);
}

extern "C" int HOST_CALL host_music_track_start(int track)
{
    char cmd[0x40];

    sprintf(cmd, "status cdaudio position track %i", track);
    return QueryNumber(cmd);
}

extern "C" int HOST_CALL host_music_current_track(void)
{
    return QueryNumber("status cdaudio current track");
}

extern "C" int HOST_CALL host_music_position(void)
{
    return QueryNumber("status cdaudio position");
}

extern "C" int HOST_CALL host_music_play_tracks(int first, int last)
{
    char cmd[0x40];
    char to[0x20];
    MCIERROR err;

    if (mciSendStringA("set cdaudio time format tmsf", 0, 0, 0) != 0) {
        return 0;
    }
    sprintf(cmd, "play cdaudio from %i", first);
    if (last != 0) {
        sprintf(to, " to %i", last);
        strcat(cmd, to);
    }
    strcat(cmd, " notify");
    err = mciSendStringA(cmd, 0, 0, host_win32_window());
    mciSendStringA("set cdaudio time format milliseconds", 0, 0, 0);
    return err == 0;
}

extern "C" int HOST_CALL host_music_play_range(int from_ms, int to_ms)
{
    char cmd[0x80];
    char part[0x20];

    strcpy(cmd, "play cdaudio");
    if (from_ms >= 0) {
        sprintf(part, " from %i", from_ms);
        strcat(cmd, part);
    }
    if (to_ms >= 0) {
        sprintf(part, " to %i", to_ms);
        strcat(cmd, part);
    }
    strcat(cmd, " notify");
    return mciSendStringA(cmd, 0, 0, host_win32_window()) == 0;
}

extern "C" unsigned int HOST_CALL host_music_disc_serial(void)
{
    char root[4] = {'A', ':', '\\', 0};

    for (char letter = 'A'; letter <= 'Z'; letter++) {
        DWORD serial;
        root[0] = letter;
        if (GetDriveTypeA(root) == DRIVE_CDROM) {
            if (GetVolumeInformationA(root, NULL, 0, &serial, NULL, NULL, NULL, 0)) {
                return serial;
            }
            return 0;
        }
    }
    return 0;
}

static BOOL CALLBACK FindCdPlayerWindow(HWND hwnd, LPARAM param)
{
    char className[200];
    GetClassNameA(hwnd, className, sizeof(className) - 1);
    if (strcmp(className, "SJE_CdPlayerClass") == 0)
        *(HWND*)param = hwnd;
    return TRUE;
}

extern "C" void* HOST_CALL host_music_find_cd_player(void)
{
    HWND found = 0;
    EnumWindows((WNDENUMPROC)FindCdPlayerWindow, (LPARAM)&found);
    return found;
}

extern "C" void HOST_CALL host_music_close_cd_player(void* window)
{
    SendMessageA((HWND)window, WM_CLOSE, 0, 0);
    SendMessageA((HWND)window, WM_QUIT, 0, 0);
    host_sleep_ms(500);
}

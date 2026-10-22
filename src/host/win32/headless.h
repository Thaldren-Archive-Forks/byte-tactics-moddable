#ifndef HOST_WIN32_HEADLESS_H
#define HOST_WIN32_HEADLESS_H

#include "../../../interfaces/host.h"

bool HostHeadless();

void* HeadlessWindowOpen(const char* title, int width, int height, int fullscreen);
void HeadlessWindowClose();
int HeadlessWindowHasFocus();
void HeadlessWindowPost(unsigned int type);
int HeadlessPumpEvents(int wait);
void HeadlessPostQuit();
int HeadlessEventNext(host_event* event);
int HeadlessKeyState(int vk);
void HeadlessCursorGet(int* x, int* y);
void HeadlessCursorSet(int x, int y);

int HeadlessDisplayModeSet(int width, int height, int flags, const unsigned char* palette);
void HeadlessDisplayClose();
int HeadlessDisplayIsOpen();
int HeadlessDisplayHasBackBuffer();
int HeadlessDisplayModeList(host_display_mode* modes, int max, int* count);
void HeadlessDisplayScreenSize(int* width, int* height);
int HeadlessFrameLock(int which, host_frame* frame);
int HeadlessFrameUnlock(int which);
int HeadlessFramePresent(int flip);
int HeadlessFramePresentRect(int x, int y, int width, int height);
int HeadlessFrameIsLost();
int HeadlessFrameRestore();
int HeadlessFrameFill(int which, unsigned int colour);
int HeadlessFramePixelFormat(host_pixel_format* format);
int HeadlessFrameText(int which, void* font, int x, int y, const char* text, int length, unsigned int rgb);
int HeadlessFrameRead(unsigned char* pixels, int pitch, unsigned char* palette);
int HeadlessPaletteCreate(const unsigned char* entries);
int HeadlessPaletteSet(int start, int count, const unsigned char* entries);
int HeadlessPaletteGet(unsigned char* entries);
int HeadlessPaletteApply();

void* HeadlessSoundOpen(void* window, int rate, int bits, int channels, int* error);
void HeadlessSoundClose();
void* HeadlessSoundNative();
void* HeadlessSoundBufferCreate(unsigned int bytes, int rate, int bits, int channels, unsigned int flags);
void* HeadlessSoundBufferDuplicate(void* buffer);
void HeadlessSoundBufferFree(void* buffer);
int HeadlessSoundBufferLock(void* buffer, unsigned int offset, unsigned int bytes, void** data, unsigned int* size);
int HeadlessSoundBufferUnlock(void* buffer, void* data, unsigned int size);
int HeadlessSoundBufferPlay(void* buffer, int loop);
int HeadlessSoundBufferStop(void* buffer);
int HeadlessSoundBufferStatus(void* buffer);
int HeadlessSoundBufferPosition(void* buffer, unsigned int* play, unsigned int* write);
int HeadlessSoundBufferSetPosition(void* buffer, unsigned int offset);
int HeadlessSoundBufferSetVolume(void* buffer, int volume);
int HeadlessSoundBufferPlace(void* buffer, const float* xyz, float min_distance, float max_distance);
int HeadlessSoundWavPlay(const void* wav, unsigned int flags);
void HeadlessSoundWavStop();
int HeadlessMixerWaveCount();
int HeadlessMixerCdDevice();
int HeadlessMixerGet(int line, int device, unsigned int* volume);
int HeadlessMixerSet(int line, int device, unsigned int volume);

int HeadlessMusicOpen();
void HeadlessMusicClose();
int HeadlessMusicStop();
int HeadlessMusicPause();
int HeadlessMusicPlaying();
int HeadlessMusicTrackCount();
int HeadlessMusicTrackIsAudio(int track);
int HeadlessMusicTrackLength(int track);
int HeadlessMusicTrackStart(int track);
int HeadlessMusicCurrentTrack();
int HeadlessMusicPosition();
int HeadlessMusicPlayTracks(int first, int last);
int HeadlessMusicPlayRange(int from_ms, int to_ms);
unsigned int HeadlessMusicDiscSerial();
void* HeadlessMusicFindCdPlayer();
void HeadlessMusicCloseCdPlayer(void* window);

#endif

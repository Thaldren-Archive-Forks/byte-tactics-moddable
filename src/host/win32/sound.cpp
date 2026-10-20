#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include "../../../interfaces/host.h"

extern "C" const GUID IID_IDirectSound3DBuffer;

struct IDirectSound3DBuffer : public IUnknown {
    virtual HRESULT __stdcall GetAllParameters(void* p) = 0;
    virtual HRESULT __stdcall GetConeAngles(LPDWORD a, LPDWORD b) = 0;
    virtual HRESULT __stdcall GetConeOrientation(void* p) = 0;
    virtual HRESULT __stdcall GetConeOutsideVolume(LPLONG p) = 0;
    virtual HRESULT __stdcall GetMaxDistance(float* p) = 0;
    virtual HRESULT __stdcall GetMinDistance(float* p) = 0;
    virtual HRESULT __stdcall GetMode(LPDWORD p) = 0;
    virtual HRESULT __stdcall GetPosition(void* p) = 0;
    virtual HRESULT __stdcall GetVelocity(void* p) = 0;
    virtual HRESULT __stdcall SetAllParameters(void* p, DWORD apply) = 0;
    virtual HRESULT __stdcall SetConeAngles(DWORD a, DWORD b, DWORD apply) = 0;
    virtual HRESULT __stdcall SetConeOrientation(float x, float y, float z, DWORD apply) = 0;
    virtual HRESULT __stdcall SetConeOutsideVolume(LONG v, DWORD apply) = 0;
    virtual HRESULT __stdcall SetMaxDistance(float d, DWORD apply) = 0;
    virtual HRESULT __stdcall SetMinDistance(float d, DWORD apply) = 0;
    virtual HRESULT __stdcall SetMode(DWORD mode, DWORD apply) = 0;
    virtual HRESULT __stdcall SetPosition(float x, float y, float z, DWORD apply) = 0;
};

namespace {

IDirectSound* g_ds;
IDirectSoundBuffer* g_primary;

void FillFormat(WAVEFORMATEX* wfx, int rate, int bits, int channels)
{
    wfx->wFormatTag = 1;
    wfx->nChannels = (WORD)channels;
    wfx->nSamplesPerSec = rate;
    wfx->nBlockAlign = (WORD)(bits / 8 * channels);
    wfx->nAvgBytesPerSec = rate * wfx->nBlockAlign;
    wfx->wBitsPerSample = (WORD)bits;
    wfx->cbSize = 0x12;
}

void ReleaseAll()
{
    if (g_primary != NULL) {
        g_primary->Release();
        g_primary = NULL;
    }
    if (g_ds != NULL) {
        g_ds->Release();
        g_ds = NULL;
    }
}

}

extern "C" void* HOST_CALL host_sound_open(void* window, int rate, int bits, int channels, int* error)
{
    WAVEFORMATEX wfx;
    DSBUFFERDESC desc;
    HRESULT hr;

    if (g_ds == NULL) {
        hr = DirectSoundCreate(0, &g_ds, 0);
        if (hr != 0) {
            goto fail;
        }
        hr = g_ds->SetCooperativeLevel((HWND)window, 2);
        if (hr != 0) {
            goto fail;
        }
        desc.dwSize = sizeof(desc);
        desc.dwFlags = 1;
        desc.dwBufferBytes = 0;
        desc.dwReserved = 0;
        desc.lpwfxFormat = 0;
        hr = g_ds->CreateSoundBuffer(&desc, &g_primary, 0);
        if (hr != 0) {
            goto fail;
        }
    }
    FillFormat(&wfx, rate, bits, channels);
    hr = g_primary->SetFormat(&wfx);
    if (hr != 0) {
        goto fail;
    }
    return g_ds;
fail:
    *error = hr == (HRESULT)0x88780078 ? HOST_SOUND_NO_DRIVER : 0;
    return NULL;
}

extern "C" void HOST_CALL host_sound_close(void)
{
    ReleaseAll();
}

extern "C" void* HOST_CALL host_sound_native(void)
{
    return g_ds;
}

extern "C" void* HOST_CALL host_sound_buffer_create(unsigned int bytes, int rate, int bits, int channels, unsigned int flags)
{
    WAVEFORMATEX wfx;
    DSBUFFERDESC desc;
    IDirectSoundBuffer* buf = NULL;

    if (g_ds == NULL) {
        return NULL;
    }
    FillFormat(&wfx, rate, bits, channels);
    desc.dwReserved = 0;
    desc.lpwfxFormat = &wfx;
    desc.dwSize = 0x14;
    desc.dwFlags = (flags & HOST_SOUND_3D) ? 0x92 : 0x82;
    desc.dwBufferBytes = bytes;
    if (g_ds->CreateSoundBuffer(&desc, &buf, 0) != 0) {
        return NULL;
    }
    return buf;
}

extern "C" void* HOST_CALL host_sound_buffer_duplicate(void* buffer)
{
    IDirectSoundBuffer* copy = NULL;

    if (g_ds == NULL || g_ds->DuplicateSoundBuffer((IDirectSoundBuffer*)buffer, &copy) != 0) {
        return NULL;
    }
    return copy;
}

extern "C" void HOST_CALL host_sound_buffer_free(void* buffer)
{
    ((IDirectSoundBuffer*)buffer)->Release();
}

extern "C" int HOST_CALL host_sound_buffer_lock(void* buffer, unsigned int offset, unsigned int bytes, void** data, unsigned int* size)
{
    DWORD got;
    void* ptr;

    if (((IDirectSoundBuffer*)buffer)->Lock(offset, bytes, &ptr, &got, 0, 0, 0) != 0) {
        return 0;
    }
    *data = ptr;
    *size = got;
    return 1;
}

extern "C" int HOST_CALL host_sound_buffer_unlock(void* buffer, void* data, unsigned int size)
{
    return ((IDirectSoundBuffer*)buffer)->Unlock(data, size, 0, 0) == 0;
}

extern "C" int HOST_CALL host_sound_buffer_play(void* buffer, int loop)
{
    return ((IDirectSoundBuffer*)buffer)->Play(0, 0, loop != 0) == 0;
}

extern "C" int HOST_CALL host_sound_buffer_stop(void* buffer)
{
    return ((IDirectSoundBuffer*)buffer)->Stop() == 0;
}

extern "C" int HOST_CALL host_sound_buffer_status(void* buffer)
{
    DWORD status;

    if (((IDirectSoundBuffer*)buffer)->GetStatus(&status) != 0) {
        return -1;
    }
    return (int)status;
}

extern "C" int HOST_CALL host_sound_buffer_position(void* buffer, unsigned int* play, unsigned int* write)
{
    DWORD p;
    DWORD w;

    if (((IDirectSoundBuffer*)buffer)->GetCurrentPosition(&p, &w) != 0) {
        return 0;
    }
    *play = p;
    *write = w;
    return 1;
}

extern "C" int HOST_CALL host_sound_buffer_set_position(void* buffer, unsigned int offset)
{
    return ((IDirectSoundBuffer*)buffer)->SetCurrentPosition(offset) == 0;
}

extern "C" int HOST_CALL host_sound_buffer_set_volume(void* buffer, int volume)
{
    return ((IDirectSoundBuffer*)buffer)->SetVolume(volume) == 0;
}

extern "C" int HOST_CALL host_sound_buffer_place(void* buffer, const float* xyz, float min_distance, float max_distance)
{
    IDirectSound3DBuffer* chan;

    if (((IDirectSoundBuffer*)buffer)->QueryInterface(IID_IDirectSound3DBuffer, (void**)&chan) != 0) {
        return 0;
    }
    if (xyz == NULL) {
        chan->SetMode(2, 0);
    } else {
        chan->SetPosition(xyz[0], xyz[1], xyz[2], 0);
        chan->SetMinDistance(min_distance, 0);
        chan->SetMaxDistance(max_distance, 0);
        chan->SetMode(0, 0);
    }
    chan->Release();
    return 1;
}

extern "C" int HOST_CALL host_mixer_wave_count(void)
{
    return (int)waveOutGetNumDevs();
}

extern "C" int HOST_CALL host_mixer_cd_device(void)
{
    int count = (int)auxGetNumDevs();

    for (int i = 0; i < count; i++) {
        AUXCAPSA caps;
        if (auxGetDevCapsA(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR && caps.wTechnology == AUXCAPS_CDAUDIO) {
            return i;
        }
    }
    return -1;
}

extern "C" int HOST_CALL host_mixer_get(int line, int device, unsigned int* volume)
{
    DWORD v;
    MMRESULT r;

    if (line == HOST_MIXER_WAVE) {
        r = waveOutGetVolume((HWAVEOUT)device, &v);
    } else {
        r = auxGetVolume(device, &v);
    }
    if (r != MMSYSERR_NOERROR) {
        return 0;
    }
    *volume = v;
    return 1;
}

extern "C" int HOST_CALL host_mixer_set(int line, int device, unsigned int volume)
{
    MMRESULT r;

    if (line == HOST_MIXER_WAVE) {
        r = waveOutSetVolume((HWAVEOUT)device, volume);
    } else {
        r = auxSetVolume(device, volume);
    }
    return r == MMSYSERR_NOERROR;
}

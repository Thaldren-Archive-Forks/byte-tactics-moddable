// Decompiled by Claude Opus 5.5, deepseek-v4.1-flash, Haiku and Opus. Names are provisional.

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "../../interfaces/host.h"

class TdfRecord;

#include "../util/tdf.h"
#include "../state/sim_globals.h"


struct FindData_0041d4c0 {
    char unknown_0[0x14];
    char name[260];                    // +0x14
};

extern int DAT_0050289c;

void HAPI_DropMissingArchives();
void ChdirToExeDirectory();
void HAPI_ResolveShadowedFiles();
int __stdcall HAPI_FindFirst(const char* path, FindData_0041d4c0* fd, int a, int b);
int __stdcall HAPI_FindNext(int handle, FindData_0041d4c0* fd);
void __stdcall HAPI_FindClose(int handle);
int __stdcall HAPI_AddArchive(char* path, int flag);
void __cdecl AllocNotifyNop(int);

// FUNCTION: 0x41d4c0
void RegisterDataArchives()
{
    FindData_0041d4c0 fd;
    char pattern[64];
    if (DAT_0050289c != 0) {
        HAPI_DropMissingArchives();
        ChdirToExeDirectory();
        sprintf(pattern, "rev%s.GP3", "31");
        int h = HAPI_FindFirst(pattern, &fd, -1, 1);
        if (h >= 0) {
            do {
                HAPI_AddArchive(fd.name, 1);
            } while (HAPI_FindNext(h, &fd) == 0);
            HAPI_FindClose(h);
        }
        h = HAPI_FindFirst("*.CCX", &fd, -1, 1);
        if (h >= 0) {
            do {
                HAPI_AddArchive(fd.name, 1);
            } while (HAPI_FindNext(h, &fd) == 0);
            HAPI_FindClose(h);
        }
        h = HAPI_FindFirst("*.UFO", &fd, -1, 1);
        if (h >= 0) {
            do {
                HAPI_AddArchive(fd.name, 0);
            } while (HAPI_FindNext(h, &fd) == 0);
            HAPI_FindClose(h);
        }
        int hpi = HAPI_FindFirst("*.HPI", &fd, -1, 1);
        int left = 10;
        if (hpi >= 0) {
            do {
                if (HAPI_AddArchive(fd.name, 0))
                    left--;
            } while (HAPI_FindNext(hpi, &fd) == 0 && left != 0);
            HAPI_FindClose(hpi);
        }
        HAPI_ResolveShadowedFiles();
    }
}

// FUNCTION: 0x41d8a0
int FUN_0041d8a0(void)
{
    return DAT_0050289c;
}

// FUNCTION: 0x41d8c0
void* __stdcall AllocZeroedWithTickOffset(unsigned int size)
{
    unsigned int pad = host_time_ms() % 1000 * 7;
    unsigned int total = pad + size;
    char* p = (char*)operator new(total);
    memset(p, 0, total);
    AllocNotifyNop((int)p);
    return p + pad;
}

// Decompiled by Opus. Names are provisional.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../interfaces/host.h"

// Shows the contents of stats.txt in a message box.
// FUNCTION: 0x47c4d0
void ShowStatsSummary()
{
    char text[0x400];
    memset(text, 0, sizeof(text));
    void* file = host_file_open_read("stats.txt");
    if (file) {
        long size = host_file_size(file);
        char* raw = size > 0 ? (char*)malloc(size) : 0;
        if (raw) {
            unsigned int got = host_file_read(file, raw, size);
            unsigned int out = 0;
            for (unsigned int i = 0; i < got && out < sizeof(text); i++) {
                if (raw[i] == 0x1a)
                    break;
                if (raw[i] == '\r' && i + 1 < got && raw[i + 1] == '\n')
                    continue;
                text[out++] = raw[i];
            }
            free(raw);
        }
        host_file_close(file);
        MessageBoxA(NULL, text, "Summary", MB_TOPMOST);
    }
}

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../interfaces/host.h"
#include "file_printf.h"

int FilePrintf(void* file, const char* fmt, ...)
{
    char stack[512];
    char* buf = stack;
    unsigned int cap = sizeof(stack);
    int len;
    va_list args;
    for (;;) {
        va_start(args, fmt);
        len = _vsnprintf(buf, cap, fmt, args);
        va_end(args);
        if (len >= 0 && (unsigned int)len < cap)
            break;
        cap *= 2;
        if (buf != stack)
            free(buf);
        buf = (char*)malloc(cap);
        if (buf == 0)
            return -1;
    }
    unsigned int written = host_file_write(file, buf, (unsigned int)len);
    if (buf != stack)
        free(buf);
    return written == (unsigned int)len ? len : -1;
}

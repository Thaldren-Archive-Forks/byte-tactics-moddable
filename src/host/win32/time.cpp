#include <windows.h>

#include "../../../interfaces/host.h"

extern "C" unsigned int HOST_CALL host_time_ms(void)
{
    return GetTickCount();
}

extern "C" unsigned long long HOST_CALL host_time_counter(void)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (unsigned long long)counter.QuadPart;
}

// FUNCTION: 0x4b6b50
extern "C" void HOST_CALL host_sleep_ms(unsigned int ms)
{
    Sleep(ms);
}

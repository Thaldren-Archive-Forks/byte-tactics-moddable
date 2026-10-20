#include <windows.h>

#include "../../../interfaces/host.h"

extern "C" unsigned int __cdecl _beginthread(void* start, unsigned int stack, void* param);

// FUNCTION: 0x4b6b20
extern "C" int HOST_CALL host_thread_start(void (HOST_CALL* entry)(void*), unsigned int stack, void* param)
{
    unsigned int result = _beginthread((void*)entry, stack, param);
    if (result != (unsigned int)-1) {
        return 1;
    }
    return 0;
}

extern "C" unsigned int HOST_CALL host_thread_id(void)
{
    return GetCurrentThreadId();
}

extern "C" int HOST_CALL host_atomic_exchange(int* target, int value)
{
    return (int)InterlockedExchange((LONG*)target, value);
}

extern "C" void* HOST_CALL host_mutex_create(void)
{
    CRITICAL_SECTION* section = (CRITICAL_SECTION*)HeapAlloc(GetProcessHeap(), 0, sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(section);
    return section;
}

extern "C" void HOST_CALL host_mutex_lock(void* mutex)
{
    EnterCriticalSection((CRITICAL_SECTION*)mutex);
}

extern "C" void HOST_CALL host_mutex_unlock(void* mutex)
{
    LeaveCriticalSection((CRITICAL_SECTION*)mutex);
}

extern "C" void HOST_CALL host_mutex_destroy(void* mutex)
{
    DeleteCriticalSection((CRITICAL_SECTION*)mutex);
    HeapFree(GetProcessHeap(), 0, mutex);
}

extern "C" void* HOST_CALL host_event_create(int manual_reset, int signalled)
{
    return CreateEventA(NULL, manual_reset, signalled, NULL);
}

extern "C" void HOST_CALL host_event_set(void* event)
{
    SetEvent((HANDLE)event);
}

extern "C" void HOST_CALL host_event_reset(void* event)
{
    ResetEvent((HANDLE)event);
}

extern "C" int HOST_CALL host_event_wait(void* event, unsigned int timeout_ms)
{
    return WaitForSingleObject((HANDLE)event, timeout_ms) == WAIT_OBJECT_0 ? 1 : 0;
}

extern "C" void HOST_CALL host_event_destroy(void* event)
{
    CloseHandle((HANDLE)event);
}

extern "C" void HOST_CALL host_thread_set_priority(int level)
{
    SetThreadPriority(GetCurrentThread(), level);
}

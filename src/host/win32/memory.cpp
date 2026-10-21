#include <windows.h>

#include "../../../interfaces/host.h"

extern "C" void* HOST_CALL host_mem_reserve(unsigned int size)
{
    return VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
}

extern "C" void* HOST_CALL host_mem_commit(void* addr, unsigned int size)
{
    return VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
}

extern "C" void HOST_CALL host_mem_decommit(void* addr, unsigned int size)
{
    VirtualFree(addr, size, MEM_DECOMMIT);
}

extern "C" void HOST_CALL host_mem_release(void* addr)
{
    VirtualFree(addr, 0, MEM_RELEASE);
}

extern "C" int HOST_CALL host_mem_protect(void* addr, unsigned int size, int access)
{
    DWORD old;
    return VirtualProtect(addr, size, access == HOST_MEM_READONLY ? PAGE_READONLY : PAGE_READWRITE, &old) != 0;
}

extern "C" int HOST_CALL host_mem_readable(const void* addr, unsigned int size)
{
    return !IsBadReadPtr(addr, size);
}

extern "C" int HOST_CALL host_image_range(const void** base, unsigned int* size)
{
    const char* image = (const char*)GetModuleHandle(0);
    if (image == NULL) {
        return 0;
    }
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)image;
    const IMAGE_NT_HEADERS* nt = (const IMAGE_NT_HEADERS*)(image + dos->e_lfanew);
    *base = image;
    *size = nt->OptionalHeader.SizeOfImage;
    return 1;
}

extern "C" void* HOST_CALL host_mem_alloc(unsigned int size)
{
    return GlobalAlloc(GMEM_FIXED, size);
}

extern "C" void HOST_CALL host_mem_free(void* block)
{
    GlobalFree(block);
}

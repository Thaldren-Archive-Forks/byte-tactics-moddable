#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../interfaces/host.h"

struct HostDir {
    long handle;
};

extern "C" void* HOST_CALL host_file_open_read(const char* path)
{
    return fopen(path, "rb");
}

extern "C" void* HOST_CALL host_file_create(const char* path, unsigned int flags)
{
    return fopen(path, (flags & HOST_FILE_TEXT) ? "w" : "w+b");
}

extern "C" void* HOST_CALL host_file_open_append(const char* path)
{
    return fopen(path, "a+b");
}

extern "C" unsigned int HOST_CALL host_file_read(void* file, void* buf, unsigned int size)
{
    return fread(buf, 1, size, (FILE*)file);
}

extern "C" unsigned int HOST_CALL host_file_write(void* file, const void* buf, unsigned int size)
{
    return fwrite(buf, 1, size, (FILE*)file);
}

extern "C" long HOST_CALL host_file_seek(void* file, long offset, int origin)
{
    if (fseek((FILE*)file, offset, origin) != 0)
        return -1;
    return ftell((FILE*)file);
}

extern "C" long HOST_CALL host_file_size(void* file)
{
    return _filelength(_fileno((FILE*)file));
}

extern "C" int HOST_CALL host_file_truncate(void* file, long size)
{
    return _chsize(_fileno((FILE*)file), size) == 0;
}

extern "C" void HOST_CALL host_file_close(void* file)
{
    fclose((FILE*)file);
}

static void FillEntry(const struct _finddata_t* fd, struct host_dir_entry* entry)
{
    strcpy(entry->name, fd->name);
    entry->attrib = fd->attrib;
    entry->size = fd->size;
    entry->time_write = (unsigned int)fd->time_write;
}

extern "C" void* HOST_CALL host_dir_first(const char* pattern, struct host_dir_entry* entry)
{
    struct _finddata_t fd;
    long handle = _findfirst(pattern, &fd);
    if (handle == -1)
        return 0;
    HostDir* dir = (HostDir*)malloc(sizeof(HostDir));
    if (dir == 0) {
        _findclose(handle);
        return 0;
    }
    dir->handle = handle;
    FillEntry(&fd, entry);
    return dir;
}

extern "C" int HOST_CALL host_dir_next(void* dir, struct host_dir_entry* entry)
{
    struct _finddata_t fd;
    if (_findnext(((HostDir*)dir)->handle, &fd) == -1)
        return 0;
    FillEntry(&fd, entry);
    return 1;
}

extern "C" void HOST_CALL host_dir_close(void* dir)
{
    _findclose(((HostDir*)dir)->handle);
    free(dir);
}

extern "C" unsigned int HOST_CALL host_file_exe_path(char* buf, unsigned int size)
{
    return GetModuleFileNameA(NULL, buf, size);
}

extern "C" unsigned int HOST_CALL host_file_full_path(const char* name, char* buf, unsigned int size)
{
    char* filePart;
    return GetFullPathNameA(name, size, buf, &filePart);
}

extern "C" int HOST_CALL host_dir_set_current(const char* path)
{
    return SetCurrentDirectoryA(path) != 0;
}

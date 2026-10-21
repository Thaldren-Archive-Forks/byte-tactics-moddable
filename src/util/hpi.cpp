// Decompiled by Opus, space-bunny-free, Claude Opus 5.5, Haiku, Sonnet 5.5, deepseek-v4.1-flash, GPT-6, GPT-6.1-sol, deepseek-v4.1, GPT-5.6-Terra, muse-spark-1.3-free, Sonnet, Space Bunny Free, mimo-v2.6-pro, DeepSeek V4.1 Flash, claude-sonnet-5-5 and opus. Names are provisional.
#include "win_types.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <io.h>
#include <direct.h>
#include <stddef.h>
#include <vector>

#include "../../interfaces/host.h"

// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
void RegisterUnitOrders(void);
void RegisterGroundOrders(void);
void EnableAICommands(void);
void RegisterAICommands(void);
void FUN_00406f40(void);
void ResetAIPlayers(void);

// A file's entry data: the block's offset, its size and whether the data is
// compressed.
#pragma pack(push, 1)
struct Info {
    int offset;                        // +0x0
    int size;                          // +0x4
    unsigned char compressed;          // +0x8
};
#pragma pack(pop)

#include "archive_entry.h"

struct ArchiveDirectory {
    int count;                         // +0x0
    ArchiveEntry* entries;             // +0x4
};

// An archive's header: the key at +0xc and the root directory at +0x10.
struct Header {
    char magic[4];                     // +0x0
    unsigned char version[4];          // +0x4
    unsigned int size;                 // +0x8
    unsigned char obfuscate;           // +0xc
    char unknown_d[3];
    ArchiveDirectory* list;            // +0x10
};

// An open archive: the file, the read position, its header, the open count
// and the mode it was opened in.
struct OPENHAPIFILE {
    void* fp;
    int pos;                           // +0x4, read position
    Header* node;                      // +0x8
    int count;                         // +0xc, handles open on this item
    int mode;                          // +0x10
    char name[0x104];                  // +0x14
};

// The display context the open archives hang from: the list of open archives
// at +0x618, the start directory at +0x628 and the last directory set at
// +0x728.
struct DisplayContext {
    char unknown_0[0x618];
    OPENHAPIFILE** files;              // +0x618
    int count;                         // +0x61c
    char unknown_620[8];
    char cwd[0x100];                   // +0x628
    char lastDir[0x100];               // +0x728
};

#include "file_handle.h"

FileHandle* __stdcall HAPI_OpenFile(char* filename, const char* mode);

// Opens a file in "a+b" mode through HAPI_OpenFile (sibling of 0x4bb5b0).
// FUNCTION: 0x4bb2c0
void __stdcall HAPI_OpenFileAppend(void* param1)
{
    HAPI_OpenFile((char*)param1, "a+b");
}

DisplayContext* GetDisplay(void);
void* __cdecl GameAllocIgnoreTag(char* name, unsigned int size);
void __cdecl GameFreeThunk(void* p);
// HAPI_FindEntry (0x4bb4e0) is gap code and stays in src/util/hpi_4bb4e0.cpp:
// tools/gapcheck.py sizes a region's functions by the next annotation in the
// file, so a file that also holds functions outside the region cannot be its
// source.
ArchiveEntry* __stdcall HAPI_FindEntry(ArchiveDirectory* list, char* path);

// Whole pixels of a 16.16 size, rounded up.
// Written as w / 65536 + (w % 65536 != 0) to expand to the original's code.
static inline int nblocks(int w)
{
    return w / 65536 + (w % 65536 != 0);
}

// Opens a file for reading (the mode comes from the caller: "rb" from 0x4bb5b0,
// "a+b" from 0x4bb2c0). If the file cannot be opened it falls back to the first
// already loaded item whose name tree has an entry without bit 0 of its flags
// set, reads that entry's texture block into a fresh "Block Sizes" buffer and
// un-obfuscates it in place.
// The block size is the 16.16 field rounded up to whole pixels, times 4.
// FUNCTION: 0x4bb2e0
FileHandle* __stdcall HAPI_OpenFile(char* filename, const char* mode)
{
    // Declared here in the order size, off, k, p: it fixes the operand order of the sums.
    int size;
    int off;
    int k;
    unsigned char* p;
    DisplayContext* state = GetDisplay();
    FileHandle* h = (FileHandle*)GameAllocIgnoreTag("File Handle", 0x118);
    memset(h, 0, 0x118);
    strncpy(h->name, filename, 0x100);
    h->name[0xff] = 0;
    h->fp = mode[0] == 'a' ? host_file_open_append(filename) : host_file_open_read(filename);
    if (h->fp) {
        h->shared = 0;
        return h;
    }
    for (int i = 0; i < state->count; i++) {
        ArchiveEntry* e = HAPI_FindEntry(state->files[i]->node->list, filename);
        if (e == 0 || (e->flags & 1))
            continue;
        if (state->files[i]->fp == 0) {
            state->files[i]->fp = host_file_open_read(state->files[i]->name);
            if (state->files[i]->fp == 0) {
                GameFreeThunk(h);
                return 0;
            }
            state->files[i]->pos = 0;
        }
        state->files[i]->count++;
        h->info = e->info;
        h->shared = state->files[i];
        h->pos = 0;
        if (h->info->compressed) {
            size = nblocks(h->info->size) * 4;
            h->buffer = (int*)GameAllocIgnoreTag("Block Sizes", size);
            off = h->info->offset;
            host_file_seek(h->shared->fp, off, HOST_SEEK_SET);
            host_file_read(h->shared->fp, h->buffer, size);
            h->shared->pos = off + size;
            p = (unsigned char*)h->buffer;
            unsigned char key = h->shared->node->obfuscate;
            if (key) {
                for (k = 0; k < size; k++)
                    p[k] = (unsigned char)(p[k] ^ 0xff ^ (unsigned char)(k + off) ^ key);
            }
        }
        return h;
    }
    GameFreeThunk(h);
    return 0;
}

// FUNCTION: 0x4bb5b0
void __stdcall HAPI_OpenFileRead(void* param1)
{
    HAPI_OpenFile((char*)param1, "rb");
}

// FUNCTION: 0x4bb5d0
int __stdcall HAPI_CloseFile(FileHandle* file)
{
    int result;
    if (file->shared != 0) {
        result = 0;
        file->shared->count--;
        if (file->shared->count == 0 && file->shared->mode == 0) {
            host_file_close(file->shared->fp);
            result = 0;
            file->shared->fp = 0;
        }
    } else {
        host_file_close(file->fp);
        result = 0;
    }
    if (file->buffer != 0) {
        GameFreeThunk(file->buffer);
    }
    if (file->buffer2 != 0) {
        GameFreeThunk(file->buffer2);
    }
    GameFreeThunk(file);
    return result;
}

// FUNCTION: 0x4bb650
bool __stdcall HAPI_IsInArchive(FileHandle* obj)
{
    return obj->shared != 0;
}

// FUNCTION: 0x4bb670
void FileHandle::SetFileName(const char* text)
{
    strncpy(name, text, sizeof(name));
    name[sizeof(name) - 1] = 0;
}

// Creates a file for writing ("w+b") and wraps it in a zeroed "File Handle"
// that remembers the file name.
// FUNCTION: 0x4bb6a0
FileHandle* __stdcall HAPI_CreateFile(char* path)
{
    void* fp = host_file_create(path, 0);
    if (fp) {
        FileHandle* file = (FileHandle*)GameAllocIgnoreTag("File Handle", sizeof(FileHandle));
        memset(file, 0, sizeof(FileHandle));
        strncpy(file->name, path, sizeof(file->name));
        file->name[sizeof(file->name) - 1] = 0;
        file->fp = fp;
        file->shared = 0;
        return file;
    }
    return 0;
}

// FUNCTION: 0x4bb710
long __stdcall HAPI_SeekFile(FileHandle* file, long pos)
{
    if (file->shared != 0) {
        unsigned int old = file->pos;
        if (pos == -1) {
            file->pos = file->info->size;
        } else {
            file->pos = pos;
        }
        if (((old ^ file->pos) & 0xffff0000) != 0 && file->buffer2 != 0) {
            GameFreeThunk(file->buffer2);
            file->buffer2 = 0;
        }
        return 0;
    }
    long result;
    if (pos == -1) {
        result = host_file_seek(file->fp, 0, HOST_SEEK_END);
    } else {
        result = host_file_seek(file->fp, pos, HOST_SEEK_SET);
    }
    if (result == -1) {
        return -1;
    }
    return result;
}

// FUNCTION: 0x4bb7a0
long __stdcall HAPI_TellFile(FileHandle* file)
{
    if (file->shared != 0) {
        return file->pos;
    }
    return host_file_seek(file->fp, 0, HOST_SEEK_CUR);
}

int __stdcall SquashUnpack(unsigned char* dst, unsigned char* src);
char* __stdcall SquashErrorString(int code);
void __stdcall FatalError(char* text);

static inline int BlockOffset(FileHandle* file, int& counter, int b, int tableSize)
{
    int off = tableSize;
    for (counter = 0; counter < b; ++counter)
        off += file->buffer[counter];
    return off;
}

// FUNCTION: 0x4bb7c0
int __stdcall HAPI_readfromfile(FileHandle* file, unsigned char* buf, int size)
{
    int n;
    unsigned char* dst;
    int remaining;
    unsigned char* comp;
    int i;
    int blocks;
    int tableSize;
    int b;
    OPENHAPIFILE* item = file->shared;
    if (item != 0) {
        Info* info = file->info;
        n = size;
        if (info->size - (int)file->pos < n)
            n = info->size - (int)file->pos;
        if (info->compressed != 0) {
            i = 0;
            // Statement order i, blocks, tableSize, remaining decides where n is reloaded.
            blocks = (((n + file->pos - 1) & 0xffff0000) - (file->pos & 0xffff0000) >> 16) + 1;
            tableSize = ((info->size % 65536 != 0) + info->size / 65536) * 4;
            remaining = n;
            dst = buf;
            while (i < blocks) {
                b = file->pos >> 16;
                if (file->buffer2 == 0) {
                    int k;
                    int off = BlockOffset(file,k,b,tableSize);
                    off += file->info->offset;
                    host_file_seek(file->shared->fp, off, HOST_SEEK_SET);
                    file->shared->pos = off;
                    file->buffer2 = (unsigned char*)GameAllocIgnoreTag("Uncompressed Block", 0x10000);
                    comp = (unsigned char*)GameAllocIgnoreTag("Compressed Buffer", file->buffer[k]);
                    int got = host_file_read(file->shared->fp, comp, file->buffer[k]);
                    if (got != file->buffer[k]) {
                        GameFreeThunk(file->buffer2);
                        file->buffer2 = 0;
                        GameFreeThunk(comp);
                        return -1;
                    }
                    file->shared->pos += got;
                    unsigned char key = file->shared->node->obfuscate;
                    if (key) {
                        for (int k = 0; k < got; k++)
                            comp[k] = (unsigned char)(comp[k] ^ 0xff ^ (unsigned char)(k + off) ^ key);
                    }
                    int err = SquashUnpack(file->buffer2, comp);
                    if (err) {
                        char msg[1000];
                        sprintf(msg, "[HAPI_readfromfile] Decompression Error: %s\n", SquashErrorString(err));
                        sprintf(msg + strlen(msg), "block %d of %d\n", i, blocks);
                        sprintf(msg + strlen(msg), "base name '%s'\n", file->shared->name);
                        sprintf(msg + strlen(msg), "length = %d\n", size);
                        sprintf(msg + strlen(msg), "name = '%s'\n", file->name);
                        FatalError(msg);
                    }
                    GameFreeThunk(comp);
                }
                int chunk = ((b + 1) << 16) - file->pos;
                if (chunk > remaining)
                    chunk = remaining;
                int o = file->pos & 0xffff;
                if (chunk == 1)
                    *dst = file->buffer2[o];
                else
                    memcpy(dst, file->buffer2 + o, chunk);
                dst += chunk;
                HAPI_SeekFile(file, file->pos + chunk);
                remaining -= chunk;
                i++;
            }
            goto done;
        }
        // Read through file->info, not the cached info local.
        int off = file->pos + file->info->offset;
        if (off != item->pos) {
            host_file_seek(item->fp, off, HOST_SEEK_SET);
            file->shared->pos = off;
        }
        n = host_file_read(file->shared->fp, buf, n);
        unsigned char key = file->shared->node->obfuscate;
        if (key) {
            for (int i = 0; i < n; i++)
                buf[i] = (unsigned char)(buf[i] ^ 0xff ^ (unsigned char)(i + off) ^ key);
        }
        if (n < 0)
            goto done;
        file->pos += n;
        file->shared->pos += n;
        goto done;
    }
    n = host_file_read(file->fp, buf, size);
done:
    return n;
}

// FUNCTION: 0x4bbbe0
unsigned int __stdcall HAPI_WriteFile(FileHandle* file, void* buf, unsigned int size)
{
    if (file->shared != 0)
        return 0xffffffff;
    return host_file_write(file->fp, buf, size);
}

// FUNCTION: 0x4bbc10
void __stdcall RenameFile(const char* oldName, const char* newName)
{
    rename(oldName, newName);
}

// __stdcall wrapper around the CRT's remove (delete a file), like its
// neighbour 0x4bbc10 (rename). remove and _rmdir are the same code apart from
// the function they import (DeleteFileA, RemoveDirectoryA); the original
// calls remove at 0x4e7990.
// FUNCTION: 0x4bbc30
void __stdcall RemoveFile(const char* path)
{
    remove(path);
}

// Closes a file handle opened by HAPI_OpenFile and returns its size.
// Sibling of 0x4bbd00, which reports the size without closing.
// FUNCTION: 0x4bbc40
long __stdcall HAPI_FileLengthByName(char* name)
{
    FileHandle* h = (FileHandle*)HAPI_OpenFile(name, "rb");
    if (h == 0)
        return 0;

    long len;
    if (h->shared != 0)
        len = h->info->size;
    else if (h->fp != 0)
        len = host_file_size(h->fp);
    else
        len = 0;

    if (h->shared != 0) {
        h->shared->count--;
        if (h->shared->count == 0 && h->shared->mode == 0) {
            host_file_close(h->shared->fp);
            h->shared->fp = 0;
        }
    } else {
        host_file_close(h->fp);
    }

    if (h->buffer != 0)
        GameFreeThunk(h->buffer);
    if (h->buffer2 != 0)
        GameFreeThunk(h->buffer2);
    GameFreeThunk(h);

    return len;
}

// FUNCTION: 0x4bbd00
long __stdcall HAPI_FileLength(FileHandle* file)
{
    if (file->shared != 0)
        return file->info->size;
    if (file->fp != 0)
        return host_file_size(file->fp);
    return 0;
}

// Opens a file, seeks to an offset and reads a block into the caller's
// buffer; on success the buffer is returned, otherwise null. The file handle
// is closed (and its shared refcount dropped) on both paths. The refcount,
// fclose and buffer-free blocks are the inlined body of 0x4bb5d0.
// FUNCTION: 0x4bbd30
void* __stdcall HAPI_ReadFileAt(char* name, void* buffer, long pos, unsigned int size)
{
    FileHandle* file = HAPI_OpenFile(name, "rb");
    if (file == 0) {
        return 0;
    }
    if (HAPI_SeekFile(file, pos) == -1) {
        goto fail;
    }
    if (HAPI_readfromfile(file, (unsigned char*)buffer, size) <= 0) {
        goto fail;
    }
    if (file->shared != 0) {
        file->shared->count--;
        if (file->shared->count == 0 && file->shared->mode == 0) {
            host_file_close(file->shared->fp);
            file->shared->fp = 0;
        }
    } else {
        host_file_close(file->fp);
    }
    if (file->buffer != 0) {
        GameFreeThunk(file->buffer);
    }
    if (file->buffer2 != 0) {
        GameFreeThunk(file->buffer2);
    }
    GameFreeThunk(file);
    return buffer;

fail:
    if (file->shared != 0) {
        file->shared->count--;
        if (file->shared->count == 0 && file->shared->mode == 0) {
            host_file_close(file->shared->fp);
            file->shared->fp = 0;
        }
    } else {
        host_file_close(file->fp);
    }
    if (file->buffer != 0) {
        GameFreeThunk(file->buffer);
    }
    if (file->buffer2 != 0) {
        GameFreeThunk(file->buffer2);
    }
    GameFreeThunk(file);
    return 0;
}

// Loads a whole file into a named heap block: opens the name (falling back to
// an already loaded item through HAPI_OpenFile), takes the length from the
// shared item's info block or from the file descriptor, rewinds, copies the
// name into a local buffer, strips the directory from it (the same in-place
// strip as 0x4bb150, so the leaf name is what names the block), allocates the
// block, reads into it and stores the length through `size`. `size` is set to
// -1 first, and again after the open, so a caller that only wanted the size
// sees a failure. The tail is HAPI_CloseFile (close, release the two block
// pointers, free the handle) written out inline, with its fclose results dead
// because the return value is the block.
// FUNCTION: 0x4bbe50
char* __stdcall HAPI_LoadFile(char* name, int* size)
{
    char buf[0x100];
    char* data = 0;
    FileHandle* f;
    int len;

    if (size) {
        *size = -1;
    }
    f = HAPI_OpenFile(name, "rb");
    if (!f) {
        return 0;
    }
    if (size) {
        *size = -1;
    }
    if (f->shared) {
        len = f->info->size;
    } else if (f->fp) {
        len = host_file_size(f->fp);
    } else {
        len = 0;
    }
    // An if / else if / else chain with the failure arms first, not gotos or success-first.
    if (len <= 0) {
        data = 0;
    } else if (HAPI_SeekFile(f, 0) == -1) {
        data = 0;
    } else {
        strcpy(buf, name);
        {
            // Two indices into the same array: pointers would peel the first iteration.
            int n = (int)strlen(buf);
            int i = n - 1;
            while (i >= 0 && buf[i] != '\\') {
                i--;
            }
            int j = i + 1;
            int k = 0;
            do {
                buf[k] = buf[j];
                k++;
            } while (buf[j++] != 0);
        }
        data = (char*)GameAllocIgnoreTag(buf, len);
        if (HAPI_readfromfile(f, (unsigned char*)data, len) <= 0) {
            GameFreeThunk(data);
            data = 0;
            goto close;
        }
        if (size) {
            *size = len;
        }
    }
close:
    if (f->shared != 0) {
        f->shared->count--;
        if (f->shared->count == 0 && f->shared->mode == 0) {
            host_file_close(f->shared->fp);
            f->shared->fp = 0;
        }
    } else {
        host_file_close(f->fp);
    }
    if (f->buffer != 0) {
        GameFreeThunk(f->buffer);
    }
    if (f->buffer2 != 0) {
        GameFreeThunk(f->buffer2);
    }
    GameFreeThunk(f);
    return data;
}

// Loads a whole file into a freshly allocated buffer. It takes the length from
// the file handle (cached size when the handle is shared, else _filelength),
// rejects an empty file, seeks to the start, copies the path into a local
// buffer, strips the directory with a backward scan for '\\', allocates that
// many bytes tagged with the base name, reads the file into them and returns
// the buffer. The size is written back through the third argument, which is
// preset to -1 so a failure leaves it at -1.
// FUNCTION: 0x4bbff0
void* __stdcall HAPI_LoadOpenFile(char* name, FileHandle* file, unsigned int* outSize)
{
    long len;
    char buf[256];
    void* data;
    int n;
    int i;
    int j;
    int k;

    if (outSize != 0) {
        *outSize = 0xffffffff;
    }
    if (file->shared != 0) {
        len = file->info->size;
    } else if (file->fp != 0) {
        len = host_file_size(file->fp);
    } else {
        len = 0;
    }
    if (len <= 0) {
        return 0;
    }
    if (HAPI_SeekFile(file, 0) == -1) {
        return 0;
    }
    strcpy(buf, name);
    n = strlen(buf);
    i = n - 1;
    while (i >= 0 && buf[i] != '\\') {
        i--;
    }
    j = i + 1;
    k = 0;
    // Two indices into the same array, not pointers.
    do {
        buf[k] = buf[j];
        k++;
    } while (buf[j++] != 0);
    data = GameAllocIgnoreTag(buf, len);
    if (HAPI_readfromfile(file, (unsigned char*)data, len) <= 0) {
        GameFreeThunk(data);
        return 0;
    }
    if (outSize != 0) {
        *outSize = len;
    }
    return data;
}

// Reads a whole file into the caller's buffer: open "rb", take its size, seek
// to the start and read that many bytes. The buffer is returned when the read
// succeeds; a read of zero bytes frees it and returns null. On any failure the
// file is closed and null is returned.
// FUNCTION: 0x4bc120
int* __stdcall HAPI_LoadFileInto(char* name, int* buf)
{
    FileHandle* file = HAPI_OpenFile(name, "rb");
    long size;
    if (file == 0)
        goto fail;
    if (file->shared != 0)
        size = file->info->size;
    else if (file->fp != 0)
        size = host_file_size(file->fp);
    else
        size = 0;
    if (size <= 0)
        goto fail;
    if (HAPI_SeekFile(file, 0) < 0)
        goto fail;
    size = HAPI_readfromfile(file, (unsigned char*)buf, size);
    if (size < 0)
        goto fail;
    if (file->shared != 0) {
        file->shared->count--;
        if (file->shared->count == 0 && file->shared->mode == 0) {
            host_file_close(file->shared->fp);
            file->shared->fp = 0;
        }
    } else {
        host_file_close(file->fp);
    }
    if (file->buffer != 0)
        GameFreeThunk(file->buffer);
    if (file->buffer2 != 0)
        GameFreeThunk(file->buffer2);
    GameFreeThunk(file);
    if (size <= 0) {
        GameFreeThunk(buf);
        return 0;
    }
    return buf;
fail:
    if (file != 0) {
        if (file->shared != 0) {
            file->shared->count--;
            if (file->shared->count == 0 && file->shared->mode == 0) {
                host_file_close(file->shared->fp);
                file->shared->fp = 0;
            }
        } else {
            host_file_close(file->fp);
        }
        if (file->buffer != 0)
            GameFreeThunk(file->buffer);
        if (file->buffer2 != 0)
            GameFreeThunk(file->buffer2);
        GameFreeThunk(file);
    }
    return 0;
}

// Writes a buffer to a new file; returns the number of bytes written or -1.
// FUNCTION: 0x4bc290
int __stdcall WriteBufferToFile(char* filename, void* data, int size)
{
    void* f = host_file_create(filename, 0);
    if (f == NULL) {
        return -1;
    }
    int written = host_file_write(f, data, size);
    host_file_close(f);
    return written;
}

// Writes the current drive letter ("C") into buf as a string.
// FUNCTION: 0x4bc2e0
void __stdcall GetCurrentDriveLetter(char* buf)
{
    buf[0] = _getdrive() + '@';
    buf[1] = 0;
}

// FUNCTION: 0x4bc300
int __stdcall ChangeDrive(char* drive)
{
    if (drive == 0) {
        return -1;
    }
    return _chdrive(*drive - 0x40);
}

// Gets the current directory of a drive ("C:..." or the current drive when
// null) into buf and returns buf. The drive number is computed on each path
// (`- '@'` in both branches, tail-merged into one `sub`); a single `d - '@'`
// after the if gives `add eax, -0x40` scheduled after the first push.
// FUNCTION: 0x4bc320
char* __stdcall GetDriveDirectory(char* drive, char* buf, int size)
{
    int n;
    if (drive == NULL)
        n = (char)(_getdrive() + '@') - '@';
    else
        n = *drive - '@';
    _getdcwd(n, buf, size);
    return buf;
}

// __stdcall wrapper around the CRT's _chdir, like 0x4bbc30 (_rmdir).
// FUNCTION: 0x4bc360
void __stdcall ChangeDirectory(const char* path)
{
    _chdir(path);
}

// Case-insensitive wildcard match of a string against a pattern that may
// contain '?' (any one character) and '*' (zero or more characters). The
// pattern positions still alive after each input character are kept in a
// stack of at most 100 entries, so a '*' backtracks in linear time.
// Needed for the operand order of the pat[stack[i]] load.
// FUNCTION: 0x4bc370
int __stdcall MatchWildcard(const char* str, const char* pat)
{
    int stack[100];
    int n;
    int i;
    char c;

    stack[0] = 0;
    n = 1;
    while ((c = toupper(*str++)) != 0) {
        for (i = 0; i < n; i++) {
            int idx = stack[i];
            // Subscripted by stack[i], not idx: the element must be read from memory.
            char pc = toupper(pat[stack[i]]);
            if (pc == '?' || pc == c) {
                stack[i] = idx + 1;
            } else if (pc == '*') {
                if (n < 100)
                    stack[n++] = idx + 1;
            } else {
                if (--n == 0)
                    return 0;
                stack[i] = stack[n];
                i--;
            }
        }
    }
    for (i = 0; i < n; i++) {
        char pc = pat[stack[i]];
        if (pc == 0)
            return 1;
        if (pc == '*' && pat[stack[i] + 1] == 0)
            return 1;
    }
    return 0;
}


#pragma pack(push, 1)
struct FindFiles {
    char dir[0x100];         // +0x000
    char pattern[0x100];     // +0x100
    int state;
    char recursive;          // +0x204
    union {
        void* handle;
        ArchiveDirectory* list;
    };
    int index;               // +0x209
};
#pragma pack(pop)

void* __cdecl GameAllocIgnoreTag(char* name, unsigned int size);
void __cdecl GameFreeThunk(void* p);
int __stdcall HAPI_FindNext(FindFiles* f, struct _finddata_t* fd);

// Strips the directory from a path in place ("a\\b\\c.txt" -> "c.txt").
static char* StripDir(char* path)
{
    int len = (int)strlen(path);
    int i = len - 1;
    while (i >= 0 && path[i] != '\\')
        i--;
    int j = i + 1;
    int k = 0;
    do {
        path[k] = path[j];
        k++;
    } while (path[j++] != 0);
    return path;
}

static void CopyDirEntry(struct _finddata_t* fd, const struct host_dir_entry* entry)
{
    fd->attrib = entry->attrib;
    fd->time_create = 0;
    fd->time_access = 0;
    fd->time_write = entry->time_write;
    fd->size = entry->size;
    strcpy(fd->name, entry->name);
}

// FUNCTION: 0x4bc4b0
int __stdcall HAPI_FindFirst(const char* path, void* fd, int state, char recursive)
{
    FindFiles* f = (FindFiles*)GameAllocIgnoreTag("Find Files structure", 0x20d);
    strcpy(f->dir, path);
    for (int i = strlen(f->dir); i >= 0; i--) {
        if (f->dir[i] == '\\') {
            f->dir[i + 1] = '\0';
            break;
        }
    }
    strcpy(f->pattern, path);
    StripDir(f->pattern);
    if (strcmp(f->pattern, "*.*") == 0)
        strcpy(f->pattern, "*");
    f->recursive = recursive;
    f->state = state;
    if (state == -1) {
        struct host_dir_entry entry;
        f->handle = host_dir_first(path, &entry);
        if (f->handle != 0) {
            CopyDirEntry((struct _finddata_t*)fd, &entry);
            return (int)f;
        }
        if (f->recursive == 0)
            goto fail;
        f->state = 0;
    }
    f->index = -1;
    if (HAPI_FindNext(f, (struct _finddata_t*)fd) != -1)
        return (int)f;
fail:
    GameFreeThunk(f);
    return -1;
}


ArchiveDirectory* __stdcall HAPI_FindDirectory(ArchiveDirectory* list, char* path);

// FUNCTION: 0x4bc640
int __stdcall HAPI_FindNext(FindFiles* f, struct _finddata_t* fd)
{
    if (f == (FindFiles*)-1 || f == 0)
        return -1;
    if (f->state < 0) {
        struct host_dir_entry entry;
        if (host_dir_next(f->handle, &entry)) {
            CopyDirEntry(fd, &entry);
            return 0;
        }
        host_dir_close(f->handle);
        f->state = 0;
        f->index = -1;
        if (!f->recursive)
            return -1;
    }
    DisplayContext* g = GetDisplay();
    // Both latch statements stay in the increment clause.
    for (; f->state < g->count; f->index = -1, f->state++) {
        if (f->index < 0) {
            f->list = HAPI_FindDirectory(g->files[f->state]->node->list, f->dir);
            if (f->list == 0)
                continue;
        }
        while (++f->index < f->list->count) {
            // The index sits in a local: read inline, the entry address operands swap.
            int i = f->index;
            ArchiveEntry* e = &f->list->entries[i];
            if (MatchWildcard(e->text, f->pattern) && !(e->flags & 2)) {
                if (e->flags & 1) {
                    fd->attrib = 0x11;
                    fd->size = 0;
                } else {
                    fd->attrib = 1;
                    fd->size = e->info->size;
                }
                fd->time_create = 0;
                fd->time_access = 0;
                fd->time_write = 0;
                strcpy(fd->name, e->text);
                return 0;
            }
        }
        if (!f->recursive)
            return -1;
    }
    return -1;
}

// Closes the search handle (if open) and frees the search; -1 for no search.
// FUNCTION: 0x4bc8d0
int __stdcall HAPI_FindClose(FindFiles* f)
{
    if (f == (FindFiles*)-1 || f == 0)
        return -1;
    int result;
    if (f->state < 0) {
        host_dir_close(f->handle);
        result = 0;
    } else
        result = 0;
    GameFreeThunk(f);
    return result;
}

// FUNCTION: 0x4bc930
int __stdcall CountDirectoryEntries(const char* path, int flag)
{
    struct _finddata_t fd;
    int count = 0;
    int handle = HAPI_FindFirst(path, &fd, -1, 1);

    if (handle != -1) {
        do {
            if (strcmp(fd.name, ".") != 0 && strcmp(fd.name, "..") != 0
                && (flag == 0 || (fd.attrib & 0x10))) {
                count++;
            }
        } while (HAPI_FindNext((FindFiles*)handle, &fd) != -1);
        if (handle != 0) {
            FindFiles* h = (FindFiles*)handle;
            if (h->state < 0)
                host_dir_close(h->handle);
            GameFreeThunk(h);
        }
    }
    return count;
}

#include "string_ref.h"

typedef std::vector<StringRef> FileList;

// FUNCTION: 0x4bca30
void __stdcall ListDirectory(const char* path, int dirsOnly, FileList* list)
{
    struct _finddata_t fd;
    int h = HAPI_FindFirst(path, &fd, -1, 1);
    if (h != -1) {
        do {
            if (strcmp(fd.name, ".") != 0 && strcmp(fd.name, "..") != 0
                && (dirsOnly == 0 || (fd.attrib & 0x10) != 0)) {
                StringRef key(fd.name);
                list->insert(list->end(), 1, key);
            }
        } while (HAPI_FindNext((FindFiles*)h, &fd) != -1);
        if (h != 0) {
            FindFiles* f = (FindFiles*)h;
            if (f->state < 0)
                host_dir_close(f->handle);
            GameFreeThunk(f);
        }
    }
}

static inline int Next(int h, struct _finddata_t* fd) { int r = HAPI_FindNext((FindFiles*)h, fd); return r; }

// Recursive directory walk: opens path with the search 0x4bc4b0 and, for every
// entry whose name is neither "." nor "..", either descends into the
// subdirectory or, when the name matches the wildcard pattern, appends
// "path\\name" to the vector passed in. The search handle is closed at the end
// (the inlined body of 0x4bc8d0).
// FUNCTION: 0x4bcb50
void __stdcall FindFilesRecursive(char* path, const char* pat, FileList* tree, int state, int recursive)
{
    char buf[0x100];
    struct _finddata_t fd;

    sprintf(buf, "%s\\*", path);
    int h = HAPI_FindFirst(buf, &fd, state, recursive);
    if (h != -1) {
        // A second variable, cast from h: the -1 test is dropped if written on h.
        FindFiles* f = (FindFiles*)h;
        // Never read: the extra live value demotes the handle from ebx to ebp.
        int r;
        do {
            if (strcmp(fd.name, ".") != 0 && strcmp(fd.name, "..") != 0) {
                if ((fd.attrib & 0x10) != 0) {
                    sprintf(buf, "%s\\%s", path, fd.name);
                    FindFilesRecursive(buf, pat, tree, f->state, 0);
                } else if (MatchWildcard(fd.name, pat)) {
                    sprintf(buf, "%s\\%s", path, fd.name);
                    StringRef key(buf);
                    tree->insert(tree->end(), 1, key);
                }
            }
        } while ((r = HAPI_FindNext((FindFiles*)h, &fd)) != -1);
        if (f != (FindFiles*)-1 && f != 0) {
            if (f->state < 0)
                host_dir_close(f->handle);
            GameFreeThunk(f);
        }
    }
}

// Walks the search opened by HAPI_FindFirst and stops on the `index`-th entry
// that is neither "." nor ".." (skipping entries that fail the flag test when
// `flag` is set). The close of the search is HAPI_FindClose inlined.
// FUNCTION: 0x4bcd10
void __stdcall GetDirectoryEntry(struct _finddata_t* fd, int index, const char* pattern, int flag)
{
    int count = 0;
    FindFiles* handle = (FindFiles*)HAPI_FindFirst(pattern, fd, -1, 1);
    if (handle != (FindFiles*)-1) {
        do {
            if (strcmp(fd->name, ".") != 0 && strcmp(fd->name, "..") != 0) {
                if (flag == 0 || (fd->attrib & 0x10) != 0) {
                    if (count == index)
                        break;
                    count++;
                }
            }
        } while (HAPI_FindNext(handle, fd) != -1);
        if (handle != 0) {
            if (handle->state < 0)
                host_dir_close(handle->handle);
            GameFreeThunk(handle);
        }
    }
}


// Stores the current directory of the current drive. The drive number is
// converted on each path separately (`n = d - '@'` in both branches); a
// single `d - '@'` after the if picks edx and interleaves the subtraction
// with the argument pushes.
// FUNCTION: 0x4bce10
void SaveStartDirectory()
{
    DisplayContext* g = GetDisplay();
    char buf[12];
    char d = _getdrive() + '@';
    char* p = g->cwd;
    int n;
    if (buf != NULL)
        n = d - '@';
    else
        n = (char)(_getdrive() + '@') - '@';
    _getdcwd(n, p, 0x100);
}

// FUNCTION: 0x4bce60
int __stdcall SetLastDirectory(char* path)
{
    DisplayContext* base = GetDisplay();
    int result = _chdir(path);
    if (result == 0) {
        strncpy(base->lastDir, path, 0x100);
    }
    return result;
}

// FUNCTION: 0x4bcea0
void __stdcall GetLastDirectory(char* dst)
{
    DisplayContext* s = GetDisplay();
    strncpy(dst, s->lastDir, 0x100);
}

// FUNCTION: 0x4bcec0
void __stdcall GetStartDirectory(char* param)
{
    DisplayContext* p = GetDisplay();
    char* src = p->cwd;
    strncpy(param, src, 0x100);
}

// FUNCTION: 0x4bcee0
void RestoreStartDirectory() {
    DisplayContext* p = GetDisplay();
    _chdir(p->cwd);
}

// Creates every directory along a path (like "mkdir -p").
// FUNCTION: 0x4bcf00
void __stdcall MakeDirectoryPath(char* path)
{
    char buf[260];
    strcpy(buf, path);
    for (char* p = buf; *p; p++) {
        if (*p == '\\' || *p == '/') {
            *p = 0;
            _mkdir(buf);
            *p = '\\';
        }
    }
    _mkdir(buf);
}


// Drops one reference on a handle, closing and freeing whatever it still holds.
static void Close_004bcf80(FileHandle* f)
{
    if (f->shared) {
        f->shared->count--;
        if (f->shared->count == 0 && f->shared->mode == 0) {
            host_file_close(f->shared->fp);
            f->shared->fp = 0;
        }
    } else {
        host_file_close(f->fp);
    }
    if (f->buffer)
        GameFreeThunk(f->buffer);
    if (f->buffer2)
        GameFreeThunk(f->buffer2);
    GameFreeThunk(f);
}

// Copies a file into an already open destination handle, 0x19000 bytes at a
// time, and returns the number of bytes copied (0 on failure). The size to
// copy comes from the source handle: the cached block size when it is one of
// the game's packed items, else _filelength of the real file, else 0, and an
// empty source is a failure. The destination has to be a real file: when it
// is a packed item itself the write is refused by counting -1 bytes written,
// which never matches the chunk size.
// FUNCTION: 0x4bcf80
int __stdcall HAPI_CopyIntoFile(FileHandle* dst, char* name)
{
    int len;
    int left;
    int got;
    int written;
    void* buf = 0;
    FileHandle* f = HAPI_OpenFile(name, "rb");
    if (f == 0)
        return 0;
    if (f->shared)
        len = f->info->size;
    else if (f->fp)
        len = host_file_size(f->fp);
    else
        len = 0;
    if (len == 0)
        goto fail;
    if (HAPI_SeekFile(f, 0) == -1)
        goto fail;
    buf = GameAllocIgnoreTag("COPY BUFFER", 0x19000);
    left = len;
    while (left != 0) {
        got = HAPI_readfromfile(f, (unsigned char*)buf, 0x19000);
        if (got <= 0)
            goto fail;
        if (dst->shared)
            written = -1;
        else
            written = (int)host_file_write(dst->fp, buf, got);
        if (written != got)
            goto fail;
        left -= got;
    }
    Close_004bcf80(f);
    // Returns len, kept separate from the loop counter left; not dst.
    return len;
fail:
    if (buf)
        GameFreeThunk(buf);
    Close_004bcf80(f);
    return 0;
}

// FUNCTION: 0x4bd150
int __stdcall HAPI_PackageBuildNullStub(int, int)
{
    return 0;
}

// The package buffer 0x4bd160 and 0x4bd3b0 grow: the current size and the
// block of data.
struct HapiBuf {
    unsigned int size;
    char* buf;
};

// The 20-byte "HAPI" header written at the front of a package.
struct Hapi_004bd160 {
    char magic[4];
    char a4;
    char a5;
    char a6;
    char a7;
    unsigned int size;
    unsigned char key;
    char ad;
    unsigned short ae;
    int extra;
};

void* __cdecl GameReallocTagged(void* p, const char* name, unsigned int size);
void __stdcall HAPI_WriteArchiveData(char* path, char* base, int off, void* f,
                           void (__cdecl* cb)(unsigned), unsigned extra,
                           int key, int flags);
unsigned int __stdcall HAPI_BuildArchiveDirectory(char* path, HapiBuf* out, int* total);

static inline unsigned int Grow(HapiBuf* b, unsigned int n)
{
    unsigned int old = b->size;
    b->size += n;
    b->buf = (char*)GameReallocTagged(b->buf, "Package Data", b->size);
    return old;
}

// Writes an HPI package: a 20-byte "HAPI"
// header in a growing buffer {size, buf}, the directory built by HAPI_BuildArchiveDirectory,
// the file data from HAPI_WriteArchiveData, the directory encrypted with the key, and
// a copyright line at the end.
#include <time.h>

// FUNCTION: 0x4bd160
int __stdcall HAPI_PackDirectory(char* srcname, char* dstname, void (__cdecl* cb)(int),
                           unsigned int key, int flags)
{
    char* extra = 0;
    char year[8];
    char copyright[0x40];
    int off;
    void* f;
    int i, n;
    unsigned char* p;

    if (cb)
        cb(0);
    struct HapiBuf sb;
    memset(&sb, 0, sizeof(sb));
    Grow(&sb, 20);
    off = HAPI_BuildArchiveDirectory(srcname, &sb, (int*)&extra);

    {
        struct Hapi_004bd160* h = (struct Hapi_004bd160*)sb.buf;
        unsigned int m;
        unsigned int t;
        unsigned char k;
        strncpy(sb.buf, "HAPI", 4);
        h->a4 = 0;
        h->a5 = 0;
        h->a6 = 1;
        h->a7 = 0;
        h->size = sb.size;
        m = key & 0xff;
        if ((unsigned char)key == 0)
            k = 0;
        else {
            t = (m >> 2) | (m << 6);
            k = ~t;
        }
        h->key = k;
        h->ad = 0;
        h->ae = 0;
        h->extra = off;
    }

    f = host_file_create(dstname, 0);
    if (!f) {
        if (sb.buf)
            GameFreeThunk(sb.buf);
        return 0;
    }
    host_file_write(f, sb.buf, sb.size);
    if (cb)
        cb(5);
    HAPI_WriteArchiveData(srcname, sb.buf, off, f, (void (__cdecl*)(unsigned))cb, (unsigned)extra, key, flags);
    if (cb)
        cb(0x5f);
    n = (int)sb.size - 20;
    p = (unsigned char*)sb.buf + 20;
    if ((unsigned char)key) {
        for (i = 0; i < n; i++)
            p[i] = (char)~((unsigned char)(i + 20) ^ (unsigned char)key ^ p[i]);
    }
    host_file_seek(f, 0, HOST_SEEK_SET);
    host_file_write(f, sb.buf, sb.size);
    {
    time_t now;
    struct tm* t;
    now = time(0);
    t = localtime(&now);
    sprintf(year, "%i", t->tm_year + 1900);
    strcpy(copyright, "Copyright 0000 Cavedog Entertainment");
    strncpy(strstr(copyright, "0000"), year, 4);
    host_file_seek(f, 0, HOST_SEEK_END);
    host_file_write(f, copyright, strlen(copyright));
    host_file_close(f);
    if (sb.buf)
        GameFreeThunk(sb.buf);
    }
    return 1;
}

#pragma pack(push, 1)
// A file's record in a package directory: the offset of its data and its size.
struct Node_004bd3b0 {
    unsigned int offset; // +0
    unsigned int size;   // +4
    unsigned char flags; // +8
};
#pragma pack(pop)

extern char g_packageDataName[];  // "Package Data"
extern char g_dirWildcard[];  // "\\*"
extern char g_star[];        // "*"
extern char g_dotDot[];      // ".."
extern char g_dotExtSep[];   // "."
extern char g_pathSepBackslash[];  // "\\"

// Builds one package directory in the growing buffer `out` (size,
// pointer): a header {count, entries offset}, then one 9-byte entry per file
// or subdirectory (name offset, data offset, directory bit), recursing into
// subdirectories and adding a 9-byte file record per file. Returns the
// header's offset; *total collects the file sizes.
// FUNCTION: 0x4bd3b0
unsigned int __stdcall HAPI_BuildArchiveDirectory(char* path, HapiBuf* out, int* total)
{
    char buf[0x104];
    struct _finddata_t fd;
    char* base;
    FindFiles* h;
    int trailing;
    unsigned int root;
    unsigned int entries;
    int k;

    // Written out, not through Grow: Grow loads out->size into the wrong register.
    unsigned int nsize = out->size;
    root = nsize;
    nsize += 8;
    out->size = nsize;
    base = (char*)GameReallocTagged(out->buf, g_packageDataName, nsize);
    out->buf = base;
    *(unsigned int*)(base + root) = 0;

    strcpy(buf, path);
    if (buf[strlen(buf) - 1] != '\\') {
        strcat(buf, g_dirWildcard);
        trailing = 0;
    } else {
        strcat(buf, g_star);
        trailing = 1;
    }

    h = (FindFiles*)HAPI_FindFirst(buf, &fd, -1, 1);
    if (h != (FindFiles*)-1) {
        do {
            if (strcmp(fd.name, g_dotExtSep) != 0 && strcmp(fd.name, g_dotDot) != 0)
                ++(*(unsigned int*)(base + root));
        } while (HAPI_FindNext(h, &fd) == 0);
        if (h != 0) {
            if (h->state < 0)
                host_dir_close(h->handle);
            GameFreeThunk(h);
        }
    }

    entries = Grow(out, *(unsigned int*)(base + root) * 9);
    *(unsigned int*)(out->buf + root + 4) = entries;

    h = (FindFiles*)HAPI_FindFirst(buf, &fd, -1, 1);
    if (h != (FindFiles*)-1) {
        k = 0;
        do {
            if (strcmp(fd.name, g_dotExtSep) != 0 && strcmp(fd.name, g_dotDot) != 0) {
                unsigned int nameOff = Grow(out, strlen(fd.name) + 1);
                strcpy(out->buf + nameOff, fd.name);
                // Two steps: a one-expression pointer gets regrouped as (entries + k) + buf.
                ArchiveEntry* e = (ArchiveEntry*)(out->buf + entries);
                e += k;
                e->name = nameOff;
                e->flags &= 1;
                if (fd.attrib & 0x10) {
                    e->flags |= 1;
                    strcpy(buf, path);
                    if (!trailing)
                        strcat(buf, g_pathSepBackslash);
                    strcat(buf, fd.name);
                    unsigned int sub = HAPI_BuildArchiveDirectory(buf, out, total);
                    e = (ArchiveEntry*)(out->buf + entries);
                    e += k;
                    e->data = sub;
                } else {
                    e->flags &= ~1;
                    unsigned int nodeOff = Grow(out, 9);
                    e = (ArchiveEntry*)(out->buf + entries);
                    e += k;
                    e->data = nodeOff;
                    // Addressed through e->data: this second use of e keeps the store unfolded.
                    Node_004bd3b0* node = (Node_004bd3b0*)(out->buf + e->data);
                    node->size = fd.size;
                    node->offset = 0;
                    node->flags = 0;
                    *total += node->size;
                }
                k++;
            }
        } while (HAPI_FindNext(h, &fd) == 0);
        if (h != 0) {
            if (h->state < 0)
                host_dir_close(h->handle);
            GameFreeThunk(h);
        }
    }
    return root;
}

#pragma pack(push, 1)
struct Node_004bd830 {
    char unknown_0[0xc];
    unsigned char obfuscate;             // +0xc
};
#pragma pack(pop)

// SquashPack, SquashMaxPackedSize, Node_004bd830 and this second
// HAPI_WriteArchiveData declaration are kept for their symbol ids: they hold
// 0x4bb2e0's allocation (docs/c2-regalloc.md).
int __stdcall SquashPack(void* chunk, int* chunkSize, char* data,
                           int size, int method, int encrypt);
unsigned int __stdcall SquashMaxPackedSize(unsigned int value, int mode);
void __stdcall HAPI_WriteArchiveData(char* path, char* base, int off, void* f,
                            void (__cdecl* cb)(unsigned), unsigned extra,
                            int key, int flags);


extern char g_hapiCopyright[];         // "Copyright 0000 Cavedog Entertainment"

// 4bdd70 passes the entry's data offset to the definition (0x4be010), which
// takes it as the directory pointer, so it keeps its own declaration.
void __stdcall HAPI_RelocateDirectory(ArchiveDirectory* h, int delta);
void __stdcall HAPI_RelocateDirectory(int name, int base);

static inline int Bad_004bdd70(void* f, Header* hdr, char* copyright)
{
    if (strncmp(hdr->magic, "HAPI", 4) != 0 || hdr->version[0] != 0 || hdr->version[1] != 0 ||
        hdr->version[2] != 1 || hdr->version[3] != 0)
        return 1;
    int len = strlen(g_hapiCopyright);
    host_file_seek(f, -len, HOST_SEEK_END);
    host_file_read(f, copyright, len);
    copyright[len] = 0;
    strncpy(copyright + (strstr(g_hapiCopyright, "0000") - g_hapiCopyright), "0000", 4);
    if (strcmp(copyright, g_hapiCopyright) == 0)
        return 0;
    return 1;
}

// FUNCTION: 0x4bdd70
OPENHAPIFILE* __stdcall HAPI_OpenArchive(const char* name, int mode)
{
    void* f = host_file_open_read(name);
    if (f == 0)
        return 0;
    OPENHAPIFILE* h = (OPENHAPIFILE*)GameAllocIgnoreTag("OPENHAPIFILE structure", 0x118);
    h->fp = f;
    h->pos = -1;
    h->count = 0;
    h->mode = mode;
    host_file_full_path(name, h->name, 0x104);
    Header hdr;
    host_file_read(f, &hdr, 0x14);
    char copyright[0x40];
    if (Bad_004bdd70(f, &hdr, copyright)) {
        host_file_close(f);
        GameFreeThunk(h);
        return 0;
    }
    h->node = (Header*)GameAllocIgnoreTag("HAPIFILE header", hdr.size);
    host_file_seek(f, 0, HOST_SEEK_SET);
    host_file_read(f, h->node, hdr.size);
    {
        Header* base = h->node;
        // Key derivation: a byte key for the condition, an unsigned w for the split rotate,
        // and a second byte local r receiving the result.
        unsigned char key = base->obfuscate;
        unsigned int w = key;
        unsigned char r = key;
        if (key) { unsigned int hi = w >> 6; w = w << 2; hi = hi | w; r = (unsigned char)~hi; }
        base->obfuscate = r;
        hdr.obfuscate = h->node->obfuscate;
        {
        unsigned char k;
        int i;
        int n;
        unsigned char* p;
        p = (unsigned char*)h->node;
        p += 0x14;
        k = hdr.obfuscate;
        n = hdr.size - 0x14;
        if (k != 0) {
            for (i = 0; i < n; i++)
                p[i] = (unsigned char)((i + 0x14) ^ k ^ ~p[i]);
        }
        }
        base = h->node;
        base->list = (ArchiveDirectory*)((char*)base->list + (int)base);
        Header* b = h->node;
        ArchiveDirectory* t = b->list;
        t->entries = (ArchiveEntry*)((char*)t->entries + (int)b);
        for (int i = t->count - 1; i >= 0; i--) {
            ArchiveEntry* e = &t->entries[i];
            e->name += (int)b;
            e->data += (int)b;
            if (e->flags & 1)
                HAPI_RelocateDirectory(e->data, (int)b);
        }
    }
    if (mode == 0) {
        host_file_close(f);
        h->fp = 0;
    }
    return h;
}

// FUNCTION: 0x4be010
void __stdcall HAPI_RelocateDirectory(ArchiveDirectory* h, int delta)
{
    h->entries = (ArchiveEntry*)((int)h->entries + delta);
    for (int i = h->count - 1; i >= 0; i--) {
        ArchiveEntry* p = (ArchiveEntry*)((int)h->entries + i * 9);
        p->name += delta;
        p->data += delta;
        if (p->flags & 1)
            HAPI_RelocateDirectory(p->dir, delta);
    }
}

// Closes a file record: closes its FILE (if open), then frees its buffer at
// +0x8 and the record itself.
// FUNCTION: 0x4be070
void __stdcall HAPI_CloseArchive(OPENHAPIFILE* f)
{
    if (f != 0) {
        if (f->fp != 0)
            host_file_close(f->fp);
        GameFreeThunk(f->node);
        GameFreeThunk(f);
    }
}

extern char g_hapiFileArrayName[];     // "HAPIFILE array"

// FUNCTION: 0x4be0b0
void* __stdcall HAPI_AddArchive(LPCSTR name, int mode)
{
    DisplayContext* display = GetDisplay();
    char fullPath[0x100];
    host_file_full_path(name, fullPath, 0x100);

    for (int i = 0; i < display->count; i++) {
        if (_strcmpi(fullPath, display->files[i]->name) == 0)
            return 0;
    }

    void* file = HAPI_OpenArchive(name, mode);
    if (!file)
        return 0;

    display->files = (OPENHAPIFILE**)GameReallocTagged(display->files, g_hapiFileArrayName, display->count * 4 + 4);
    display->files[display->count] = (OPENHAPIFILE*)file;
    display->count++;
    return file;
}

static void FreeRecord_004be180(OPENHAPIFILE* p)
{
    if (p != 0) {
        if (p->fp != 0)
            host_file_close(p->fp);
        GameFreeThunk(p->node);
        GameFreeThunk(p);
    }
}

// Walks the record list and probes each record whose file is not open yet:
// opens the record's filename in "rb" and, if it can be opened, closes it
// again (the record only checks that the file exists). If the open fails the
// record is freed and removed from the list, shifting the tail down.
// FUNCTION: 0x4be180
void HAPI_DropMissingArchives(void)
{
    DisplayContext* state = GetDisplay();
    int i;
    for (i = 0; i < state->count; i++) {
        if (state->files[i]->fp == 0) {
            state->files[i]->fp = host_file_open_read(state->files[i]->name);
            OPENHAPIFILE* rec = state->files[i];
            if (rec->fp == 0) {
                // Passed state->files[i], not rec: keeps the redundant file test.
                FreeRecord_004be180(state->files[i]);
                state->files[i] = 0;
                for (int j = i; j < state->count - 1; j++)
                    state->files[j] = state->files[j + 1];
                state->count--;
                i--;
            } else {
                host_file_close(state->files[i]->fp);
                state->files[i]->fp = 0;
            }
        }
    }
}

// Takes a record out of the state's list: closes the record's file, frees its
// buffer and the record itself, then shifts the tail of the list down over the
// hole and shrinks the count. A record that is not in the list is left alone.
// FUNCTION: 0x4be270
void __stdcall HAPI_RemoveArchive(OPENHAPIFILE* pRecord)
{
    DisplayContext* state = GetDisplay();
    int i;
    for (i = 0; i < state->count; i++) {
        if (state->files[i] == pRecord) {
            OPENHAPIFILE* p = state->files[i];
            if (p != 0) {
                if (p->fp != 0)
                    host_file_close(p->fp);
                GameFreeThunk(p->node);
                GameFreeThunk(p);
            }
            state->files[i] = 0;
            for (int j = i; j < state->count - 1; j++) {
                state->files[j] = state->files[j + 1];
            }
            state->count--;
            return;
        }
    }
}

// FUNCTION: 0x4be3b0
void __stdcall HAPI_ClearShadowFlags(ArchiveDirectory* list)
{
    for (int i = list->count - 1; i >= 0; i--) {
        list->entries[i].flags &= ~2;
        if (list->entries[i].flags & 1)
            HAPI_ClearShadowFlags(list->entries[i].dir);
    }
}


// Walks a directory tree (the search 0x4bc4b0 allocates, the same one 0x4bcb50
// uses) and, for every plain file, marks the matching entry of every open
// HAPI archive: the entry named "path + file name" is looked up with
// HAPI_FindEntry in each archive's directory (from the search's current archive
// index on) and gets bit 2 set unless bit 1 is already set. Sub directories
// other than "." and ".." are entered with the path extended by "name\\".
// FUNCTION: 0x4be400
void __stdcall HAPI_MarkShadowedFiles(char* path, int state, int recursive)
{
    DisplayContext* d = GetDisplay();
    char buf[0x100];
    struct _finddata_t fd;
    int i;

    strcpy(buf, path);
    strcat(buf, "*");
    int h = HAPI_FindFirst(buf, &fd, state, recursive);
    if (h == -1)
        return;
    // A second variable, cast from h: the -1 test is dropped if written on h.
    FindFiles* f = (FindFiles*)h;
    do {
        if (fd.attrib & 0x10) {
            // Two nested ifs, not one &&.
            if (strcmp(fd.name, ".") != 0) {
                if (strcmp(fd.name, "..") != 0) {
                    strcpy(buf, path);
                    strcat(buf, fd.name);
                    strcat(buf, "\\");
                    HAPI_MarkShadowedFiles(buf, f->state, 0);
                }
            }
        } else {
            i = f->state;
            if (i < 0)
                i = 0;
            else
                i++;
            for (; i < d->count; i++) {
                strcpy(buf, path);
                strcat(buf, fd.name);
                ArchiveEntry* e = HAPI_FindEntry(d->files[i]->node->list, buf);
                if (e && !(e->flags & 1))
                    e->flags |= 2;
            }
        }
    } while (HAPI_FindNext((FindFiles*)h, &fd) != -1);
    if (f) {
        if (f->state < 0)
            host_dir_close(f->handle);
        GameFreeThunk(f);
    }
}

// The path helpers before 0x4bb190 (0x4bb0a0 to 0x4bb150, in their own files),
// then three more prototypes: the symbol ids these declarations take keep
// 0x4bb2e0's allocation (docs/c2-regalloc.md).
int __stdcall HasExtension(char* name);
char* __stdcall StripExtension(char* name);
char* __stdcall StripFileName(char* name);
char* __stdcall StripPath(char* name);
void RegisterVtolOrders(void);
void StepAllGafSequences(void);
void ResetNetStats(void);

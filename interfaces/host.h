#ifndef INTERFACES_HOST_H
#define INTERFACES_HOST_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define HOST_CALL __cdecl
#else
#define HOST_CALL
#endif

// Major version of the host interface, changed when a function is removed or its signature changes.
#define HOST_VERSION_MAJOR 1
// Minor version of the host interface, changed when a function is added.
#define HOST_VERSION_MINOR 1

// Timeout for host_event_wait that never expires.
#define HOST_WAIT_FOREVER 0xffffffffu

// Setting scope: a value of the game's own, kept per user.
#define HOST_SCOPE_GAME 0
// Setting scope: a value of the machine, named by a full section path with name NULL for the section's default value.
#define HOST_SCOPE_SYSTEM 1

// Setting scope: a value of the machine that is only read, named like HOST_SCOPE_SYSTEM.
#define HOST_SCOPE_SYSTEM_READ 2

// Setting type: a 32-bit integer.
#define HOST_SETTING_INT 1
// Setting type: a zero-terminated string.
#define HOST_SETTING_STRING 2
// Setting type: a block of bytes.
#define HOST_SETTING_BLOB 3

// Result of host_setting_get and host_setting_set when the section could not be opened.
#define HOST_SETTING_NO_ACCESS (-1)

// Message style: the plain box.
#define HOST_MESSAGE_PLAIN 0
// Message style: the error icon.
#define HOST_MESSAGE_ERROR 1
// Message style: the box blocks every window of the machine until it is answered.
#define HOST_MESSAGE_SYSTEM_MODAL 2
// Message style: the box stays above every other window.
#define HOST_MESSAGE_TOPMOST 4

// Returns a monotonic clock in milliseconds that wraps at 2^32.
unsigned int HOST_CALL host_time_ms(void);
// Returns the high-resolution counter, a monotonic tick count of the host's own unit.
unsigned long long HOST_CALL host_time_counter(void);
// Blocks the calling thread for at least the given milliseconds.
void HOST_CALL host_sleep_ms(unsigned int ms);

// Starts a thread that runs entry(param) with the given stack size and returns 1, or returns 0 when it cannot start.
int HOST_CALL host_thread_start(void (HOST_CALL* entry)(void*), unsigned int stack, void* param);
// Returns an id that is unique to the calling thread and never 0.
unsigned int HOST_CALL host_thread_id(void);
// Stores value in *target as one atomic step and returns what was there before.
int HOST_CALL host_atomic_exchange(int* target, int value);

// Creates a mutex that the same thread may lock again, and returns its handle.
void* HOST_CALL host_mutex_create(void);
// Blocks until the calling thread owns the mutex.
void HOST_CALL host_mutex_lock(void* mutex);
// Gives up one lock of the mutex.
void HOST_CALL host_mutex_unlock(void* mutex);
// Frees the mutex.
void HOST_CALL host_mutex_destroy(void* mutex);

// Creates an event, signalled or not, that a wait resets itself unless manual_reset is set, and returns its handle.
void* HOST_CALL host_event_create(int manual_reset, int signalled);
// Signals the event and releases a thread waiting on it.
void HOST_CALL host_event_set(void* event);
// Clears the event's signal.
void HOST_CALL host_event_reset(void* event);
// Waits up to timeout_ms for the event to be signalled and returns 1 when it was, 0 when the time ran out.
int HOST_CALL host_event_wait(void* event, unsigned int timeout_ms);
// Frees the event.
void HOST_CALL host_event_destroy(void* event);

// Reads a named setting into buf (size holds its capacity and receives the length) and returns 1, 0 when it is missing, or HOST_SETTING_NO_ACCESS.
int HOST_CALL host_setting_get(int scope, const char* section, const char* name, int type, void* buf, unsigned int* size);
// Writes a named setting (size is the byte count of a blob) and returns 1, or HOST_SETTING_NO_ACCESS when a machine section cannot be opened.
int HOST_CALL host_setting_set(int scope, const char* section, const char* name, int type, const void* buf, unsigned int size);

// Shows text in a box titled title over the owner window (NULL for none) and returns when it is answered.
void HOST_CALL host_message(void* owner, const char* title, const char* text, unsigned int style);
// Shows text as an error box when it is not NULL, then ends the program with exit code 1 after the C library's cleanup.
void HOST_CALL host_fatal(void* owner, const char* title, const char* text);
// Ends the program at once with the exit code, running no cleanup.
void HOST_CALL host_exit(int code);
// Writes a line of text to the host's log.
void HOST_CALL host_log(const char* text);

// Opens an existing file under the game directory for reading only and returns its handle, or NULL when it cannot be opened.
void* HOST_CALL host_file_open_read(const char* path);
// Creates a file under the game directory, or empties it when it exists, for reading and writing, and returns its handle or NULL.
void* HOST_CALL host_file_create(const char* path, unsigned int flags);
// Opens a file under the game directory for reading and appending, creating it when it is missing, and returns its handle or NULL.
void* HOST_CALL host_file_open_append(const char* path);
// Reads up to size bytes into buf and returns how many were read, fewer at the end of the file.
unsigned int HOST_CALL host_file_read(void* file, void* buf, unsigned int size);
// Writes size bytes from buf and returns how many were written.
unsigned int HOST_CALL host_file_write(void* file, const void* buf, unsigned int size);
// Moves the file position by offset from origin and returns the new position from the start, or -1 on failure.
long HOST_CALL host_file_seek(void* file, long offset, int origin);
// Returns the length of the file in bytes, or -1 on failure.
long HOST_CALL host_file_size(void* file);
// Sets the length of the file to size bytes and returns 1, or 0 on failure.
int HOST_CALL host_file_truncate(void* file, long size);
// Closes the file and frees its handle.
void HOST_CALL host_file_close(void* file);

// File creation flag: write newlines as the host's text files end their lines.
#define HOST_FILE_TEXT 1

// Seek origin: offset is from the start of the file.
#define HOST_SEEK_SET 0
// Seek origin: offset is from the current position.
#define HOST_SEEK_CUR 1
// Seek origin: offset is from the end of the file.
#define HOST_SEEK_END 2

// Entry attribute bit: the entry is read-only.
#define HOST_ATTR_READONLY 0x01
// Entry attribute bit: the entry is hidden.
#define HOST_ATTR_HIDDEN 0x02
// Entry attribute bit: the entry belongs to the system.
#define HOST_ATTR_SYSTEM 0x04
// Entry attribute bit: the entry is a directory.
#define HOST_ATTR_DIRECTORY 0x10
// Entry attribute bit: the entry is marked for archiving.
#define HOST_ATTR_ARCHIVE 0x20

// One entry of a directory listing.
struct host_dir_entry {
    // The entry's name, without its directory.
    char name[260];
    // The entry's HOST_ATTR_ bits.
    unsigned int attrib;
    // The entry's length in bytes.
    unsigned int size;
    // The entry's last write time, in seconds since 1970-01-01 UTC.
    unsigned int time_write;
};

// Lists the entries under the game directory that match pattern (* and ? wildcards), fills entry with the first, and returns the listing's handle, or NULL when nothing matches.
void* HOST_CALL host_dir_first(const char* pattern, struct host_dir_entry* entry);
// Fills entry with the listing's next entry and returns 1, or returns 0 when there are no more.
int HOST_CALL host_dir_next(void* dir, struct host_dir_entry* entry);
// Ends the listing and frees its handle.
void HOST_CALL host_dir_close(void* dir);

#ifdef __cplusplus
}
#endif

#endif

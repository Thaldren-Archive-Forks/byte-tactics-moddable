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
#define HOST_VERSION_MINOR 9

// Timeout for host_event_wait that never expires.
#define HOST_WAIT_FOREVER 0xffffffffu

// Memory access: pages that can only be read.
#define HOST_MEM_READONLY 0
// Memory access: pages that can be read and written.
#define HOST_MEM_READWRITE 1

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
// Sets the calling thread's priority, from -2 (lowest) through 0 (normal) to 2 (highest).
void HOST_CALL host_thread_set_priority(int level);

// Creates a mutex that the same thread may lock again, and returns its handle.
void* HOST_CALL host_mutex_create(void);
// Blocks until the calling thread owns the mutex.
void HOST_CALL host_mutex_lock(void* mutex);
// Gives up one lock of the mutex.
void HOST_CALL host_mutex_unlock(void* mutex);
// Frees the mutex.
void HOST_CALL host_mutex_destroy(void* mutex);

// Reserves size bytes of address space for read and write access once committed and returns its start, or NULL when none is free.
void* HOST_CALL host_mem_reserve(unsigned int size);
// Commits size bytes at addr inside a reserved range as read-write pages and returns addr, or NULL when it cannot.
void* HOST_CALL host_mem_commit(void* addr, unsigned int size);
// Returns the committed pages at addr to the reserved state.
void HOST_CALL host_mem_decommit(void* addr, unsigned int size);
// Gives back the whole range that host_mem_reserve returned at addr.
void HOST_CALL host_mem_release(void* addr);
// Sets the access of the whole pages at addr and returns nonzero on success.
int HOST_CALL host_mem_protect(void* addr, unsigned int size, int access);
// Allocates size bytes from a heap apart from the C library's and returns them, or NULL when none are free.
void* HOST_CALL host_mem_alloc(unsigned int size);
// Frees a block that host_mem_alloc returned.
void HOST_CALL host_mem_free(void* block);
// Returns 1 when the size bytes at addr can be read, else 0.
int HOST_CALL host_mem_readable(const void* addr, unsigned int size);
// Writes the base and byte size of the running program's image and returns 1, or returns 0 when it cannot be found.
int HOST_CALL host_image_range(const void** base, unsigned int* size);

// Returns the command line the program was started with.
const char* HOST_CALL host_command_line(void);

// Appends a report of the exception (an EXCEPTION_POINTERS on Win32) to ErrorLog.txt beside the program, naming handler as the code that caught it.
void HOST_CALL host_crash_report(void* exception, const char* handler);
// Makes an exception that nothing else catches write the crash report with the handler name "Global Exception Handler".
void HOST_CALL host_crash_install(void);
// Passes value to DebugFunc1 of DebugHelper.dll and returns its result, or shows why it could not and returns value.
int HOST_CALL host_debug_helper(int value);
// Breaks into the debugger if one is attached.
void HOST_CALL host_debug_break(void);

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
// Returns 1 when another copy of the game already holds name, else claims name for this process and returns 0.
int HOST_CALL host_instance_running(const char* name);
// Writes the name of the signed-in user into buf of size bytes and returns its length with the terminator, or 0 on failure (including a buf too small).
unsigned int HOST_CALL host_user_name(char* buf, unsigned int size);

// The machine's physical memory, in bytes.
struct host_memory {
    // The machine's total physical memory.
    unsigned int total;
    // The physical memory that is free.
    unsigned int available;
};

// Fills status with the machine's physical memory.
void HOST_CALL host_memory_status(struct host_memory* status);

// Event type: the window is being destroyed.
#define HOST_EVENT_DESTROY 0x002
// Event type: the window gained (a is nonzero) or lost (a is 0) the focus.
#define HOST_EVENT_FOCUS 0x006
// Event type: the player asked to close the window; the window stays open until host_window_close.
#define HOST_EVENT_CLOSE 0x010
// Event type: a key went down; a is its virtual-key code.
#define HOST_EVENT_KEY_DOWN 0x100
// Event type: a key went up; a is its virtual-key code.
#define HOST_EVENT_KEY_UP 0x101
// Event type: a character was typed; a is its code.
#define HOST_EVENT_CHAR 0x102
// Event type: a key went down while Alt was held, or F10; a is its virtual-key code.
#define HOST_EVENT_SYS_KEY_DOWN 0x104
// Event type: a key went up while Alt was held, or F10; a is its virtual-key code.
#define HOST_EVENT_SYS_KEY_UP 0x105
// Event type: the mouse moved.
#define HOST_EVENT_MOUSE_MOVE 0x200
// Event type: the left button went down.
#define HOST_EVENT_LBUTTON_DOWN 0x201
// Event type: the left button went up.
#define HOST_EVENT_LBUTTON_UP 0x202
// Event type: the left button was double-clicked.
#define HOST_EVENT_LBUTTON_DOUBLE 0x203
// Event type: the right button went down.
#define HOST_EVENT_RBUTTON_DOWN 0x204
// Event type: the right button went up.
#define HOST_EVENT_RBUTTON_UP 0x205
// Event type: the right button was double-clicked.
#define HOST_EVENT_RBUTTON_DOUBLE 0x206
// Event type: the middle button went down.
#define HOST_EVENT_MBUTTON_DOWN 0x207
// Event type: the middle button went up.
#define HOST_EVENT_MBUTTON_UP 0x208
// Event type: the wheel turned; a is the signed distance, 120 per notch.
#define HOST_EVENT_WHEEL 0x20a
// Event type: a device such as a disc was added or removed; a and b are the platform's details.
#define HOST_EVENT_DEVICE_CHANGE 0x219
// Event type: the window may realise its palette.
#define HOST_EVENT_PALETTE_QUERY 0x30f
// Event type: another window changed the system palette; a is nonzero when this window caused it.
#define HOST_EVENT_PALETTE_CHANGED 0x311
// Event type: a media command finished; a and b are the platform's details.
#define HOST_EVENT_MEDIA_NOTIFY 0x3b9

// Button bit in host_event.buttons: the left button is held.
#define HOST_BUTTON_LEFT 0x01
// Button bit in host_event.buttons: the right button is held.
#define HOST_BUTTON_RIGHT 0x02
// Button bit in host_event.buttons: Shift is held.
#define HOST_BUTTON_SHIFT 0x04
// Button bit in host_event.buttons: Ctrl is held.
#define HOST_BUTTON_CONTROL 0x08
// Button bit in host_event.buttons: the middle button is held.
#define HOST_BUTTON_MIDDLE 0x10

// One event from the game window.
struct host_event {
    // The event's HOST_EVENT_ type.
    unsigned int type;
    // The first detail of the type, such as the virtual-key code.
    unsigned int a;
    // The second detail of the type.
    unsigned int b;
    // The mouse's x in the window for mouse events, or on the screen for HOST_EVENT_WHEEL.
    int x;
    // The mouse's y in the window for mouse events, or on the screen for HOST_EVENT_WHEEL.
    int y;
    // The HOST_BUTTON_ bits held, for mouse events.
    unsigned int buttons;
    // host_time_ms when the host received the event.
    unsigned int time_ms;
};

// Opens the game window with the given title and client size, at the screen's top left when fullscreen is set, and returns its native handle or NULL.
void* HOST_CALL host_window_open(const char* title, int width, int height, int fullscreen);
// Destroys the game window, which queues HOST_EVENT_DESTROY.
void HOST_CALL host_window_close(void);
// Returns 1 when the game window has the keyboard focus.
int HOST_CALL host_window_has_focus(void);
// Queues an event of the given type for the game window, to arrive after the platform messages already waiting, with a and b as 0 (for types such as HOST_EVENT_CLOSE).
void HOST_CALL host_window_post(unsigned int type);
// Handles the calling thread's waiting platform messages, first waiting for one when wait is set, and returns 0 when the quit request was among them.
int HOST_CALL host_pump_events(int wait);
// Asks the calling thread's next host_pump_events to return 0.
void HOST_CALL host_post_quit(void);
// Takes the oldest queued game window event into *event and returns 1, or returns 0 when none is queued.
int HOST_CALL host_event_next(struct host_event* event);
// Returns a key's state: bit 15 set while the virtual key vk is held, and bit 0 set when it was pressed since the last call.
int HOST_CALL host_key_state(int vk);
// Stores the mouse cursor's position on the screen.
void HOST_CALL host_cursor_get(int* x, int* y);
// Moves the mouse cursor to a position on the screen.
void HOST_CALL host_cursor_set(int x, int y);

// Frame result: the call worked.
#define HOST_FRAME_OK 0
// Frame result: the surface was lost; the game restores it with host_frame_restore and redraws the whole frame.
#define HOST_FRAME_LOST 1
// Frame result: the call failed, or the display has no such frame.
#define HOST_FRAME_FAILED 2

// Frame selector: the frame the game draws into.
#define HOST_FRAME_BACK 0
// Frame selector: the frame on the screen.
#define HOST_FRAME_FRONT 1

// Display mode flag: set up only the frame on the screen, for a video player that has no display of its own.
#define HOST_DISPLAY_PRIMARY_ONLY 1

// Result of host_display_mode_set: the display could not be set up.
#define HOST_DISPLAY_FAILED 0
// Result of host_display_mode_set: the display is set up.
#define HOST_DISPLAY_OK 1
// Result of host_display_mode_set with HOST_DISPLAY_PRIMARY_ONLY: the display is set up but its frame could not be made.
#define HOST_DISPLAY_NO_FRAME 2

// One display mode of the machine.
struct host_display_mode {
    // The mode's width in pixels.
    int width;
    // The mode's height in pixels.
    int height;
    // The mode's bits for each pixel.
    int bits;
    // The mode's refresh rate in hertz, or 0 when it is not known.
    int refresh;
};

// An 8-bit frame, locked for drawing.
struct host_frame {
    // The first pixel, one byte for each, valid until host_frame_unlock.
    unsigned char* pixels;
    // The distance in bytes from a row to the next.
    int pitch;
    // The frame's width in pixels.
    int width;
    // The frame's height in pixels.
    int height;
};

// The layout of a frame's pixels, as the masks of the bits of each colour.
struct host_pixel_format {
    // The bits for each pixel.
    int bits;
    // The bits of the red part.
    unsigned int red;
    // The bits of the green part.
    unsigned int green;
    // The bits of the blue part.
    unsigned int blue;
};

// Sets the screen to an 8-bit width by height mode with a back frame and a palette of the 256 four-byte red, green, blue and zero entries (NULL for none), and returns a HOST_DISPLAY_ result; HOST_DISPLAY_PRIMARY_ONLY in flags sets up only the frame on the screen.
int HOST_CALL host_display_mode_set(int width, int height, int flags, const unsigned char* palette);
// Releases the display and its frames and palette, or does nothing when none is set up.
void HOST_CALL host_display_close(void);
// Returns 1 when a display is set up.
int HOST_CALL host_display_is_open(void);
// Returns 1 when the display has a back frame, even if setting it up failed later.
int HOST_CALL host_display_has_back_buffer(void);
// Stores up to max of the machine's display modes in modes and their number in *count, and returns 1, or 0 when they could not be listed.
int HOST_CALL host_display_mode_list(struct host_display_mode* modes, int max, int* count);

// Locks a frame (HOST_FRAME_BACK or HOST_FRAME_FRONT) for writing into *frame and returns a HOST_FRAME_ result.
int HOST_CALL host_frame_lock(int which, struct host_frame* frame);
// Ends the lock of host_frame_lock and returns 1, or returns 0 when the display has no such frame.
int HOST_CALL host_frame_unlock(int which);
// Shows the back frame by swapping it with the frame on the screen when flip is set, or else by copying it to the window; returns a HOST_FRAME_ result.
int HOST_CALL host_frame_present(int flip);
// Returns 1 when the frames were lost.
int HOST_CALL host_frame_is_lost(void);
// Restores the frames, which leaves them empty, and returns 1, or returns 0 when they could not be restored.
int HOST_CALL host_frame_restore(void);
// Fills a frame with one palette index and returns 1, or returns 0 on failure.
int HOST_CALL host_frame_fill(int which, unsigned int colour);
// Stores the pixel layout of the frame on the screen in *format and returns 1, or returns 0 on failure.
int HOST_CALL host_frame_pixel_format(struct host_pixel_format* format);
// Draws length bytes of text at x and y of a frame in the 0xrrggbb colour, with the host's own font handle font (NULL for the default font), and returns 1, or returns 0 on failure.
int HOST_CALL host_frame_text(int which, void* font, int x, int y, const char* text, int length, unsigned int rgb);
// Copies the back frame to pixels (rows pitch bytes apart) and its 256 four-byte palette entries to palette, and returns 1, or returns 0 on failure.
int HOST_CALL host_frame_read(unsigned char* pixels, int pitch, unsigned char* palette);

// Returns a drawing context for the window (the native handle of host_window_open), or NULL on failure; end it with host_frame_context_release.
void* HOST_CALL host_frame_context_get(void* window);
// Ends a context from host_frame_context_get on the same window.
void HOST_CALL host_frame_context_release(void* window, void* context);
// Selects the display palette (a native palette handle) into the context, without forcing it to the background, and realises it.
void HOST_CALL host_frame_context_palette(void* context, void* palette);

// Makes the display palette from 256 four-byte entries and attaches it to the frame on the screen, and returns 1, or returns 0 on failure.
int HOST_CALL host_palette_create(const unsigned char* entries);
// Sets count four-byte entries from index start of the display palette and returns 1, or returns 0 on failure.
int HOST_CALL host_palette_set(int start, int count, const unsigned char* entries);
// Stores the display palette's 256 four-byte entries in entries and returns 1, or returns 0 on failure.
int HOST_CALL host_palette_get(unsigned char* entries);
// Attaches the display palette to the frame on the screen again and returns 1, or returns 0 when there is none or it failed.
int HOST_CALL host_palette_apply(void);

// Opens an existing file, with a path relative to the game directory or absolute, for reading only and returns its handle, or NULL when it cannot be opened.
void* HOST_CALL host_file_open_read(const char* path);
// Creates a file, with a path relative to the game directory or absolute, or empties it when it exists, for reading and writing (for writing only when flags has HOST_FILE_TEXT), and returns its handle or NULL.
void* HOST_CALL host_file_create(const char* path, unsigned int flags);
// Opens a file, with a path relative to the game directory or absolute, for reading and appending, creating it when it is missing, and returns its handle or NULL.
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

// Lists the entries that match pattern (* and ? wildcards, a path relative to the game directory or absolute), fills entry with the first, and returns the listing's handle, or NULL when nothing matches.
void* HOST_CALL host_dir_first(const char* pattern, struct host_dir_entry* entry);
// Fills entry with the listing's next entry and returns 1, or returns 0 when there are no more.
int HOST_CALL host_dir_next(void* dir, struct host_dir_entry* entry);
// Ends the listing and frees its handle.
void HOST_CALL host_dir_close(void* dir);
// Writes the path of the game's executable, a full path with its file name, into buf of size bytes and returns its length, or 0 on failure.
unsigned int HOST_CALL host_file_exe_path(char* buf, unsigned int size);
// Writes the full path of name (relative to the current directory or absolute) into buf of size bytes and returns its length, or 0 on failure; when buf is too small it returns the size needed, which is larger than size.
unsigned int HOST_CALL host_file_full_path(const char* name, char* buf, unsigned int size);
// Makes path the current directory, against which relative paths resolve, and returns 1, or 0 on failure.
int HOST_CALL host_dir_set_current(const char* path);
// Reads the integer key of section from the ini file at path and returns it, or fallback when it is missing (the value of GetPrivateProfileInt).
int HOST_CALL host_file_config_int(const char* path, const char* section, const char* key, int fallback);
// Reads the string key of section from the ini file at path into buf of size bytes (fallback when it is missing) and returns its length (the value of GetPrivateProfileString).
unsigned int HOST_CALL host_file_config_string(const char* path, const char* section, const char* key, const char* fallback, char* buf, unsigned int size);

// Copies up to size bytes of the clipboard's text into buf, taking the clipboard for the owner window, and returns how many were copied, or 0 when there is no text.
unsigned int HOST_CALL host_clipboard_text_get(void* owner, char* buf, unsigned int size);

// Sound buffer flag: the buffer can be placed in space with host_sound_buffer_place.
#define HOST_SOUND_3D 0x01

// Sound buffer status bit: the buffer is playing.
#define HOST_SOUND_PLAYING 0x01
// Sound buffer status bit: the buffer plays in a loop.
#define HOST_SOUND_LOOPING 0x04

// Result of host_sound_open when the machine has no sound driver.
#define HOST_SOUND_NO_DRIVER (-1)

// Opens the sound device for window, when it is not open yet, and sets its output to rate samples a second of bits-bit samples on channels channels; returns its handle, or NULL with *error set to 0 or HOST_SOUND_NO_DRIVER (host_sound_close then releases what was opened).
void* HOST_CALL host_sound_open(void* window, int rate, int bits, int channels, int* error);
// Closes the sound device, or does nothing when none is open; call it after every buffer is freed, and after a failed host_sound_open.
void HOST_CALL host_sound_close(void);
// Returns the host's own sound device object for a video decoder that plays its sound through it, or NULL when the device is not open.
void* HOST_CALL host_sound_native(void);
// Creates a buffer of bytes bytes of PCM sound at rate samples a second, bits bits a sample and channels channels, with HOST_SOUND_ flags, and returns its handle or NULL.
void* HOST_CALL host_sound_buffer_create(unsigned int bytes, int rate, int bits, int channels, unsigned int flags);
// Creates a second buffer that shares buffer's sound but plays on its own, and returns its handle or NULL.
void* HOST_CALL host_sound_buffer_duplicate(void* buffer);
// Frees the buffer and its handle.
void HOST_CALL host_sound_buffer_free(void* buffer);
// Gives write access to bytes bytes of the buffer from offset, sets *data and *size to the part that can be written, and returns 1, or 0 on failure.
int HOST_CALL host_sound_buffer_lock(void* buffer, unsigned int offset, unsigned int bytes, void** data, unsigned int* size);
// Ends the write access that host_sound_buffer_lock gave to data, after size bytes were written, and returns 1, or 0 on failure.
int HOST_CALL host_sound_buffer_unlock(void* buffer, void* data, unsigned int size);
// Starts the buffer playing from its position, in a loop when loop is nonzero, and returns 1, or 0 on failure.
int HOST_CALL host_sound_buffer_play(void* buffer, int loop);
// Stops the buffer, keeping its position, and returns 1, or 0 on failure.
int HOST_CALL host_sound_buffer_stop(void* buffer);
// Returns the buffer's HOST_SOUND_PLAYING and HOST_SOUND_LOOPING bits, 0 when it is stopped, or -1 on failure.
int HOST_CALL host_sound_buffer_status(void* buffer);
// Sets *play to the byte offset being played and *write to the first offset that is safe to write, and returns 1, or 0 on failure.
int HOST_CALL host_sound_buffer_position(void* buffer, unsigned int* play, unsigned int* write);
// Moves the buffer's play position to offset bytes and returns 1, or 0 on failure.
int HOST_CALL host_sound_buffer_set_position(void* buffer, unsigned int offset);
// Sets the buffer's volume in hundredths of a decibel, from 0 (unchanged) down to -10000 (silent), and returns 1, or 0 on failure.
int HOST_CALL host_sound_buffer_set_volume(void* buffer, int volume);
// Places a HOST_SOUND_3D buffer at the point xyz, heard fully within min_distance and no quieter beyond max_distance, or with xyz NULL plays it unplaced; returns 1, or 0 when the buffer cannot be placed.
int HOST_CALL host_sound_buffer_place(void* buffer, const float* xyz, float min_distance, float max_distance);

// Wave flag: the sound repeats until host_sound_wav_stop.
#define HOST_WAV_LOOP 0x01
// Wave flag: the call does nothing when a wave is already playing.
#define HOST_WAV_KEEP 0x02

// Starts playing the WAV file image at wav, which must stay valid while it plays, without waiting for it to end, and returns 1, or 0 on failure.
int HOST_CALL host_sound_wav_play(const void* wav, unsigned int flags);
// Stops the wave that host_sound_wav_play started, without waiting for it to end.
void HOST_CALL host_sound_wav_stop(void);

// Mixer line: the wave output devices.
#define HOST_MIXER_WAVE 0
// Mixer line: the CD audio device.
#define HOST_MIXER_CD 1

// Returns the number of wave output devices.
int HOST_CALL host_mixer_wave_count(void);
// Returns the device number of the first CD audio mixer device, or -1 when there is none.
int HOST_CALL host_mixer_cd_device(void);
// Sets *volume to the device's volume, the left channel in the low 16 bits and the right in the high 16, each 0 to 0xffff, and returns 1, or 0 on failure.
int HOST_CALL host_mixer_get(int line, int device, unsigned int* volume);
// Sets the device's volume, packed as host_mixer_get returns it, and returns 1, or 0 on failure.
int HOST_CALL host_mixer_set(int line, int device, unsigned int volume);

// Opens the music device (the CD drive, or the host's own music files) with positions in milliseconds and returns 1, 0 when it could not be opened, or -1 when it opened but its positions could not be set to milliseconds (it stays open).
int HOST_CALL host_music_open(void);
// Stops and closes the music device.
void HOST_CALL host_music_close(void);
// Stops the music and returns 1, or 0 on failure.
int HOST_CALL host_music_stop(void);
// Pauses the music and returns 1, or 0 on failure.
int HOST_CALL host_music_pause(void);
// Returns 1 when the music is playing, 0 when it is not, or -1 on failure.
int HOST_CALL host_music_playing(void);
// Returns the number of tracks, counting a data track, or -1 on failure.
int HOST_CALL host_music_track_count(void);
// Returns 1 when the track (counted from 1) is audio, 0 when it is data or other, or -1 on failure.
int HOST_CALL host_music_track_is_audio(int track);
// Returns the track's length in milliseconds, or -1 on failure.
int HOST_CALL host_music_track_length(int track);
// Returns the position where the track starts in milliseconds, or -1 on failure.
int HOST_CALL host_music_track_start(int track);
// Returns the number of the track at the play position, or -1 on failure.
int HOST_CALL host_music_current_track(void);
// Returns the play position in milliseconds, or -1 on failure.
int HOST_CALL host_music_position(void);
// Plays from the start of track first up to the start of track last, or to the end with last 0, posting HOST_EVENT_MEDIA_NOTIFY to the window when it ends; returns 1, or 0 on failure.
int HOST_CALL host_music_play_tracks(int first, int last);
// Plays from the position from_ms, or the current position with from_ms -1, up to to_ms, or to the end with to_ms -1, posting HOST_EVENT_MEDIA_NOTIFY to the window when it ends; returns 1, or 0 on failure.
int HOST_CALL host_music_play_range(int from_ms, int to_ms);
// Returns the volume serial number of the music disc, or 0 when there is no disc.
unsigned int HOST_CALL host_music_disc_serial(void);
// Returns the handle of the CD player program's window, or NULL when it has none.
void* HOST_CALL host_music_find_cd_player(void);
// Closes the CD player program's window that host_music_find_cd_player returned, and waits for it to go.
void HOST_CALL host_music_close_cd_player(void* window);

// The session record of a network game: the host fills it and the game reads some of its fields, so the game keeps its bytes in the state block.
struct host_net;
// The pair of interface handles host_net_create_play_interface fills, kept by the caller.
struct host_net_pair;

// A 16-byte globally unique id, copied and compared whole.
struct host_guid {
    unsigned long data[4];
};

// Returns the text of a network error (the result of a host_net call), never NULL.
const char* HOST_CALL host_net_error_string(int error);
// Returns the result of the last session enumeration by host_net_get_games.
int HOST_CALL host_net_enum_sessions_result(void);
// Frees a block a host_net call handed to the game.
void HOST_CALL host_net_free(void* block);
// Sets whether packets are sent guaranteed and returns the setting before.
int HOST_CALL host_net_guarantee_packets(int enable);
// Sends a packet from player id from to id to and returns 0 or a network error.
int HOST_CALL host_net_send_packet(struct host_net* net, unsigned long from, unsigned long to, void* data, unsigned long size);
// Receives the next packet into data (size holds its capacity and receives the length) and returns 0 or a network error.
int HOST_CALL host_net_receive_packet(struct host_net* net, void* data, unsigned long* size);
// Names the open session and sets its four user values, and returns 1 on success.
int HOST_CALL host_net_update_game_info(struct host_net* net, char* name, char* data, int user1, int user2, int user3, int user4);
// Opens a new session with the name, password and user values and returns 1 on success.
int HOST_CALL host_net_create_new_game(struct host_net* net, char* name, char* password, int user1, int user2, int user3, int user4);
// Creates or joins the session of the lobby connection, filling the settings it lacks, and returns 1 on success.
int HOST_CALL host_net_create_or_join_lobby_game(struct host_net* net, char* password, unsigned long max_players, unsigned long user1, unsigned long user2, unsigned long user3, unsigned long user4);
// Closes the session and frees the interfaces and the lobby connection.
void HOST_CALL host_net_uninit(struct host_net* net);
// Sets the default player limit and enumeration timeout.
void HOST_CALL host_net_init_defaults(struct host_net* net);
// Lists the sessions into sessions, calling host_pump_events then poll (when not NULL) after each try until poll returns nonzero, and returns their count or -1 on failure.
int HOST_CALL host_net_get_games(struct host_net* net, void* sessions, int (HOST_CALL* poll)(void));
// Closes the session and returns 0 on failure, 1 otherwise.
int HOST_CALL host_net_quit_game(struct host_net* net);
// Joins the session with the instance id and returns 1 on success.
int HOST_CALL host_net_join_game(struct host_net* net, const struct host_guid* guid);
// Fills guids, conns and names with the connection types of the application and returns 1 on success.
int HOST_CALL host_net_get_connections(struct host_net* net, void* guids, void* conns, void* names, const struct host_guid* application);
// Returns nonzero when the session asks for a password.
unsigned int HOST_CALL host_net_password_required(struct host_net* net);
// Opens the lobby the game was started from and returns 1 on success.
int HOST_CALL host_net_init_lobbied_connection(struct host_net* net);
// Opens the connection with the id in *connection and returns 1 on success.
int HOST_CALL host_net_init_connection(struct host_net* net, int* connection);
// Adds a local player to the session, stores its id in *id and returns 1 on success.
int HOST_CALL host_net_add_player(struct host_net* net, unsigned long* id, char* short_name, char* long_name, char* name, short version_major, short version_minor);
// Removes a player from the session and returns 1 on success.
int HOST_CALL host_net_remove_player(struct host_net* net, unsigned long id);
// Reads a player's name into data (size holds its capacity and receives the length) and returns 0 or a network error.
int HOST_CALL host_net_get_player_name(struct host_net* net, unsigned long id, void* data, unsigned long* size);
// Sets a player's name and returns 0 or a network error.
int HOST_CALL host_net_set_player_name(struct host_net* net, unsigned long id, void* name, unsigned long flags);
// Calls callback for each player of the session and returns 0 or a network error.
int HOST_CALL host_net_enum_players(struct host_net* net, void* session, void* callback, void* context, unsigned long flags);
// Builds an address from count elements into address (size holds its capacity and receives the length) and returns 0 or a network error.
int HOST_CALL host_net_create_compound_address(struct host_net* net, void* elements, unsigned long count, void* address, unsigned long* size);
// Calls callback for each element of an address and returns 0 or a network error.
int HOST_CALL host_net_enum_address(struct host_net* net, void* callback, void* address, unsigned long size, void* context);
// Creates the interface handles of a service provider into *pair and returns 0 or a network error.
int HOST_CALL host_net_create_play_interface(const struct host_guid* provider, struct host_net_pair* pair);
// Releases the interface handles of a pair.
int HOST_CALL host_net_release_play_interface(struct host_net_pair* pair);
// Reads a player's address into data (size holds its capacity and receives the length) and returns 0 or a network error.
int HOST_CALL host_net_get_player_address(struct host_net_pair* pair, unsigned long player, void* data, unsigned long* size);

#ifdef __cplusplus
}
#endif

#endif

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
    // The mouse's x in the window, for mouse events.
    int x;
    // The mouse's y in the window, for mouse events.
    int y;
    // The HOST_BUTTON_ bits held, for mouse events.
    unsigned int buttons;
    // host_time_ms when the host received the event.
    unsigned int time_ms;
};

// Opens the game window with the given title and client size, covering the screen when fullscreen is set, and returns its native handle or NULL.
void* HOST_CALL host_window_open(const char* title, int width, int height, int fullscreen);
// Destroys the game window, which queues HOST_EVENT_DESTROY.
void HOST_CALL host_window_close(void);
// Returns 1 when the game window has the keyboard focus.
int HOST_CALL host_window_has_focus(void);
// Queues an event of the given type for the game window, to arrive after the platform messages already waiting.
void HOST_CALL host_window_post(unsigned int type);
// Handles the calling thread's waiting platform messages, first waiting for one when wait is set, and returns 0 when the quit request was among them.
int HOST_CALL host_pump_events(int wait);
// Asks the calling thread's next host_pump_events to return 0.
void HOST_CALL host_post_quit(void);
// Takes the oldest queued game window event into *event and returns 1, or returns 0 when none is queued.
int HOST_CALL host_event_next(struct host_event* event);
// Returns a key's state: bit 15 set while the virtual key vk is held.
int HOST_CALL host_key_state(int vk);
// Stores the mouse cursor's position on the screen.
void HOST_CALL host_cursor_get(int* x, int* y);
// Moves the mouse cursor to a position on the screen.
void HOST_CALL host_cursor_set(int x, int y);

#ifdef __cplusplus
}
#endif

#endif

# vdm - A Custom Display Server

A minimal display server built from scratch in C. No Wayland, no X11. Own protocol, own IPC, own rendering model.

## Dependencies

- GCC (C11 support)
- Make
- pkg-config
- libdrm
- libinput
- libudev
- libdbus-1 (for future notification daemon)

Install on Debian/Ubuntu:
```bash
sudo apt install gcc make pkg-config libdrm-dev libinput-dev libudev-dev libdbus-1-dev
```

Install on Arch:
```bash
sudo pacman -S gcc make pkgconf libdrm libinput dbus
```

## Building

Debug build (default):
```bash
make
```

Release build:
```bash
make MODE=release
```

Clean:
```bash
make clean
```

## Running

**WARNING**: This takes exclusive control of your display. Run from a TTY (Ctrl+Alt+F3).

```bash
sudo make run
```

Or directly:
```bash
sudo build/debug/vdm
```

Press **Ctrl+Alt+Backspace** to exit the display server.

The server sets `VDM_DISPLAY` environment variable so clients can find the socket at `/run/user/$UID/vdm-0`.

## Demo Apps

Demos auto-retry connecting for up to 5 seconds, so you can launch them
before or after the server starts. Close them with ESC (launcher) or
by killing the process.

```bash
sudo VDM_DISPLAY=/run/user/0/vdm-0 ./build/debug/demo-box
sudo VDM_DISPLAY=/run/user/0/vdm-0 ./build/debug/demo-terminal
sudo VDM_DISPLAY=/run/user/0/vdm-0 ./build/debug/demo-launcher
```

Or use the helper script to launch everything:
```bash
./run-dm.sh
```

## Architecture

```
project/
├── Makefile
├── display-server/
│   ├── main.c           # Entry point, event loop
│   ├── drm.c/h          # DRM/KMS hardware output
│   ├── socket.c/h       # Unix domain socket IPC
│   ├── shm.c/h          # Shared memory pixel buffers
│   ├── input.c/h        # libinput integration
│   ├── compositor.c/h   # Window compositing
│   └── window.c/h       # Window management
├── libclient/
│   ├── client.c/h       # Client library (apps link against this)
├── log/
│   ├── log.c/h          # Shared logging library
└── demo/
    ├── demo-box.c       # Bouncing box demo
    ├── demo-terminal.c  # Fake terminal demo
    └── demo-launcher.c  # App launcher demo
```

## Protocol

Binary protocol over Unix domain socket. Fixed-size structs, no JSON, no text.

### Client → Server
- `MSG_CREATE_WINDOW` - Create a new window
- `MSG_COMMIT_BUFFER` - Commit framebuffer changes
- `MSG_DESTROY_WINDOW` - Destroy a window
- `MSG_MOVE_WINDOW` - Move window position

### Server → Client
- `MSG_WINDOW_CREATED` - Window created with shm fd
- `MSG_KEY_EVENT` - Keyboard event
- `MSG_MOUSE_MOVE` - Mouse movement
- `MSG_MOUSE_BUTTON` - Mouse button press/release
- `MSG_WINDOW_RESIZE` - Window resize notification
- `MSG_WINDOW_CLOSE` - Window close request

## Logging

Logs written to `/var/log/vdm/`:
- `display.log` - Display server logs
- `vdm.log` - Combined logs

Log levels: TRACE, DEBUG, INFO, WARN, ERROR, FATAL

Send SIGUSR1 to enable TRACE logging for 60 seconds:
```bash
kill -USR1 <display-server-pid>
```

## Client Library API

```c
display_t*  display_connect(void);
window_t*   window_create(display_t*, int w, int h, const char *title);
uint32_t*   window_get_buffer(window_t*);
void        window_commit(window_t*);
void        window_move(window_t*, int x, int y);
void        window_destroy(window_t*);
int         event_poll(window_t*, event_t *out);
void        display_disconnect(display_t*);
```

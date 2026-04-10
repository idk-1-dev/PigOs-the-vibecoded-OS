#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include "display-server.h"

static int mouse_fd = -1;
static int kbd_fd = -1;

int input_init(void) {
    const char *input_devs[] = {
        "/dev/input/event0",
        "/dev/input/event1",
        "/dev/input/mouse0",
        "/dev/input/mice",
        NULL
    };

    for (int i = 0; input_devs[i]; i++) {
        int fd = open(input_devs[i], O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            char name[256] = "unknown";
            ioctl(fd, EVIOCGNAME(sizeof(name)), name);
            
            if (strstr(name, "mouse") || strstr(name, "Mouse")) {
                mouse_fd = fd;
                printf("Mouse: %s\n", name);
            } else if (strstr(name, "keyboard") || strstr(name, "Keyboard")) {
                kbd_fd = fd;
                printf("Keyboard: %s\n", name);
            } else {
                if (mouse_fd < 0) mouse_fd = fd;
                else if (kbd_fd < 0) kbd_fd = fd;
            }
        }
    }

    if (mouse_fd < 0) mouse_fd = open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
    if (kbd_fd < 0) kbd_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);

    printf("Input subsystem initialized\n");
    return 0;
}

void input_finish(void) {
    if (mouse_fd >= 0) close(mouse_fd);
    if (kbd_fd >= 0) close(kbd_fd);
    mouse_fd = -1;
    kbd_fd = -1;
}

int input_poll(MouseState *mouse, KeyboardState *kbd) {
    if (!mouse || !kbd) return -1;

    mouse->moved = false;
    kbd->pressed = false;
    kbd->released = false;

    if (mouse_fd >= 0) {
        struct input_event ev;
        ssize_t n = read(mouse_fd, &ev, sizeof(ev));
        if (n > 0) {
            if (ev.type == EV_REL) {
                if (ev.code == REL_X) mouse->dx = ev.value;
                else if (ev.code == REL_Y) mouse->dy = ev.value;
                mouse->moved = true;
            } else if (ev.type == EV_KEY) {
                if (ev.code == BTN_LEFT) {
                    mouse->buttons = (mouse->buttons & ~1) | (ev.value ? 1 : 0);
                } else if (ev.code == BTN_RIGHT) {
                    mouse->buttons = (mouse->buttons & ~2) | (ev.value ? 2 : 0);
                } else if (ev.code == BTN_MIDDLE) {
                    mouse->buttons = (mouse->buttons & ~4) | (ev.value ? 4 : 0);
                }
            }
        }
    }

    if (kbd_fd >= 0) {
        struct input_event ev;
        ssize_t n = read(kbd_fd, &ev, sizeof(ev));
        if (n > 0 && ev.type == EV_KEY) {
            kbd->key = ev.code;
            kbd->unicode = xkb_translate_key(ev.code, false);
            kbd->pressed = (ev.value == 1);
            kbd->released = (ev.value == 0);
        }
    }

    return 0;
}

#include "input.h"
#include "../log/log.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <linux/input.h>
#include <sys/ioctl.h>

static int open_restricted(const char *path, int flags, void *user_data) {
    (void)user_data;
    int fd = open(path, flags);
    if (fd < 0) {
        LOG_ERROR("Failed to open input device %s: %s", path, strerror(errno));
        return fd;
    }

    if (ioctl(fd, EVIOCGRAB, 1) < 0) {
        LOG_WARN("EVIOCGRAB failed for %s: %s (continuing anyway)", path, strerror(errno));
    } else {
        LOG_DEBUG("EVIOCGRAB succeeded for %s", path);
    }

    return fd;
}

static void close_restricted(int fd, void *user_data) {
    (void)user_data;
    ioctl(fd, EVIOCGRAB, 0);
    close(fd);
}

static const struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

int input_init(input_ctx_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));

    ctx->udev = udev_new();
    if (!ctx->udev) {
        LOG_ERROR("Failed to create udev context");
        return -1;
    }

    ctx->libinput = libinput_udev_create_context(&interface, NULL, ctx->udev);
    if (!ctx->libinput) {
        LOG_ERROR("Failed to create libinput context");
        udev_unref(ctx->udev);
        return -1;
    }

    if (libinput_udev_assign_seat(ctx->libinput, "seat0") != 0) {
        LOG_ERROR("Failed to assign seat");
        libinput_unref(ctx->libinput);
        udev_unref(ctx->udev);
        return -1;
    }

    ctx->xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx->xkb_ctx) {
        LOG_ERROR("Failed to create xkb context");
        return -1;
    }

    ctx->xkb_keymap = xkb_keymap_new_from_names(ctx->xkb_ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!ctx->xkb_keymap) {
        LOG_ERROR("Failed to create xkb keymap");
        xkb_context_unref(ctx->xkb_ctx);
        return -1;
    }

    ctx->xkb_state = xkb_state_new(ctx->xkb_keymap);
    if (!ctx->xkb_state) {
        LOG_ERROR("Failed to create xkb state");
        xkb_keymap_unref(ctx->xkb_keymap);
        xkb_context_unref(ctx->xkb_ctx);
        return -1;
    }

    LOG_INFO("xkbcommon initialized with system layout");
    return 0;
}

void input_shutdown(input_ctx_t *ctx) {
    if (ctx->xkb_state) xkb_state_unref(ctx->xkb_state);
    if (ctx->xkb_keymap) xkb_keymap_unref(ctx->xkb_keymap);
    if (ctx->xkb_ctx) xkb_context_unref(ctx->xkb_ctx);
    if (ctx->libinput)
        libinput_unref(ctx->libinput);
    if (ctx->udev)
        udev_unref(ctx->udev);
    memset(ctx, 0, sizeof(*ctx));
}

static void handle_keyboard_key(input_ctx_t *ctx, struct libinput_event *event) {
    struct libinput_event_keyboard *kbe = libinput_event_get_keyboard_event(event);
    uint32_t keycode = libinput_event_keyboard_get_key(kbe);
    enum libinput_key_state state = libinput_event_keyboard_get_key_state(kbe);
    int pressed = (state == LIBINPUT_KEY_STATE_PRESSED) ? 1 : 0;

    xkb_keycode_t xkb_keycode = keycode + 8;

    if (keycode == 29 || keycode == 97)
        ctx->ctrl_held = pressed;
    else if (keycode == 56 || keycode == 100)
        ctx->alt_held = pressed;

    if (keycode == 14 && pressed && ctx->ctrl_held && ctx->alt_held) {
        LOG_INFO("Ctrl+Alt+Backspace detected, exiting");
        ctx->want_exit = 1;
        return;
    }

    if (ctx->xkb_state) {
        xkb_state_update_key(ctx->xkb_state, xkb_keycode,
                             pressed ? XKB_KEY_DOWN : XKB_KEY_UP);

        if (pressed) {
            ctx->last_text_len = xkb_state_key_get_utf8(
                ctx->xkb_state, xkb_keycode,
                ctx->last_text, sizeof(ctx->last_text));
        }
    }

    ctx->has_key_event = 1;
    ctx->last_keycode = keycode;
    ctx->last_key_pressed = pressed;

    LOG_DEBUG("Key event: keycode=%u pressed=%d text='%.*s'",
              keycode, pressed, ctx->last_text_len, ctx->last_text);
}

static void handle_pointer_motion(input_ctx_t *ctx, struct libinput_event *event) {
    struct libinput_event_pointer *pe = libinput_event_get_pointer_event(event);
    double dx = libinput_event_pointer_get_dx(pe);
    double dy = libinput_event_pointer_get_dy(pe);

    ctx->mouse_x += (int)dx;
    ctx->mouse_y += (int)dy;

    if (ctx->mouse_x < 0) ctx->mouse_x = 0;
    if (ctx->mouse_y < 0) ctx->mouse_y = 0;

    ctx->has_mouse_event = 1;

    LOG_TRACE("Mouse move: x=%d y=%d", ctx->mouse_x, ctx->mouse_y);
}

static void handle_pointer_button(input_ctx_t *ctx, struct libinput_event *event) {
    struct libinput_event_pointer *pe = libinput_event_get_pointer_event(event);
    uint32_t button = libinput_event_pointer_get_button(pe);
    enum libinput_button_state state = libinput_event_pointer_get_button_state(pe);

    ctx->has_mouse_event = 1;
    ctx->last_button = button;
    ctx->last_button_pressed = (state == LIBINPUT_BUTTON_STATE_PRESSED) ? 1 : 0;
}

static void handle_pointer_motion_absolute(input_ctx_t *ctx, struct libinput_event *event) {
    struct libinput_event_pointer *pe = libinput_event_get_pointer_event(event);
    double x = libinput_event_pointer_get_absolute_x(pe);
    double y = libinput_event_pointer_get_absolute_y(pe);

    ctx->mouse_x = (int)x;
    ctx->mouse_y = (int)y;
    ctx->has_mouse_event = 1;
}

int input_dispatch(input_ctx_t *ctx) {
    struct libinput_event *event;
    int events = 0;

    libinput_dispatch(ctx->libinput);

    while ((event = libinput_get_event(ctx->libinput)) != NULL) {
        enum libinput_event_type type = libinput_event_get_type(event);

        switch (type) {
        case LIBINPUT_EVENT_KEYBOARD_KEY:
            handle_keyboard_key(ctx, event);
            events++;
            break;
        case LIBINPUT_EVENT_POINTER_MOTION:
            handle_pointer_motion(ctx, event);
            events++;
            break;
        case LIBINPUT_EVENT_POINTER_BUTTON:
            handle_pointer_button(ctx, event);
            events++;
            break;
        case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
            handle_pointer_motion_absolute(ctx, event);
            events++;
            break;
        default:
            break;
        }

        libinput_event_destroy(event);
    }

    return events;
}

#ifndef MYWM_INPUT_H
#define MYWM_INPUT_H

#include <stdint.h>
#include <libinput.h>
#include <libudev.h>
#include <xkbcommon/xkbcommon.h>

#define INPUT_KEY_MAX 256

typedef struct {
    struct udev *udev;
    struct libinput *libinput;
    struct xkb_context *xkb_ctx;
    struct xkb_keymap *xkb_keymap;
    struct xkb_state *xkb_state;
    int mouse_x;
    int mouse_y;
    uint32_t last_keycode;
    uint8_t last_key_pressed;
    uint32_t last_button;
    uint8_t last_button_pressed;
    int has_mouse_event;
    int has_key_event;
    int want_exit;
    int ctrl_held;
    int alt_held;
    char last_text[8];
    int last_text_len;
} input_ctx_t;

int input_init(input_ctx_t *ctx);
void input_shutdown(input_ctx_t *ctx);
int input_dispatch(input_ctx_t *ctx);

#endif

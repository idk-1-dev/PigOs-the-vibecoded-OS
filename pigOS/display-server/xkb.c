#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xkbcommon/xkbcommon.h>
#include "display-server.h"

static struct xkb_context *xkb_ctx = NULL;
static struct xkb_keymap *keymap = NULL;
static struct xkb_state *xkb_state = NULL;

int xkb_init(void) {
    xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!xkb_ctx) {
        fprintf(stderr, "Failed to create xkbcommon context\n");
        return -1;
    }

    struct xkb_rule_names names = {
        .rules = "evdev",
        .model = "pc105",
        .layout = "us",
        .variant = NULL,
        .options = NULL
    };

    keymap = xkb_keymap_new_from_names(xkb_ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        fprintf(stderr, "Failed to create keymap\n");
        xkb_context_unref(xkb_ctx);
        xkb_ctx = NULL;
        return -1;
    }

    xkb_state = xkb_state_new(keymap);
    if (!xkb_state) {
        fprintf(stderr, "Failed to create xkb state\n");
        xkb_keymap_unref(keymap);
        keymap = NULL;
        xkb_context_unref(xkb_ctx);
        xkb_ctx = NULL;
        return -1;
    }

    printf("xkbcommon initialized\n");
    return 0;
}

void xkb_finish(void) {
    if (xkb_state) {
        xkb_state_unref(xkb_state);
        xkb_state = NULL;
    }
    if (keymap) {
        xkb_keymap_unref(keymap);
        keymap = NULL;
    }
    if (xkb_ctx) {
        xkb_context_unref(xkb_ctx);
        xkb_ctx = NULL;
    }
}

uint32_t xkb_translate_key(uint32_t scancode, bool shift) {
    if (!xkb_state) return scancode;

    uint32_t mods = shift ? 1 : 0;
    xkb_state_update_mask(xkb_state, mods, 0, 0, 0, 0, 0);

    xkb_keysym_t keysym = xkb_state_key_get_one_sym(xkb_state, scancode + 8);
    
    char buffer[64];
    int len = xkb_keysym_get_name(keysym, buffer, sizeof(buffer));
    if (len > 0) {
        if (buffer[0] >= 'a' && buffer[0] <= 'z' && !shift) {
            return (uint32_t)buffer[0];
        }
        if (buffer[0] >= 'A' && buffer[0] <= 'Z' && shift) {
            return (uint32_t)buffer[0];
        }
    }

    return xkb_keysym_to_utf32(keysym);
}

#include "display-server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int display_server_init(DisplayContext *ctx) {
    if (!ctx) return -1;
    
    memset(ctx, 0, sizeof(DisplayContext));
    ctx->drm_fd = -1;
    ctx->initialized = false;
    
    if (udev_init() < 0) {
        fprintf(stderr, "udev init failed\n");
        return -1;
    }
    
    if (drm_init(ctx) < 0) {
        fprintf(stderr, "DRM init failed\n");
        udev_finish();
        return -1;
    }
    
    if (drm_create_fb(ctx) < 0) {
        fprintf(stderr, "Framebuffer creation failed\n");
        drm_finish(ctx);
        udev_finish();
        return -1;
    }
    
    if (input_init() < 0) {
        fprintf(stderr, "Input init failed (non-fatal)\n");
    }
    
    if (xkb_init() < 0) {
        fprintf(stderr, "xkbcommon init failed (non-fatal)\n");
    }
    
    g_display = ctx;
    return 0;
}

void display_server_finish(DisplayContext *ctx) {
    if (!ctx) return;
    
    input_finish();
    xkb_finish();
    drm_finish(ctx);
    udev_finish();
    
    g_display = NULL;
}

int display_server_poll(DisplayContext *ctx, MouseState *mouse, KeyboardState *kbd) {
    (void)ctx;
    return input_poll(mouse, kbd);
}

void display_server_flip(DisplayContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    drm_flip(ctx);
}

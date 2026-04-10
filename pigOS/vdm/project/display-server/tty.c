#include "tty.h"
#include "../log/log.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <errno.h>
#include <stdio.h>

static void tty_save_state(tty_ctx_t *ctx) {
    if (ctx->fd < 0) return;

    struct termios term;
    if (ioctl(ctx->fd, TCGETS, &term) == 0) {
        ctx->orig_termios = term;
        ctx->termios_saved = true;
    }
}

static void tty_setup_graphics(tty_ctx_t *ctx) {
    if (ctx->fd < 0) return;

    if (ioctl(ctx->fd, KDSETMODE, KD_GRAPHICS) < 0) {
        LOG_ERROR("Failed to set KD_GRAPHICS mode: %s", strerror(errno));
        return;
    }
    LOG_INFO("VT set to KD_GRAPHICS mode - keyboard will not leak to TTY");
}

static void tty_restore_text(tty_ctx_t *ctx) {
    if (ctx->fd < 0) return;

    if (ioctl(ctx->fd, KDSETMODE, KD_TEXT) < 0) {
        LOG_WARN("Failed to restore KD_TEXT mode: %s", strerror(errno));
    } else {
        LOG_INFO("VT restored to KD_TEXT mode");
    }

    if (ctx->termios_saved) {
        ioctl(ctx->fd, TCSETS, &ctx->orig_termios);
    }
}

int tty_init(tty_ctx_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->fd = -1;

    ctx->fd = open("/dev/tty0", O_RDWR | O_NOCTTY);
    if (ctx->fd < 0) {
        LOG_WARN("Cannot open /dev/tty0, trying /dev/tty1");
        ctx->fd = open("/dev/tty1", O_RDWR | O_NOCTTY);
    }
    if (ctx->fd < 0) {
        LOG_WARN("Cannot open any TTY, running without VT control");
        ctx->active = true;
        return 0;
    }

    tty_save_state(ctx);
    tty_setup_graphics(ctx);
    ctx->active = true;

    LOG_INFO("TTY initialized, VT in graphics mode");
    return 0;
}

void tty_shutdown(tty_ctx_t *ctx) {
    if (!ctx->active) return;

    tty_restore_text(ctx);

    if (ctx->fd >= 0) {
        close(ctx->fd);
        ctx->fd = -1;
    }
    ctx->active = false;
    LOG_INFO("TTY shutdown complete");
}

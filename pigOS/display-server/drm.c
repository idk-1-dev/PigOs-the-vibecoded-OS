#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "display-server.h"

static uint32_t gem_handle;
static uint32_t fb_id;
static void *map = NULL;

int drm_init(DisplayContext *ctx) {
    const char *dev_path = udev_find_gpu();
    if (!dev_path) {
        fprintf(stderr, "No GPU device found\n");
        return -1;
    }

    ctx->drm_fd = open(dev_path, O_RDWR | O_CLOEXEC);
    if (ctx->drm_fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", dev_path, strerror(errno));
        return -1;
    }

    ctx->width = 1024;
    ctx->height = 768;
    ctx->front_buffer = NULL;
    ctx->back_buffer = NULL;
    ctx->initialized = true;

    printf("DRM initialized on %s\n", dev_path);
    return 0;
}

void drm_finish(DisplayContext *ctx) {
    if (!ctx->initialized) return;

    if (map) {
        munmap(map, ctx->width * ctx->height * 4);
    }
    if (ctx->drm_fd >= 0) {
        close(ctx->drm_fd);
        ctx->drm_fd = -1;
    }
    ctx->initialized = false;
}

int drm_create_fb(DisplayContext *ctx) {
    if (ctx->drm_fd < 0 || !ctx->initialized) return -1;

    size_t size = ctx->width * ctx->height * 4;

    map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
               ctx->drm_fd, 0);
    if (map == MAP_FAILED) {
        fprintf(stderr, "Failed to mmap: %s\n", strerror(errno));
        return -1;
    }

    ctx->back_buffer = (uint32_t *)map;
    ctx->front_buffer = (uint32_t *)map;

    printf("Framebuffer created: %ux%u\n", ctx->width, ctx->height);
    return 0;
}

void drm_flip(DisplayContext *ctx) {
    if (!ctx->initialized || ctx->drm_fd < 0) return;
    if (!ctx->front_buffer || !ctx->back_buffer) return;

    uint32_t *tmp = ctx->front_buffer;
    ctx->front_buffer = ctx->back_buffer;
    ctx->back_buffer = tmp;
}

#include "drm.h"
#include "../log/log.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <poll.h>
#include <dirent.h>
#include <drm_fourcc.h>

static int find_drm_device(char *path, size_t len) {
    const char *base = "/dev/dri/";
    DIR *dir;
    struct dirent *ent;

    dir = opendir(base);
    if (!dir) return -1;

    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "card", 4) == 0) {
            snprintf(path, len, "%s%s", base, ent->d_name);
            closedir(dir);
            return 0;
        }
    }
    closedir(dir);
    return -1;
}

static void page_flip_handler(int fd, unsigned int frame,
                              unsigned int sec, unsigned int usec,
                              void *data) {
    vdm_drm_t *ctx = (vdm_drm_t *)data;
    (void)fd; (void)frame; (void)sec; (void)usec;
    ctx->flipping = 0;
}

int drm_init(vdm_drm_t *ctx) {
    char path[256];
    drmVersionPtr ver;
    int ret;

    memset(ctx, 0, sizeof(*ctx));

    ret = find_drm_device(path, sizeof(path));
    if (ret < 0) {
        LOG_ERROR("Failed to find DRM device");
        return -1;
    }

    LOG_INFO("Opening DRM device: %s", path);

    ctx->fd = open(path, O_RDWR | O_CLOEXEC);
    if (ctx->fd < 0) {
        LOG_ERROR("Failed to open DRM device: %s", strerror(errno));
        return -1;
    }

    ver = drmGetVersion(ctx->fd);
    if (ver) {
        LOG_INFO("DRM driver: %s v%d.%d.%d",
                 ver->name, ver->version_major,
                 ver->version_minor, ver->version_patchlevel);
        drmFreeVersion(ver);
    }

    if (drmSetMaster(ctx->fd) < 0) {
        LOG_ERROR("Failed to set DRM master: %s", strerror(errno));
        close(ctx->fd);
        return -1;
    }

    ret = drmSetClientCap(ctx->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if (ret < 0) {
        LOG_WARN("Universal planes not supported, continuing anyway");
    }

    ctx->evctx.version = DRM_EVENT_CONTEXT_VERSION;
    ctx->evctx.page_flip_handler = page_flip_handler;

    return 0;
}

int drm_set_mode(vdm_drm_t *ctx) {
    drmModeRes *res;
    drmModeConnector *conn = NULL;
    drmModeEncoder *enc = NULL;
    drmModeCrtc *crtc;
    drmModePlaneRes *plane_res;
    drmModePlane *plane;
    int i;

    res = drmModeGetResources(ctx->fd);
    if (!res) {
        LOG_ERROR("Failed to get DRM resources");
        return -1;
    }

    for (i = 0; i < res->count_connectors; i++) {
        conn = drmModeGetConnector(ctx->fd, res->connectors[i]);
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0)
            break;
        if (conn) drmModeFreeConnector(conn);
        conn = NULL;
    }

    if (!conn) {
        LOG_ERROR("No connected connector found");
        drmModeFreeResources(res);
        return -1;
    }

    ctx->connector_id = conn->connector_id;
    ctx->mode = conn->modes[0];
    ctx->width = ctx->mode.hdisplay;
    ctx->height = ctx->mode.vdisplay;

    for (i = 0; i < res->count_encoders; i++) {
        enc = drmModeGetEncoder(ctx->fd, res->encoders[i]);
        if (enc && enc->encoder_id == conn->encoder_id)
            break;
        if (enc) drmModeFreeEncoder(enc);
        enc = NULL;
    }

    if (!enc) {
        LOG_ERROR("No encoder found for connector");
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        return -1;
    }

    for (i = 0; i < res->count_crtcs; i++) {
        if (enc->possible_crtcs & (1 << i)) {
            crtc = drmModeGetCrtc(ctx->fd, res->crtcs[i]);
            if (crtc) {
                ctx->crtc_id = res->crtcs[i];
                ctx->crtc_index = i;
                drmModeFreeCrtc(crtc);
                break;
            }
        }
    }

    plane_res = drmModeGetPlaneResources(ctx->fd);
    if (plane_res) {
        for (i = 0; i < (int)plane_res->count_planes; i++) {
            plane = drmModeGetPlane(ctx->fd, plane_res->planes[i]);
            if (plane) {
                if (plane->possible_crtcs & (1 << ctx->crtc_index)) {
                    uint32_t fmt = DRM_FORMAT_XRGB8888;
                    for (int k = 0; k < (int)plane->count_formats; k++) {
                        if (plane->formats[k] == fmt) {
                            ctx->plane_id = plane->plane_id;
                            drmModeFreePlane(plane);
                            drmModeFreePlaneResources(plane_res);
                            goto found_plane;
                        }
                    }
                }
                drmModeFreePlane(plane);
            }
        }
        drmModeFreePlaneResources(plane_res);
    }

found_plane:
    if (!ctx->plane_id) {
        LOG_WARN("No suitable plane found, using primary plane fallback");
        ctx->plane_id = 0;
    }

    LOG_INFO("DRM device opened: %dx%d@%dhz",
             ctx->width, ctx->height,
             ctx->mode.vrefresh);

    drmModeFreeEncoder(enc);
    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
    return 0;
}

static int drm_create_gem_buffer(vdm_drm_t *ctx, uint8_t **out_map, uint32_t *out_handle, uint32_t *out_fb_id) {
    struct drm_mode_create_dumb create;
    struct drm_mode_map_dumb map_req;
    uint32_t handles[4] = {0};
    uint32_t pitches[4] = {0};
    uint32_t offsets[4] = {0};
    uint32_t fb_id;
    void *map;
    int ret;

    memset(&create, 0, sizeof(create));
    create.width = ctx->width;
    create.height = ctx->height;
    create.bpp = 32;

    ret = drmIoctl(ctx->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
    if (ret < 0) {
        LOG_ERROR("Failed to create dumb buffer: %s", strerror(errno));
        return -1;
    }

    handles[0] = create.handle;
    pitches[0] = create.pitch;

    ret = drmModeAddFB2(ctx->fd, ctx->width, ctx->height,
                        DRM_FORMAT_XRGB8888, handles,
                        pitches, offsets, &fb_id, 0);
    if (ret < 0) {
        LOG_ERROR("Failed to add framebuffer: %s", strerror(errno));
        struct drm_mode_destroy_dumb destroy = { .handle = create.handle };
        drmIoctl(ctx->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        return -1;
    }

    memset(&map_req, 0, sizeof(map_req));
    map_req.handle = create.handle;
    ret = drmIoctl(ctx->fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req);
    if (ret < 0) {
        LOG_ERROR("Failed to map dumb buffer: %s", strerror(errno));
        drmModeRmFB(ctx->fd, fb_id);
        struct drm_mode_destroy_dumb destroy = { .handle = create.handle };
        drmIoctl(ctx->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        return -1;
    }

    map = mmap(0, create.size, PROT_READ | PROT_WRITE, MAP_SHARED,
               ctx->fd, map_req.offset);
    if (map == MAP_FAILED) {
        LOG_ERROR("Failed to mmap dumb buffer: %s", strerror(errno));
        drmModeRmFB(ctx->fd, fb_id);
        struct drm_mode_destroy_dumb destroy = { .handle = create.handle };
        drmIoctl(ctx->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        return -1;
    }

    memset(map, 0, create.size);

    *out_map = (uint8_t *)map;
    *out_handle = create.handle;
    *out_fb_id = fb_id;
    return 0;
}

int drm_create_framebuffers(vdm_drm_t *ctx) {
    int ret;

    ret = drm_create_gem_buffer(ctx, &ctx->buffers[0], &ctx->gem_handles[0], &ctx->fb_ids[0]);
    if (ret < 0) return -1;

    ret = drm_create_gem_buffer(ctx, &ctx->buffers[1], &ctx->gem_handles[1], &ctx->fb_ids[1]);
    if (ret < 0) return -1;

    ctx->stride = ctx->width * 4;
    ctx->front_buf = 0;
    ctx->back_buf = 1;

    ret = drmModeSetCrtc(ctx->fd, ctx->crtc_id, ctx->fb_ids[ctx->front_buf],
                         0, 0, &ctx->connector_id, 1, &ctx->mode);
    if (ret < 0) {
        LOG_ERROR("Failed to set CRTC: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int drm_page_flip(vdm_drm_t *ctx) {
    int ret;

    if (ctx->flipping) return -1;

    uint32_t tmp = ctx->front_buf;
    ctx->front_buf = ctx->back_buf;
    ctx->back_buf = tmp;

    ret = drmModePageFlip(ctx->fd, ctx->crtc_id, ctx->fb_ids[ctx->front_buf],
                          DRM_MODE_PAGE_FLIP_EVENT, ctx);
    if (ret < 0) {
        LOG_ERROR("DRM page flip failed: %s", strerror(errno));
        tmp = ctx->front_buf;
        ctx->front_buf = ctx->back_buf;
        ctx->back_buf = tmp;
        return -1;
    }

    ctx->flipping = 1;
    return 0;
}

void drm_wait_flip(vdm_drm_t *ctx) {
    struct pollfd pfd;

    while (ctx->flipping) {
        pfd.fd = ctx->fd;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 16) > 0) {
            if (pfd.revents & POLLIN)
                drmHandleEvent(ctx->fd, &ctx->evctx);
        }
    }
}

void drm_handle_event(vdm_drm_t *ctx) {
    drmHandleEvent(ctx->fd, &ctx->evctx);
}

void drm_fill_screen(vdm_drm_t *ctx, uint32_t color) {
    uint8_t *buf = ctx->buffers[ctx->back_buf];
    uint32_t *pixels = (uint32_t *)buf;
    int total = ctx->width * ctx->height;

    for (int i = 0; i < total; i++)
        pixels[i] = color;
}

void drm_shutdown(vdm_drm_t *ctx) {
    if (ctx->buffers[0]) {
        drmWaitVBlank(ctx->fd, NULL);
        munmap(ctx->buffers[0], ctx->stride * ctx->height);
    }
    if (ctx->buffers[1])
        munmap(ctx->buffers[1], ctx->stride * ctx->height);

    for (int i = 0; i < 2; i++) {
        if (ctx->fb_ids[i])
            drmModeRmFB(ctx->fd, ctx->fb_ids[i]);
        if (ctx->gem_handles[i]) {
            struct drm_mode_destroy_dumb destroy = { .handle = ctx->gem_handles[i] };
            drmIoctl(ctx->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        }
    }

    if (ctx->fd >= 0) {
        drmDropMaster(ctx->fd);
        close(ctx->fd);
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->fd = -1;
}

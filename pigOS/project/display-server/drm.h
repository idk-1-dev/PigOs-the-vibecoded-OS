#ifndef VDM_DRM_H
#define VDM_DRM_H

#include <stdint.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#define DRM_MAX_PLANES 32
#define DRM_MAX_CRTCS 32
#define DRM_MAX_CONNECTORS 32

typedef struct {
    int fd;
    uint32_t crtc_id;
    uint32_t connector_id;
    uint32_t plane_id;
    uint32_t crtc_index;
    drmModeModeInfo mode;
    uint32_t fb_id;
    int width;
    int height;
    int stride;
    int bpp;
    uint8_t *map;
    int map_size;
    uint32_t front_buf;
    uint32_t back_buf;
    uint8_t *buffers[2];
    uint32_t fb_ids[2];
    uint32_t gem_handles[2];
    volatile int flipping;
    drmEventContext evctx;
} vdm_drm_t;

int drm_init(vdm_drm_t *ctx);
void drm_shutdown(vdm_drm_t *ctx);
int drm_set_mode(vdm_drm_t *ctx);
int drm_create_framebuffers(vdm_drm_t *ctx);
int drm_page_flip(vdm_drm_t *ctx);
void drm_wait_flip(vdm_drm_t *ctx);
void drm_fill_screen(vdm_drm_t *ctx, uint32_t color);
void drm_handle_event(vdm_drm_t *ctx);

#endif

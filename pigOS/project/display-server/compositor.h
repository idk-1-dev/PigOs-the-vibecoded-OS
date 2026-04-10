#ifndef VDM_COMPOSITOR_H
#define VDM_COMPOSITOR_H

#include <stdint.h>
#include "drm.h"
#include "window.h"
#include "input.h"

#define CURSOR_WIDTH 16
#define CURSOR_HEIGHT 16

typedef struct {
    vdm_drm_t *drm;
    window_manager_t *wm;
    input_ctx_t *input;

    int focused_window_id;
    int cursor_x;
    int cursor_y;
    uint8_t cursor_data[CURSOR_WIDTH * CURSOR_HEIGHT * 4];
    int cursor_visible;

    uint32_t *composite_buf;
    int composite_stride;

    uint64_t last_frame_time;
    int frame_count;
} compositor_t;

int compositor_init(compositor_t *comp, vdm_drm_t *drm,
                    window_manager_t *wm, input_ctx_t *input);
void compositor_shutdown(compositor_t *comp);
void compositor_render(compositor_t *comp);
void compositor_route_input(compositor_t *comp);
void compositor_set_cursor(compositor_t *comp, int x, int y);
void compositor_draw_cursor(compositor_t *comp);
void composite_window(compositor_t *comp, window_t *win);

#endif

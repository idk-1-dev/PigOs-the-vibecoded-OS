#include "compositor.h"
#include "../log/log.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

static uint64_t get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void generate_cursor(compositor_t *comp) {
    uint32_t *pixels = (uint32_t *)comp->cursor_data;
    int w = CURSOR_WIDTH;
    int h = CURSOR_HEIGHT;

    memset(comp->cursor_data, 0, sizeof(comp->cursor_data));

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            uint32_t pixel;

            if ((x == 0) || (y == 0 && x < 10) || (x == 1 && y < 11) ||
                (y == x && x < 13) || (x == 3 && y >= 4 && y < 10) ||
                (y == 4 && x >= 4 && x < 8) || (x == 7 && y >= 5 && y < 8) ||
                (y == 7 && x >= 4 && x < 7)) {
                pixel = 0xFF000000;
            } else if ((x == 1 && y < 10) || (y == 1 && x < 9) ||
                       (y == x && x < 12) || (x == 2 && y >= 3 && y < 10) ||
                       (y == 3 && x >= 3 && x < 8) || (x == 6 && y >= 4 && y < 8) ||
                       (y == 6 && x >= 3 && x < 7)) {
                pixel = 0xFFFFFFFF;
            } else {
                pixel = 0x00000000;
            }
            pixels[idx] = pixel;
        }
    }
}

int compositor_init(compositor_t *comp, vdm_drm_t *drm,
                    window_manager_t *wm, input_ctx_t *input) {
    memset(comp, 0, sizeof(*comp));
    comp->drm = drm;
    comp->wm = wm;
    comp->input = input;
    comp->focused_window_id = -1;
    comp->cursor_x = drm->width / 2;
    comp->cursor_y = drm->height / 2;
    comp->cursor_visible = 1;
    comp->last_frame_time = get_time_ms();

    comp->composite_stride = drm->width * 4;
    comp->composite_buf = (uint32_t *)malloc(
        (size_t)drm->width * drm->height * 4);
    if (!comp->composite_buf) {
        LOG_ERROR("Failed to allocate composite buffer");
        return -1;
    }

    generate_cursor(comp);
    return 0;
}

void compositor_shutdown(compositor_t *comp) {
    if (comp->composite_buf)
        free(comp->composite_buf);
    memset(comp, 0, sizeof(*comp));
}

static inline uint32_t blend_pixel(uint32_t src, uint32_t dst) {
    uint8_t sa = (src >> 24) & 0xFF;
    if (sa == 255) return src;

    uint8_t sr = (src >> 16) & 0xFF;
    uint8_t sg = (src >> 8) & 0xFF;
    uint8_t sb = src & 0xFF;

    if (sa == 0) {
        return (0xFF << 24) | (sr << 16) | (sg << 8) | sb;
    }

    float a = sa / 255.0f;
    uint8_t dr = (dst >> 16) & 0xFF;
    uint8_t dg = (dst >> 8) & 0xFF;
    uint8_t db = dst & 0xFF;

    uint8_t r = (uint8_t)(sr * a + dr * (1.0f - a));
    uint8_t g = (uint8_t)(sg * a + dg * (1.0f - a));
    uint8_t b = (uint8_t)(sb * a + db * (1.0f - a));

    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

void composite_window(compositor_t *comp, window_t *win) {
    uint32_t *src = (uint32_t *)win->buffer.ptr;
    uint32_t *dst = comp->composite_buf;
    int sw = win->width;
    int sh = win->height;
    int dw = comp->drm->width;
    int dh = comp->drm->height;
    int ox = win->x;
    int oy = win->y;

    if (!src) return;

    int start_x = ox < 0 ? -ox : 0;
    int start_y = oy < 0 ? -oy : 0;
    int end_x = sw;
    int end_y = sh;

    if (ox + sw > dw) end_x = dw - ox;
    if (oy + sh > dh) end_y = dh - oy;

    if (start_x >= end_x || start_y >= end_y) return;

    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            int dx = ox + x;
            int dy = oy + y;

            if (dx < 0 || dx >= dw || dy < 0 || dy >= dh) continue;

            uint32_t sp = src[y * sw + x];
            dst[dy * dw + dx] = blend_pixel(sp, dst[dy * dw + dx]);
        }
    }
}

void compositor_draw_cursor(compositor_t *comp) {
    if (!comp->cursor_visible) return;

    uint32_t *cursor = (uint32_t *)comp->cursor_data;
    uint32_t *dst = comp->composite_buf;
    int dw = comp->drm->width;
    int dh = comp->drm->height;

    for (int y = 0; y < CURSOR_HEIGHT; y++) {
        for (int x = 0; x < CURSOR_WIDTH; x++) {
            int dx = comp->cursor_x + x;
            int dy = comp->cursor_y + y;
            if (dx < 0 || dx >= dw || dy < 0 || dy >= dh) continue;

            uint32_t cp = cursor[y * CURSOR_WIDTH + x];
            uint8_t ca = (cp >> 24) & 0xFF;
            if (ca == 0) continue;

            dst[dy * dw + dx] = blend_pixel(cp, dst[dy * dw + dx]);
        }
    }
}

void compositor_render(compositor_t *comp) {
    uint64_t frame_start = get_time_ms();
    vdm_drm_t *drm = comp->drm;
    uint32_t *dst = comp->composite_buf;
    int total_pixels = drm->width * drm->height;

    int any_dirty = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (comp->wm->windows[i].visible && comp->wm->windows[i].has_pending_commit) {
            any_dirty = 1;
            break;
        }
    }

    if (any_dirty) {
        memset(dst, 0x1a, (size_t)total_pixels * 4);

        for (int i = 0; i < MAX_WINDOWS; i++) {
            window_t *win = &comp->wm->windows[i];
            if (!win->visible) continue;
            composite_window(comp, win);
            win->has_pending_commit = 0;
        }
    }

    compositor_draw_cursor(comp);

    uint8_t *back_buf = drm->buffers[drm->back_buf];
    memcpy(back_buf, dst, (size_t)total_pixels * 4);

    if (drm_page_flip(drm) < 0) {
        drm_wait_flip(drm);
        drm_page_flip(drm);
    }

    uint64_t frame_end = get_time_ms();
    double frame_ms = (double)(frame_end - frame_start);
    double target_ms = 16.67;

    LOG_TRACE("Frame complete: %.2fms", frame_ms);

    if (frame_ms > target_ms * 1.5) {
        LOG_WARN("Frame too slow: %.2fms (target %.2fms)", frame_ms, target_ms);
    }

    comp->last_frame_time = frame_end;
    comp->frame_count++;
}

void compositor_route_input(compositor_t *comp) {
    input_ctx_t *input = comp->input;
    window_t *win;

    if (input->has_mouse_event) {
        comp->cursor_x = input->mouse_x;
        comp->cursor_y = input->mouse_y;

        if (comp->cursor_x < 0) comp->cursor_x = 0;
        if (comp->cursor_y < 0) comp->cursor_y = 0;
        if (comp->cursor_x >= comp->drm->width) comp->cursor_x = comp->drm->width - 1;
        if (comp->cursor_y >= comp->drm->height) comp->cursor_y = comp->drm->height - 1;

        win = window_find_at(comp->wm, comp->cursor_x, comp->cursor_y);
        if (win) {
            if ((int)win->id != comp->focused_window_id) {
                LOG_DEBUG("Input routed to window=%u", win->id);
            }
            comp->focused_window_id = (int)win->id;
        } else {
            comp->focused_window_id = -1;
        }
        input->has_mouse_event = 0;
    }
}

void compositor_set_cursor(compositor_t *comp, int x, int y) {
    comp->cursor_x = x;
    comp->cursor_y = y;
}

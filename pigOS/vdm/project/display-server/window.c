#include "window.h"
#include "../log/log.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void window_manager_init(window_manager_t *wm) {
    memset(wm, 0, sizeof(*wm));
    wm->next_id = 1;
}

int window_create(window_manager_t *wm, int client_idx, int pid,
                  int w, int h, const char *title) {
    window_t *win;
    int i;

    for (i = 0; i < MAX_WINDOWS; i++) {
        if (!wm->windows[i].visible) break;
    }
    if (i >= MAX_WINDOWS) {
        LOG_ERROR("Maximum window count reached");
        return -1;
    }

    win = &wm->windows[i];
    memset(win, 0, sizeof(*win));
    win->id = wm->next_id++;
    win->pid = pid;
    win->client_idx = client_idx;
    win->x = 0;
    win->y = 0;
    win->width = w;
    win->height = h;
    win->visible = 1;
    win->has_pending_commit = 0;
    strncpy(win->title, title, WINDOW_TITLE_MAX - 1);
    win->title[WINDOW_TITLE_MAX - 1] = '\0';

    if (shm_create(&win->buffer, w, h) < 0) {
        LOG_ERROR("Failed to allocate shm for window id=%u: %s", win->id, strerror(errno));
        win->visible = 0;
        return -1;
    }

    wm->window_count++;
    LOG_INFO("Window created: id=%u pid=%d size=%dx%d title='%s'",
             win->id, pid, w, h, title);
    return (int)win->id;
}

void window_destroy(window_manager_t *wm, uint32_t id) {
    window_t *win = window_find(wm, id);
    if (!win) return;

    shm_destroy(&win->buffer);
    win->visible = 0;
    wm->window_count--;
    LOG_INFO("Window destroyed: id=%u", id);
}

window_t *window_find(window_manager_t *wm, uint32_t id) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (wm->windows[i].visible && wm->windows[i].id == id)
            return &wm->windows[i];
    }
    return NULL;
}

window_t *window_find_at(window_manager_t *wm, int x, int y) {
    window_t *top = NULL;

    for (int i = 0; i < MAX_WINDOWS; i++) {
        window_t *w = &wm->windows[i];
        if (!w->visible) continue;
        if (x >= w->x && x < w->x + w->width &&
            y >= w->y && y < w->y + w->height) {
            top = w;
        }
    }
    return top;
}

void window_move(window_manager_t *wm, uint32_t id, int x, int y) {
    window_t *win = window_find(wm, id);
    if (!win) return;
    win->x = x;
    win->y = y;
}

void window_mark_committed(window_manager_t *wm, uint32_t id) {
    window_t *win = window_find(wm, id);
    if (!win) return;
    win->has_pending_commit = 1;
}

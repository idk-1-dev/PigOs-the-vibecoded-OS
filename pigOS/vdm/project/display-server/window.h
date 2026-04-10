#ifndef MYWM_WINDOW_H
#define MYWM_WINDOW_H

#include <stdint.h>
#include "shm.h"

#define WINDOW_TITLE_MAX 64
#define MAX_WINDOWS 128

typedef struct {
    uint32_t id;
    int pid;
    int client_idx;
    int x;
    int y;
    int width;
    int height;
    char title[WINDOW_TITLE_MAX];
    shm_buf_t buffer;
    int visible;
    int has_pending_commit;
} window_t;

typedef struct {
    window_t windows[MAX_WINDOWS];
    int window_count;
    uint32_t next_id;
} window_manager_t;

void window_manager_init(window_manager_t *wm);
int window_create(window_manager_t *wm, int client_idx, int pid,
                  int w, int h, const char *title);
void window_destroy(window_manager_t *wm, uint32_t id);
window_t *window_find(window_manager_t *wm, uint32_t id);
window_t *window_find_at(window_manager_t *wm, int x, int y);
void window_move(window_manager_t *wm, uint32_t id, int x, int y);
void window_mark_committed(window_manager_t *wm, uint32_t id);

#endif

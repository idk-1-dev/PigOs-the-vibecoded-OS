#ifndef MYWM_CLIENT_H
#define MYWM_CLIENT_H

#include <stdint.h>

#define CLIENT_SOCKET_PATH_MAX 256
#define CLIENT_TITLE_MAX 64

typedef enum {
    CLIENT_MSG_CREATE_WINDOW   = 1,
    CLIENT_MSG_COMMIT_BUFFER   = 2,
    CLIENT_MSG_DESTROY_WINDOW  = 3,
    CLIENT_MSG_MOVE_WINDOW     = 4,

    CLIENT_MSG_WINDOW_CREATED  = 101,
    CLIENT_MSG_KEY_EVENT       = 102,
    CLIENT_MSG_MOUSE_MOVE      = 103,
    CLIENT_MSG_MOUSE_BUTTON    = 104,
    CLIENT_MSG_WINDOW_RESIZE   = 105,
    CLIENT_MSG_WINDOW_CLOSE    = 106
} client_msg_type_t;

typedef struct {
    uint32_t type;
    uint32_t width;
    uint32_t height;
    char title[CLIENT_TITLE_MAX];
} client_msg_create_window_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
} client_msg_commit_buffer_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
} client_msg_destroy_window_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
    int32_t x;
    int32_t y;
} client_msg_move_window_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
    int shm_fd;
} client_msg_window_created_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
    uint32_t keycode;
    uint8_t pressed;
    char text[8];
} client_msg_key_event_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
    int32_t x;
    int32_t y;
} client_msg_mouse_move_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
    uint32_t button;
    uint8_t pressed;
} client_msg_mouse_button_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
    uint32_t w;
    uint32_t h;
} client_msg_window_resize_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
} client_msg_window_close_t;

typedef enum {
    EVENT_KEY,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_BUTTON,
    EVENT_RESIZE,
    EVENT_CLOSE,
    EVENT_NONE
} event_type_t;

typedef struct {
    event_type_t type;
    uint32_t window_id;
    union {
        struct { uint32_t keycode; uint8_t pressed; char text[8]; } key;
        struct { int32_t x; int32_t y; } mouse_move;
        struct { uint32_t button; uint8_t pressed; } mouse_button;
        struct { uint32_t w; uint32_t h; } resize;
    };
} event_t;

typedef struct {
    int fd;
    char socket_path[CLIENT_SOCKET_PATH_MAX];
} display_t;

typedef struct {
    display_t *display;
    uint32_t id;
    int width;
    int height;
    int shm_fd;
    uint32_t *buffer;
    char title[CLIENT_TITLE_MAX];
} window_t;

display_t*  display_connect(void);
window_t*   window_create(display_t *disp, int w, int h, const char *title);
uint32_t*   window_get_buffer(window_t *win);
void        window_commit(window_t *win);
void        window_move(window_t *win, int x, int y);
void        window_destroy(window_t *win);
int         event_poll(window_t *win, event_t *out);
void        display_disconnect(display_t *disp);

#endif

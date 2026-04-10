#ifndef MYWM_SOCKET_H
#define MYWM_SOCKET_H

#include <stdint.h>
#include <stddef.h>

#define SOCKET_PATH_MAX 256
#define MAX_CLIENTS 64

typedef enum {
    MSG_CREATE_WINDOW   = 1,
    MSG_COMMIT_BUFFER   = 2,
    MSG_DESTROY_WINDOW  = 3,
    MSG_MOVE_WINDOW     = 4,

    MSG_WINDOW_CREATED  = 101,
    MSG_KEY_EVENT       = 102,
    MSG_MOUSE_MOVE      = 103,
    MSG_MOUSE_BUTTON    = 104,
    MSG_WINDOW_RESIZE   = 105,
    MSG_WINDOW_CLOSE    = 106
} msg_type_t;

typedef struct {
    uint32_t type;
    uint32_t width;
    uint32_t height;
    char title[64];
} msg_create_window_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
} msg_commit_buffer_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
} msg_destroy_window_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
    int32_t x;
    int32_t y;
} msg_move_window_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
    int shm_fd;
} msg_window_created_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
    uint32_t keycode;
    uint8_t pressed;
    char text[8];
} msg_key_event_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
    int32_t x;
    int32_t y;
} msg_mouse_move_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
    uint32_t button;
    uint8_t pressed;
} msg_mouse_button_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
    uint32_t w;
    uint32_t h;
} msg_window_resize_t;

typedef struct {
    uint32_t type;
    uint32_t window_id;
} msg_window_close_t;

typedef struct {
    int fd;
    int pid;
    int active;
} client_t;

typedef struct {
    int listen_fd;
    char path[SOCKET_PATH_MAX];
    client_t clients[MAX_CLIENTS];
    int client_count;
} socket_server_t;

int socket_server_init(socket_server_t *srv);
void socket_server_shutdown(socket_server_t *srv);
int socket_server_accept(socket_server_t *srv);
int socket_server_send(int fd, const void *data, size_t len);
int socket_server_send_with_fd(int fd, const void *data, size_t len, int anc_fd);
int socket_server_recv_msg(int fd, void *buf, size_t len);

#endif

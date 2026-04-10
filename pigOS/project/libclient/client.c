#include "client.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

static int send_msg(int fd, const void *data, size_t len) {
    ssize_t n = send(fd, data, len, MSG_NOSIGNAL);
    return (n == (ssize_t)len) ? 0 : -1;
}

static int send_msg_with_peek(int fd, const void *data, size_t len) {
    ssize_t n = send(fd, data, len, MSG_NOSIGNAL);
    return (n == (ssize_t)len) ? 0 : -1;
}

static int recv_exact(int fd, void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = recv(fd, (char *)buf + total, len - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}

static int recv_msg_with_fd(int fd, void *data, size_t len) {
    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct iovec io;
    char cmsg_buf[64];
    int recv_fd_val = -1;

    memset(&msg, 0, sizeof(msg));
    io.iov_base = data;
    io.iov_len = len;
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);

    if (recvmsg(fd, &msg, 0) < 0) return -1;

    cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
        memcpy(&recv_fd_val, CMSG_DATA(cmsg), sizeof(int));
    }

    return recv_fd_val;
}

static int recv_fd(int fd) {
    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct iovec io;
    char buf[1];
    char cmsg_buf[64];
    int recv_fd_val;

    memset(&msg, 0, sizeof(msg));
    io.iov_base = buf;
    io.iov_len = sizeof(buf);
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);

    if (recvmsg(fd, &msg, 0) < 0) return -1;

    cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg || cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS)
        return -1;

    memcpy(&recv_fd_val, CMSG_DATA(cmsg), sizeof(int));
    return recv_fd_val;
}

display_t* display_connect(void) {
    display_t *disp;
    const char *env_path;
    struct sockaddr_un addr;

    disp = calloc(1, sizeof(*disp));
    if (!disp) return NULL;

    env_path = getenv("VDM_DISPLAY");
    if (!env_path) {
        free(disp);
        return NULL;
    }

    strncpy(disp->socket_path, env_path, sizeof(disp->socket_path) - 1);
    disp->socket_path[sizeof(disp->socket_path) - 1] = '\0';

    disp->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (disp->fd < 0) {
        free(disp);
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, disp->socket_path, sizeof(addr.sun_path) - 1);

    if (connect(disp->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(disp->fd);
        free(disp);
        return NULL;
    }

    return disp;
}

window_t* window_create(display_t *disp, int w, int h, const char *title) {
    client_msg_create_window_t req;
    client_msg_window_created_t resp;
    window_t *win;
    int shm_fd;
    void *ptr;
    size_t size;

    if (!disp) return NULL;

    memset(&req, 0, sizeof(req));
    req.type = CLIENT_MSG_CREATE_WINDOW;
    req.width = w;
    req.height = h;
    strncpy(req.title, title, CLIENT_TITLE_MAX - 1);
    req.title[CLIENT_TITLE_MAX - 1] = '\0';

    if (send_msg(disp->fd, &req, sizeof(req)) < 0) return NULL;

    shm_fd = recv_msg_with_fd(disp->fd, &resp, sizeof(resp));
    if (shm_fd < 0) return NULL;
    if (resp.type != CLIENT_MSG_WINDOW_CREATED) {
        close(shm_fd);
        return NULL;
    }

    size = (size_t)w * h * 4;
    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        close(shm_fd);
        return NULL;
    }

    win = calloc(1, sizeof(*win));
    if (!win) {
        munmap(ptr, size);
        close(shm_fd);
        return NULL;
    }

    win->display = disp;
    win->id = resp.window_id;
    win->width = w;
    win->height = h;
    win->shm_fd = shm_fd;
    win->buffer = (uint32_t *)ptr;
    strncpy(win->title, title, CLIENT_TITLE_MAX - 1);
    win->title[CLIENT_TITLE_MAX - 1] = '\0';

    return win;
}

uint32_t* window_get_buffer(window_t *win) {
    if (!win) return NULL;
    return win->buffer;
}

void window_commit(window_t *win) {
    client_msg_commit_buffer_t msg;

    if (!win) return;

    msg.type = CLIENT_MSG_COMMIT_BUFFER;
    msg.window_id = win->id;
    msync(win->buffer, (size_t)win->width * win->height * 4, MS_SYNC);
    send_msg(win->display->fd, &msg, sizeof(msg));
}

void window_move(window_t *win, int x, int y) {
    client_msg_move_window_t msg;

    if (!win) return;

    msg.type = CLIENT_MSG_MOVE_WINDOW;
    msg.window_id = win->id;
    msg.x = x;
    msg.y = y;
    send_msg(win->display->fd, &msg, sizeof(msg));
}

void window_destroy(window_t *win) {
    client_msg_destroy_window_t msg;
    size_t size;

    if (!win) return;

    msg.type = CLIENT_MSG_DESTROY_WINDOW;
    msg.window_id = win->id;
    send_msg(win->display->fd, &msg, sizeof(msg));

    size = (size_t)win->width * win->height * 4;
    if (win->buffer) munmap(win->buffer, size);
    if (win->shm_fd >= 0) close(win->shm_fd);
    free(win);
}

int event_poll(window_t *win, event_t *out) {
    uint32_t msg_type;
    ssize_t n;
    int fd;

    if (!win || !out) return -1;

    fd = win->display->fd;

    n = recv(fd, &msg_type, sizeof(msg_type), MSG_PEEK | MSG_DONTWAIT);
    if (n <= 0) {
        out->type = EVENT_NONE;
        return 0;
    }

    switch (msg_type) {
    case CLIENT_MSG_KEY_EVENT: {
        client_msg_key_event_t msg;
        if (recv_exact(fd, &msg, sizeof(msg)) < 0) return -1;
        out->type = EVENT_KEY;
        out->window_id = msg.window_id;
        out->key.keycode = msg.keycode;
        out->key.pressed = msg.pressed;
        memcpy(out->key.text, msg.text, sizeof(out->key.text));
        break;
    }
    case CLIENT_MSG_MOUSE_MOVE: {
        client_msg_mouse_move_t msg;
        if (recv_exact(fd, &msg, sizeof(msg)) < 0) return -1;
        out->type = EVENT_MOUSE_MOVE;
        out->window_id = msg.window_id;
        out->mouse_move.x = msg.x;
        out->mouse_move.y = msg.y;
        break;
    }
    case CLIENT_MSG_MOUSE_BUTTON: {
        client_msg_mouse_button_t msg;
        if (recv_exact(fd, &msg, sizeof(msg)) < 0) return -1;
        out->type = EVENT_MOUSE_BUTTON;
        out->window_id = msg.window_id;
        out->mouse_button.button = msg.button;
        out->mouse_button.pressed = msg.pressed;
        break;
    }
    case CLIENT_MSG_WINDOW_RESIZE: {
        client_msg_window_resize_t msg;
        if (recv_exact(fd, &msg, sizeof(msg)) < 0) return -1;
        out->type = EVENT_RESIZE;
        out->window_id = msg.window_id;
        out->resize.w = msg.w;
        out->resize.h = msg.h;
        break;
    }
    case CLIENT_MSG_WINDOW_CLOSE: {
        client_msg_window_close_t msg;
        if (recv_exact(fd, &msg, sizeof(msg)) < 0) return -1;
        out->type = EVENT_CLOSE;
        out->window_id = msg.window_id;
        break;
    }
    default:
        out->type = EVENT_NONE;
        return 0;
    }

    return 1;
}

void display_disconnect(display_t *disp) {
    if (!disp) return;
    if (disp->fd >= 0) close(disp->fd);
    free(disp);
}

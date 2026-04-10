#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/socket.h>

#include "../log/log.h"
#include "drm.h"
#include "socket.h"
#include "shm.h"
#include "window.h"
#include "input.h"
#include "compositor.h"
#include "tty.h"

#define TARGET_FPS 60
#define FRAME_TIME_US (1000000 / TARGET_FPS)

static volatile int running = 1;

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

static void setup_signals(void) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
}

static void set_display_env(const char *socket_path) {
    char env[512];
    snprintf(env, sizeof(env), "VDM_DISPLAY=%s", socket_path);
    putenv(env);
}

static int handle_client_message(socket_server_t *srv, int client_idx,
                                 window_manager_t *wm, compositor_t *comp) {
    (void)comp;
    client_t *client = &srv->clients[client_idx];
    uint32_t msg_type;
    ssize_t n;

    n = recv(client->fd, &msg_type, sizeof(msg_type), MSG_PEEK);
    if (n <= 0) return -1;

    switch (msg_type) {
    case MSG_CREATE_WINDOW: {
        msg_create_window_t msg;
        n = recv(client->fd, &msg, sizeof(msg), 0);
        if (n != (ssize_t)sizeof(msg)) return -1;

        int win_id = window_create(wm, client_idx, client->pid,
                                   msg.width, msg.height, msg.title);
        if (win_id < 0) {
            msg_window_close_t resp = {
                .type = MSG_WINDOW_CLOSE,
                .window_id = 0
            };
            socket_server_send(client->fd, &resp, sizeof(resp));
            return 0;
        }

        window_t *win = window_find(wm, (uint32_t)win_id);
        msg_window_created_t resp = {
            .type = MSG_WINDOW_CREATED,
            .window_id = (uint32_t)win_id,
            .shm_fd = win->buffer.fd
        };

        socket_server_send_with_fd(client->fd, &resp, sizeof(resp), win->buffer.fd);
        break;
    }

    case MSG_COMMIT_BUFFER: {
        msg_commit_buffer_t msg;
        n = recv(client->fd, &msg, sizeof(msg), 0);
        if (n != (ssize_t)sizeof(msg)) return -1;

        window_t *win = window_find(wm, msg.window_id);
        if (!win) {
            LOG_WARN("Commit from unknown window id=%u", msg.window_id);
            return 0;
        }

        window_mark_committed(wm, msg.window_id);
        LOG_DEBUG("Buffer committed: window=%u", msg.window_id);
        break;
    }

    case MSG_DESTROY_WINDOW: {
        msg_destroy_window_t msg;
        n = recv(client->fd, &msg, sizeof(msg), 0);
        if (n != (ssize_t)sizeof(msg)) return -1;

        window_destroy(wm, msg.window_id);
        break;
    }

    case MSG_MOVE_WINDOW: {
        msg_move_window_t msg;
        n = recv(client->fd, &msg, sizeof(msg), 0);
        if (n != (ssize_t)sizeof(msg)) return -1;

        window_move(wm, msg.window_id, msg.x, msg.y);
        break;
    }

    default:
        LOG_WARN("Unknown message type: %u from client %d", msg_type, client_idx);
        return -1;
    }

    return 0;
}

static void handle_client_disconnect(socket_server_t *srv, int client_idx,
                                     window_manager_t *wm) {
    client_t *client = &srv->clients[client_idx];

    LOG_INFO("Client disconnected: pid=%d", client->pid);

    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (wm->windows[i].visible && wm->windows[i].client_idx == client_idx) {
            window_destroy(wm, wm->windows[i].id);
        }
    }

    close(client->fd);
    client->active = 0;
    client->fd = -1;
    srv->client_count--;
}

static void send_event_to_client(socket_server_t *srv, int client_idx,
                                 const void *data, size_t len) {
    if (client_idx < 0 || client_idx >= MAX_CLIENTS) return;
    if (!srv->clients[client_idx].active) return;
    socket_server_send(srv->clients[client_idx].fd, data, len);
}

static void broadcast_input_events(socket_server_t *srv, window_manager_t *wm,
                                   input_ctx_t *input, compositor_t *comp) {
    if (input->has_key_event) {
        window_t *win = window_find(wm, (uint32_t)comp->focused_window_id);
        if (win) {
            msg_key_event_t evt = {
                .type = MSG_KEY_EVENT,
                .window_id = win->id,
                .keycode = input->last_keycode,
                .pressed = input->last_key_pressed
            };
            memcpy(evt.text, input->last_text, sizeof(evt.text));
            send_event_to_client(srv, win->client_idx, &evt, sizeof(evt));
        }
        input->has_key_event = 0;
    }

    if (input->has_mouse_event) {
        window_t *win = window_find_at(wm, input->mouse_x, input->mouse_y);
        if (win) {
            msg_mouse_move_t evt = {
                .type = MSG_MOUSE_MOVE,
                .window_id = win->id,
                .x = input->mouse_x - win->x,
                .y = input->mouse_y - win->y
            };
            send_event_to_client(srv, win->client_idx, &evt, sizeof(evt));

            if (input->last_button_pressed) {
                msg_mouse_button_t evt2 = {
                    .type = MSG_MOUSE_BUTTON,
                    .window_id = win->id,
                    .button = input->last_button,
                    .pressed = input->last_button_pressed
                };
                send_event_to_client(srv, win->client_idx, &evt2, sizeof(evt2));
            }
        }
        input->has_mouse_event = 0;
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    log_ctx_t log_ctx;
    tty_ctx_t tty;
    vdm_drm_t drm;
    socket_server_t socket_srv;
    window_manager_t wm;
    input_ctx_t input;
    compositor_t comp;
    struct pollfd pfds[MAX_CLIENTS + 3];
    int nfds;

    g_log_ctx = &log_ctx;
    log_init(&log_ctx, "display-server", LOG_LEVEL_DEBUG);

    signal(SIGUSR1, log_sigusr1_handler);
    setup_signals();

    LOG_INFO("Starting display server");

    memset(&tty, 0, sizeof(tty));
    tty.fd = -1;
    if (tty_init(&tty) < 0) {
        LOG_WARN("TTY initialization failed, continuing anyway");
    }

    memset(&drm, 0, sizeof(drm));
    drm.fd = -1;

    if (drm_init(&drm) < 0) {
        LOG_FATAL_ABORT("Cannot continue without DRM");
    }

    if (drm_set_mode(&drm) < 0) {
        LOG_FATAL_ABORT("Cannot set display mode");
    }

    if (drm_create_framebuffers(&drm) < 0) {
        LOG_FATAL_ABORT("Cannot create framebuffers");
    }

    if (socket_server_init(&socket_srv) < 0) {
        LOG_FATAL_ABORT("Cannot create socket server");
    }

    set_display_env(socket_srv.path);

    window_manager_init(&wm);

    if (input_init(&input) < 0) {
        LOG_ERROR("Input initialization failed, continuing without input");
        memset(&input, 0, sizeof(input));
    }

    if (compositor_init(&comp, &drm, &wm, &input) < 0) {
        LOG_FATAL_ABORT("Cannot initialize compositor");
    }

    LOG_INFO("Display server ready");

    while (running) {
        nfds = 0;

        pfds[nfds].fd = socket_srv.listen_fd;
        pfds[nfds].events = POLLIN;
        pfds[nfds].revents = 0;
        nfds++;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (socket_srv.clients[i].active) {
                pfds[nfds].fd = socket_srv.clients[i].fd;
                pfds[nfds].events = POLLIN;
                pfds[nfds].revents = 0;
                nfds++;
            }
        }

        if (input.libinput) {
            pfds[nfds].fd = libinput_get_fd(input.libinput);
            pfds[nfds].events = POLLIN;
            pfds[nfds].revents = 0;
            nfds++;
        }

        pfds[nfds].fd = drm.fd;
        pfds[nfds].events = POLLIN;
        pfds[nfds].revents = 0;
        int drm_poll_idx = nfds;
        nfds++;

        int timeout = 16;
        int ret = poll(pfds, nfds, timeout);

        if (ret < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("poll failed: %s", strerror(errno));
            break;
        }

        int pfd_idx = 0;

        if (pfds[pfd_idx].revents & POLLIN) {
            socket_server_accept(&socket_srv);
        }
        pfd_idx++;

        for (int i = 0; i < MAX_CLIENTS && pfd_idx < nfds - 2; i++) {
            if (!socket_srv.clients[i].active) continue;

            if (pfds[pfd_idx].revents & (POLLIN | POLLHUP | POLLERR)) {
                if (pfds[pfd_idx].revents & POLLHUP ||
                    pfds[pfd_idx].revents & POLLERR) {
                    LOG_WARN("Client disconnected unexpectedly: pid=%d",
                             socket_srv.clients[i].pid);
                    handle_client_disconnect(&socket_srv, i, &wm);
                } else {
                    if (handle_client_message(&socket_srv, i, &wm, &comp) < 0) {
                        handle_client_disconnect(&socket_srv, i, &wm);
                    }
                }
            }
            pfd_idx++;
        }

        if (input.libinput && pfd_idx < nfds - 1) {
            if (pfds[pfd_idx].revents & POLLIN) {
                input_dispatch(&input);
            }
            pfd_idx++;
        }

        if (pfds[drm_poll_idx].revents & POLLIN) {
            drm_handle_event(&drm);
        }

        compositor_route_input(&comp);
        broadcast_input_events(&socket_srv, &wm, &input, &comp);

        compositor_render(&comp);

        if (input.want_exit) {
            running = 0;
        }
    }

    LOG_INFO("Shutting down display server");

    compositor_shutdown(&comp);
    input_shutdown(&input);
    socket_server_shutdown(&socket_srv);
    drm_shutdown(&drm);
    tty_shutdown(&tty);
    log_shutdown(&log_ctx);

    return 0;
}

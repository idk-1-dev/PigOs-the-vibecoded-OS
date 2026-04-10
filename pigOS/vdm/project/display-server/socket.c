#include "socket.h"
#include "../log/log.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/uio.h>

static char *socket_path_from_uid(char *buf, size_t len) {
    uid_t uid = getuid();
    snprintf(buf, len, "/run/user/%u/vdm-0", uid);
    return buf;
}

int socket_server_init(socket_server_t *srv) {
    struct sockaddr_un addr;

    memset(srv, 0, sizeof(*srv));
    socket_path_from_uid(srv->path, sizeof(srv->path));

    srv->listen_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (srv->listen_fd < 0) {
        LOG_ERROR("Failed to create unix socket: %s", strerror(errno));
        return -1;
    }

    unlink(srv->path);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, srv->path, sizeof(addr.sun_path) - 1);

    if (bind(srv->listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind unix socket: %s", strerror(errno));
        close(srv->listen_fd);
        return -1;
    }

    chmod(srv->path, 0660);

    if (listen(srv->listen_fd, 16) < 0) {
        LOG_ERROR("Failed to listen on unix socket: %s", strerror(errno));
        close(srv->listen_fd);
        return -1;
    }

    LOG_INFO("Unix socket created: %s", srv->path);
    return 0;
}

void socket_server_shutdown(socket_server_t *srv) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (srv->clients[i].active) {
            close(srv->clients[i].fd);
            srv->clients[i].active = 0;
        }
    }
    if (srv->listen_fd >= 0) {
        close(srv->listen_fd);
        unlink(srv->path);
    }
}

int socket_server_accept(socket_server_t *srv) {
    struct sockaddr_un addr;
    socklen_t addrlen = sizeof(addr);
    int fd;
    int i;

    fd = accept(srv->listen_fd, (struct sockaddr *)&addr, &addrlen);
    if (fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            LOG_ERROR("Failed to accept client: %s", strerror(errno));
        return -1;
    }

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (!srv->clients[i].active) break;
    }
    if (i >= MAX_CLIENTS) {
        LOG_ERROR("Too many clients");
        close(fd);
        return -1;
    }

    struct ucred cred;
    socklen_t cred_len = sizeof(cred);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len) == 0) {
        srv->clients[i].pid = cred.pid;
    } else {
        srv->clients[i].pid = 0;
    }

    srv->clients[i].fd = fd;
    srv->clients[i].active = 1;
    srv->client_count++;

    LOG_INFO("Client connected: pid=%d fd=%d", srv->clients[i].pid, fd);
    return i;
}

int socket_server_send(int fd, const void *data, size_t len) {
    ssize_t sent = send(fd, data, len, MSG_NOSIGNAL);
    return (sent == (ssize_t)len) ? 0 : -1;
}

int socket_server_send_with_fd(int fd, const void *data, size_t len, int anc_fd) {
    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct iovec io;
    char cmsg_buf[CMSG_SPACE(sizeof(int))];

    memset(&msg, 0, sizeof(msg));
    io.iov_base = (void *)data;
    io.iov_len = len;
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    *((int *)CMSG_DATA(cmsg)) = anc_fd;
    msg.msg_controllen = cmsg->cmsg_len;

    return sendmsg(fd, &msg, MSG_NOSIGNAL);
}

int socket_server_recv_msg(int fd, void *buf, size_t len) {
    ssize_t n = recv(fd, buf, len, 0);
    return (int)n;
}

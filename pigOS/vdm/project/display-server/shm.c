#include "shm.h"
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

int shm_create(shm_buf_t *buf, int width, int height) {
    int fd;
    size_t size;
    void *ptr;

    size = (size_t)width * height * 4;

    fd = memfd_create("vdm-win", 0);
    if (fd < 0) return -1;

    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }

    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        close(fd);
        return -1;
    }

    memset(ptr, 0, size);

    buf->fd = fd;
    buf->ptr = ptr;
    buf->size = size;
    buf->width = width;
    buf->height = height;
    buf->stride = width * 4;
    return 0;
}

int shm_attach(shm_buf_t *buf, int fd, int width, int height) {
    size_t size;
    void *ptr;

    size = (size_t)width * height * 4;

    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) return -1;

    buf->fd = fd;
    buf->ptr = ptr;
    buf->size = size;
    buf->width = width;
    buf->height = height;
    buf->stride = width * 4;
    return 0;
}

void shm_destroy(shm_buf_t *buf) {
    if (buf->ptr && buf->ptr != MAP_FAILED)
        munmap(buf->ptr, buf->size);
    if (buf->fd >= 0)
        close(buf->fd);
    memset(buf, 0, sizeof(*buf));
    buf->fd = -1;
}

int shm_send_fd(int sock_fd, int shm_fd) {
    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct iovec io;
    char buf[1] = { 'X' };
    char cmsg_buf[CMSG_SPACE(sizeof(int))];

    memset(&msg, 0, sizeof(msg));
    io.iov_base = buf;
    io.iov_len = sizeof(buf);
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    *((int *)CMSG_DATA(cmsg)) = shm_fd;
    msg.msg_controllen = cmsg->cmsg_len;

    return sendmsg(sock_fd, &msg, 0);
}

int shm_recv_fd(int sock_fd) {
    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct iovec io;
    char buf[1];
    char cmsg_buf[CMSG_SPACE(sizeof(int))];
    int fd;

    memset(&msg, 0, sizeof(msg));
    io.iov_base = buf;
    io.iov_len = sizeof(buf);
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);

    if (recvmsg(sock_fd, &msg, 0) < 0) return -1;

    cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg || cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS)
        return -1;

    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    return fd;
}

#ifndef MYWM_SHM_H
#define MYWM_SHM_H

#include <stdint.h>
#include <sys/types.h>

typedef struct {
    int fd;
    void *ptr;
    size_t size;
    int width;
    int height;
    int stride;
} shm_buf_t;

int shm_create(shm_buf_t *buf, int width, int height);
int shm_attach(shm_buf_t *buf, int fd, int width, int height);
void shm_destroy(shm_buf_t *buf);
int shm_send_fd(int sock_fd, int shm_fd);
int shm_recv_fd(int sock_fd);

#endif

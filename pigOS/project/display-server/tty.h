#ifndef MYWM_TTY_H
#define MYWM_TTY_H

#include <stdbool.h>
#include <termios.h>

typedef struct {
    int fd;
    int vt_num;
    bool active;
    struct termios orig_termios;
    bool termios_saved;
} tty_ctx_t;

int tty_init(tty_ctx_t *ctx);
void tty_shutdown(tty_ctx_t *ctx);

#endif

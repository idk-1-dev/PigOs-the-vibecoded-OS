#include "tty.h"
#include "../log/log.h"
#include <string.h>

int tty_init(tty_ctx_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->active = true;
    return 0;
}

void tty_shutdown(tty_ctx_t *ctx) {
    ctx->active = false;
}

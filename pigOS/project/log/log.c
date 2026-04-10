#include "log.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

static const log_level_info_t level_infos[] = {
    [LOG_LEVEL_TRACE] = { "TRACE", "\033[2m", "\033[0m" },
    [LOG_LEVEL_DEBUG] = { "DEBUG", "\033[36m", "\033[0m" },
    [LOG_LEVEL_INFO]  = { "INFO ", "\033[32m", "\033[0m" },
    [LOG_LEVEL_WARN]  = { "WARN ", "\033[33m", "\033[0m" },
    [LOG_LEVEL_ERROR] = { "ERROR", "\033[31m", "\033[0m" },
    [LOG_LEVEL_FATAL] = { "FATAL", "\033[1;91m", "\033[0m" }
};

log_ctx_t *g_log_ctx = NULL;

static void ensure_dir(const char *path) {
    char tmp[LOG_MAX_PATH];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

static void write_timestamp(char *buf, size_t len) {
    struct timespec ts;
    struct tm tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm);
    snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec,
             (int)(ts.tv_nsec / 1000000));
}

static const char *basename_file(const char *path) {
    const char *p = strrchr(path, '/');
    return p ? p + 1 : path;
}

void log_init(log_ctx_t *ctx, const char *component, log_level_t min_level) {
    const char *home = getenv("HOME");
    char log_dir[LOG_MAX_PATH];

    memset(ctx, 0, sizeof(*ctx));
    ctx->min_level = min_level;
    ctx->orig_min_level = min_level;
    ctx->component = component;
    ctx->use_color = isatty(STDERR_FILENO);
    ctx->trace_burst = 0;
    ctx->trace_burst_end = 0;
    pthread_mutex_init(&ctx->lock, NULL);

    snprintf(log_dir, sizeof(log_dir), "/var/log/vdm");
    ensure_dir(log_dir);

    snprintf(ctx->display_path, sizeof(ctx->display_path),
             "%s/display.log", log_dir);
    snprintf(ctx->combined_path, sizeof(ctx->combined_path),
             "%s/vdm.log", log_dir);

    /* Truncate old logs for a clean session */
    ctx->display_log = fopen(ctx->display_path, "w");
    ctx->combined_log = fopen(ctx->combined_path, "w");
}

void log_shutdown(log_ctx_t *ctx) {
    if (ctx->display_log) fclose(ctx->display_log);
    if (ctx->combined_log) fclose(ctx->combined_log);
    pthread_mutex_destroy(&ctx->lock);
}

void log_rotate(log_ctx_t *ctx, FILE *fp, const char *path) {
    struct stat st;
    char old[LOG_MAX_PATH], new[LOG_MAX_PATH];

    if (!fp || fstat(fileno(fp), &st) != 0) return;
    if ((unsigned long long)st.st_size < LOG_ROTATE_SIZE) return;

    fclose(fp);

    for (int i = LOG_ROTATE_COUNT; i >= 1; i--) {
        snprintf(old, sizeof(old), "%s.%d", path, i);
        snprintf(new, sizeof(new), "%s.%d", path, i + 1);
        if (i == LOG_ROTATE_COUNT) {
            unlink(old);
        } else {
            rename(old, new);
        }
    }
    rename(path, new);
    ctx->display_log = fopen(ctx->display_path, "a");
    ctx->combined_log = fopen(ctx->combined_path, "a");
}

void log_write(log_ctx_t *ctx, log_level_t level,
               const char *file, int line, const char *fmt, ...) {
    char ts[64];
    char msg[LOG_MAX_LINE];
    char line_buf[LOG_MAX_LINE * 2];
    va_list ap;
    const log_level_info_t *li;
    const char *bn;
    time_t now;

    if (!ctx) return;

    log_level_t effective_level = ctx->min_level;
    if (ctx->trace_burst) {
        now = time(NULL);
        if (now >= ctx->trace_burst_end) {
            ctx->trace_burst = 0;
            ctx->min_level = ctx->orig_min_level;
        } else {
            effective_level = LOG_LEVEL_TRACE;
        }
    }

    if (level < effective_level) return;

    li = &level_infos[level];
    bn = basename_file(file);

    write_timestamp(ts, sizeof(ts));

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    snprintf(line_buf, sizeof(line_buf),
             "[%s] [%-5s] [%s] [%s:%d] %s\n",
             ts, li->name, ctx->component, bn, line, msg);

    pthread_mutex_lock(&ctx->lock);

    log_rotate(ctx, ctx->display_log, ctx->display_path);
    log_rotate(ctx, ctx->combined_log, ctx->combined_path);

    if (ctx->display_log) {
        fputs(line_buf, ctx->display_log);
        fflush(ctx->display_log);
    }
    if (ctx->combined_log) {
        fputs(line_buf, ctx->combined_log);
        fflush(ctx->combined_log);
    }

    if (ctx->use_color) {
        fprintf(stderr, "%s%s%s", li->color, line_buf, li->reset);
    } else {
        fputs(line_buf, stderr);
    }
    fflush(stderr);

    pthread_mutex_unlock(&ctx->lock);
}

void log_set_level(log_ctx_t *ctx, log_level_t level) {
    pthread_mutex_lock(&ctx->lock);
    ctx->min_level = level;
    pthread_mutex_unlock(&ctx->lock);
}

void log_sigusr1_handler(int sig) {
    (void)sig;
    if (g_log_ctx && !g_log_ctx->trace_burst) {
        g_log_ctx->trace_burst = 1;
        g_log_ctx->orig_min_level = g_log_ctx->min_level;
        g_log_ctx->min_level = LOG_LEVEL_TRACE;
        g_log_ctx->trace_burst_end = time(NULL) + 60;
    }
}

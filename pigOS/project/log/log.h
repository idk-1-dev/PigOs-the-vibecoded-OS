#ifndef MYWM_LOG_H
#define MYWM_LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <signal.h>

#define LOG_MAX_LINE 1024
#define LOG_MAX_PATH 512
#define LOG_ROTATE_SIZE (10ULL * 1024 * 1024)
#define LOG_ROTATE_COUNT 5

typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO  = 2,
    LOG_LEVEL_WARN  = 3,
    LOG_LEVEL_ERROR = 4,
    LOG_LEVEL_FATAL = 5
} log_level_t;

typedef struct {
    const char *name;
    const char *color;
    const char *reset;
} log_level_info_t;

typedef struct {
    log_level_t min_level;
    log_level_t orig_min_level;
    const char *component;
    FILE *display_log;
    FILE *combined_log;
    char display_path[LOG_MAX_PATH];
    char combined_path[LOG_MAX_PATH];
    pthread_mutex_t lock;
    int use_color;
    volatile sig_atomic_t trace_burst;
    time_t trace_burst_end;
} log_ctx_t;

void log_init(log_ctx_t *ctx, const char *component, log_level_t min_level);
void log_shutdown(log_ctx_t *ctx);
void log_rotate(log_ctx_t *ctx, FILE *fp, const char *path);
void log_write(log_ctx_t *ctx, log_level_t level,
               const char *file, int line, const char *fmt, ...);
void log_set_level(log_ctx_t *ctx, log_level_t level);
void log_sigusr1_handler(int sig);

extern log_ctx_t *g_log_ctx;

#define LOG_TRACE(fmt, ...) \
    log_write(g_log_ctx, LOG_LEVEL_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) \
    log_write(g_log_ctx, LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
    log_write(g_log_ctx, LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) \
    log_write(g_log_ctx, LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) \
    log_write(g_log_ctx, LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) \
    log_write(g_log_ctx, LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL_ABORT(fmt, ...) do { \
    log_write(g_log_ctx, LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
    abort(); \
} while(0)

#endif

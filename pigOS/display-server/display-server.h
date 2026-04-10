#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DISPLAY_SERVER_NAME "pigDisplay"
#define DISPLAY_SERVER_VERSION "1.0"

typedef struct {
    int drm_fd;
    uint32_t width;
    uint32_t height;
    uint32_t *front_buffer;
    uint32_t *back_buffer;
    bool initialized;
} DisplayContext;

typedef struct {
    int x;
    int y;
    int dx;
    int dy;
    uint32_t buttons;
    bool moved;
} MouseState;

typedef struct {
    uint32_t key;
    uint32_t unicode;
    bool pressed;
    bool released;
} KeyboardState;

int display_server_init(DisplayContext *ctx);
void display_server_finish(DisplayContext *ctx);
int display_server_poll(DisplayContext *ctx, MouseState *mouse, KeyboardState *kbd);
void display_server_flip(DisplayContext *ctx);

int drm_init(DisplayContext *ctx);
void drm_finish(DisplayContext *ctx);
int drm_create_fb(DisplayContext *ctx);
void drm_flip(DisplayContext *ctx);

int input_init(void);
void input_finish(void);
int input_poll(MouseState *mouse, KeyboardState *kbd);

int udev_init(void);
void udev_finish(void);
const char* udev_find_gpu(void);

int xkb_init(void);
void xkb_finish(void);
uint32_t xkb_translate_key(uint32_t scancode, bool shift);

extern DisplayContext *g_display;

#include "../libclient/client.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define LAUNCHER_WIDTH 320
#define LAUNCHER_HEIGHT 400
#define ITEM_HEIGHT 50
#define ITEM_PADDING 10
#define MAX_ITEMS 16

typedef struct {
    const char *name;
    const char *icon_color;
} app_item_t;

static const app_item_t apps[] = {
    { "Terminal",     "#00FF00" },
    { "File Manager", "#00AAFF" },
    { "Web Browser",  "#FF6600" },
    { "Text Editor",  "#FFAA00" },
    { "Image Viewer", "#FF00FF" },
    { "Music Player", "#00FFFF" },
    { "Settings",     "#AAAAAA" },
    { "Calculator",   "#888888" },
};

static const int num_apps = sizeof(apps) / sizeof(apps[0]);

static int selected_item = -1;
static int hovered_item = -1;

static uint32_t parse_color(const char *hex) {
    unsigned int r = 0, g = 0, b = 0;
    if (hex[0] == '#') hex++;
    sscanf(hex, "%02x%02x%02x", &r, &g, &b);
    return (r << 16) | (g << 8) | b;
}

static void draw_text_simple(uint32_t *buf, int w, int h,
                              const char *text, int x, int y,
                              uint32_t color, int scale) {
    int len = strlen(text);
    int char_w = 6 * scale;
    int char_h = 8 * scale;
    (void)char_h;

    for (int i = 0; i < len; i++) {
        unsigned char ch = text[i];
        int cx = x + i * char_w;

        if (ch >= 'A' && ch <= 'Z') {
            int idx = ch - 'A';
            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 5; col++) {
                    int on = 0;
                    if (idx == 0) on = (col > 0 && col < 4 && row > 0) || (row == 0 && col > 0 && col < 4) || (row > 0 && row < 3 && (col == 0 || col == 4));
                    else if (idx == 1) on = (col < 4 && (row == 0 || row == 2 || row == 4)) || (col == 0 || col == 4) && (row == 1 || row == 3);
                    else if (idx == 2) on = (row == 0 || row == 4) && col < 4 || (row > 0 && row < 4 && col == 0);
                    else if (idx == 3) on = (col < 4 && (row == 0 || row == 4)) || (col == 0 || col == 4) && (row > 0 && row < 4);
                    else if (idx == 4) on = (row == 0 || row == 2 || row == 4) && col < 4 || (col == 0 && row < 3);
                    else if (idx == 5) on = (row == 0 || row == 2 || row == 4) && col < 4 || (col == 0 && row < 3) || (col == 4 && row > 2);
                    else if (idx == 6) on = (row == 0 || row == 2 || row == 4) && col < 4 || col == 0 && row < 3 || (col == 0 || col == 4) && row > 2;
                    else if (idx == 7) on = (row == 0) && col < 4 || col == 2 && row < 4;
                    else if (idx == 8) on = (col == 0 || col == 4) && row < 4 || (row == 0 || row == 4) && col < 4;
                    else if (idx == 9) on = (row == 0 || row == 4) && col < 4 || (col == 4 && row < 4);
                    else if (idx == 10) on = (col == 0 || col == 4) && row < 3 || (row == 1 || row == 3) && col > 0 && col < 4;
                    else if (idx == 11) on = col == 0 && row < 4;
                    else if (idx == 12) on = (col == 0 || col == 4) && row < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 13) on = (col == 0 || col == 4) && row < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 14) on = (row == 0 || row == 4) && col < 4 || (col == 0 || col == 4) && row > 0 && row < 4;
                    else if (idx == 15) on = (row == 0 || row == 2) && col < 4 || col == 0 && row < 3 || col == 4 && row < 2;
                    else if (idx == 16) on = (row == 0 || row == 4) && col < 4 || (col == 0 || col == 4) && row > 0 && row < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 17) on = (row == 0 || row == 2) && col < 4 || (col == 0 && row < 3) || (col == 4 && row < 2);
                    else if (idx == 18) on = (row == 0 || row == 4) && col < 4 || (col == 0 && row < 3) || (col == 4 && row > 1);
                    else if (idx == 19) on = (row == 0) && col < 4 || col == 2 && row < 4;
                    else if (idx == 20) on = (col == 0 || col == 4) && row < 4 || row == 4 && col > 0 && col < 4;
                    else if (idx == 21) on = (col == 0 || col == 4) && row < 3 || row == 2 && col > 0 && col < 4;
                    else if (idx == 22) on = (col == 0 || col == 4) && row < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 23) on = (col == 0 || col == 4) && row < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 24) on = (row == 0 || row == 4) && col < 4 || col == 2 && row > 0 && row < 4;
                    else if (idx == 25) on = (row == 0) && col < 4 || col == 2 && row < 4;

                    if (on) {
                        for (int sy = 0; sy < scale; sy++) {
                            for (int sx = 0; sx < scale; sx++) {
                                int px = cx + col * scale + sx;
                                int py = y + row * scale + sy;
                                if (px >= 0 && px < w && py >= 0 && py < h)
                                    buf[py * w + px] = color;
                            }
                        }
                    }
                }
            }
        } else if (ch >= 'a' && ch <= 'z') {
            int idx = ch - 'a';
            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 5; col++) {
                    int on = 0;
                    if (idx == 0) on = (row == 0 || row == 2 || row == 4) && col < 4 || (col == 0 || col == 4) && row > 0 && row < 4;
                    else if (idx == 1) on = col == 0 && row < 4 || row == 0 && col < 4 || (col == 4 && row > 0 && row < 3) || row == 2 && col > 0 && col < 4;
                    else if (idx == 2) on = (row == 0 || row == 4) && col < 4 || col == 0 && row > 0 && row < 4;
                    else if (idx == 3) on = col == 0 && row < 4 || row == 0 && col < 4 || (col == 4 && row > 0 && row < 3) || row == 2 && col > 0 && col < 4;
                    else if (idx == 4) on = (row == 0 || row == 2 || row == 4) && col < 4 || col == 0 && row > 0 && row < 4;
                    else if (idx == 5) on = col == 0 && row < 4 || row == 0 && col < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 6) on = (row == 0 || row == 4) && col < 4 || col == 0 && row > 0 && row < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 7) on = col == 0 && row < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 8) on = col == 4 && row < 4;
                    else if (idx == 9) on = col == 4 && row < 4 || row == 0 && col < 4;
                    else if (idx == 10) on = (col == 0 || col == 4) && row < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 11) on = col == 0 && row < 4;
                    else if (idx == 12) on = (col == 0 || col == 4) && row < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 13) on = (col == 0 || col == 4) && row < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 14) on = (row == 0 || row == 4) && col < 4 || (col == 0 || col == 4) && row > 0 && row < 4;
                    else if (idx == 15) on = col == 0 && row < 4 || row == 0 && col < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 16) on = (row == 0 || row == 4) && col < 4 || (col == 0 || col == 4) && row > 0 && row < 4;
                    else if (idx == 17) on = col == 0 && row < 4 || row == 0 && col < 4 || (col == 4 && row > 0 && row < 3);
                    else if (idx == 18) on = (row == 0 || row == 4) && col < 4 || col == 0 && row > 0 && row < 4 || row == 2 && col > 0 && col < 4;
                    else if (idx == 19) on = row == 0 && col < 4 || col == 2 && row > 0 && row < 4;
                    else if (idx == 20) on = (col == 0 || col == 4) && row < 4 || row == 4 && col > 0 && col < 4;
                    else if (idx == 21) on = (col == 0 || col == 4) && row < 3 || row == 2 && col > 0 && col < 4;
                    else if (idx == 22) on = (col == 0 || col == 4) && row < 3 || row == 2 && col > 0 && col < 4 || row == 4 && col > 0 && col < 4;
                    else if (idx == 23) on = (col == 0 || col == 4) && row < 3 || row == 2 && col > 0 && col < 4;
                    else if (idx == 24) on = (row == 0 || row == 4) && col < 4 || col == 2 && row > 0 && row < 4;
                    else if (idx == 25) on = row == 0 && col < 4 || col == 2 && row > 0 && row < 4;

                    if (on) {
                        for (int sy = 0; sy < scale; sy++) {
                            for (int sx = 0; sx < scale; sx++) {
                                int px = cx + col * scale + sx;
                                int py = y + row * scale + sy;
                                if (px >= 0 && px < w && py >= 0 && py < h)
                                    buf[py * w + px] = color;
                            }
                        }
                    }
                }
            }
        } else if (ch == ' ') {
        }
    }
}

static void render_launcher(uint32_t *buf, int w, int h) {
    memset(buf, 0x12, (size_t)w * h * 4);

    draw_text_simple(buf, w, h, "LAUNCHER", 100, 15, 0xFFCCCCCC, 2);

    for (int i = 0; i < num_apps; i++) {
        int y = 60 + i * ITEM_HEIGHT;
        uint32_t bg = (i == hovered_item) ? 0x333344 : 0x222233;
        uint32_t border = (i == selected_item) ? 0x00FF88 : 0x444455;

        for (int py = y; py < y + ITEM_HEIGHT && py < h; py++) {
            for (int px = 10; px < w - 10; px++) {
                if (px == 10 || px == w - 11 || py == y || py == y + ITEM_HEIGHT - 1)
                    buf[py * w + px] = border;
                else
                    buf[py * w + px] = bg;
            }
        }

        uint32_t icon_color = parse_color(apps[i].icon_color);
        for (int cy = y + 10; cy < y + ITEM_HEIGHT - 10; cy++) {
            for (int cx = 20; cx < 50; cx++) {
                if (cy < h && cx < w)
                    buf[cy * w + cx] = icon_color;
            }
        }

        draw_text_simple(buf, w, h, apps[i].name, 60, y + 18, 0xFFDDDDDD, 1);
    }
}

int main(void) {
    display_t *disp;
    window_t *win;
    uint32_t *buf;
    int running = 1;

    disp = NULL;
    for (int i = 0; i < 100; i++) {
        disp = display_connect();
        if (disp) break;
        struct timespec ts = { 0, 50000000 };
        nanosleep(&ts, NULL);
    }
    if (!disp) {
        fprintf(stderr, "Failed to connect to display server\n");
        return 1;
    }

    win = NULL;
    for (int i = 0; i < 100; i++) {
        win = window_create(disp, LAUNCHER_WIDTH, LAUNCHER_HEIGHT, "Launcher");
        if (win) break;
        struct timespec ts = { 0, 50000000 };
        nanosleep(&ts, NULL);
    }
    if (!win) {
        fprintf(stderr, "Failed to create window\n");
        display_disconnect(disp);
        return 1;
    }

    buf = window_get_buffer(win);

    window_move(win, 1200, 50);

    while (running) {
        event_t evt;
        while (event_poll(win, &evt) > 0) {
            if (evt.type == EVENT_CLOSE) {
                running = 0;
            } else if (evt.type == EVENT_MOUSE_MOVE) {
                int my = evt.mouse_move.y;
                hovered_item = my / ITEM_HEIGHT;
                if (hovered_item < 0) hovered_item = -1;
                if (hovered_item >= num_apps) hovered_item = num_apps - 1;
            } else if (evt.type == EVENT_MOUSE_BUTTON && evt.mouse_button.pressed) {
                int my = evt.mouse_move.y;
                int item = my / ITEM_HEIGHT;
                if (item >= 0 && item < num_apps) {
                    selected_item = item;
                }
            } else if (evt.type == EVENT_KEY && evt.key.pressed) {
                if (evt.key.keycode == 1) {
                    running = 0;
                } else if (evt.key.keycode == 103) {
                    if (hovered_item > 0) hovered_item--;
                } else if (evt.key.keycode == 108) {
                    if (hovered_item < num_apps - 1) hovered_item++;
                } else if (evt.key.keycode == 28) {
                    if (hovered_item >= 0) selected_item = hovered_item;
                }
            }
        }

        render_launcher(buf, LAUNCHER_WIDTH, LAUNCHER_HEIGHT);
        window_commit(win);

        struct timespec ts = { 0, 16000000 };
        nanosleep(&ts, NULL);
    }

    window_destroy(win);
    display_disconnect(disp);
    return 0;
}

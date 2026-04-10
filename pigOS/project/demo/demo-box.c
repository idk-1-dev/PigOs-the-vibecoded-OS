#include "../libclient/client.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define WIN_WIDTH 400
#define WIN_HEIGHT 300
#define BOX_SIZE 40

int main(void) {
    display_t *disp;
    window_t *win;
    uint32_t *buf;
    int bx = 50, by = 50;
    int dx = 3, dy = 3;
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
        win = window_create(disp, WIN_WIDTH, WIN_HEIGHT, "Bouncing Box");
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
    window_move(win, 700, 100);

    while (running) {
        event_t evt;
        while (event_poll(win, &evt) > 0) {
            if (evt.type == EVENT_CLOSE) {
                running = 0;
                break;
            }
        }
        if (!running) break;

        memset(buf, 0x1a1a2e, (size_t)WIN_WIDTH * WIN_HEIGHT * 4);

        bx += dx;
        by += dy;

        if (bx <= 0 || bx + BOX_SIZE >= WIN_WIDTH) dx = -dx;
        if (by <= 0 || by + BOX_SIZE >= WIN_HEIGHT) dy = -dy;

        for (int y = by; y < by + BOX_SIZE && y < WIN_HEIGHT; y++) {
            for (int x = bx; x < bx + BOX_SIZE && x < WIN_WIDTH; x++) {
                if (x >= 0 && y >= 0) {
                    buf[y * WIN_WIDTH + x] = 0xFF00FFAA;
                }
            }
        }

        for (int x = bx; x < bx + BOX_SIZE && x < WIN_WIDTH; x++) {
            if (x >= 0 && by >= 0 && by < WIN_HEIGHT)
                buf[by * WIN_WIDTH + x] = 0xFFFFFFFF;
            if (x >= 0 && by + BOX_SIZE - 1 >= 0 && by + BOX_SIZE - 1 < WIN_HEIGHT)
                buf[(by + BOX_SIZE - 1) * WIN_WIDTH + x] = 0xFFFFFFFF;
        }
        for (int y = by; y < by + BOX_SIZE && y < WIN_HEIGHT; y++) {
            if (y >= 0 && bx >= 0 && bx < WIN_WIDTH)
                buf[y * WIN_WIDTH + bx] = 0xFFFFFFFF;
            if (y >= 0 && bx + BOX_SIZE - 1 >= 0 && bx + BOX_SIZE - 1 < WIN_WIDTH)
                buf[y * WIN_WIDTH + (bx + BOX_SIZE - 1)] = 0xFFFFFFFF;
        }

        window_commit(win);

        struct timespec ts = { 0, 14000000 };
        nanosleep(&ts, NULL);
    }

    window_destroy(win);
    display_disconnect(disp);
    return 0;
}

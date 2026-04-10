#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include "display-server.h"

DisplayContext *g_display = NULL;
static volatile bool running = true;

static void signal_handler(int sig) {
    (void)sig;
    running = false;
}

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h, --help     Show this help message\n");
    fprintf(stderr, "  -v, --version  Show version\n");
    fprintf(stderr, "  -w WIDTH       Set width (default: 1024)\n");
    fprintf(stderr, "  -H HEIGHT      Set height (default: 768)\n");
    fprintf(stderr, "  --no-drm       Disable DRM/KMS (framebuffer only)\n");
    fprintf(stderr, "  --no-input     Disable input devices\n");
    fprintf(stderr, "\nEnvironment:\n");
    fprintf(stderr, "  DISPLAY_WIDTH   Override width\n");
    fprintf(stderr, "  DISPLAY_HEIGHT  Override height\n");
}

static void print_version(void) {
    printf("%s %s\n", DISPLAY_SERVER_NAME, DISPLAY_SERVER_VERSION);
    printf("Built with: DRM, libinput, libudev, libxkbcommon, stb_truetype\n");
}

int main(int argc, char *argv[]) {
    DisplayContext ctx = {0};
    uint32_t width = 1024;
    uint32_t height = 768;
    bool use_drm = true;
    bool use_input = true;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-H") == 0 && i + 1 < argc) {
            height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--no-drm") == 0) {
            use_drm = false;
        } else if (strcmp(argv[i], "--no-input") == 0) {
            use_input = false;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    char *env_w = getenv("DISPLAY_WIDTH");
    char *env_h = getenv("DISPLAY_HEIGHT");
    if (env_w) width = atoi(env_w);
    if (env_h) height = atoi(env_h);

    printf("Initializing %s %s\n", DISPLAY_SERVER_NAME, DISPLAY_SERVER_VERSION);
    printf("Resolution: %ux%u\n", width, height);

    if (use_drm) {
        if (udev_init() < 0) {
            fprintf(stderr, "Warning: udev init failed, falling back\n");
        }
        if (drm_init(&ctx) < 0) {
            fprintf(stderr, "Error: DRM initialization failed\n");
            return 1;
        }
    }

    if (use_input) {
        if (xkb_init() < 0) {
            fprintf(stderr, "Warning: xkbcommon init failed\n");
        }
        if (input_init() < 0) {
            fprintf(stderr, "Warning: input init failed\n");
        }
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("%s initialized successfully\n", DISPLAY_SERVER_NAME);
    printf("Running... (Ctrl+C to exit)\n");

    MouseState mouse = {0};
    KeyboardState kbd = {0};

    while (running) {
        if (use_input) {
            input_poll(&mouse, &kbd);
        }

        if (use_drm) {
            display_server_flip(&ctx);
        }

        usleep(16666);
    }

    printf("\nShutting down...\n");

    if (use_input) {
        input_finish();
        xkb_finish();
    }

    if (use_drm) {
        drm_finish(&ctx);
        udev_finish();
    }

    printf("Done.\n");
    return 0;
}

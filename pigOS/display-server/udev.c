#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libudev.h>
#include "display-server.h"

static struct udev *udev_ctx = NULL;
static struct udev_enumerate *udev_enum = NULL;

int udev_init(void) {
    udev_ctx = udev_new();
    if (!udev_ctx) {
        fprintf(stderr, "Failed to create udev context\n");
        return -1;
    }

    printf("udev initialized\n");
    return 0;
}

void udev_finish(void) {
    if (udev_enum) {
        udev_enumerate_unref(udev_enum);
        udev_enum = NULL;
    }
    if (udev_ctx) {
        udev_unref(udev_ctx);
        udev_ctx = NULL;
    }
}

const char* udev_find_gpu(void) {
    static char dev_path[256] = {0};
    
    if (!udev_ctx) {
        udev_ctx = udev_new();
        if (!udev_ctx) return NULL;
    }

    udev_enum = udev_enumerate_new(udev_ctx);
    if (!udev_enum) return NULL;

    udev_enumerate_add_match_subsystem(udev_enum, "drm");
    udev_enumerate_add_match_sysname(udev_enum, "card0*");
    udev_enumerate_scan_devices(udev_enum);

    struct udev_list_entry *entries = udev_enumerate_get_list_entry(udev_enum);
    struct udev_list_entry *entry;

    udev_list_entry_foreach(entry, entries) {
        const char *path = udev_list_entry_get_name(entry);
        if (path) {
            struct udev_device *dev = udev_device_new_from_syspath(udev_ctx, path);
            if (dev) {
                const char *devnode = udev_device_get_devnode(dev);
                if (devnode) {
                    strncpy(dev_path, devnode, sizeof(dev_path) - 1);
                    udev_device_unref(dev);
                    return dev_path;
                }
                udev_device_unref(dev);
            }
        }
    }

    const char *fallbacks[] = {
        "/dev/dri/card0",
        "/dev/dri/renderD128",
        NULL
    };
    for (int i = 0; fallbacks[i]; i++) {
        if (access(fallbacks[i], R_OK | W_OK) == 0) {
            strncpy(dev_path, fallbacks[i], sizeof(dev_path) - 1);
            return dev_path;
        }
    }

    return NULL;
}

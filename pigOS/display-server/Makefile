# pigOS Display Server Makefile
# Requires: gcc, make, pkg-config, libdrm-dev, libinput-dev, libudev-dev, libxkbcommon-dev

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2 -g
LDFLAGS ?=

# pkg-config dependencies
DRM_LIBS      := $(shell pkg-config --libs libdrm)
INPUT_LIBS    := $(shell pkg-config --libs libinput)
UDEV_LIBS     := $(shell pkg-config --libs libudev)
XKB_LIBS      := $(shell pkg-config --libs xkbcommon)

DRM_CFLAGS    := $(shell pkg-config --cflags libdrm)
INPUT_CFLAGS  := $(shell pkg-config --cflags libinput)
UDEV_CFLAGS   := $(shell pkg-config --cflags libudev)
XKB_CFLAGS    := $(shell pkg-config --cflags xkbcommon)

CFLAGS  += $(DRM_CFLAGS) $(INPUT_CFLAGS) $(UDEV_CFLAGS) $(XKB_CFLAGS) -I.
LDFLAGS += $(DRM_LIBS) $(INPUT_LIBS) $(UDEV_LIBS) $(XKB_LIBS) -lm -lpthread

SRCS    = main.c drm.c input.c udev.c xkb.c wrapper.c
OBJS    = $(SRCS:.c=.o)
TARGET  = pig-display-server

.PHONY: all clean help info

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c display-server.h stb_truetype.h
	$(CC) $(CFLAGS) -c -o $@ $<

info:
	@echo "Display Server Build Info"
	@echo "=========================="
	@echo "CC:        $(CC)"
	@echo "CFLAGS:    $(CFLAGS)"
	@echo "LDFLAGS:   $(LDFLAGS)"
	@echo "DRM:       $(DRM_LIBS)"
	@echo "INPUT:     $(INPUT_LIBS)"
	@echo "UDEV:      $(UDEV_LIBS)"
	@echo "XKB:       $(XKB_LIBS)"

help:
	@echo "pigOS Display Server Build"
	@echo "=========================="
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build display server (default)"
	@echo "  clean    - Remove built files"
	@echo "  info     - Show build configuration"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Requirements:"
	@echo "  Build:   gcc, make, pkg-config"
	@echo "  Dev:     libdrm-dev, libinput-dev, libudev-dev, libxkbcommon-dev"
	@echo "  Runtime: libdrm, libinput, libudev, libxkbcommon0"
	@echo ""
	@echo "Usage:"
	@echo "  make              # Build"
	@echo "  make info         # Show dependencies"
	@echo "  ./pig-display-server --help"

clean:
	rm -f $(OBJS) $(TARGET)

install:
	install -Dm755 $(TARGET) $(DESTDIR)/usr/bin/$(TARGET)

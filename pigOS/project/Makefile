CC = gcc
CFLAGS_COMMON = -Wall -Wextra -g -std=c11 -D_GNU_SOURCE
CFLAGS_DEBUG = $(CFLAGS_COMMON) -O0 -DDEBUG
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O2 -DNDEBUG

PKG_DRM = $(shell pkg-config --cflags --libs libdrm)
PKG_INPUT = $(shell pkg-config --cflags --libs libinput libudev)
PKG_XKB = $(shell pkg-config --cflags --libs xkbcommon)

LDFLAGS = -lpthread -lrt -lm

SRC_DIR = display-server
LOG_DIR = log
CLIENT_DIR = libclient
DEMO_DIR = demo

BUILD_DIR = build/debug
BUILD_DIR_RELEASE = build/release

MODE ?= debug

ifeq ($(MODE),release)
    BUILD = $(BUILD_DIR_RELEASE)
    CFLAGS = $(CFLAGS_RELEASE)
else
    BUILD = $(BUILD_DIR)
    CFLAGS = $(CFLAGS_DEBUG)
endif

LOG_SRC = $(LOG_DIR)/log.c
LOG_OBJ = $(BUILD)/log/log.o

DS_SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/drm.c $(SRC_DIR)/socket.c \
          $(SRC_DIR)/shm.c $(SRC_DIR)/input.c $(SRC_DIR)/window.c \
          $(SRC_DIR)/compositor.c $(SRC_DIR)/tty.c
DS_OBJS = $(patsubst %.c,$(BUILD)/%.o,$(DS_SRCS)) $(LOG_OBJ)
DS_BIN = $(BUILD)/vdm

CLIENT_SRC = $(CLIENT_DIR)/client.c
CLIENT_OBJ = $(BUILD)/libclient/client.o
CLIENT_LIB = $(BUILD)/libclient.a
CLIENT_HDR = $(CLIENT_DIR)/client.h

DEMO_BOX_SRC = $(DEMO_DIR)/demo-box.c
DEMO_TERM_SRC = $(DEMO_DIR)/demo-terminal.c
DEMO_LAUNCHER_SRC = $(DEMO_DIR)/demo-launcher.c

DEMO_BOX_BIN = $(BUILD)/demo-box
DEMO_TERM_BIN = $(BUILD)/demo-terminal
DEMO_LAUNCHER_BIN = $(BUILD)/demo-launcher

.PHONY: all clean run debug release all_bins

all: $(MODE)

debug:
	@$(MAKE) MODE=debug all_bins

release:
	@$(MAKE) MODE=release all_bins

all_bins: $(DS_BIN) $(CLIENT_LIB) $(DEMO_BOX_BIN) $(DEMO_TERM_BIN) $(DEMO_LAUNCHER_BIN)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. $(PKG_DRM) $(PKG_INPUT) $(PKG_XKB) -c $< -o $@

$(LOG_OBJ): $(LOG_SRC)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(DS_BIN): $(DS_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@ $(PKG_DRM) $(PKG_INPUT) $(PKG_XKB) $(LDFLAGS)

$(CLIENT_OBJ): $(CLIENT_SRC)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(CLIENT_LIB): $(CLIENT_OBJ)
	@mkdir -p $(dir $@)
	ar rcs $@ $^

$(DEMO_BOX_BIN): $(DEMO_BOX_SRC) $(CLIENT_LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. $< $(CLIENT_LIB) -o $@ $(LDFLAGS)

$(DEMO_TERM_BIN): $(DEMO_TERM_SRC) $(CLIENT_LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -I. $< $(CLIENT_LIB) -o $@ $(LDFLAGS) -lm -lutil

$(DEMO_LAUNCHER_BIN): $(DEMO_LAUNCHER_SRC) $(CLIENT_LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Wno-parentheses -I. $< $(CLIENT_LIB) -o $@ $(LDFLAGS)

run: all
	sudo $(DS_BIN)

clean:
	rm -rf build

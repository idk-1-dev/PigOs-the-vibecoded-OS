# pigOS Display Server

## Overview
pigDisplay - A Wayland-style display server for pigOS with DRM/KMS, libinput, libudev, and libxkbcommon support.

## Features
- DRM/KMS display driver
- libinput mouse/keyboard support
- libudev GPU detection
- libxkbcommon keyboard layout
- Bundled stb_truetype.h for font rendering

## Dependencies

### Build Tools
- gcc
- make
- pkg-config

### Development Libraries (dev packages)
- libdrm-dev
- libinput-dev
- libudev-dev
- libxkbcommon-dev

### Runtime Libraries
- libdrm
- libinput
- libudev
- libxkbcommon0

### Bundled (no install needed)
- stb_truetype.h (included)

## Build

```bash
cd display-server
make
```

Or with custom compiler:
```bash
make CC=clang
```

## Usage

```bash
./pig-display-server --help
./pig-display-server -w 1920 -H 1080
```

### Options
- `-h, --help`     Show help
- `-v, --version`  Show version
- `-w WIDTH`       Set width (default: 1024)
- `-H HEIGHT`      Set height (default: 768)
- `--no-drm`       Disable DRM/KMS
- `--no-input`     Disable input devices

### Environment Variables
- `DISPLAY_WIDTH`  Override width
- `DISPLAY_HEIGHT` Override height

## Architecture

```
display-server/
├── main.c              # Entry point
├── display-server.h    # Public API header
├── drm.c               # DRM/KMS driver
├── input.c             # libinput integration
├── udev.c              # libudev GPU detection
├── xkb.c               # libxkbcommon keyboard
├── wrapper.c            # High-level API
├── stb_truetype.h      # Font rendering (bundled)
└── Makefile            # Build system
```

## Integration with pigOS

The display server provides graphics and input capabilities that can be integrated with the pigOS window manager (pigWM) for a modern desktop experience.

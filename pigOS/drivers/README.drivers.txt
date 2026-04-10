pigOS drivers/ - Hardware Drivers
=================================

vga.h: VGA 80x25 Text Mode Driver
---------------------------------
- 80x25 text mode with CP437 character set
- Full color support (16 foreground + 16 background)
- Box drawing characters (╔═╗╚╝║╠╣┌┐└┘▄▀█░▒▓)
- True color support added (24-bit RGB via ANSI escape codes)
- Functions:
  - vpc() - Print character (handles \n \r \b \t)
  - vps() - Print string
  - vpln() - Print line
  - vat() - Print at position
  - vfill() - Fill rectangle
  - vstr() - Print string at position
  - vclear() - Clear screen
  - vset() - Set color
  - vset_truecolor() - Set 24-bit true color (terminal mode)
- Hardware cursor management
- Auto-scrolling at bottom
- Scrollback buffer (100 lines)

keyboard.h: PS/2 Keyboard Driver
---------------------------------
- Direct PS/2 keyboard support
- Full scancode translation
- Special keys:
  - Arrows (UP, DOWN, LEFT, RIGHT)
  - Function keys (F1-F12)
  - Home, End, Page Up, Page Down
  - Delete, Insert
  - CTRL, SHIFT, ALT
- Returns special key codes via KEY_* constants

ps2.h: PS/2 Controller Driver
-------------------------------
- Combined PS/2 controller with keyboard support
- Mouse support disabled (waiting for IRQ)
- Keyboard + mouse combined handling

network.h: Network Drivers
-------------------------
- RTL8139 PCI detection and initialization
- VirtIO-net support (stub)
- E1000 support (stub)
- Network stack: lwIP (lightweight IP)
- Commands:
  - ifconfig - Show network config
  - ping - ICMP ping
  - netstat - Network statistics
  - pigget <url> - Download file
  - pigcurl <url> - Fetch URL
- DHCP client support
- TCP/UDP socket API (via lwIP)

Display Drivers
---------------
vdm/ - Vibecoded Display Manager
  - Real GPU support for desktop
  - libdrm, libinput, libxkbcommon
  - Requires actual hardware (not QEMU)

display-server/ - Simple compositor
  - Stub for future GUI support
  - DRM/KMS integration
  - libinput for mouse/keyboard
  - Unix domain socket IPC
  - Shared memory pixel buffers

driver subdirectories:
- vga/ - VGA text mode
- kbd/ - Keyboard
- ps2/ - PS/2 controller
- net/ - Network (RTL8139, lwIP)
- mouse/ - Mouse driver (disabled)
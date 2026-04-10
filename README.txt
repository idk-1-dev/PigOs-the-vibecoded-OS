    ____  _       ___  ____
   |  _ \( ) ___ / _ \/ ___|
   | |_) / | |/ _ \ | | \___ \
   |  __/  | | (_) | |_| |___) |
   |_|     |_|\___/ \___/|____/
pigOS v1.0 - x86_64 Operating System
=========================================================

## REQUIREMENTS

### Ubuntu/Debian:
```
sudo apt install nasm gcc binutils grub-pc-bin grub-common xorriso mtools qemu-system-x86
```

### Fedora/RHEL:
```
sudo dnf install nasm gcc binutils grub2-tools xorriso mtools qemu-system-x86
```

### Arch Linux:
```
sudo pacman -S nasm gcc binutils grub xorriso mtools qemu
```

*(before u run any of those check ur system to see if you have them)*

## HOW TO BUILD
```
cd pigOS
make clean && make
```

## HOW TO RUN
```
make run
```

Or manually:
```
qemu-system-x86_64 -cdrom pigOS.iso -m 128M -vga std -net nic,model=rtl8139 -net user
```

Exit QEMU: Ctrl+Alt+Q

## BOOT ON REAL HARDWARE

### Ventoy USB:
```
1. Copy pigOS.iso to Ventoy USB
2. Boot from USB
3. Select pigOS from menu
```

### BIOS Boot:
```
1. Burn ISO to USB (Rufus, Etcher, etc.)
2. Boot from USB
3. Select pigOS
```

## LOGIN

Default credentials (required for wm/VDM):
```
Username: root
Password: toor
```

## FIRST COMMANDS
```
help          - list all commands
fastfetch     - system info display
wm            - launch VDM login (requires login)
wm            - launch PigWM desktop after login
ls / ls -la   - list files
cat file      - view file contents
vi file       - modal text editor (i=insert, ESC=normal, :wq=save)
nano file     - simple text editor
adduser name  - create new user
passwd        - change password
snake         - play snake game
tetris        - play tetris
matrix        - matrix effect
fortune       - random fortune
```

## SHELL CONTROLS
```
BACKSPACE     - delete character
LEFT/RIGHT    - move cursor
UP/DOWN       - command history
TAB           - autocomplete
CTRL+C        - cancel
```

## WINDOW MANAGER (wm)
After login, type 'wm' to access desktop:
```
F2 = Terminal window
F3 = File Manager (Robin)
F5 = Power Menu
ESC = Exit to shell
TAB = Cycle windows
```

## VDM - Vibecoded Display Manager
```
Ported to pigOS from erofs
Location: /vdm/
Run: bash /vdm/run-dm.sh
Note: Requires real GPU hardware, won't work in QEMU
```

## ARCHITECTURE
```
- GRUB bootloader
- pigKernel (x86_64 Long Mode)
- larpshell v5.9.3 (based on lash)
- VDM - Vibecoded Display Manager (ported from erofs)
- PigWM (text-mode window manager)
- pigFS (in-memory filesystem)
- poing init system
- User management system
- lwIP TCP/IP stack
```

## FEATURES
```
- Login system with password authentication
- User accounts (adduser, su, passwd)
- Full shell with history & autocomplete
- Text editors (vi, nano)
- Window manager (PigWM)
- VDM display manager (real hardware)
- Games (snake, tetris, matrix)
- Package manager (rpk)
- Network stack (RTL8139, virtio-net)
- Crypto tools (pigcrypt, pighash, pigkey)
```

## CREDITS
```
- larpshell: github.com/usr-undeleted/lash
- VDM: from erofs project
- lwIP: TCP/IP stack
- stb: Sean Barrett's single-file libraries
```

## VERSION
```
pigOS v1.0 - built with alot of AIs
```

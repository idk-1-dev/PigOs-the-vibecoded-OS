pigOS kernel/ - pigKernel Core
==============================

kernel.c: Main kernel entry point
---------------------------------
- Shows FLARE/POUG bootloader splash screen
- Verbose poing init boot log
- Initializes: users, poing, network, WM, shell
- Handles all system calls
- Manages VFS and memory

mem.h: Memory allocator
-----------------------
- Bump allocator (4MB) at physical address 0x400000
- km() - allocate memory
- kf() - free (no-op, simplified)
- kms() - set memory
- kmc() - copy memory
- ksl() - string length
- ksc() - string compare
- ksnc() - string n-compare
- kcp() - string copy
- kcat() - string concatenate
- kia() - integer to ascii
- kuia() - unsigned integer to ascii

users.h: User/group system
--------------------------
- adduser <name> - Create new user
- deluser <name> - Delete user
- usermod -G <group> <user> - Add user to group
- groupadd <name> - Create group
- passwd [user] - Change password (UPDATES user database)
- rok - Run as root (elevated execution)
- r- - Switch to root user

IMPORTANT: Password changes via 'passwd' are now stored in the user
database and validated at login. Previously was hardcoded.

pigfs.h: Virtual Filesystem
--------------------------
- VNode-based read-only file system
- MemFile in-memory files (touch, pnano saves)
- Path resolution with normalization
- ls, cd, cat, grep, find commands
- rm with critical path protection (prevents kernel panic)
- Persistent deletion store (survives soft reboot)

poing.h: poing init system
--------------------------
- Service-based init system
- Units: syslog, network, display, shell, wm, etc.
- poingctl command for control:
  - status  - Show all units and their state
  - start   - Start a service
  - stop    - Stop a service  
  - enable  - Enable service to start at boot
  - disable - Disable service
  - restart - Restart a service
  - reload  - Reload configuration
  - reboot  - Reboot system
  - halt    - Halt system
  - shutdown - Shutdown system
  - sleep   - Suspend system

poing/poing.h: Service Units
-----------------------
- syslog.service - System logging
- network.service - Network stack
- display.service - Display server
- shell.service - Shell/terminal
- wm.service - Window manager

Logger System
--------------
- log_poing() - Log from poing services
- log_kern() - Kernel logging
- Accessible via /var/log/poing.log and /var/log/kernel.log

Critical Path Protection
-------------------------
Deleting certain paths causes kernel panic:
- / (root)
- /boot, /kernel, /drivers, /shell, /crypto
- /boot/pig.bin, /boot/boot.asm, /boot/grub.cfg
- /kernel/kernel.c, /kernel/pigfs.h, /kernel/mem.h

This prevents system from becoming unbootable.

Login System
------------
The login now validates against the user database in users.h.
- Default: root / toor
- Change with: passwd root
- New users added with adduser have default password "changeme"
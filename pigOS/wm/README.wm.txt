pigOS wm/ - PigWM Window Manager
================================

PigWM: Windows-inspired text-mode compositor
--------------------------------------------
- Login screen after shell login
- Menubar (top) with pigOS branding
- Taskbar (bottom) with start button + open windows
- Desktop wallpaper (synthwave pattern + pig art)
- Desktop icons (Terminal, Robin, pigssl, poingctl, README)
- CP437 box-drawing windows with title bars, [X] buttons
- Multiple windows with Z-ordering
- TAB to cycle window focus
- F2 opens terminal
- F3 opens Robin file explorer
- F5 opens power menu
- ESC returns to shell

PigComp: Compositor
-------------------
- Z-ordered window rendering
- Handles window drawing and focus
- Taskbar integration

Robin: File Explorer
--------------------
- Sidebar with directory tree
- Main panel with file listing
- Fake-but-pretty file browser
- Navigate directories, view files

VDM - Vibecoded Display Manager
-------------------------------
- Ported to pigOS from custom VDM project
- Login screen with username/password
- Location: /project/
- Requires real GPU hardware
- Default login: root / toor

Desktop Icons
-------------
- Terminal - Opens shell
- Robin - File explorer
- pigssl - SSL/TLS info
- poingctl - Init system control
- README - Documentation

Window Controls
---------------
[X] - Close window
[─] - Minimize
[□] - Maximize (if implemented)

Keyboard Shortcuts
------------------
F2    - Open terminal
F3    - Open file browser (Robin)
F5    - Open power menu
TAB   - Cycle window focus
ESC   - Return to shell

Login Flow
-----------
1. Shell starts after boot
2. Type 'wm' to launch desktop
3. Login screen appears
4. Enter username and password
5. Desktop loads after authentication

To change password:
  passwd root
  (works for both shell and wm login)

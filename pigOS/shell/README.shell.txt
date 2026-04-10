pigOS shell/ - larpshell v5.9.3
================================

OVERVIEW
-------
larpshell is the primary shell for pigOS. Based on lash by github.com/usr-undeleted/lash.

150+ built-in commands including:

SYSTEM COMMANDS
---------------
help              - Show all available commands
about             - About pigOS
version           - Show version information
uname             - System information
dmesg             - Show boot messages
panic             - Fake kernel panic (easter egg)
reboot            - Reboot system
halt              - Halt system
shutdown          - Shutdown system
pigfetch         - System information with pig ASCII art

INIT SYSTEM
----------
poingctl          - poing init system control
                    Usage: poingctl status|start|stop|enable|disable|restart|reboot|halt|shutdown|sleep

USER MANAGEMENT
---------------
adduser <name>    - Create new user
deluser <name>    - Delete user
usermod -G <grp> <user> - Add user to group
groupadd <name>   - Create group
passwd [user]     - Change password (UPDATES DATABASE, use with login)
rok               - Elevated execution (sudo-like)
r-                - Switch to root user

FILE COMMANDS
------------
ls [-la] [dir]    - List directory (supports subdirectories)
cd <dir>          - Change directory
pwd               - Print working directory
cat <file>        - View file contents
more <file>       - View file (paginated)
head <file>       - Show first lines
tail <file>       - Show last lines
grep <pat> [file] - Search in files
find [path] -name <pat> - Find files by name
stat <file>       - File information
mkdir <dir>       - Create directory
touch <file>      - Create empty file
rm <file>         - Remove file (with critical path protection)
cp <src> <dst>    - Copy file
mv <src> <dst>    - Move file

PROCESS MANAGEMENT
-------------------
ps                - Show processes
top                - Task manager (text mode)
htop               - Interactive process viewer
kill <pid>        - Kill process by PID
killall <name>    - Kill all processes by name
pgrep <pat>       - Find process by pattern
pkill <pat>       - Kill processes by pattern

NETWORK COMMANDS
----------------
ifconfig          - Show network config
ip                - Show network config
ping <host>       - Ping a host
netstat           - Network statistics
traceroute <host> - Trace route to host
nslookup <host>   - DNS lookup
pigget <url>      - Download file
pigcurl <url>     - Fetch URL content

HARDWARE INFO
-------------
lspci             - List PCI devices
lsusb             - List USB devices
lsmod             - List kernel modules
lscpu             - CPU information
lsmem             - Memory information
lshw              - Hardware information
lsblk             - Block devices

PACKAGE MANAGER (rpk)
--------------------
rpk on <pkg>      - Install package
rpk install <pkg> - Install package (alias)
rpk remove <pkg>  - Remove package
rpk list          - List installed packages
rpk search <pat>  - Search available packages
rpk info <pkg>    - Show package information
rpk roam pk [q]   - Browse package database

CRYPTO COMMANDS
---------------
pigssl            - SSL/TLS information
pigcrypt <text>   - Encrypt text
pigdecrypt <text> - Decrypt text
pighash <text>   - Hash text
pigkey            - Generate RSA key pair
pigcert          - Generate certificate

UTILITY COMMANDS
----------------
calc <a> <op> <b> - Calculator (+ - * / %)
base64 <text>    - Base64 encode
xxd <text>       - Hex dump
history          - Show command history
alias [name=val] - Create/view aliases
which <cmd>      - Find command location
type <cmd>       - Show command type

APPLICATIONS
------------
wm                 - Launch PigWM desktop (login required)
vdm                 - Vibecoded Display Manager (login required)
robin              - File browser
browser / pigbrowse - Web browser (requires network)
llm <question>    - AI chat
python3           - Python REPL (simulated)
pip3              - Python package manager
make              - Build system (simulated)
gdb               - GNU debugger (simulated)

GAMES
-----
snake             - Snake game
tetris            - Tetris game
matrix            - Matrix digital rain effect
fortune          - Random fortune cookie
cowpig            - ASCII cow art

TEXT EDITORS
-----------
pi <file>         - Modal editor (vim-like)
                   i = insert mode
                   ESC = normal mode
                   :wq = save and quit
                   :q = quit
                   :q! = quit without saving
                   dd = delete line
                   yy = yank line

pnano <file>       - Simple nano-like editor
                   ^X = exit
                   ^O = save
                   ^Y = page up
                   ^V = page down

SHELL FEATURES
-------------
TAB completion    - Commands and files
                    - Single match: auto-completes
                    - Multiple matches: shows list below input
                    - New prompt shown with typed text preserved

UP/DOWN arrows    - Navigate command history
CTRL+C            - Cancel current input
CTRL+L            - Clear screen
Backspace         - Delete characters

Colored prompt    - Shows: [user@pigOS:dir]$
                    - Green = user
                    - Cyan = pigOS
                    - Yellow = current directory

LOGIN (required for wm/vdm)
-------------------------
First login uses default credentials:
  Username: root
  Password: toor

After login, password can be changed:
  passwd root
  New password: [enter new password]

Password is stored in user database and validated against it.
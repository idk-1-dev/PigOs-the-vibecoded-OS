pigOS Command Reference - Complete List
=====================================

SYSTEM COMMANDS
-------------
help              - Show all commands
about             - About pigOS
version           - Show version
uname             - System info (-a, -r, -m, -n, -s)
hostname          - Show/set hostname
uptime            - Show system uptime
date             - Show/set date/time
dmesg             - Show boot messages
clear / cls       - Clear screen
color             - Cycle through colors
reboot            - Reboot system
halt / poweroff   - Halt system
shutdown         - Shutdown system
panic             - Fake kernel panic (easter egg)
poingctl          - Init system control
polctl           - Policy control

USER MANAGEMENT
--------------
adduser <name>     - Create new user
deluser <name>     - Delete user
usermod <opts>    - Modify user (-G <group> <user>)
groupadd <name>    - Create group
groupdel <name>   - Delete group
groups [user]     - Show user groups
passwd [user]     - Change password
whoami            - Show current user
id               - Show user/group IDs
users             - List logged in users
who / w / last    - Who is logged in
rok               - Run as root
su <user>         - Switch user
r-                - Switch to root
chsh             - Change shell

FILE COMMANDS
-----------
ls [opts] [dir]   - List (-la, -lh, -l, -R, -r, -S, -t)
ll / la / lh     - ls aliases
tree [dir]       - Directory tree
cat <file>       - View file
more / less      - Paginated view
head / tail      - First/last lines
grep <pat> [file] - Search in files
find [path] -name <pat> - Find files
wc               - Word count
diff             - Compare files
sort             - Sort lines
uniq             - Unique lines
cut              - Cut columns
awk              - Pattern processing
sed              - Stream editor
xargs            - Argument list builder
tee              - Tee output

mkdir <dir>       - Create directory
rmdir <dir>      - Remove directory
rm [opts] <file> - Remove (-r, -f, -i, -v)
cp <src> <dst>   - Copy
mv <src> <dst>   - Move
touch <file>      - Create file
ln <src> <link> - Create link
chmod <mode> <file> - Change mode
chown <owner> <file> - Change owner
stat <file>       - File info

pwd               - Print working directory
cd [dir]          - Change directory
pushd / popd      - Directory stack
file  - File type

TEXT EDITORS
-----------
pi <file>         - Vim-like editor
                   i = insert mode
                   ESC = normal mode
                   :wq = save/quit
                   :q = quit
                   :q! = force quit
                   dd/yy = delete/yank line

pnano <file>     - Nano-like editor
                   ^X = exit
                   ^O = save
                   ^Y = page up
                   ^V = page down

pivi <file>      - Vi Improved (basic)

PROCESS MANAGEMENT
----------------
ps [opts]         - Processes (-a, -e, -f, -l, -w)
top               - Task manager
htop             - Interactive top
kill <pid>        - Kill process
killall <name>    - Kill by name
pgrep <pat>       - Find process
pkill <pat>       - Kill by pattern
nice <prio> <cmd> - Set priority
jobs              - Background jobs
bg / fg           - Background/foreground
nohup            - Run immune to hangup
wait             - Wait for process

NETWORK COMMANDS
-------------
ifconfig / ip     - Network config
ping <host>       - Ping host
netstat / ss     - Network stats
pigget <url>     - Download file
wget <url>       - Download
curl <url>       - Fetch URL
pigcurl <url>     - Fetch with libcurl
nslookup <host>   - DNS lookup
dig               - DNS dig
traceroute <host> - Trace route
arp / ip neigh    - ARP table
route / ip route  - Routing table
netpg            - NetPG tool
pignet           - PigNet tool
dhcpcd           - DHCP client

HARDWARE INFO
------------
lspci             - List PCI
lsusb             - List USB
lsmod             - Kernel modules
modprobe <mod>    - Load module
rmmod <mod>       - Remove module
lscpu             - CPU info
lsmem             - Memory info
lshw              - Hardware info
lsblk             - Block devices
df                - Disk usage
du                - Directory usage
free              - Memory usage
mount / umount     - Mount filesystem
fdisk <dev>       - Partition tool
mkfs <dev>       - Make filesystem
fsck <fs>         - Filesystem check
dd                - Copy/convert
sync              - Sync filesystem

PACKAGE MANAGER (rpk)
--------------------
rpk <cmd> [opts]  - Package manager
rpk on <pkg>     - Install package
rpk install <pkg> - Install alias
rpk remove <pkg>  - Remove package
rpk list          - List packages
rpk search <pat>  - Search
rpk info <pkg>    - Package info
rpk deps <pkg>   - Show dependencies
rpk roam [q]     - Browse database

pigpkg            - Alternative package manager

CRYPTO COMMANDS
--------------
pigssl            - SSL/TLS info
pigcrypt <text>   - Encrypt text
pigdecrypt <text> - Decrypt text
pighash <text>   - Hash text
pigkey            - Generate RSA key
pigcert          - Generate certificate

BASH COMPATIBILITY
----------------
echo <text>       - Print text
printf <fmt>     - Print formatted
export <var>=<val> - Export variable
env              - Show environment
alias [name=val]   - Create alias
unalias <name>    - Remove alias
which <cmd>      - Find command
type <cmd>       - Command type

history           - Command history
set / unset       - Set variables
eval <cmd>        - Evaluate
source <file>     - Source script
!!                - Last command

calc / bc         - Calculator
pigmath           - Math operations
seq <start>       - Sequence
yes               - Repeat output
true / false      - Boolean
sleep <sec>       - Sleep
countdown <sec>   - Countdown
timer             - Timer
watch <cmd>       - Watch command
timeout <sec> <cmd> - Run with timeout
tee              - Tee to file

base64 <text>    - Base64 encode/decode
xxd <text>       - Hex dump
strings           - Extract strings
od               - Octal dump
tar [opts] <files> - Archive
gzip / gunzip    - Compress
zip / unzip      - Zip archive

man / info        - Manual pages
whatis <cmd>     - Command info
apropos <pat>     - Search manpages

DESKTOP
-------
wm                - Launch PigWM
robin             - File explorer
pigbrowse         - Web browser
browser          - Browser alias
compositor        - Compositor

AI/CHAT
-----
llm <question>    - AI chat
pigai <question> - AI assistant
ask <question>   - Ask AI
chat <question>  - Chat mode

PYTHON/DEVELOPMENT
----------------
python3           - Python REPL
pip3 <cmd>      - Python package manager
make <target>    - Build
go <cmd>        - Go tool
larpshell        - Shell startup

GAMES
-----
snake             - Snake game
tetris           - Tetris game
matrix           - Matrix effect
fortune          - Fortune cookie
cowpig           - ASCII cow
figpig           - Figlet pig
banner           - Banner text
cal              - Calendar

NOTES/TODO
---------
todo <task>       - Todo list
note <text>       - Make note
remind <time> <task> - Reminder
piglog           - Log entries

EXTRAS
------
dmesg             - Kernel log (also in system)
discord / dc      - Discord bot features
pigfetch         - System info display

TOTAL: 150+ commands

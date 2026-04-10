# pigOS Command Reference - Complete List

## SYSTEM COMMANDS

`help` - Show all commands  
`about` - About pigOS  
`version` - Show version  
`uname` - System info (-a, -r, -m, -n, -s)  
`hostname` - Show/set hostname  
`uptime` - Show system uptime  
`date` - Show/set date/time  
`dmesg` - Show boot messages  
`clear / cls` - Clear screen  
`color` - Cycle through colors  
`reboot` - Reboot system  
`halt / poweroff` - Halt system  
`shutdown` - Shutdown system  
`panic` - Fake kernel panic (easter egg)  
`poingctl` - Init system control  
`polctl` - Policy control  

## USER MANAGEMENT

`adduser <name>` - Create new user  
`deluser <name>` - Delete user  
`usermod <opts>` - Modify user (-G <group> <user>)  
`groupadd <name>` - Create group  
`groupdel <name>` - Delete group  
`groups [user]` - Show user groups  
`passwd [user]` - Change password  
`whoami` - Show current user  
`id` - Show user/group IDs  
`users` - List logged in users  
`who / w / last` - Who is logged in  
`rok` - Run as root  
`su <user>` - Switch user  
`r-` - Switch

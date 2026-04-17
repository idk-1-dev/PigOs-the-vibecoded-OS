#pragma once
// rpk - pigOS Package Manager - Fixed: installed packages actually usable
#include "../drivers/vga/vga.h"
#include "../kernel/mem.h"
#include "../kernel/pigfs.h"

#define RPK_MAX 48
typedef struct {
    char name[32]; char version[16]; char desc[64];
    int installed; int size_kb;
} RpkPkg;

static RpkPkg rpk_db[]={
    {"btop",      "4.0.0",  "System monitor (TUI)",          0,128},
    {"htop",      "3.3.0",  "Interactive process viewer",    0, 64},
    {"pivi",      "1.0.0",  "Advanced text editor",          0,512},
    {"python3",   "3.12.0", "Python interpreter",            0,4096},
    {"pip3",      "23.3.0", "Python package installer",      0, 64},
    {"neofetch",  "7.1.0",  "System info tool",              0, 32},
    {"curl",      "8.5.0",  "Transfer data with URLs",       0,256},
    {"wget",      "1.21.4", "Network downloader",            0,128},
    {"git",       "2.44.0", "Version control system",        0,512},
    {"pnano",     "1.0.0",  "Simple text editor",             0, 64},
    {"bash",      "5.2.0",  "Bourne Again Shell",             0,256},
    {"gcc",       "13.2.0", "GNU C compiler",                0,2048},
    {"nmap",      "7.94.0", "Network scanner",                0,256},
    {"tcpdump",   "4.99.4", "Packet analyzer",               0,128},
    {"openssl",   "3.2.1",  "TLS/SSL toolkit",               0,1024},
    {"ssh",       "9.6.0",  "Secure shell client/server",    0,256},
    {"tmux",      "3.4.0",  "Terminal multiplexer",          0,128},
    {"zsh",       "5.9.0",  "Z shell",                       0,512},
    {"fish",      "3.7.0",  "Friendly interactive shell",    0,256},
    {"make",      "4.4.1",  "Build tool",                    0,512},
    {"gdb",       "14.1.0", "GNU debugger",                  0,1024},
    {"strace",    "6.6.0",  "System call tracer",            0,256},
    {"lsof",      "4.98.0", "List open files",                0, 64},
    {"tree",      "2.1.1",  "Directory tree viewer",         0, 16},
    {"jq",        "1.7.1",  "JSON processor",                0, 64},
    {"ripgrep",   "14.1.0", "Fast grep (rg)",                0,128},
    {"fd",        "9.0.0",  "Fast find",                     0, 64},
    {"", "", "", 0, 0}
};

// Track what commands are available after install
#define RPK_INSTALLED_CMDS 32
static char rpk_installed_cmds[RPK_INSTALLED_CMDS][32];
static int rpk_ncmds=0;

static int rpk_cmd_installed(const char*n){
    for(int i=0;i<rpk_ncmds;i++) if(!ksc(rpk_installed_cmds[i],n)) return 1;
    return 0;
}

static int rpk_find_pkg(const char*n){
    for(int i=0;rpk_db[i].name[0];i++) if(!ksc(rpk_db[i].name,n)) return i;
    return -1;
}

// Use the same DNS resolver path as nslookup/ping for repository lookup.
static int rpk_resolve_repo(void){
    extern int resolve_hostname_probe_for_tools(const char*);
    return resolve_hostname_probe_for_tools("google.com");
}

static void rpk_progress(const char*action,const char*pkg){
    vset(C_LCYAN,C_BLACK); vps("rpk: "); vps(action); vps(" "); vset(C_YELLOW,C_BLACK); vps(pkg); vrst(); vpc('\n');
    vps("  [");
    for(int i=0;i<32;i++){
        vat(0xDB,COL(C_LGREEN,C_BLACK),vx,vy); vx++;
        for(volatile int d=0;d<2000000;d++);
    }
    vpln("] done");
}

static void cmd_rpk_run(const char*pkg){
    // Simulate running an installed package
    if(!ksc(pkg,"btop")||!ksc(pkg,"htop")){
        extern void cmd_top_sh();
        // Show a btop/htop style display
        vclear();
        uint8_t hb=COL(C_BLACK,C_LGREEN);
        vfill(0,0,VGA_W,1,' ',hb);
        char title[64]; kcp(title," "); kcat(title,pkg); kcat(title," - pigOS System Monitor");
        vstr(0,0,hb,title);
        vset(C_WHITE,C_BLACK);
        vpc('\n');
        // CPU meter
        vstr(1,2,COL(C_LCYAN,C_BLACK),"CPU[");
        vfill(5,2,70,1,0xDB,COL(C_LGREEN,C_BLACK));
        vstr(76,2,COL(C_WHITE,C_BLACK),"100%]");
        vstr(1,3,COL(C_LCYAN,C_BLACK),"Mem[");
        vfill(5,3,22,1,0xDB,COL(C_GREEN,C_BLACK));
        vfill(27,3,48,1,' ',COL(C_BLACK,C_BLACK));
        vstr(76,3,COL(C_WHITE,C_BLACK)," 32% ]");
        vpc('\n'); vpc('\n');
        vset(C_LCYAN,C_BLACK); vpln("  PID   USER   PRI  NI  VIRT  RES  CPU%  MEM%  CMD"); vrst();
        vpln("  100   root    20   0  4096  1024  99.9  32.0  pigKernel");
        vpln("    1   root    20   0   512    32   0.0   0.1  poing");
        vpln("  200   root    20   0   256    64   0.0   0.5  larpshell");
        vpc('\n');
        vset(C_DGREY,C_BLACK); vpln("[q=quit  arrows=scroll  F10=quit]"); vrst();
        int cc; do{cc=kb_get();}while(cc!='q'&&cc!=27&&cc!=KEY_F10);
        vclear();
    } else if(!ksc(pkg,"python3")){
        vclear();
        vset(C_LGREEN,C_BLACK);
        vpln("Python 3.12.0 (pigOS build, Jan 1 1970)");
        vpln("[GCC 13.2.0] on pigOS");
        vpln("Type \"help\", \"copyright\", \"credits\" or \"license\" for more information.");
        vrst();
        vps(">>> ");
        // Simple Python REPL simulation
        char pybuf[256]=""; int pylen=0;
        while(1){
            int c=kb_get();
            if(c==27||(!ksc(pybuf,"exit()")&&c=='\n')) break;
            if(c=='\n'){
                pybuf[pylen]=0; vpc('\n');
                if(!ksc(pybuf,"exit()")||!ksc(pybuf,"quit()")) break;
                else if(ksnc(pybuf,"print(",6)==0){
                    vset(C_WHITE,C_BLACK);
                    const char*pp=pybuf+6;
                    while(*pp&&*pp!=')')vpc((uint8_t)*pp++);
                    vpc('\n');
                } else if(ksnc(pybuf,"import",6)==0){
                    vps("[pigOS Python: import simulated] ");vpln(pybuf+7);
                } else { vpln(pybuf); }
                pylen=0; pybuf[0]=0; vps(">>> ");
            } else if(c=='\b'){ if(pylen>0){pylen--;vpc('\b');} }
            else if(c>=32&&c<127&&pylen<255){ pybuf[pylen++]=(char)c; vpc((uint8_t)c); }
        }
        vclear(); vrst();
    } else if(!ksc(pkg,"pivi")||!ksc(pkg,"pi")){
        
        
    } else if(!ksc(pkg,"ssh")){
        vpln("ssh: pigOS SSH client (simulated)");
        vpln("Usage: ssh [user@]host");
        vpln("[no network - add -net nic,model=rtl8139 -net user to QEMU]");
    } else if(!ksc(pkg,"git")){
        vclear();
        vset(C_LRED,C_BLACK); vpln("git version 2.44.0 (pigOS build)"); vrst();
        vpln("usage: git <command> [options]");
        vpln("  init      Create empty repository");
        vpln("  status    Show working tree status");
        vpln("  add       Stage files");
        vpln("  commit    Record changes");
        vpln("  log       Show commit history");
        vpln("[pigOS git: virtual FS, no real storage]");
    } else if(!ksc(pkg,"make")){
        vpln("make: running Makefile...");
        vpln("CC=gcc CFLAGS=-m64 -ffreestanding...");
        vpln("make: Nothing to be done (no real build system in kernel mode)");
    } else {
        // Generic: show the package ran
        vps(pkg); vps(" v"); vps(rpk_db[rpk_find_pkg(pkg)].version);
        vpln(" (pigOS build)");
        vps("["); vps(pkg); vpln("] running... (simulated - type q or ESC to exit)");
        int cc; do{cc=kb_get();}while(cc!='q'&&cc!=27);
    }
}

static void cmd_rpk(const char*args){
    if(!args||!*args){
        vset(C_LCYAN,C_BLACK); vpln("rpk - pigOS Package Manager v1.0"); vrst();
        vpln("Usage: rpk <command> [pkg]");
        vpln("  on <pkg>        - install package");
        vpln("  install <pkg>   - install package");
        vpln("  remove <pkg>    - remove package");
        vpln("  list            - list installed");
        vpln("  roam pk [q]     - search packages");
        vpln("  info <pkg>      - package info");
        vpln("  update          - refresh list");
        vpln("  upgrade         - upgrade all");
        vpln("  clean           - clean cache");
        vpln("  deps <pkg>      - show deps");
        return;
    }

    char sub[32]; int si=0;
    const char*p=args;
    while(*p&&*p!=' '&&si<31) sub[si++]=*p++;
    sub[si]=0; while(*p==' ')p++;
    const char*pkg=p;

    if(!ksc(sub,"install")||!ksc(sub,"on")){
        if(!*pkg){vpln("rpk: package name required");return;}
        int idx=rpk_find_pkg(pkg);
        if(idx<0){vset(C_LRED,C_BLACK);vps("rpk: package '");vps(pkg);vpln("' not found. Try: rpk roam pk");vrst();return;}
        if(rpk_db[idx].installed){vps(pkg);vpln(" is already installed");return;}
        if(rpk_resolve_repo()==0){
            vset(C_DGREY,C_BLACK);vps("rpk: repo resolved via DNS\n");vrst();
        } else {
            vset(C_YELLOW,C_BLACK);vps("rpk: repo DNS unresolved, using cached metadata\n");vrst();
        }
        rpk_progress("downloading",pkg);
        rpk_progress("installing",pkg);
        rpk_db[idx].installed=1;
        // Register as runnable command
        if(rpk_ncmds<RPK_INSTALLED_CMDS) kcp(rpk_installed_cmds[rpk_ncmds++],pkg);
        // Add to /usr/bin
        char bp[64]; kcp(bp,"/usr/bin/"); kcat(bp,pkg);
        MemFile*m=memfs_create(bp);
        if(m) kcp(m->content,"[installed binary]");
        vset(C_LGREEN,C_BLACK); vps("rpk: installed "); vps(pkg);
        char sb[8]; kia(rpk_db[idx].size_kb,sb);
        vps(" ("); vps(sb); vpln("KB)"); vrst();
    } else if(!ksc(sub,"remove")){
        if(!*pkg){vpln("rpk: package name required");return;}
        int idx=rpk_find_pkg(pkg);
        if(idx<0){vset(C_LRED,C_BLACK);vps("rpk: '");vps(pkg);vpln("' not found");vrst();return;}
        if(!rpk_db[idx].installed){vps(pkg);vpln(" is not installed");return;}
        if(!ksc(pkg,"larpshell")||!ksc(pkg,"poing")||!ksc(pkg,"rpk")){
            vset(C_LRED,C_BLACK); vpln("rpk: cannot remove essential package"); vrst(); return;
        }
        rpk_progress("removing",pkg);
        rpk_db[idx].installed=0;
        // Remove from cmd list
        for(int i=0;i<rpk_ncmds;i++){
            if(!ksc(rpk_installed_cmds[i],pkg)){
                for(int j=i;j<rpk_ncmds-1;j++) kcp(rpk_installed_cmds[j],rpk_installed_cmds[j+1]);
                rpk_installed_cmds[--rpk_ncmds][0]=0; break;
            }
        }
        vset(C_LGREEN,C_BLACK); vps("rpk: removed "); vpln(pkg); vrst();
    } else if(!ksc(sub,"list")){
        vset(C_LCYAN,C_BLACK); vpln("Installed packages:"); vrst();
        int cnt=0;
        for(int i=0;rpk_db[i].name[0];i++){
            if(!rpk_db[i].installed) continue;
            vset(C_LGREEN,C_BLACK); vps("  "); vps(rpk_db[i].name);
            vset(C_DGREY,C_BLACK); vps("\t"); vps(rpk_db[i].version); vps("\t");
            vset(C_WHITE,C_BLACK); vpln(rpk_db[i].desc); cnt++;
        }
        char sb[8]; kia(cnt,sb); vps(sb); vpln(" packages installed");
    } else if(!ksc(sub,"roam")){
        // "roam pk [query]"
        if(*pkg=='p'&&*(pkg+1)=='k'){pkg+=2;while(*pkg==' ')pkg++;}
        vset(C_LCYAN,C_BLACK);
        vps("Available packages");
        if(*pkg){vps(" matching '");vps(pkg);vps("'");}
        vpln(":"); vrst();
        int ql=ksl(pkg);
        for(int i=0;rpk_db[i].name[0];i++){
            if(ql>0&&ksnc(rpk_db[i].name,pkg,ql)!=0&&ksnc(rpk_db[i].desc,pkg,ql)!=0) continue;
            if(rpk_db[i].installed) vset(C_LGREEN,C_BLACK);
            else vset(C_WHITE,C_BLACK);
            vps("  "); vps(rpk_db[i].name);
            vset(C_DGREY,C_BLACK); vps("\t"); vps(rpk_db[i].version); vps("\t");
            vset(C_WHITE,C_BLACK); vpln(rpk_db[i].desc);
        }
    } else if(!ksc(sub,"info")){
        int idx=rpk_find_pkg(pkg);
        if(idx<0){vps("rpk: '");vps(pkg);vpln("' not found");return;}
        RpkPkg*r=&rpk_db[idx];
        vset(C_LCYAN,C_BLACK); vps("Package: "); vset(C_YELLOW,C_BLACK); vpln(r->name); vrst();
        vps("Version:  "); vpln(r->version);
        vps("Desc:     "); vpln(r->desc);
        char sb[8]; kia(r->size_kb,sb);
        vps("Size:     "); vps(sb); vpln("KB");
        vps("Status:   "); vpln(r->installed?"installed":"available");
    } else if(!ksc(sub,"update")){
        vset(C_LCYAN,C_BLACK); vpln("rpk: refreshing package database..."); vrst();
        if(rpk_resolve_repo()==0){
            vset(C_DGREY,C_BLACK);vpln("rpk: repo DNS ok");vrst();
        } else {
            vset(C_YELLOW,C_BLACK);vpln("rpk: repo DNS failed (offline cache mode)");vrst();
        }
        for(volatile int i=0;i<50000000;i++);
        vpln("rpk: 32 packages available");
    } else if(!ksc(sub,"upgrade")){
        vpln("rpk: checking upgrades... all packages up to date");
    } else if(!ksc(sub,"clean")){
        vpln("rpk: cache cleaned");
    } else if(!ksc(sub,"deps")){
        vps("Dependencies for "); vpln(*pkg?pkg:"?");
        vpln("  larpshell >= 2.0  poing >= 1.0");
    } else {
        vset(C_LRED,C_BLACK); vps("rpk: unknown: "); vpln(sub); vrst();
        vpln("rpk help");
    }
}

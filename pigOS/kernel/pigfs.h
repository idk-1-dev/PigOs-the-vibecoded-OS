#pragma once
// pigOS Virtual Filesystem - Fixed path resolution
#include "../drivers/vga/vga.h"
#include "mem.h"

typedef struct { const char* path; int is_dir; const char* content; } VNode;
static const VNode VFS[] = {
    {"/",1,NULL},{"/boot",1,NULL},{"/kernel",1,NULL},{"/drivers",1,NULL},
    {"/drivers/vga",1,NULL},{"/drivers/kbd",1,NULL},{"/drivers/ps2",1,NULL},{"/drivers/net",1,NULL},
    {"/shell",1,NULL},{"/shell/games",1,NULL},{"/wm",1,NULL},{"/crypto",1,NULL},
    {"/dev",1,NULL},{"/proc",1,NULL},    {"/etc",1,NULL},{"/etc/skel",1,NULL},{"/root",1,NULL},
    {"/home",1,NULL},{"/tmp",1,NULL},{"/usr",1,NULL},{"/usr/bin",1,NULL},
    {"/usr/lib",1,NULL},{"/var",1,NULL},{"/var/log",1,NULL},{"/bin",1,NULL},{"/sbin",1,NULL},
    {"/proc/net",1,NULL},{"/kernel/poing",1,NULL},
    // Root files
    {"/README.txt",0,"pigOS v1.0 x86_64 - type 'help' for commands\ntype 'wm' for desktop\n"},
    {"/Makefile",0,"CC=gcc\nNASM=nasm\nCFLAGS=-m64 -ffreestanding\nall: dirs pigOS.iso\n"},
    {"/linker.ld",0,"ENTRY(_start)\nSECTIONS { . = 1M; .text BLOCK(4K): ... }\n"},
    // /boot
    {"/boot/README.txt",0,"boot/ - FLARE Bootloader\nboot.asm: x86_64 Long Mode bootstrap\npig.bin: compiled pigKernel ELF\ngrub.cfg: hidden GRUB config\n"},
    {"/boot/boot.asm",0,"; FLARE x86_64 bootstrap\n; PML4->PDPT->PD page tables (2MB pages)\n; Long Mode, calls kernel_main(magic, mb_info)\n"},
    {"/boot/pig.bin",0,"[pigKernel ELF binary - 151KB]"},
    {"/boot/grub.cfg",0,"# FLARE - 5s countdown\nset timeout=5\nset timeout_style=countdown\nmenuentry \"pigOS\" { multiboot /boot/pig.bin; boot }"},
    // /kernel
    {"/kernel/README.txt",0,"kernel/ - pigKernel Core\nkernel.c:  main entry, FLARE splash, verbose poing boot\nmem.h:     4MB bump allocator @ 0x400000\nusers.h:   user/group system\npigfs.h:   virtual filesystem\npoing/:    poing init system (13 units)\n"},
    {"/kernel/kernel.c",0,"// pigKernel - FLARE splash + verbose poing boot + shell_run()"},
    {"/kernel/mem.h",0,"// 4MB bump allocator @ 0x400000\n// km() kf() kmc() kms() ksl() ksc()"},
    {"/kernel/users.h",0,"// User/group: adduser, usermod, groupadd, passwd, rok, su"},
    {"/kernel/pigfs.h",0,"// Virtual FS: path resolution, ls, cat, cd, grep, find, rm, touch"},
    {"/kernel/poing/poing.h",0,"// poing init system v1.0\n// poingctl status|start|stop|enable|disable|restart|reload|reboot|halt|shutdown|sleep"},
    // /drivers
    {"/drivers/README.txt",0,"drivers/ - Hardware Drivers\nvga/vga.h:     VGA 80x25 CP437 + hardware cursor\nkbd/keyboard.h: PS/2 keyboard (arrows, F-keys, ctrl)\nps2/ps2.h:     Combined PS/2 controller\nnet/network.h: RTL8139 PCI detection\n"},
    {"/drivers/vga/vga.h",0,"// VGA 80x25 CP437 driver\n// Hardware cursor, colors, box chars"},
    {"/drivers/kbd/keyboard.h",0,"// PS/2 keyboard driver"},
    {"/drivers/ps2/ps2.h",0,"// PS/2 combined controller\n// Mouse disabled until IRQ support added"},
    {"/drivers/net/network.h",0,"// RTL8139 Ethernet driver\n// PCI detection, TX/RX skeleton"},
    // /shell
    {"/shell/README.txt",0,"shell/ - larpshell v2.2\nshell.h:    150+ commands\nfastfetch.h: fastfetch with pig art\npolctl.h:   log viewer (journalctl-style)\nrpk.h:      package manager\nnetpg.h:    network manager\nvi.h:       pi/pivi modal editor\ngames/:     snake tetris matrix\n"},
    {"/shell/shell.h",0,"// larpshell v2.2\n// 150+ commands, tab complete, history, pi, pnano"},
    {"/shell/vi.h",0,"// pi/pivi modal editor\n// i=insert ESC=normal :wq=save+quit dd=delete"},
    {"/shell/fastfetch.h",0,"// fastfetch - system info with pig ASCII art"},
    {"/shell/polctl.h",0,"// polctl - log viewer (-f -n N -p PRI -u UNIT)"},
    {"/shell/rpk.h",0,"// rpk - package manager (on/install/remove/list/roam)"},
    {"/shell/netpg.h",0,"// netpg - network manager (status/up/down/ip/dhcp)"},
    {"/shell/games/tetris.h",0,"// Tetris game with 7 tetrominoes"},
    // /wm
    {"/wm/README.txt",0,"wm/ - PigWM compositor\nWin10-style desktop, taskbar, menubar\nF2=terminal window  F3=Robin  F5=power\nTAB=cycle focus  ESC=shell\n"},
    {"/wm/wm.h",0,"// PigWM - Win10-style WM"},
    // /crypto
    {"/crypto/README.txt",0,"crypto/ - pigssl\npigssl.h: pigcrypt pigdecrypt pighash pigkey pigcert\n"},
    {"/crypto/pigssl.h",0,"// pigssl crypto subsystem"},
    // /dev
    {"/dev/tty0",0,"[VGA console - 80x25 text]"},
    {"/dev/kbd",0,"[PS/2 keyboard]"},
    {"/dev/mouse",0,"[PS/2 mouse - disabled until IRQ]"},
    {"/dev/null",0,""},{"/dev/zero",0,"[zeros]"},{"/dev/random",0,"[entropy]"},
    {"/dev/mem",0,"[physical memory]"},{"/dev/eth0",0,"[RTL8139 - 10/100Mbps]"},
    // /proc
    {"/proc/cpuinfo",0,"processor\t: 0\nvendor_id\t: pigOS\nmodel name\t: x86_64 Virtual CPU\nflags\t\t: fpu pae lm nx sse2\nbogomips\t: 9999.99\n"},
    {"/proc/meminfo",0,"MemTotal:\t4096 kB\nMemFree:\t3072 kB\nMemAvailable:\t3072 kB\n"},
    {"/proc/version",0,"pigOS 1.0.0 pigKernel #1 SMP x86_64"},
    {"/proc/uptime",0,"999.0 999.0"},
    {"/proc/mounts",0,"pigfs / pigfs rw 0 0\ndevfs /dev devfs rw 0 0\nproc /proc proc rw 0 0\n"},
    {"/proc/net/dev",0,"Inter-|Receive|Transmit\nlo:0 0\neth0:0 0\n"},
    // /etc
    {"/etc/passwd",0,"root:x:0:0:root:/root:/bin/pig\n"},
    {"/etc/shadow",0,"root:$pig$hash:19000:0:99999:7:::\n"},
    {"/etc/group",0,"root:x:0:root\nwheel:x:1:root\nusers:x:2:\n"},
    {"/etc/hostname",0,"pigOS\n"},{"/etc/fstab",0,"pigfs / pigfs defaults 0 0\n"},
    {"/etc/pig.conf",0,"INIT=poing\nSHELL=/bin/pig\nWM=PigWM\nNET=pig-net\n"},
    {"/etc/hosts",0,"127.0.0.1 localhost\n127.0.0.1 pigOS\n"},
    {"/etc/resolv.conf",0,"nameserver 8.8.8.8\nnameserver 8.8.4.4\n"},
    {"/etc/skel/.lashrc",0,"# .lashrc - larpshell config\nalias ll='ls -la'\nalias la='ls -la'\nalias cls='clear'\nalias wd='fastfetch'\n"},
    // /usr/bin
    {"/usr/bin/pig",0,"[larpshell binary]"},{"/usr/bin/rpk",0,"[rpk binary]"},
    {"/usr/bin/netpg",0,"[netpg binary]"},{"/usr/bin/pigssl",0,"[pigssl binary]"},
    // /var/log - will be populated at runtime
    {"/var/log/poing.log",0,NULL},{"/var/log/kernel.log",0,NULL},{"/var/log/net.log",0,NULL},{"/var/log/netdebug.log",0,NULL},
    {NULL,0,NULL}
};

// ── In-memory files (touch, pnano saves, etc.) ────────────────
#define MEMFS_MAX 64
typedef struct { char path[128]; char content[2048]; int used; int is_dir; } MemFile;
static MemFile memfs[MEMFS_MAX];
static int memfs_n=0;

// Runtime log storage
#define LOG_MAX 128
#define LOG_LINE 96
static char poing_log[LOG_MAX][LOG_LINE];
static int poing_log_n=0;
static char kern_log[LOG_MAX][LOG_LINE];
static int kern_log_n=0;

static void log_poing(const char*msg){
    if(poing_log_n<LOG_MAX){
        kcp(poing_log[poing_log_n++],msg);
    }
}
static void log_kern(const char*msg){
    if(kern_log_n<LOG_MAX){
        kcp(kern_log[kern_log_n++],msg);
    }
}

// ── Persistent deletion store (survives soft reboot) ──────────
// We store deleted_paths at a fixed physical address 0x600000
// so rm'd files stay deleted even after poingctl reboot
#define DEL_MAGIC  0xDEAD1337u
#define DELETED_MAX 64
typedef struct {
    uint32_t magic;
    uint32_t count;
    char paths[DELETED_MAX][128];
} __attribute__((packed)) DelStore;

// Pointer to physical address 0x600000 (6MB - above kernel)
static DelStore* const del_store = (DelStore*)0x600000;

static void del_store_init(){
    if(del_store->magic != DEL_MAGIC){
        del_store->magic = DEL_MAGIC;
        del_store->count = 0;
        for(int i=0;i<DELETED_MAX;i++) del_store->paths[i][0]=0;
    }
}
static int  deleted_n=0; // local mirror count
static char deleted_paths[DELETED_MAX][128]; // local mirror

static void del_sync_from_store(){
    del_store_init();
    deleted_n=(int)del_store->count;
    if(deleted_n>DELETED_MAX) deleted_n=DELETED_MAX;
    for(int i=0;i<deleted_n;i++) kcp(deleted_paths[i],del_store->paths[i]);
}
static void del_sync_to_store(){
    del_store_init();
    del_store->count=(uint32_t)deleted_n;
    for(int i=0;i<deleted_n;i++) kcp(del_store->paths[i],deleted_paths[i]);
}

static int fs_is_deleted(const char*p){
    // Check persistent store (survives reboot)
    del_store_init();
    uint32_t cnt=del_store->count; if(cnt>DELETED_MAX)cnt=DELETED_MAX;
    for(uint32_t i=0;i<cnt;i++) if(!ksc(del_store->paths[i],p)) return 1;
    // Also check local (for new deletions this session)
    for(int i=0;i<deleted_n;i++) if(!ksc(deleted_paths[i],p)) return 1;
    return 0;
}
static void fs_mark_deleted(const char*p){
    if(deleted_n<DELETED_MAX){
        kcp(deleted_paths[deleted_n],p); deleted_n++;
    }
    // Persist immediately
    del_sync_to_store();
}
static void fs_restore(const char*p){
    // Remove from persistent store (for testing)
    for(int i=0;i<deleted_n;i++) if(!ksc(deleted_paths[i],p)){
        for(int j=i;j<deleted_n-1;j++) kcp(deleted_paths[j],deleted_paths[j+1]);
        deleted_n--; break;
    }
    del_sync_to_store();
}

static MemFile* memfs_find(const char*p){
    for(int i=0;i<memfs_n;i++) if(memfs[i].used&&!ksc(memfs[i].path,p)&&!fs_is_deleted(memfs[i].path)) return &memfs[i];
    return NULL;
}
static MemFile* memfs_create(const char*p){
    for(int i=0;i<memfs_n;i++) if(memfs[i].used&&!ksc(memfs[i].path,p)&&!fs_is_deleted(memfs[i].path)) return &memfs[i];
    if(memfs_n>=MEMFS_MAX) return NULL;
    MemFile*m=&memfs[memfs_n++];
    kcp(m->path,p); m->content[0]=0; m->used=1; m->is_dir=0;
    int l=ksl(p);
    if(l>1&&p[l-1]=='/') m->is_dir=1;
    return m;
}

static const VNode* fs_find_vnode(const char*p){
    if(fs_is_deleted(p)) return NULL;
    for(int i=0;VFS[i].path;i++) if(!ksc(VFS[i].path,p)) return &VFS[i];
    return NULL;
}

// ── Path resolution ──────────────────────────────────────────
static void path_normalize(char*p){
    // Remove trailing slash (unless root)
    int l=ksl(p);
    if(l>1&&p[l-1]=='/') p[l-1]=0;
    // Collapse // -> /
    for(int i=0;p[i];i++){
        if(p[i]=='/'&&p[i+1]=='/'){
            // shift left
            int j=i;
            while(p[j]){p[j]=p[j+1];j++;}
            i--;
        }
    }
}

static void fs_resolve(const char*cwd,const char*path,char*out,int outsz){
    if(!path||!*path||(path[0]=='.'&&path[1]==0)){
        kcp(out,cwd);path_normalize(out);return;}
    if(!ksc(path,"~")){kcp(out,"/");return;}

    if(path[0]=='/'){
        int i=0; while(path[i]&&i<outsz-1){out[i]=path[i];i++;} out[i]=0;
    } else {
        // Relative - build on cwd
        int ol=0;
        // Copy cwd without trailing slash
        int cl=ksl(cwd);
        for(int i=0;i<cl&&ol<outsz-1;i++) out[ol++]=cwd[i];
        out[ol]=0;

        const char*p=path;
        while(*p){
            while(*p=='/') p++; // skip leading slashes in component
            if(!*p) break;

            if(p[0]=='.'&&p[1]=='.'&&(p[2]=='/'||!p[2])){
                // Go up
                int l=ol;
                while(l>1&&out[l-1]!='/') l--;
                if(l>1) l--; // remove the slash too
                if(l<1)l=1; // never go below root
                out[l]=0; ol=l;
                p+=2; if(*p=='/') p++;
                continue;
            }
            if(p[0]=='.'&&(p[1]=='/'||!p[1])){
                p++; if(*p=='/') p++;
                continue;
            }

            // Add component
            if(ol>=outsz-2) break;
            // Add separator
            if(ol>0&&out[ol-1]!='/') out[ol++]='/';
            // Copy component name
            while(*p&&*p!='/'&&ol<outsz-1) out[ol++]=*p++;
            out[ol]=0;
            if(*p=='/') p++;
        }
    }
    if(!out[0]){out[0]='/';out[1]=0;}
    path_normalize(out);
}

// ── ls ────────────────────────────────────────────────────────
static void fs_ls(const char*cwd,const char*arg){
    int long_fmt=0,show_hidden=0;
    char path_arg[256]=".";

    const char*p=arg?arg:"";
    while(*p==' ')p++;
    while(*p=='-'){
        p++;
        while(*p&&*p!=' '&&*p!='-'){
            if(*p=='l')long_fmt=1;
            if(*p=='a'||*p=='A')show_hidden=1;
            if(*p=='h')long_fmt=1;
            p++;
        }
        while(*p==' ')p++;
    }
    if(*p){ kcp(path_arg,p); int al=ksl(path_arg);if(al>1&&path_arg[al-1]=='/')path_arg[al-1]=0; }

    char path[256];
    fs_resolve(cwd,path_arg,path,256);

    // Find target - check VFS first
    const VNode*target=fs_find_vnode(path);
    
    // Also check memfs - try exact path first
    MemFile*mf=NULL;
    if(!target){
        mf=memfs_find(path);
        if(mf && !mf->is_dir){
            vpln(mf->path); return;
        }
        // Try with trailing slash for directories
        if(!mf){
            int pl=ksl(path);
            char try_path[256];
            kcp(try_path,path);
            if(pl>1 && path[pl-1]!='/'){
                kcat(try_path,"/");
                mf=memfs_find(try_path);
                if(mf && mf->is_dir){
                    target=(VNode*)-1;
                    kcp(path,mf->path);
                } else if(mf && !mf->is_dir){
                    vpln(mf->path); return;
                } else {
                    mf=NULL;
                }
            }
        }
        // Also try without trailing slash
        if(!mf){
            int pl=ksl(path);
            char try_path[256];
            kcp(try_path,path);
            if(pl>1 && path[pl-1]=='/'){
                try_path[pl-1]=0;
                mf=memfs_find(try_path);
                if(mf && mf->is_dir){
                    target=(VNode*)-1;
                    kcp(path,mf->path);
                } else if(mf && !mf->is_dir){
                    vpln(mf->path); return;
                } else {
                    mf=NULL;
                }
            }
        }
        // Try absolute path variant
        if(!mf && !target){
            char abs[256]; 
            if(path_arg[0]=='/'){kcp(abs,path_arg);}
            else{kcp(abs,"/");kcat(abs,path_arg);}
            int al=ksl(abs);if(al>1&&abs[al-1]=='/')abs[al-1]=0;
            mf=memfs_find(abs);
            if(mf && !mf->is_dir){vpln(mf->path); return;}
            if(mf && mf->is_dir){
                target=(VNode*)-1;
                kcp(path,mf->path);
            }
            // Try abs with trailing slash
            if(!mf){
                kcat(abs,"/");
                mf=memfs_find(abs);
                if(mf && mf->is_dir){
                    target=(VNode*)-1;
                    kcp(path,mf->path);
                }
            }
        }
    }

    if(!target && !mf){
        vset(C_LRED,C_BLACK);
        vps("ls: cannot access '"); vps(path_arg); vpln("': No such file or directory");
        vrst(); return;
    }
    
    if(mf && !mf->is_dir){
        vpln(mf->path); return;
    }
    
    int is_memfs_dir = (mf != NULL && mf->is_dir);
    if(!target && !is_memfs_dir){
        vset(C_LRED,C_BLACK);
        vps("ls: cannot access '"); vps(path_arg); vpln("': Not a directory");
        vrst(); return;
    }

    if(show_hidden){
        if(ksl(path)>1 || path[0]!='/'){
            vset(C_LBLUE,C_BLACK);
            if(long_fmt){ vpln("drwxr-xr-x  root root    -  ."); vpln("drwxr-xr-x  root root    -  .."); }
            else { vps(".  ../  "); }
            vrst();
        }
    }

    int plen=ksl(path);
    int found=0;
    for(int i=0;VFS[i].path;i++){
        if(fs_is_deleted(VFS[i].path)) continue;
        const char*vp=VFS[i].path;
        int vl=ksl(vp);
        if(vl<=plen) continue;
        if(plen==1&&path[0]=='/'){
            if(vp[0]!='/') continue;
            const char*rest=vp+1; if(!*rest) continue;
            int hs=0; for(const char*q=rest;*q;q++) if(*q=='/'){ hs=1;break;} if(hs) continue;
        } else {
            if(ksnc(vp,path,plen)!=0) continue;
            if(vp[plen]!='/') continue;
            const char*rest=vp+plen+1; if(!*rest) continue;
            int hs=0; for(const char*q=rest;*q;q++) if(*q=='/'){ hs=1;break;} if(hs) continue;
        }
        const char*sl=vp; for(const char*q=vp;*q;q++) if(*q=='/')sl=q+1; if(*sl=='/')sl++;
        if(!*sl) continue;
        if(!show_hidden && sl[0]=='.') continue;

        if(long_fmt){
            if(VFS[i].is_dir){ vset(C_LBLUE,C_BLACK); vps("drwxr-xr-x  root root    - "); vps(sl); vpc('/'); vpc('\n'); }
            else { int sz=VFS[i].content?ksl(VFS[i].content):0; char sb[8];kia(sz,sb); vset(C_WHITE,C_BLACK); vps("-rw-r--r--  root root "); int sl2=ksl(sb);while(sl2++<5)vpc(' '); vps(sb); vps(" "); vps(sl); vpc('\n'); }
        } else {
            if(VFS[i].is_dir){ vset(C_LBLUE,C_BLACK); vps(sl); vps("/  "); }
            else { vset(C_WHITE,C_BLACK); vps(sl); vps("  "); }
        }
        found++;
    }
    // memfs files in this dir
    // First pass: collect unique entry names
    char entries[64][128];
    int entry_is_dir[64];
    int entry_count=0;
    for(int i=0;i<memfs_n;i++){
        if(!memfs[i].used) continue;
        // Skip deleted paths
        if(fs_is_deleted(memfs[i].path)) continue;
        const char*mp=memfs[i].path;
        int ml=ksl(mp);
        
        if(ml<=plen) continue;
        if(ksnc(mp,path,plen)!=0) continue;
        
        const char*rel=mp+plen;
        if(path[plen-1]!='/'){
            if(*rel!='/') continue;
            rel++;
        }
        
        const char*slash=0;
        for(const char*s=rel;*s;s++) if(*s=='/'){slash=s;break;}
        
        char entry[128];
        if(slash){
            int elen=slash-rel;
            if(elen>=128||elen<=0) continue;
            for(int j=0;j<elen;j++) entry[j]=rel[j];
            entry[elen]=0;
        } else {
            kcp(entry,rel);
        }
        
        if(!show_hidden && entry[0]=='.') continue;
        
        // Check if already collected
        int dup=0;
        for(int j=0;j<entry_count;j++){
            if(!ksc(entries[j],entry)){
                if(memfs[i].is_dir) entry_is_dir[j]=1;
                dup=1;break;
            }
        }
        if(!dup&&entry_count<64){
            kcp(entries[entry_count],entry);
            entry_is_dir[entry_count]=memfs[i].is_dir;
            entry_count++;
        }
    }
    
    for(int i=0;i<entry_count;i++){
        if(long_fmt){
            if(entry_is_dir[i]){ vset(C_LBLUE,C_BLACK); vps("drwxr-xr-x  root root    - "); vps(entries[i]); vps("/\n"); }
            else{ vset(C_LGREEN,C_BLACK); vps("-rw-r--r--  root root   -  "); vps(entries[i]); vpc('\n'); }
        } else {
            if(entry_is_dir[i]){ vset(C_LBLUE,C_BLACK); vps(entries[i]); vps("/  "); }
            else{ vset(C_LGREEN,C_BLACK); vps(entries[i]); vps("  "); }
        }
    }
    if(!long_fmt) vpc('\n');
    vrst();
}

// ── cat ───────────────────────────────────────────────────────
static void fs_cat(const char*cwd,const char*arg){
    if(!arg||!*arg){vpln("usage: cat <file>");return;}
    // Skip flags
    const char*a=arg;
    if(a[0]=='-'){while(*a&&*a!=' ')a++;while(*a==' ')a++;}
    if(!*a){vpln("usage: cat <file>");return;}

    char path[256];
    fs_resolve(cwd,a,path,256);

    // 1) Check memfs
    MemFile*mf=memfs_find(path);
    if(mf){ if(mf->content[0])vps(mf->content); vpc('\n'); return; }

    // 2) Check VFS
    const VNode*n=fs_find_vnode(path);
    if(!n){
        vset(C_LRED,C_BLACK);
        vps("cat: "); vps(a); vpln(": No such file or directory");
        vrst(); return;
    }
    if(n->is_dir){ vset(C_LRED,C_BLACK); vps("cat: "); vps(a); vpln(": Is a directory"); vrst(); return; }
    // Special runtime-filled files
    if(!ksc(n->path,"/var/log/poing.log")){
        if(poing_log_n==0){ vpln("[poing.log empty - start services to generate logs]"); return; }
        for(int i=0;i<poing_log_n;i++) vpln(poing_log[i]); return;
    }
    if(!ksc(n->path,"/var/log/kernel.log")){
        if(kern_log_n==0){ vpln("[kernel.log empty]"); return; }
        for(int i=0;i<kern_log_n;i++) vpln(kern_log[i]); return;
    }
    if(!ksc(n->path,"/var/log/netdebug.log")){
        extern void rtl_net_log_get(char*,int);
        char buf[2048]; rtl_net_log_get(buf,2048);
        if(buf[0]){vps(buf);vpc('\n');}
        else vpln("[netdebug.log empty - run rtldebug first]");
        return;
    }
    if(n->content) vps(n->content);
    vpc('\n');
}

// ── touch ─────────────────────────────────────────────────────
static void fs_touch(const char*cwd,const char*arg){
    if(!arg||!*arg){vpln("usage: touch <file>");return;}
    char path[256]; fs_resolve(cwd,arg,path,256);
    MemFile*m=memfs_find(path);
    if(m){vpln("touch: file exists");return;}
    const VNode*n=fs_find_vnode(path);
    if(n){vpln("touch: file exists in pigfs");return;}
    m=memfs_create(path);
    if(m){ vps("touch: created '"); vps(path); vpln("'"); }
    else vpln("touch: memory full");
}

// ── rm - actually removes ─────────────────────────────────────
// Critical paths - protected from deletion
static const char* CRITICAL_PATHS[] = {
    "/", "/etc", "/dev", "/proc", "/root", "/boot", "/kernel", "/drivers", "/shell", "/crypto",
    "/boot/pig.bin", "/boot/boot.asm", "/boot/grub.cfg",
    "/kernel/kernel.c", "/kernel/pigfs.h", "/kernel/mem.h", NULL
};

static int is_critical_path(const char*path){
    // Check for rm -rf / (root directory) - EXACT match only
    if(path[0]=='/' && path[1]==0) return 1;
    // Only check critical paths for ABSOLUTE paths (starting with /)
    // Relative paths like "hi" cannot be critical
    if(path[0] != '/') return 0;
    // Now check - but EXCLUDE "/" prefix from matching
    // Only match if path EQUALS the critical path or starts with critical path + "/"
    for(int i=0;CRITICAL_PATHS[i];i++){
        int cl=ksl(CRITICAL_PATHS[i]);
        // Skip the root "/" - handled above
        if(cl==1 && CRITICAL_PATHS[i][0]=='/') continue;
        // Check if path starts with this critical path AND is followed by / or end
        if(ksnc(path, CRITICAL_PATHS[i], cl)==0){
            // Must be exact match or have / after the critical path
            if(path[cl]==0 || path[cl]=='/') return 1;
        }
    }
    return 0;
}

static void NORETURN_KPANIC(const char*reason, const char*path){
    vclear();
    uint8_t pc=COL(C_WHITE,C_BLUE);
    vfill(0,0,VGA_W,VGA_H,' ',pc);
    int row=0;
    vstr(0,row++,pc,"[  0.000000] BUG: kernel NULL pointer dereference");
    vstr(0,row++,pc,"[  0.000001] #PF: supervisor read access in kernel mode");
    row++;
    vstr(0,row++,pc,"[  0.000002] Oops: Critical filesystem corruption");
    vstr(0,row++,COL(C_YELLOW,C_BLUE),"[  0.000003] REASON: pigfs: critical path removed from VFS");
    vstr(0,row++,COL(C_LRED,C_BLUE),  "[  0.000003] PATH:   ");
    vstr(10,row-1,COL(C_LRED,C_BLUE), path);
    row++;
    vstr(0,row++,pc,"[  0.000004] RIP: fs_rm+0x4f/0xb2 [pigfs]");
    vstr(0,row++,pc,"[  0.000005] RAX: dead00000000dead RBX: ffffc9001c013e80");
    vstr(0,row++,pc,"[  0.000006] Call Trace:");
    vstr(0,row++,pc,"[  0.000006]   fs_rm+0x4f/0xb2");
    vstr(0,row++,pc,"[  0.000007]   sh_dispatch+0x183/0x5c2");
    vstr(0,row++,pc,"[  0.000007]   shell_run+0x42/0x137");
    vstr(0,row++,pc,"[  0.000008]   kernel_main+0x2e/0x42");
    row++;
    vstr(0,row++,COL(C_YELLOW,C_BLUE),"[  0.000009] Kernel panic - not syncing: VFS critical path deleted");
    vstr(0,row++,COL(C_YELLOW,C_BLUE),"[  0.000009] pigOS cannot recover - system halted.");
    row++;
    vstr(0,row++,COL(C_LRED,C_BLUE),  "[  0.000010] --- SYSTEM HALTED ---");
    vstr(0,row++,COL(C_LGREY,C_BLUE), "[  0.000010] Power off the machine or restart QEMU.");
    vstr(0,row++,pc,reason);
    // Halt forever - no auto-reboot
    vga_update_hw_cursor();
    __asm__ volatile("cli; hlt");
    while(1) __asm__ volatile("hlt");
}

static void fs_rm(const char*cwd,const char*arg){
    if(!arg||!*arg){vpln("usage: rm [-rf] <file>");return;}
    int recursive=0;
    int no_preserve_root=0;
    
    // Copy to temp buffer we can modify
    char tmp[512]; kcp(tmp, arg);
    char*a = tmp;
    char*file_arg = NULL;
    
    // Skip leading spaces
    while(*a==' ')a++;
    
    // Tokenize into words
    char* tokens[32];
    int tc = 0;
    char* start = a;
    while(*start && tc < 32){
        // Skip spaces
        while(*start==' ')start++;
        if(!*start)break;
        tokens[tc++] = start;
        // Find next space
        while(*start && *start!=' ')start++;
        if(*start){*start=0;start++;}
    }
    
    // Parse tokens - find flags and file
    for(int i=0;i<tc;i++){
        char* t = tokens[i];
        if(t[0]=='-'){
            // Flag
            if(t[1]=='-'){
                // Long flag
                if(!ksnc(t+2,"no-preserve-root",16)){
                    no_preserve_root = 1;
                }
            } else {
                // Short flags
                for(int j=1;t[j];j++){
                    if(t[j]=='r')recursive = 1;
                }
            }
        } else {
            // This is the file argument - take the LAST non-flag token
            file_arg = t;
        }
    }
    
    if(!file_arg || !*file_arg){vpln("usage: rm [-rf] <file>");return;}

    char path[256]; fs_resolve(cwd,file_arg,path,256);
    
    // Check for rm / or rm -rf / (root) - require --no-preserve-root
    // Check both the resolved path AND the argument
    int is_root = (path[0]=='/' && path[1]==0);
    if(!is_root){
        // Also check if arg starts with / and has nothing else
        int al = ksl(a);
        if(al>0 && a[0]=='/' && (al==1 || (al>1 && a[1]==' '))) is_root = 1;
    }
    if(is_root && !no_preserve_root){
        vset(C_LRED,C_BLACK);
        vpln("rm: it is dangerous to operate recursively on '/'");
        vpln("use --no-preserve-root to override");
        vrst();
        return;
    }
    
    // Check if file/dir exists first
    const VNode*n=fs_find_vnode(path);
    MemFile*mf=memfs_find(path);
    
    // If doesn't exist
    if(!n && !mf){
        vset(C_LRED,C_BLACK); vps("rm: cannot remove '"); vps(a); vpln("': No such file or directory"); vrst(); return;
    }
    
    // Check if it's a directory - require -r flag
    if((n && n->is_dir) || (mf && mf->is_dir)){
        if(!recursive){
            vset(C_LRED,C_BLACK);vps("rm: cannot remove '");vps(a);vpln("': Is a directory (use rm -r)");
            vrst();return;
        }
    }
    
    // Check if this is a critical system path - causes kernel panic!
    if(is_critical_path(path)){
        NORETURN_KPANIC("[PANIC] Critical system path deleted - VFS integrity lost",path);
    }
    
    // Delete the file/dir
    if(mf){ mf->used=0; vps("rm: removed '"); vps(path); vpln("'"); return; }
    if(n){
        fs_mark_deleted(path);
        vps("rm: removed '"); vps(path); vpln("'");
        return;
    }
}

// ── grep with real search ─────────────────────────────────────
static int grep_icase=0;
static int grep_linenum=0;
static int grep_list=0;
static int grep_found=0;
static int grep_pl=0;
static char grep_pat[256];
static const char* grep_pipe_input=NULL;  // For pipe support: cat file | grep pattern
static int grep_pipe_input_len=0;

// Called from shell when pipe is active
static void grep_set_pipe_input(const char*data){
    grep_pipe_input=data;
    if(data){
        grep_pipe_input_len=0;
        while(data[grep_pipe_input_len]) grep_pipe_input_len++;
    } else {
        grep_pipe_input_len=0;
    }
}

static void grep_line(const char*line,int linelen,int lineno,const char*fname){
    for(int i=0;i<=linelen-grep_pl;i++){
        int ok=1;
        for(int j=0;j<grep_pl&&ok;j++){
            char lc=(char)line[i+j], pc=grep_pat[j];
            if(grep_icase){if(lc>='A'&&lc<='Z')lc+=32;if(pc>='A'&&pc<='Z')pc+=32;}
            if(lc!=pc) ok=0;
        }
        if(ok){
            grep_found++;
            if(grep_list){vps(fname);vpc('\n');return;}
            if(fname&&fname[0]){vps(fname);vps(":");}
            if(grep_linenum){char lb[8];kia(lineno,lb);vset(C_LCYAN,C_BLACK);vps(lb);vps(":");vrst();}
            vset(C_LGREEN,C_BLACK);
            for(int k=0;k<linelen;k++) vpc((uint8_t)line[k]);
            vpc('\n'); vrst();
            return;
        }
    }
}

static void grep_content(const char*content,const char*fname){
    if(!content) return;
    int lineno=0;
    const char*line=content;
    while(*line){
        lineno++;
        const char*eol=line; while(*eol&&*eol!='\n')eol++;
        int linelen=eol-line;
        grep_line(line,linelen,lineno,fname);
        line=*eol?eol+1:eol;
    }
}

static void fs_grep(const char*cwd,const char*args){
    if(!args||!*args){vpln("usage: grep [-r][-i][-l][-n] <pattern> [file]");return;}
    grep_icase=0;grep_linenum=0;grep_list=0;grep_found=0;grep_pl=0;
    const char*p=args;
    while(*p==' ')p++;
    while(*p=='-'){
        p++;
        while(*p&&*p!=' '&&*p!='-'){
            if(*p=='r'){} if(*p=='i')grep_icase=1;
            if(*p=='l')grep_list=1;  if(*p=='n')grep_linenum=1;
            p++;
        }
        while(*p==' ')p++;
    }
    int pi=0;
    while(*p&&*p!=' '&&pi<255) grep_pat[pi++]=*p++;
    grep_pat[pi]=0;
    while(*p==' ')p++;
    char file_arg[256]="";
    if(*p) kcp(file_arg,p);
    grep_pl=ksl(grep_pat);
    if(!grep_pl){vpln("grep: pattern required");return;}

    if(!file_arg[0]){
        // No file arg - check for pipe input first
        if(grep_pipe_input&&grep_pipe_input_len>0){
            grep_content(grep_pipe_input,"(pipe)");
            grep_pipe_input=NULL;
            grep_pipe_input_len=0;
        } else {
            // Search all VFS files
            for(int i=0;VFS[i].path;i++){
                if(VFS[i].is_dir||fs_is_deleted(VFS[i].path)) continue;
                if(VFS[i].content) grep_content(VFS[i].content,VFS[i].path);
            }
            for(int i=0;i<memfs_n;i++){
                if(!memfs[i].used||memfs[i].is_dir) continue;
                grep_content(memfs[i].content,memfs[i].path);
            }
        }
    } else {
        char path[256];
        fs_resolve(cwd,file_arg,path,256);
        const VNode*vn=fs_find_vnode(path);
        if(vn&&vn->is_dir){
            // Recursive grep on directory
            int dl=ksl(path);
            for(int i=0;VFS[i].path;i++){
                if(VFS[i].is_dir||fs_is_deleted(VFS[i].path)) continue;
                int fl=ksl(VFS[i].path);
                if(fl>dl&&VFS[i].path[dl]=='/'){
                    int ok=1;
                    for(int j=0;j<dl;j++){if(VFS[i].path[j]!=path[j]){ok=0;break;}}
                    if(ok&&VFS[i].content) grep_content(VFS[i].content,VFS[i].path+dl);
                }
            }
            for(int i=0;i<memfs_n;i++){
                if(!memfs[i].used||memfs[i].is_dir) continue;
                int fl=ksl(memfs[i].path);
                if(fl>dl&&memfs[i].path[dl]=='/'){
                    int ok=1;
                    for(int j=0;j<dl;j++){if(memfs[i].path[j]!=path[j]){ok=0;break;}}
                    if(ok) grep_content(memfs[i].content,memfs[i].path+dl);
                }
            }
        } else {
            const char*content=NULL;
            MemFile*mf=memfs_find(path);
            if(mf) content=mf->content;
            else if(vn&&!vn->is_dir) content=vn->content;
            if(!content){vset(C_LRED,C_BLACK);vps("grep: ");vps(file_arg);vpln(": No such file");vrst();return;}
            grep_content(content,file_arg);
        }
    }
}

// ── find with -name flag ──────────────────────────────────────
static void fs_find_cmd(const char*cwd,const char*args){
    char startpath[256]=".";
    char namepattern[64]="";
    int type_d=0,type_f=0;
    const char*p=args?args:"";
    while(*p==' ')p++;
    if(*p&&*p!='-'){int i=0;while(*p&&*p!=' '&&i<255){startpath[i++]=*p++;}startpath[i]=0;while(*p==' ')p++;}
    while(*p){
        while(*p==' ')p++;
        if(!*p) break;
        if(*p=='-'){p++;
            if(ksnc(p,"name",4)==0){p+=4;while(*p==' ')p++;int i=0;while(*p&&*p!=' '&&i<63)namepattern[i++]=*p++;namepattern[i]=0;}
            else if(ksnc(p,"type",4)==0){p+=4;while(*p==' ')p++;if(*p=='d'){type_d=1;p++;}else if(*p=='f'){type_f=1;p++;}}
            else while(*p&&*p!=' ')p++;
        } else while(*p&&*p!=' ')p++;
    }
    char base[256]; fs_resolve(cwd,startpath,base,256);
    int blen=ksl(base);
    int npl=ksl(namepattern);
    int found=0;

    for(int i=0;VFS[i].path;i++){
        if(fs_is_deleted(VFS[i].path)) continue;
        const char*vp=VFS[i].path;
        if(blen>1&&ksnc(vp,base,blen)!=0) continue;
        if(type_d&&!VFS[i].is_dir) continue;
        if(type_f&&VFS[i].is_dir) continue;
        if(npl>0){
            const char*sl=vp; for(const char*q=vp;*q;q++) if(*q=='/')sl=q+1; if(*sl=='/')sl++;
            // Wildcard match
            int match=0;
            if(namepattern[0]=='*'&&npl>1){
                const char*suf=namepattern+1; int sl2=ksl(suf); int sll=ksl(sl);
                if(sll>=sl2&&ksnc(sl+sll-sl2,suf,sl2)==0) match=1;
                // also substring
                for(int j=0;sl[j];j++) if(ksnc(sl+j,suf,sl2)==0){match=1;break;}
            } else if(!ksc(sl,namepattern)) match=1;
            else {
                // Simple substring
                int pl2=ksl(namepattern),sll2=ksl(sl);
                for(int j=0;j<=sll2-pl2;j++) if(ksnc(sl+j,namepattern,pl2)==0){match=1;break;}
            }
            if(!match) continue;
        }
        vpln(vp); found++;
    }
    // Also memfs
    for(int i=0;i<memfs_n;i++){
        if(!memfs[i].used) continue;
        const char*mp=memfs[i].path;
        if(blen>1&&ksnc(mp,base,blen)!=0) continue;
        if(type_d) continue;
        const char*sl=mp; for(const char*q=mp;*q;q++) if(*q=='/')sl=q+1; if(*sl=='/')sl++;
        if(npl>0&&ksc(sl,namepattern)!=0) continue;
        vpln(mp); found++;
    }
    if(!found) vpln("[find: no matches]");
}

static void fs_cd(char*cwd,const char*arg){
    if(!arg||!*arg){kcp(cwd,"/");return;}
    if(!ksc(arg,"~")){kcp(cwd,"/");return;}
    char path[256]; fs_resolve(cwd,arg,path,256);
    
    // Try exact match in memfs (dir or file)
    MemFile*m=memfs_find(path);
    if(m&&m->is_dir){kcp(cwd,path);return;}
    
    // Try with trailing slash for dirs
    int l=ksl(path);
    if(l>1&&path[l-1]!='/'){
        path[l]='/';path[l+1]=0;
        m=memfs_find(path);
        if(m&&m->is_dir){kcp(cwd,path);return;}
        path[l]=0;
    }
    
    // Try VFS (check both with and without trailing slash)
    const VNode*n=fs_find_vnode(path);
    if(n&&n->is_dir){kcp(cwd,path);return;}
    if(!n){
        char try2[256];
        kcp(try2,path);
        int tl=ksl(try2);
        if(tl>1&&try2[tl-1]!='/'){try2[tl]='/';try2[tl+1]=0;}
        n=fs_find_vnode(try2);
        if(n&&n->is_dir){kcp(cwd,try2);return;}
    }
    if(!n){
        // Also try absolute path
        if(arg[0]!='/'){
            char abs[256]; abs[0]='/'; kcp(abs+1,arg);
            int al=ksl(abs); if(al>1&&abs[al-1]=='/') abs[al-1]=0;
            n=fs_find_vnode(abs);
            if(n&&n->is_dir){kcp(cwd,abs);return;}
        }
        vset(C_LRED,C_BLACK);vps("cd: ");vps(arg);vpln(": No such file or directory");vrst();return;
    }
    if(!n->is_dir){vset(C_LRED,C_BLACK);vps("cd: ");vps(arg);vpln(": Not a directory");vrst();return;}
    kcp(cwd,path);
}
static void fs_pwd(const char*cwd){vpln(cwd);}

#pragma once
// pigOS Users & Groups - users.h
#include "../drivers/vga/vga.h"
#include "../drivers/ps2/ps2.h"
#include "mem.h"
#include "pigfs.h"
#define MAX_U 16
#define MAX_G 16
typedef struct{char name[32];char pass[64];int uid,gid;char home[64];int active;}User;
typedef struct{char name[32];int gid;int mem[MAX_U];int nm;}Group;
static User users[MAX_U];
static Group groups[MAX_G];
static int nu=0,ng=0;
static int cur_uid=0;
static char cur_user[32]="root";

static void users_init(){
    kcp(users[0].name,"root");kcp(users[0].pass,"toor");users[0].uid=0;users[0].gid=0;kcp(users[0].home,"/");users[0].active=1;nu=1;
    kcp(groups[0].name,"root");groups[0].gid=0;groups[0].nm=1;groups[0].mem[0]=0;ng=1;
    kcp(groups[1].name,"wheel");groups[1].gid=1;groups[1].nm=1;groups[1].mem[0]=0;ng=2;
    kcp(groups[2].name,"users");groups[2].gid=2;groups[2].nm=0;ng=3;
    
    extern MemFile* memfs_create(const char*);
    
    // /home/ and /etc/ already in VFS - only create what's NOT in VFS
    // Create /etc/skel/ (not in VFS)
    MemFile* skel = memfs_create("/etc/skel/");
    if(skel){skel->is_dir=1;kcp(skel->content,"[dir]");}
    
    MemFile* skel_lashrc = memfs_create("/etc/skel/.lashrc");
    if(skel_lashrc){
        kcp(skel_lashrc->content, "# .lashrc - larpshell config\nalias ll='ls -la'\nalias cls='clear'\n");
    }
    
    MemFile* skel_profile = memfs_create("/etc/skel/.lashrc-profile");
    if(skel_profile){
        kcp(skel_profile->content, "# .lashrc-profile\n[ -f ~/.lashrc ] && . ~/.lashrc\n");
    }
}
static int ufind(const char*n){for(int i=0;i<nu;i++)if(users[i].active&&!ksc(users[i].name,n))return i;return -1;}
static int gfind(const char*n){for(int i=0;i<ng;i++)if(!ksc(groups[i].name,n))return i;return -1;}

static void cmd_adduser(const char*n){
    if(!n||!*n){vpln("usage: adduser <name>");return;}
    if(ufind(n)>=0){vpln("adduser: user exists");return;}
    if(nu>=MAX_U){vpln("adduser: too many users");return;}
    
    extern MemFile* memfs_create(const char*);
    extern MemFile* memfs_find(const char*);
    
    kcp(users[nu].name,n);
    kcp(users[nu].pass,"changeme");
    users[nu].uid=1000+nu;
    users[nu].gid=2;
    users[nu].active=1;
    kcp(users[nu].home,"/home/");
    kcat(users[nu].home,n);
    kcat(users[nu].home,"/");
    nu++;
    
    int gi=gfind("users");
    if(gi>=0&&groups[gi].nm<MAX_U)groups[gi].mem[groups[gi].nm++]=nu-1;
    
    char home_path[96];
    kcp(home_path, users[nu-1].home);
    MemFile* hm = memfs_create(home_path);
    if(hm){hm->is_dir=1;kcp(hm->content,"[dir]");}
    
    // Copy .lashrc from /etc/skel/
    MemFile* skel_lashrc = memfs_find("/etc/skel/.lashrc");
    if(skel_lashrc){
        char lpath[128];
        kcp(lpath, home_path);
        kcat(lpath, ".lashrc");
        MemFile* lashrc = memfs_create(lpath);
        if(lashrc){
            kcp(lashrc->content, skel_lashrc->content);
        }
    }
    
    // Copy .lashrc-profile from /etc/skel/
    MemFile* skel_profile = memfs_find("/etc/skel/.lashrc-profile");
    if(skel_profile){
        char profpath[128];
        kcp(profpath, home_path);
        kcat(profpath, ".lashrc-profile");
        MemFile* prof = memfs_create(profpath);
        if(prof){
            kcp(prof->content, skel_profile->content);
        }
    }
    
    vps("adduser: created '");vps(n);vpln("'");
    vps("home: ");vpln(home_path);
    
    extern int memfs_n;
    vps("total memfs entries: "); char mn[8]; kia(memfs_n,mn); vpln(mn);
    
    char testpath[128];
    kcp(testpath, home_path); kcat(testpath, ".lashrc");
    MemFile* tf = memfs_find(testpath);
    if(tf){ vpln(".lashrc: OK (copied from /etc/skel/)"); } else { vpln(".lashrc: FAILED"); }
    
    vps("files: .lashrc .lashrc-profile copied from /etc/skel/\n");
}

static void cmd_passwd(const char*n){
    const char*u=n&&*n?n:cur_user;
    int i=ufind(u);
    if(i<0){vpln("passwd: user not found");return;}
    vps("Changing password for ");vpln(u);
    vps("New password: "); vrst();
    char np[64]; int ni=0;
    while(1){
        int c=kb_get();
        if(c=='\n')break;
        if(c=='\b'&&ni>0){ni--;continue;}
        if(c>=32&&c<127&&ni<63)np[ni++]=(char)c;
    }
    np[ni]=0; vpc('\n');
    if(ni<1){vpln("passwd: password unchanged");return;}
    kcp(users[i].pass,np);
    vpln("passwd: password updated successfully");
}

static void cmd_usermod(const char*a){
    if(!a||!*a){vpln("usage: usermod -G <group> <user>");return;}
    const char*p=a;
    if(p[0]=='-'&&p[1]=='G'){p+=2;while(*p==' ')p++;char g[32];int gi=0;while(*p&&*p!=' '&&gi<31)g[gi++]=*p++;g[gi]=0;while(*p==' ')p++;int ui=ufind(p),gidx=gfind(g);if(ui<0){vpln("usermod: user not found");return;}if(gidx<0){vpln("usermod: group not found");return;}groups[gidx].mem[groups[gidx].nm++]=ui;vps("usermod: added '");vps(p);vps("' to '");vps(g);vpln("'");}
    else vpln("usermod: use -G flag");
}

static void cmd_groupadd(const char*n){
    if(!n||!*n){vpln("usage: groupadd <name>");return;}
    if(ng>=MAX_G){vpln("groupadd: too many groups");return;}
    kcp(groups[ng].name,n);groups[ng].gid=200+ng;groups[ng].nm=0;ng++;
    vps("groupadd: created '");vps(n);vpln("'");
}

static void cmd_groups(const char*n){
    const char*u=n&&*n?n:cur_user;int ui=ufind(u);if(ui<0){vpln("groups: not found");return;}
    vps(u);vps(" : ");for(int i=0;i<ng;i++){for(int j=0;j<groups[i].nm;j++){if(groups[i].mem[j]==ui){vps(groups[i].name);vpc(' ');break;}}}vpc('\n');
}

static void cmd_id(){
    vset(C_LCYAN,C_BLACK);char b[8];kia(cur_uid,b);vps("uid=");vps(b);vps("(");vps(cur_user);vps(") gid=0(root) groups=0(root),1(wheel)\n");vrst();
}

#pragma once
// ================================================================
// POING - pigOS Init System v1.0
// ================================================================
#include "../../drivers/vga/vga.h"
#include "../../kernel/mem.h"

#define POING_MAX 32
typedef enum { US_STOPPED=0, US_STARTING, US_RUNNING, US_FAILED } UState;
typedef struct {
    char name[48]; char desc[64];
    UState state; int enabled; int pid;
} PoingUnit;

static PoingUnit pu[POING_MAX];
static int pnu=0;
static int prl=5; // runlevel

static void poing_add(const char*n,const char*d,int en,UState st){
    if(pnu>=POING_MAX)return;
    kcp(pu[pnu].name,n); kcp(pu[pnu].desc,d);
    pu[pnu].enabled=en; pu[pnu].state=st; pu[pnu].pid=100+pnu; pnu++;
}

static void poing_init(){
    poing_add("poing-early.unit",   "Early kernel setup",           1,US_RUNNING);
    poing_add("poing-udev.unit",    "Device manager (udev-pig)",    1,US_RUNNING);
    poing_add("poing-vga.unit",     "VGA console driver",           1,US_RUNNING);
    poing_add("poing-kbd.unit",     "PS/2 keyboard",                1,US_RUNNING);
    poing_add("poing-mouse.unit",   "PS/2 mouse",                   1,US_RUNNING);
    poing_add("poing-mem.unit",     "Memory allocator",             1,US_RUNNING);
    poing_add("poing-net.unit",     "pig-net network manager",      1,US_RUNNING);
    poing_add("poing-fs.unit",      "pigfs virtual filesystem",     1,US_RUNNING);
    poing_add("poing-crypto.unit",  "pigssl crypto subsystem",      1,US_RUNNING);
    poing_add("poing-wm.unit",      "PigWM window manager",         1,US_RUNNING);
    poing_add("poing-shell.unit",   "larpshell on tty0",            1,US_RUNNING);
    poing_add("poing-log.unit",     "piglog system logger",         1,US_RUNNING);
    poing_add("poing-cron.unit",    "pig-cron task scheduler",      1,US_RUNNING);
    poing_add("sshd.unit",          "SSH daemon",                   0,US_STOPPED);
    poing_add("httpd.unit",         "pig-httpd web server",         0,US_STOPPED);
    poing_add("bluetooth.unit",     "Bluetooth manager",            0,US_STOPPED);
}

static void poingctl_status(){
    vset(C_LCYAN,C_BLACK);
    vpln("  \x01 POING - pigOS Init System v1.0");
    vrst();
    char rb[4]; kia(prl,rb);
    vps("  Runlevel: "); vpln(rb);
    vpln("");
    for(int i=0;i<pnu;i++){
        PoingUnit*u=&pu[i];
        char pb[8]; kia(u->pid,pb);
        if(u->state==US_RUNNING){
            vset(C_LGREEN,C_BLACK); vps("  \x10 ");
        } else if(u->state==US_FAILED){
            vset(C_LRED,C_BLACK); vps("  \xFB ");
        } else {
            vset(C_DGREY,C_BLACK); vps("  o  ");
        }
        vset(C_WHITE,C_BLACK);
        vps(u->name); vps(" (pid="); vps(pb); vps(") - "); vpln(u->desc);
    }
}

static void poingctl_do(const char*sub,const char*n){
    if(!ksc(sub,"status")||!ksc(sub,"list")){ poingctl_status(); return; }
    if(!ksc(sub,"reboot")){
        vpln("poing: rebooting..."); for(volatile int i=0;i<3000000;i++); outb(0x64,0xFE); return;
    }
    if(!ksc(sub,"halt")||!ksc(sub,"shutdown")||!ksc(sub,"poweroff")){
        vclear(); vset(C_LRED,C_BLACK);
        vpln("\n  poing: system shutdown");
        vpln("  stopping all units...");
        vpln("  it is now safe to power off.");
        vset(C_WHITE,C_BLACK);
        for(volatile int i=0;i<200000000;i++);
        outb(0xF4,0x00); __asm__("cli;hlt"); return;
    }
    if(!ksc(sub,"sleep")){
        vset(C_LCYAN,C_BLACK); vpln("poing: going to sleep (press any key)"); vrst();
        kb_get(); return;
    }
    if(!n||!*n){ vpln("poingctl: unit name required"); return; }
    for(int i=0;i<pnu;i++){
        if(!ksc(pu[i].name,n)){
            if(!ksc(sub,"start")){pu[i].state=US_RUNNING;pu[i].enabled=1;vps("poing: started ");vpln(n);}
            else if(!ksc(sub,"stop")){pu[i].state=US_STOPPED;pu[i].enabled=0;vps("poing: stopped ");vpln(n);}
            else if(!ksc(sub,"enable")){pu[i].enabled=1;vps("poing: enabled ");vpln(n);}
            else if(!ksc(sub,"disable")){pu[i].enabled=0;vps("poing: disabled ");vpln(n);}
            else if(!ksc(sub,"restart")){pu[i].state=US_STOPPED;pu[i].state=US_RUNNING;vps("poing: restarted ");vpln(n);}
            else if(!ksc(sub,"reload")){vps("poing: reloaded ");vpln(n);}
            else {vps("poingctl: unknown: ");vpln(sub);}
            return;
        }
    }
    vps("poingctl: unit not found: "); vpln(n);
}

static void cmd_poingctl(const char*args){
    if(!args||!*args){ vpln("usage: poingctl <status|start|stop|enable|disable|restart|reload|reboot|halt|sleep> [unit]"); return; }
    char sub[32]; int si=0;
    const char*p=args;
    while(*p&&*p!=' '&&si<31) sub[si++]=*p++;
    sub[si]=0;
    while(*p==' ')p++;
    poingctl_do(sub,p);
}

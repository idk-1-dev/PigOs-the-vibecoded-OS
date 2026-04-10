#pragma once
// ============================================================
// POING - pigOS Init System
// Like systemd but not systemd. Like runit but not runit.
// ============================================================
#include "../drivers/vga.h"
#include "../kernel/mem.h"

#define POING_MAX_UNITS 32
#define POING_MAX_NAME  48

typedef enum { UNIT_STOPPED=0, UNIT_STARTING, UNIT_RUNNING, UNIT_FAILED } UnitState;

typedef struct {
    char name[POING_MAX_NAME];
    char desc[64];
    UnitState state;
    int enabled;
    int pid;
} PoingUnit;

static PoingUnit punits[POING_MAX_UNITS];
static int pnu=0;
static int poing_runlevel=3;

static void poing_add(const char*n,const char*desc,int enabled,UnitState st){
    if(pnu>=POING_MAX_UNITS)return;
    kcp(punits[pnu].name,n); kcp(punits[pnu].desc,desc);
    punits[pnu].enabled=enabled; punits[pnu].state=st; punits[pnu].pid=pnu+100;
    pnu++;
}

static void poing_init(){
    poing_add("poing-early.unit",   "Early kernel environment setup",1,UNIT_RUNNING);
    poing_add("poing-udev.unit",    "Device manager (udev-pig)",     1,UNIT_RUNNING);
    poing_add("poing-vga.unit",     "VGA text console driver",       1,UNIT_RUNNING);
    poing_add("poing-kbd.unit",     "PS/2 keyboard driver",          1,UNIT_RUNNING);
    poing_add("poing-mem.unit",     "Memory allocator service",      1,UNIT_RUNNING);
    poing_add("poing-net.unit",     "Network manager (pig-net)",     1,UNIT_RUNNING);
    poing_add("poing-fs.unit",      "Virtual filesystem (pigfs)",    1,UNIT_RUNNING);
    poing_add("poing-crypto.unit",  "Crypto subsystem (pigssl)",     1,UNIT_RUNNING);
    poing_add("poing-wm.unit",      "Window manager (PigWM)",        1,UNIT_RUNNING);
    poing_add("poing-shell.unit",   "pigOS shell (larpshell)",       1,UNIT_RUNNING);
    poing_add("poing-login.unit",   "Login manager",                 1,UNIT_RUNNING);
    poing_add("poing-cron.unit",    "Task scheduler",                1,UNIT_RUNNING);
    poing_add("poing-log.unit",     "System logger (piglog)",        1,UNIT_RUNNING);
    poing_add("netpoug.unit",       "Network health monitor (netpoug)", 0,UNIT_STOPPED);
    poing_add("sshd.unit",          "SSH daemon (no network yet)",   0,UNIT_STOPPED);
    poing_add("httpd.unit",         "HTTP server (pig-httpd)",       0,UNIT_STOPPED);
    poing_add("bluetooth.unit",     "Bluetooth manager",             0,UNIT_STOPPED);
}

static void poingctl_status(){
    extern int net_ok;
    extern volatile uint32_t rtl_rx_count,rtl_tx_count;
    extern int rtl_hw_ok;
    vset(C_LCYAN,C_BLACK); vpln("  POING - pigOS Init System"); vset(C_WHITE,C_BLACK);
    char rb[4]; kia(poing_runlevel,rb);
    vps("  Runlevel: "); vpln(rb);
    vpln("  "); 
    for(int i=0;i<pnu;i++){
        PoingUnit*u=&punits[i];
        // Special handling for netpoug - check real network status
        if(!ksc(u->name,"netpoug.unit")){
            if(rtl_hw_ok&&net_ok){
                u->state=UNIT_RUNNING; u->enabled=1;
            } else if(rtl_hw_ok){
                u->state=UNIT_STOPPED; u->enabled=0;
            } else {
                u->state=UNIT_FAILED; u->enabled=0;
            }
        }
        // Special handling for poing-net - check real network status
        if(!ksc(u->name,"poing-net.unit")){
            if(rtl_hw_ok&&net_ok){
                u->state=UNIT_RUNNING;
            } else if(rtl_hw_ok){
                u->state=UNIT_STARTING;
            } else {
                u->state=UNIT_FAILED;
            }
        }
        if(u->state==UNIT_RUNNING){vset(C_LGREEN,C_BLACK);vps("  \x10 ");}
        else if(u->state==UNIT_FAILED){vset(C_LRED,C_BLACK);vps("  \xFB ");}
        else{vset(C_DGREY,C_BLACK);vps("  o ");}
        vset(C_WHITE,C_BLACK);
        char pb[8]; kia(u->pid,pb);
        vps(u->name); vps(" (pid="); vps(pb); vps(") - "); vpln(u->desc);
        // Show extra info for netpoug
        if(!ksc(u->name,"netpoug.unit")){
            vps("    HW: ");
            if(rtl_hw_ok){vset(C_LGREEN,C_BLACK);vpln("RTL8139 detected");}
            else{vset(C_LRED,C_BLACK);vpln("NO RTL8139");}
            vps("    Link: ");
            if(net_ok){vset(C_LGREEN,C_BLACK);vpln("UP - 10.0.2.15");}
            else{vset(C_LRED,C_BLACK);vpln("DOWN - run: ip link set eth0 up");}
            vset(C_WHITE,C_BLACK);
            char rxb[16],txb[16];kia(rtl_rx_count,rxb);kia(rtl_tx_count,txb);
            vps("    RX: ");vps(rxb);vps(" packets  TX: ");vps(txb);vpln(" packets");
            vrst();
        }
    }
}

static void poingctl_start(const char*n){
    for(int i=0;i<pnu;i++){
        if(!ksc(punits[i].name,n)){
            punits[i].state=UNIT_RUNNING; punits[i].enabled=1;
            vps("poing: started "); vpln(n); return;
        }
    }
    vps("poing: unit not found: "); vpln(n);
}

static void poingctl_stop(const char*n){
    for(int i=0;i<pnu;i++){
        if(!ksc(punits[i].name,n)){
            punits[i].state=UNIT_STOPPED; punits[i].enabled=0;
            vps("poing: stopped "); vpln(n); return;
        }
    }
    vps("poing: unit not found: "); vpln(n);
}

static void poingctl_enable(const char*n){
    for(int i=0;i<pnu;i++){
        if(!ksc(punits[i].name,n)){
            punits[i].enabled=1;
            vps("poing: enabled "); vpln(n); return;
        }
    }
    vps("poing: unit not found: "); vpln(n);
}

static void poingctl_disable(const char*n){
    for(int i=0;i<pnu;i++){
        if(!ksc(punits[i].name,n)){
            punits[i].enabled=0;
            vps("poing: disabled "); vpln(n); return;
        }
    }
    vps("poing: unit not found: "); vpln(n);
}

static void cmd_poingctl(const char*args){
    if(!args||!*args){ vpln("usage: poingctl <status|start|stop|enable|disable|list|reboot|halt|sleep> [unit]"); return; }
    char sub[32]; int si=0;
    const char*p=args;
    while(*p&&*p!=' '&&si<31) sub[si++]=*p++;
    sub[si]=0;
    while(*p==' ')p++;

    if(!ksc(sub,"status")||!ksc(sub,"list")){
        // If unit name provided, filter to just that unit
        if(p&&*p){
            extern int net_ok;
            extern volatile uint32_t rtl_rx_count,rtl_tx_count;
            extern int rtl_hw_ok;
            int found=0;
            int pl=0; while(p[pl]) pl++;
            for(int i=0;i<pnu;i++){
                PoingUnit*u=&punits[i];
                // Match partial names: "netpoug" matches "netpoug.unit"
                int nl=0; while(u->name[nl]) nl++;
                int match=0;
                if(nl>=pl){
                    match=1;
                    for(int j=0;j<pl;j++){
                        char a=u->name[j],b=p[j];
                        if(a>='A'&&a<='Z')a+=32;
                        if(b>='A'&&b<='Z')b+=32;
                        if(a!=b){match=0;break;}
                    }
                }
                if(match){
                    found=1;
                    // Update live status for network units
                    if(!ksc(u->name,"netpoug.unit")){
                        u->state=(rtl_hw_ok&&net_ok)?UNIT_RUNNING:(rtl_hw_ok?UNIT_STOPPED:UNIT_FAILED);
                        u->enabled=(rtl_hw_ok&&net_ok)?1:0;
                    }
                    if(!ksc(u->name,"poing-net.unit")){
                        u->state=(rtl_hw_ok&&net_ok)?UNIT_RUNNING:(rtl_hw_ok?UNIT_STARTING:UNIT_FAILED);
                    }
                    vset(C_LGREEN,C_BLACK);
                    if(u->state==UNIT_RUNNING) vps("  * ");
                    else if(u->state==UNIT_FAILED){vset(C_LRED,C_BLACK);vps("  X ");}
                    else{vset(C_DGREY,C_BLACK);vps("  o ");}
                    vset(C_WHITE,C_BLACK);
                    char pb[8];kia(u->pid,pb);
                    vps(u->name);vps(" (pid=");vps(pb);vps(") - ");vpln(u->desc);
                    // Show network details for netpoug
                    if(!ksc(u->name,"netpoug.unit")){
                        vps("    HW: ");
                        if(rtl_hw_ok){vset(C_LGREEN,C_BLACK);vpln("RTL8139 detected");}
                        else{vset(C_LRED,C_BLACK);vpln("NO RTL8139");}
                        vps("    Link: ");
                        if(net_ok){vset(C_LGREEN,C_BLACK);vpln("UP - 10.0.2.15");}
                        else{vset(C_LRED,C_BLACK);vpln("DOWN - run: dhcpcd eth0");}
                        vset(C_WHITE,C_BLACK);
                        char rxb[16],txb[16];kia(rtl_rx_count,rxb);kia(rtl_tx_count,txb);
                        vps("    RX: ");vps(rxb);vps(" packets  TX: ");vps(txb);vpln(" packets");
                        vrst();
                    }
                    break;
                }
            }
            if(!found){vps("poingctl: unit not found: ");vpln(p);}
        } else {
            poingctl_status();
        }
    }
    else if(!ksc(sub,"start"))   poingctl_start(p);
    else if(!ksc(sub,"stop"))    poingctl_stop(p);
    else if(!ksc(sub,"enable"))  poingctl_enable(p);
    else if(!ksc(sub,"disable")) poingctl_disable(p);
    else if(!ksc(sub,"restart")){ poingctl_stop(p); poingctl_start(p); }
    else if(!ksc(sub,"reload"))  { vps("poing: reloaded "); vpln(p); }
    else if(!ksc(sub,"reboot"))  { vpln("poing: rebooting..."); for(volatile int i=0;i<3000000;i++); outb(0x64,0xFE); }
    else if(!ksc(sub,"halt"))    { vpln("poing: halting..."); __asm__("cli;hlt"); }
    else { vps("poingctl: unknown command: "); vpln(sub); }
}

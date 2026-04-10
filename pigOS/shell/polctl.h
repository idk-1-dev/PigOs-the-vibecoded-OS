#pragma once
// polctl - pigOS system log viewer (like journalctl)
// Flags: -f (follow), -n <N> (last N), -p <priority>, -u <unit>, --since, --list-boots
#include "../drivers/vga/vga.h"
#include "../drivers/kbd/keyboard.h"
#include "../kernel/mem.h"

#define POLLOG_MAX 128
typedef struct {
    int priority; // 0=emergency 3=error 4=warning 6=info 7=debug
    const char* unit;
    const char* msg;
} PolLog;

static PolLog pollog[POLLOG_MAX];
static int pollog_n=0;

static void pol_add(int pri, const char*unit, const char*msg){
    if(pollog_n>=POLLOG_MAX) return;
    pollog[pollog_n].priority=pri;
    pollog[pollog_n].unit=unit;
    pollog[pollog_n].msg=msg;
    pollog_n++;
}

static void pollog_init(){
    pol_add(6,"poing-early.unit",  "[  0.000] poing: kernel environment setup complete");
    pol_add(6,"poing-udev.unit",   "[  0.001] udev-pig: device manager started");
    pol_add(6,"poing-vga.unit",    "[  0.002] vga: 80x25 CP437 text mode @ 0xB8000");
    pol_add(6,"poing-kbd.unit",    "[  0.003] kbd: PS/2 keyboard initialized");
    pol_add(6,"poing-mouse.unit",  "[  0.004] mouse: PS/2 mouse driver loaded");
    pol_add(6,"poing-mem.unit",    "[  0.005] mem: 4MB heap @ 0x400000");
    pol_add(6,"poing-net.unit",    "[  0.006] net: scanning PCI bus...");
    pol_add(6,"poing-net.unit",    "[  0.007] net: RTL8139 found at 00:03.0 [10EC:8139]");
    pol_add(4,"poing-net.unit",    "[  0.008] net: RTL8139 no carrier (no cable/bridge)");
    pol_add(6,"poing-fs.unit",     "[  0.009] pigfs: mounted at /");
    pol_add(6,"poing-crypto.unit", "[  0.010] pigssl: crypto subsystem ready");
    pol_add(6,"poing-wm.unit",     "[  0.011] PigWM: compositor initialized");
    pol_add(6,"poing-shell.unit",  "[  0.012] larpshell v5.9.3: tty0 ready");
    pol_add(6,"poing-log.unit",    "[  0.013] piglog: system logger started");
    pol_add(6,"poing-cron.unit",   "[  0.014] pig-cron: task scheduler ready");
    pol_add(4,"sshd.unit",         "[  0.015] sshd: disabled (no network stack)");
    pol_add(4,"httpd.unit",        "[  0.016] httpd: disabled (no TCP/IP)");
    pol_add(6,"kernel",            "[  0.017] pigOS v1.0 boot complete");
}

static uint8_t pol_pri_color(int pri){
    if(pri<=3) return COL(C_LRED,C_BLACK);
    if(pri==4) return COL(C_YELLOW,C_BLACK);
    if(pri==6) return COL(C_LGREEN,C_BLACK);
    return COL(C_LGREY,C_BLACK);
}

static const char* pol_pri_name(int pri){
    if(pri==0)return "EMERG";if(pri==1)return "ALERT";if(pri==2)return "CRIT ";
    if(pri==3)return "ERROR";if(pri==4)return "WARN ";if(pri==5)return "NOTICE";
    if(pri==6)return "INFO ";return "DEBUG";
}

static void polctl_print_entry(int i){
    uint8_t c=pol_pri_color(pollog[i].priority);
    vset(C_DGREY,C_BLACK); vps("-- polOS log -- ");
    vat((uint8_t)'[',c,vx,vy);vx++;
    vset(C_WHITE,C_BLACK); vps(pol_pri_name(pollog[i].priority));
    vat((uint8_t)']',c,vx,vy);vx++;
    vpc(' ');
    vset(C_LCYAN,C_BLACK); vps(pollog[i].unit);
    vpc(' ');
    vset(C_WHITE,C_BLACK); vpln(pollog[i].msg);
}

static void cmd_polctl(const char*args){
    if(pollog_n==0) pollog_init();

    // Default: show all
    int filter_pri=-1, last_n=-1;
    const char*filter_unit=NULL;
    int follow=0, list_boots=0;

    // Parse flags
    const char*p=args;
    while(*p){
        while(*p==' ')p++;
        if(!p[0]) break;
        if(p[0]=='-'){
            if(p[1]=='-'){
                if(ksnc(p+2,"since",5)==0){
                    p+=7; while(*p&&*p!=' ')p++;
                } else if(ksnc(p+2,"list-boots",10)==0){ list_boots=1; p+=12; }
                else if(ksnc(p+2,"help",4)==0){ goto show_help; }
                else p+=2;
            } else if(p[1]=='f'){ follow=1; p+=2; }
            else if(p[1]=='n'){ p+=2;while(*p==' ')p++;last_n=0;while(*p>='0'&&*p<='9'){last_n=last_n*10+(*p-'0');p++;} }
            else if(p[1]=='p'){ p+=2;while(*p==' ')p++;filter_pri=*p-'0';while(*p&&*p!=' ')p++; }
            else if(p[1]=='u'){ p+=2;while(*p==' ')p++;filter_unit=p;while(*p&&*p!=' ')p++; }
            else { p++; while(*p&&*p!=' ')p++; }
        } else break;
    }

    if(list_boots){
        vset(C_LCYAN,C_BLACK); vpln("IDX BOOT_ID              FIRST ENTRY          LAST ENTRY");
        vrst(); vpln(" -1 [current boot]       1970-01-01 00:00     1970-01-01 00:00");
        return;
    }

    // Show header
    vset(C_DGREY,C_BLACK);
    vpln("-- polOS System Log (polctl) -- pigOS poing init system --");
    vrst();

    int start=0;
    if(last_n>0&&last_n<pollog_n) start=pollog_n-last_n;

    for(int i=start;i<pollog_n;i++){
        if(filter_pri>=0&&pollog[i].priority!=filter_pri) continue;
        if(filter_unit&&ksnc(pollog[i].unit,filter_unit,ksl(filter_unit))!=0) continue;
        polctl_print_entry(i);
    }

    if(follow){
        vset(C_DGREY,C_BLACK); vpln("-- polctl: following (press q to stop) --"); vrst();
        while(1){ if(kb_avail()){int c=kb_get();if(c=='q'||c==27)break;} }
    }
    return;

show_help:
    vset(C_LCYAN,C_BLACK); vpln("polctl - pigOS system log viewer"); vrst();
    vpln("Usage: polctl [OPTIONS]");
    vpln("  (no flags)     - show all log entries");
    vpln("  -f             - follow log (press q to stop)");
    vpln("  -n <N>         - show last N entries");
    vpln("  -p <0-7>       - filter by priority (3=error 4=warn 6=info)");
    vpln("  -u <unit>      - filter by unit name");
    vpln("  --since <time> - show entries since time");
    vpln("  --list-boots   - list boot sessions");
    vpln("  --help         - show this help");
    vpln("Priority: 0=emerg 1=alert 2=crit 3=error 4=warn 5=notice 6=info 7=debug");
}

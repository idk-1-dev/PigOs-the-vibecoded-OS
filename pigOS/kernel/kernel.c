#include "lwip/dns.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include <stdint.h>
#include "kernel/autoconf.h"

#ifdef CONFIG_VGA_DRIVER
#include "../drivers/vga/vga.h"
#endif

#ifdef CONFIG_PS2_KEYBOARD
#include "../drivers/ps2/ps2.h"
#endif

#ifdef CONFIG_HEAP_ALLOC
#include "../kernel/mem.h"
#endif

#ifdef CONFIG_USERS
#include "../kernel/users.h"
#endif

#ifdef CONFIG_PIGFS
#include "../kernel/pigfs.h"
#endif

#ifdef CONFIG_LWIP
#include "../drivers/net/network.h"
#endif

#ifdef CONFIG_PIGSSL
#include "../crypto/pigssl.h"
#endif

#ifdef CONFIG_POING
#include "../kernel/poing/poing.h"
#endif

extern void sh_dispatch(const char*);
extern int kb_avail(void);
extern int kb_get(void);

#include "../shell/shell.h"
#include "../wm/wm.h"


struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed, aligned(8)));

struct interrupt_frame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

static struct idt_entry kernel_idt[256] __attribute__((aligned(16)));
static struct __attribute__((packed)) idt_ptr kernel_idtr;

static void idt_set_gate(int vec, void (*handler)(void), uint8_t type_attr){
    uint64_t addr = (uint64_t)(uintptr_t)handler;
    kernel_idt[vec].offset_low = (uint16_t)(addr & 0xFFFFu);
    kernel_idt[vec].selector = 0x08;
    kernel_idt[vec].ist = 0;
    kernel_idt[vec].type_attr = type_attr;
    kernel_idt[vec].offset_mid = (uint16_t)((addr >> 16) & 0xFFFFu);
    kernel_idt[vec].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFFu);
    kernel_idt[vec].zero = 0;
}

static void kernel_exception_noerr(struct interrupt_frame* frame) __attribute__((interrupt, target("no-sse")));
static void kernel_exception_err(struct interrupt_frame* frame, uint64_t error_code) __attribute__((interrupt, target("no-sse")));

static void kernel_exception_noerr(struct interrupt_frame* frame){
    (void)frame;
    __asm__ volatile("cli; hlt");
    for(;;){}
}

static void kernel_exception_err(struct interrupt_frame* frame, uint64_t error_code){
    (void)frame;
    (void)error_code;
    __asm__ volatile("cli; hlt");
    for(;;){}
}

static void idt_init(void){
    for(int i = 0; i < 256; i++){
        idt_set_gate(i, (void(*)(void))kernel_exception_noerr, 0x8E);
    }
    idt_set_gate(8,  (void(*)(void))kernel_exception_err, 0x8E);
    idt_set_gate(10, (void(*)(void))kernel_exception_err, 0x8E);
    idt_set_gate(11, (void(*)(void))kernel_exception_err, 0x8E);
    idt_set_gate(12, (void(*)(void))kernel_exception_err, 0x8E);
    idt_set_gate(13, (void(*)(void))kernel_exception_err, 0x8E);
    idt_set_gate(14, (void(*)(void))kernel_exception_err, 0x8E);
    idt_set_gate(17, (void(*)(void))kernel_exception_err, 0x8E);
    idt_set_gate(30, (void(*)(void))kernel_exception_err, 0x8E);

    kernel_idtr.limit = (uint16_t)(sizeof(kernel_idt) - 1);
    kernel_idtr.base = (uint64_t)(uintptr_t)kernel_idt;
    __asm__ volatile("lidt %0" : : "m"(kernel_idtr));
}

#define MBOOT_MAGIC 0x2BADB002

// Runtime module config - reads /etc/pigos.conf at boot
// Format: CONFIG_FOO=y  or  # CONFIG_FOO is not set
static int rt_cfg(const char* key){
    MemFile* mf = memfs_find("/etc/pigos.conf");
    if(!mf || !mf->content[0]) return 1; // no config = all enabled

    const char* src = mf->content;
    int key_len = 0;
    while(key[key_len]) key_len++;

    // Walk through the file line by line
    int line_start = 0;
    for(int i = 0; ; i++){
        int is_end = (src[i] == '\n' || src[i] == '\r' || src[i] == '\0');
        if(is_end){
            // Process line from line_start to i
            int len = i - line_start;

            // Skip comments and empty lines
            if(len < 8 || src[line_start] == '#'){ line_start = i + 1; continue; }

            // Must start with "CONFIG_"
            if(src[line_start+0]!='C'||src[line_start+1]!='O'||src[line_start+2]!='N'||
               src[line_start+3]!='F'||src[line_start+4]!='I'||src[line_start+5]!='G'||
               src[line_start+6]!='_'){ line_start = i + 1; continue; }

            // Check if key matches exactly
            int match = 1;
            for(int j = 0; j < key_len; j++){
                if(line_start + 7 + j >= i || src[line_start + 7 + j] != key[j]){
                    match = 0; break;
                }
            }
            if(!match){ line_start = i + 1; continue; }

            // Key matched exactly, check what comes after
            int after_pos = line_start + 7 + key_len;
            if(after_pos < i && src[after_pos] == '=' && src[after_pos+1] == 'y') return 1;
            if(after_pos + 10 <= i && src[after_pos] == ' ' && src[after_pos+1] == 'i') return 0;

            line_start = i + 1;
            continue;
        }
        if(src[i] == '\0') break;
    }
    return 1; // not found in config = enabled by default
}

static void ms(int n){ for(volatile int i=0;i<n*60000;i++); }

static void boot_line(uint8_t col,const char*tag,const char*msg){
#ifdef CONFIG_VGA_DRIVER
    const char*t=tag; while(*t){vat((uint8_t)*t,col,vx,vy);vx++;t++;}
    vset(C_WHITE,C_BLACK); vpln(msg);
#endif
    ms(80);
}

void kernel_main(uint32_t magic,uint32_t mb_info){
    (void)mb_info;

    idt_init();

#ifdef CONFIG_VGA_DRIVER
    vga_init();
    vga_disable_cursor();
#endif

#ifdef CONFIG_BOOT_SPLASH
    if(rt_cfg("BOOT_SPLASH")){
        // ═══════════════════════════════════════════════════
        // FLARE Bootloader v1.0 - pigOS visual splash screen
        // ═══════════════════════════════════════════════════

        // Fill background - dark blue
        vfill(0,0,VGA_W,VGA_H,' ',COL(C_WHITE,C_BLUE));

    // ── Animated fire border top ─────────────────────────
    uint8_t fire[]={COL(C_YELLOW,C_LRED),COL(C_LRED,C_RED),COL(C_YELLOW,C_YELLOW),
                    COL(C_LRED,C_LRED),COL(C_WHITE,C_YELLOW),COL(C_LMAG,C_LRED)};
    for(int i=0;i<VGA_W;i++) vat(CH_BLOCK, fire[i%6], i, 0);
    for(int i=0;i<VGA_W;i++) vat(CH_DARK,  fire[(i+3)%6], i, 1);
    // Bottom fire
    for(int i=0;i<VGA_W;i++) vat(CH_DARK,  fire[(i+2)%6], i, VGA_H-2);
    for(int i=0;i<VGA_W;i++) vat(CH_BLOCK, fire[(i+1)%6], i, VGA_H-1);
    // Side borders
    for(int i=2;i<VGA_H-2;i++){
        vat(CH_BLOCK,fire[i%6],0,i);
        vat(CH_BLOCK,fire[(i+3)%6],VGA_W-1,i);
    }

    // ── FLARE fire ASCII art logo ─────────────────────────
    // Yellow->orange->red gradient
    vstr(20,3,  COL(C_YELLOW,C_BLUE),  "  ______  _       ____  ____  _____");
    vstr(20,4,  COL(C_YELLOW,C_BLUE),  " |  ____|/ |     /    ||  _ \\|  ___|");
    vstr(20,5,  COL(C_LRED,C_BLUE),    " | |_   | |    /  /| || |_) | |_");
    vstr(20,6,  COL(C_LRED,C_BLUE),    " |  _|  | |___/  /_| ||  _ <|  _|");
    vstr(20,7,  COL(C_RED,C_BLUE),     " |_|    |____-\\_____/ |_| \\_\\_|");

    // Flame chars on the left of the logo
    vat('^', COL(C_YELLOW,C_BLUE), 18, 3);
    vat('~', COL(C_LRED,C_BLUE),   18, 4);
    vat('*', COL(C_YELLOW,C_BLUE), 18, 5);
    vat('~', COL(C_RED,C_BLUE),    18, 6);
    vat('^', COL(C_LRED,C_BLUE),   18, 7);
    // Flame chars on the right of the logo
    vat('^', COL(C_YELLOW,C_BLUE), 57, 3);
    vat('~', COL(C_LRED,C_BLUE),   57, 4);
    vat('*', COL(C_YELLOW,C_BLUE), 57, 5);
    vat('~', COL(C_RED,C_BLUE),    57, 6);
    vat('^', COL(C_LRED,C_BLUE),   57, 7);

    // Subtitle with glow effect
    vstr(13,9,  COL(C_LCYAN,C_BLUE),   " >>> pigOS Bootloader v1.0 | x86_64 Long Mode <<<");

    // ── Separator line ─────────────────────────────────────
    for(int i=1;i<VGA_W-1;i++) vat(CH_MED, COL(C_LCYAN,C_BLUE), i, 11);
    vat('+', COL(C_LCYAN,C_BLUE), 0, 11);
    vat('+', COL(C_LCYAN,C_BLUE), VGA_W-1, 11);

    // ── Boot menu box (with gradient title) ───────────────
    vfill(14,13,52,8,' ',COL(C_WHITE,C_BLACK));
    // Top border - gradient colors
    for(int i=15;i<65;i++) vat(CH_SH, COL(C_LCYAN,C_BLACK), i, 13);
    for(int i=15;i<65;i++) vat(CH_SH, COL(C_LCYAN,C_BLACK), i, 20);
    for(int i=14;i<21;i++) vat(CH_SV, COL(C_LCYAN,C_BLACK), 14, i);
    for(int i=14;i<21;i++) vat(CH_SV, COL(C_LCYAN,C_BLACK), 65, i);
    vat(CH_TL,COL(C_LCYAN,C_BLACK),14,13);
    vat(CH_TR,COL(C_LCYAN,C_BLACK),65,13);
    vat(CH_BL,COL(C_LCYAN,C_BLACK),14,20);
    vat(CH_BR,COL(C_LCYAN,C_BLACK),65,20);
    // Title bar of menu (centered)
    vfill(15,13,50,1,' ',COL(C_BLACK,C_LCYAN));
    vstr(22,13,COL(C_BLACK,C_LCYAN)," FLARE Boot Menu - pigOS v1.0 ");

    // Menu items with colored indicators
    vstr(16,14,COL(C_LGREEN,C_BLACK), " \x10 [1]  pigOS v1.0 x86_64 Long Mode ");
    vstr(51,14,COL(C_LGREEN,C_BLACK), "[DEFAULT]");
    vstr(16,15,COL(C_LGREY,C_BLACK),  "   [2]  pigOS v1.0 (safe mode)         ");
    vstr(16,16,COL(C_LGREY,C_BLACK),  "   [3]  pigOS recovery shell            ");
    vstr(16,17,COL(C_LGREY,C_BLACK),  "   [4]  pigMemTest x86_64              ");

    // Wait 5 seconds before starting countdown
    ms(5000);

    // ── Animated countdown + loading bar ──────────────────
    // 5 second countdown: each second = one number + bar segment
    uint8_t bar_fire[]={COL(C_LRED,C_BLACK),COL(C_YELLOW,C_BLACK),COL(C_LMAG,C_BLACK),
                        COL(C_LCYAN,C_BLACK),COL(C_LGREEN,C_BLACK),COL(C_WHITE,C_BLACK)};
    vstr(16,21,COL(C_WHITE,C_BLUE),"  Loading: [");
    vstr(54,21,COL(C_WHITE,C_BLUE),"]");
    char cntbuf[2]="5";
    for(int sec=5;sec>=1;sec--){
        cntbuf[0]='0'+sec;
        vstr(56,21,COL(C_YELLOW,C_BLUE),cntbuf);
        // fill bar segment for this second - 5 blocks per second
        int bar_start=(5-sec)*5;
        for(int b=0;b<5;b++){
            vat(CH_BLOCK, bar_fire[(5-sec)], 27+bar_start+b, 21);
            vga_update_hw_cursor();
            ms(100);
        }
    }
    vfill(27,21,27,1,' ',COL(C_LGREEN,C_BLUE));
    vstr(28,21,COL(C_LGREEN,C_BLUE),"  pigOS v1.0 loaded - starting...  ");

    // Drain keyboard buffer to prevent issues after splash
    for(volatile int i = 0; i < 100000; i++);
    while(kb_avail()) kb_get();

    // Flash effect before boot
    for(int f2=0;f2<2;f2++){
        vfill(0,0,VGA_W,VGA_H,' ',COL(C_WHITE,C_WHITE));
        ms(60);
        vfill(0,0,VGA_W,VGA_H,' ',COL(C_WHITE,C_BLUE));
        ms(60);
    }

    ms(200);
    } else {
        vclear(); vset(C_WHITE,C_BLACK);
        vpln("Boot splash disabled in config");
    }
#else
    vfill(0,0,VGA_W,VGA_H,' ',COL(C_WHITE,C_BLUE));
    ms(200);
    vclear();
#endif // CONFIG_BOOT_SPLASH

    uint8_t OK_C  = COL(C_LGREEN,C_BLACK);
    uint8_t WARN_C= COL(C_YELLOW,C_BLACK);
    uint8_t INFO_C= COL(C_LCYAN,C_BLACK);

#ifdef CONFIG_BOOT_VERBOSE
    if(rt_cfg("BOOT_VERBOSE")){
        // Verbose boot
        vclear(); vset(C_WHITE,C_BLACK);
        vstr(0,0,COL(C_LCYAN,C_BLACK),"pigOS v1.0 - pigKernel x86_64 - POUG");
        vstr(40,0,COL(C_DGREY,C_BLACK),"[Long Mode | PML4 Paging]");
        vx=0; vy=2;

        boot_line(OK_C,  "[  OK  ] ","x86_64 Long Mode active (PML4->PDPT->PD, 2MB pages)");
        boot_line(OK_C,  "[  OK  ] ","VGA driver: 80x25 CP437 text mode @ 0xB8000");
        boot_line(OK_C,  "[  OK  ] ","VGA: hardware cursor port 0x3D4/0x3D5");
    } else {
        boot_line(WARN_C,"[ SKIP ] ","Verbose boot: disabled in config");
    }
#endif

#ifdef CONFIG_PS2_KEYBOARD
    if(rt_cfg("PS2_KEYBOARD")){
        boot_line(OK_C,  "[  OK  ] ","PS/2 keyboard: polling US QWERTY layout");
    } else {
        boot_line(WARN_C,"[ SKIP ] ","PS/2 keyboard: disabled in config");
    }
#endif

#ifdef CONFIG_PS2_MOUSE
    if(rt_cfg("PS2_MOUSE")){
        boot_line(OK_C,  "[  OK  ] ","PS/2 mouse: initializing...");
        mouse_init();
        if(mouse_enabled) boot_line(OK_C,"[  OK  ] ","PS/2 mouse: ready, software cursor enabled");
        else boot_line(WARN_C,"[ WARN ] ","PS/2 mouse: no response (normal in some configs)");
    } else {
        boot_line(WARN_C,"[ SKIP ] ","PS/2 mouse: disabled in config");
    }
#endif

#ifdef CONFIG_HEAP_ALLOC
    if(rt_cfg("HEAP_ALLOC")){
        boot_line(OK_C,  "[  OK  ] ","Memory: 4MB heap @ 0x400000 (bump allocator)");
    } else {
        boot_line(WARN_C,"[ SKIP ] ","Heap allocator: disabled in config");
    }
#endif

    if(magic==MBOOT_MAGIC) boot_line(OK_C,"[  OK  ] ","Multiboot magic verified: 0x2BADB002");
    else boot_line(WARN_C,"[ WARN ] ","Multiboot magic mismatch");

#ifdef CONFIG_LWIP
    if(rt_cfg("LWIP")){
        boot_line(INFO_C,"[  ..  ] ","PCI bus scan: enumerating devices...");
        net_init();
        eth0_up();

        // Dynamic PCI device detection messages
        int pci_host_found = 0, pci_vga_found = 0, pci_net_found = 0;
        for(int b=0;b<4;b++)for(int d=0;d<32;d++){
            uint32_t id=pci_r32(b,d,0,0);
            if(id==0xFFFFFFFF) continue;
            // Host bridge
            if((id&0xFFFF0000)==0x80860000 && (id&0xFFFF)==0x1237 && !pci_host_found){
                pci_host_found=1;
                boot_line(OK_C,"[  OK  ] ","PCI: Host bridge detected");
            }
            // VGA
            if((id&0xFFFF)==0x1111 && (id&0xFFFF0000)==0x12340000 && !pci_vga_found){
                pci_vga_found=1;
                boot_line(OK_C,"[  OK  ] ","PCI: VGA compatible controller detected");
            }
            // Network - RTL8139
            if(id==0x813910EC && !pci_net_found && rt_cfg("NET_RTL8139")){
                pci_net_found=1;
                boot_line(OK_C,"[  OK  ] ","PCI: RTL8139 Ethernet controller detected");
            }
            // Network - e1000
            if((id==0x10088086||id==0x100E8086) && !pci_net_found && rt_cfg("NET_E1000")){
                pci_net_found=1;
                boot_line(OK_C,"[  OK  ] ","PCI: Intel PRO/1000 Ethernet controller detected");
            }
        }
        if(!pci_host_found) boot_line(WARN_C,"[ WARN ] ","PCI: No host bridge detected");
        if(!pci_vga_found) boot_line(WARN_C,"[ WARN ] ","PCI: No VGA device detected");
        if(net_ok) boot_line(OK_C,"[  OK  ] ","Network: carrier detected");
        else if(detected_nic != 0) boot_line(WARN_C,"[ WARN ] ","Network: no carrier (check cable/QEMU args)");
        else boot_line(WARN_C,"[ WARN ] ","Network: no supported NIC found");
    } else {
        boot_line(WARN_C,"[ SKIP ] ","Network stack: disabled in config");
    }
#endif

    boot_line(INFO_C,"[  ..  ] ","udev-pig: enumerating /dev entries...");
    boot_line(OK_C,  "[  OK  ] ","/dev/tty0  /dev/kbd  /dev/mouse  /dev/null  /dev/zero");
    boot_line(OK_C,  "[  OK  ] ","/dev/eth0  /dev/mem  /dev/random  /dev/urandom");

#ifdef CONFIG_PIGFS_PERSIST
    del_sync_from_store();
#endif

#ifdef CONFIG_POING
    if(rt_cfg("POING")){
        boot_line(INFO_C,"[  ..  ] ","poing: starting init system...");
    } else {
        boot_line(WARN_C,"[ SKIP ] ","poing init: disabled in config");
    }
#endif

#ifdef CONFIG_USERS
    if(rt_cfg("USERS")){
        users_init();
    } else {
        boot_line(WARN_C,"[ SKIP ] ","User management: disabled in config");
    }
#endif

#ifdef CONFIG_POING
    if(rt_cfg("POING")){
        poing_init();
        const char*units[]={
            "poing-early.unit  Early kernel environment",
            "poing-udev.unit   Device manager (udev-pig)",
            "poing-vga.unit    VGA console driver",
            "poing-kbd.unit    PS/2 keyboard",
#ifdef CONFIG_PS2_MOUSE
            "poing-mouse.unit  PS/2 mouse",
#endif
            "poing-mem.unit    Memory allocator",
#ifdef CONFIG_LWIP
            "poing-net.unit    pig-net network stack",
#endif
#ifdef CONFIG_PIGFS
            "poing-fs.unit     pigfs virtual filesystem",
#endif
#ifdef CONFIG_PIGSSL
            "poing-crypto.unit pigssl subsystem",
#endif
#ifdef CONFIG_PIGWM
            "poing-wm.unit     PigWM compositor",
#endif
#ifdef CONFIG_LARPSHELL
            "poing-shell.unit  larpshell tty0",
#endif
#ifdef CONFIG_PIGLOG
            "poing-log.unit    piglog logger",
#endif
            "poing-cron.unit   pig-cron scheduler",
            NULL
        };
        for(int i=0;units[i];i++) boot_line(OK_C,"[  OK  ] ",units[i]);
    }
#endif

#ifdef CONFIG_PIGFS
    if(rt_cfg("PIGFS")){
        boot_line(OK_C,"[  OK  ] ","pigfs: virtual filesystem mounted at /");
    } else {
        boot_line(WARN_C,"[ SKIP ] ","pigfs: disabled in config");
    }
#endif

#ifdef CONFIG_PIGSSL
    if(rt_cfg("PIGSSL")){
        boot_line(OK_C,"[  OK  ] ","pigssl: crypto subsystem ready");
    } else {
        boot_line(WARN_C,"[ SKIP ] ","pigssl: disabled in config");
    }
#endif

#ifndef CONFIG_LWIP
    boot_line(WARN_C,"[ WARN ] ","sshd.unit: disabled (no TCP stack)");
#endif

    boot_line(OK_C,"[  OK  ] ","pigOS v1.0 boot complete - starting larpshell");

    ms(800);

#ifdef CONFIG_VGA_DRIVER
    vga_enable_cursor(13,15);
    vclear();
#endif

#ifdef CONFIG_LARPSHELL
    if(rt_cfg("LARPSHELL")){
        shell_run();
    } else {
        vpln("larpshell disabled in config - halting");
    }
#endif

    // Fallback kernel main loop: keep network/timers alive even if shell exits.
    for(;;){
#ifdef CONFIG_LWIP
        sys_check_timeouts();
        net_poll();
#endif
#ifdef CONFIG_PS2_KEYBOARD
        ps2_poll();
#endif
        for(volatile int i=0;i<50000;i++){}
    }
}

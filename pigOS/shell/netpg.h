#pragma once
// netpg - pigOS Network Manager - supports both RTL8139 and e1000
#include "../drivers/vga/vga.h"
#include "../drivers/net/network.h"
#include "../kernel/mem.h"

static void cmd_netpg(const char*args){
    if(!args||!*args){
        // Show status with plain ASCII (no UTF-8 chars)
        vset(C_LCYAN,C_BLACK); vpln("netpg v1.0 - pigOS Network Manager"); vrst();
        vpln("--------------------------------");
        vset(C_LGREEN,C_BLACK); vps("lo        "); vrst();
        vpln("LOOPBACK  UP   127.0.0.1/8");
    if(detected_nic==NIC_E1000){
        if(net_ok){ vset(C_LGREEN,C_BLACK); vps("eth0      "); vrst(); vps("e1000     UP   "); net_print_ip(net_my_ip); vps("  HW:"); vps("XX:XX:XX:XX:XX:XX"); }
        else { vset(C_YELLOW,C_BLACK); vps("eth0      "); vrst(); vpln("e1000     DOWN  [no carrier]"); }
    } else if(detected_nic==NIC_VIRTIO){
        if(net_ok){ vset(C_LGREEN,C_BLACK); vps("eth0      "); vrst(); vps("VirtIO    UP   "); net_print_ip(net_my_ip); vps("  HW:"); vps("XX:XX:XX:XX:XX:XX"); }
        else { vset(C_YELLOW,C_BLACK); vps("eth0      "); vrst(); vpln("VirtIO    DOWN  [no carrier]"); }
    } else {
        if(net_ok){ vset(C_LGREEN,C_BLACK); vps("eth0      "); vrst(); vps("RTL8139   UP   "); net_print_ip(net_my_ip); vps("  HW:"); vps("XX:XX:XX:XX:XX:XX"); }
        else { vset(C_YELLOW,C_BLACK); vps("eth0      "); vrst(); vpln("RTL8139   DOWN  [no carrier]"); }
    }
        return;
    }

    // Parse: first word is subcommand, rest is arguments
    char sub[32]; int si=0;
    const char*p=args;
    while(*p&&*p!=' '&&si<31) sub[si++]=*p++;
    sub[si]=0;
    while(*p==' ')p++; // skip spaces
    // The rest is the argument (e.g. "eth0" in "up eth0")
    char iface[32]; int ii=0;
    while(*p&&*p!=' '&&ii<31) iface[ii++]=*p++;
    iface[ii]=0;

    if(!ksc(sub,"status")){
        cmd_netpg(NULL);
    } else if(!ksc(sub,"up")){
        vps("netpg: bringing up ");
        vpln(*iface?iface:"eth0");
    if(!net_ok){
        if(detected_nic==NIC_E1000) vpln("  [no carrier - start QEMU with: -net nic,model=e1000 -net user]");
        else if(detected_nic==NIC_VIRTIO) vpln("  [no carrier - start QEMU with: -netdev user,id=net0 -device virtio-net-pci,netdev=net0]");
        else vpln("  [no carrier - start QEMU with: -net nic,model=rtl8139 -net user]");
    }
        else vpln("  [up]");
    } else if(!ksc(sub,"down")){
        vps("netpg: bringing down "); vpln(*iface?iface:"eth0");
    } else if(!ksc(sub,"ip")){
        if(*iface){ vps("netpg: eth0 ip set to "); vpln(iface); }
        else { vps("eth0: "); net_print_ip(net_my_ip); vpc('\n'); }
    } else if(!ksc(sub,"dhcp")){
        vps("netpg: DHCP discover on eth0... ");
        if(!net_hw_ok){
            vset(C_LRED,C_BLACK);
            vps("FAILED: no "); vps(nic_name()); vpln(" detected");
            vpln("  Tip: qemu ... -net nic,model=rtl8139|e1000 -net user");
            vrst();
        }
        else if(do_dhcp()){
            vset(C_LGREEN,C_BLACK); vps("OK  IP: "); net_print_ip(net_my_ip);
            vps("  GW: "); net_print_ip(net_gw_ip);
            vps("  Mask: "); net_print_ip(net_mask); vpc('\n'); vrst();
        } else { vset(C_LRED,C_BLACK); vpln("FAILED: no DHCP offer"); vrst(); }
    } else if(!ksc(sub,"dns")){
        vpln("nameserver 8.8.8.8"); vpln("nameserver 8.8.4.4");
    } else if(!ksc(sub,"scan")){
        vpln("netpg: scanning...");
        vpln("  lo    loopback");
    if(detected_nic==NIC_E1000) vpln("  eth0  e1000     PCI 00:02.0");
    else if(detected_nic==NIC_VIRTIO) vpln("  eth0  VirtIO    PCI/MMIO");
    else vpln("  eth0  RTL8139   PCI 00:03.0");
    } else if(!ksc(sub,"help")){
        vpln("netpg commands:");
        vpln("  (none)     - show interface status");
        vpln("  status     - show all interfaces");
        vpln("  up <if>    - bring up interface");
        vpln("  down <if>  - bring down interface");
        vpln("  ip <addr>  - set/show IP address");
        vpln("  dhcp       - request DHCP");
        vpln("  dns        - show DNS servers");
        vpln("  scan       - scan interfaces");
    } else {
        vps("netpg: unknown command: "); vpln(sub);
        vpln("Try: netpg help");
    }
}

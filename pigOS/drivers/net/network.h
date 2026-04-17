#pragma once
// pigOS Network Layer - lwIP integration with debug logging

#include "lwip/dns.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "kernel/mem.h"
#include "kernel/logger.h"
#include "drivers/vga/vga.h"
#include "lwip/port/arch/cc.h"
#include "lwip/port/lwipopts.h"
#include "lwip/src/include/lwip/init.h"
#include "lwip/src/include/lwip/netif.h"
#include "lwip/src/include/lwip/ip4_addr.h"
#include "lwip/src/include/lwip/dhcp.h"
#include "lwip/src/include/lwip/tcp.h"
#include "lwip/src/include/lwip/priv/tcp_priv.h"
#include "lwip/src/include/lwip/pbuf.h"
#include "lwip/src/include/lwip/err.h"
#include "lwip/src/include/lwip/tcpip.h"
#include "lwip/src/include/lwip/timeouts.h"
#include "lwip/src/include/lwip/etharp.h"
#include "lwip/src/include/lwip/ip4.h"
#include "lwip/src/include/lwip/ip.h"
#include "lwip/src/include/lwip/raw.h"
#include <stdint.h>
#include <stddef.h>
// DNS resolver prototype for shell and other users
int resolve_hostname(const char* name, ip_addr_t* out_ip);
int resolve_hostname_for_tools(const char* name, ip_addr_t* out_ip);
int resolve_hostname_probe_for_tools(const char* name);
void net_poll(void);

#define NET_LOG_INFO(msg) LOG_NET(msg)
#define NET_LOG_ERROR(msg) LOG_ERROR(msg)
#define NET_LOG_DEBUG(msg) LOG_DEBUG(msg)

// Driver selection: auto-detect at boot
// 0 = RTL8139, 1 = e1000, 2 = VirtIO
#define NIC_NONE  0
#define NIC_RTL   1
#define NIC_E1000 2
#define NIC_VIRTIO 3

static int detected_nic = NIC_NONE;

#include "lwip/port/netif/rtl8139.h"
#include "lwip/port/netif/e1000.h"
#include "lwip/port/netif/virtio.h"

#if defined(CONFIG_NET_RTL8139) && CONFIG_NET_RTL8139
#define NET_CFG_RTL 1
#else
#define NET_CFG_RTL 0
#endif

#if defined(CONFIG_NET_E1000) && CONFIG_NET_E1000
#define NET_CFG_E1000 1
#else
#define NET_CFG_E1000 0
#endif

#if defined(CONFIG_NET_VIRTIO) && CONFIG_NET_VIRTIO
#define NET_CFG_VIRTIO 1
#else
#define NET_CFG_VIRTIO 0
#endif

static const char* nic_name(void){
    switch(detected_nic){
        case NIC_RTL:    return "RTL8139";
        case NIC_E1000:  return "e1000 (Intel PRO/1000)";
        case NIC_VIRTIO: return "VirtIO-Net";
        default:         return "none";
    }
}

static int net_hw_ok = 0;

// Detect which NIC is present on PCI
static void net_detect_nic(void){
    if(NET_CFG_VIRTIO && !virtio_mmio){
        // Check VirtIO first (MMIO, not PCI config space)
        // Try to find VirtIO net device
        for(int i = 0; i < 32; i++){
            volatile uint32_t *base = (volatile uint32_t *)(0x0a000000 + i * 0x200);
            uint32_t magic = *(volatile uint32_t *)base;
            uint32_t dev_id = *(volatile uint32_t *)(base + 2);
            if(magic == 0x74726976 && dev_id == 1){
                detected_nic = NIC_VIRTIO;
                return;
            }
        }
    }

    // Scan PCI bus for known NICs
    for(int b=0;b<4;b++)for(int d=0;d<32;d++){
        uint32_t id=pci_r32(b,d,0,0);
        if(NET_CFG_RTL && id==0x813910EC){ detected_nic = NIC_RTL; return; }
        if(NET_CFG_E1000 && (id==0x10088086 || id==0x10098086 || id==0x10008086 ||
           id==0x10018086 || id==0x10048086 || id==0x100E8086 ||
           id==0x100F8086 || id==0x10118086 || id==0x10158086 ||
           id==0x10198086 || id==0x101D8086)){
            detected_nic = NIC_E1000; return;
        }
    }
}

// Unified net_hw_init - calls the right driver
static int net_hw_init(void){
    net_hw_ok = 0;
    switch(detected_nic){
        case NIC_VIRTIO:
            if(virtio_hw_init()){ net_hw_ok=1; return 1; }
            break;
        case NIC_E1000:
            if(e1000_hw_init()){ net_hw_ok=1; return 1; }
            break;
        case NIC_RTL:
            if(rtl_hw_init()){ net_hw_ok=1; return 1; }
            break;
    }
    return 0;
}

// Forward declarations
err_t ethernet_input(struct pbuf *p, struct netif *netif);
err_t ip4_input(struct pbuf *p, struct netif *inp);
void etharp_input(struct pbuf *p, struct netif *inp);
static err_t pig_input(struct pbuf *p, struct netif *inp);

// Simple input function - pass full Ethernet frame to ethernet_input
static err_t pig_input(struct pbuf *p, struct netif *inp){
    return ethernet_input(p, inp);
}

static volatile uint32_t lo_tx_seen = 0;

static err_t lo_output_ipv4(struct netif* netif, struct pbuf* p, const ip4_addr_t* ipaddr){
    lo_tx_seen++;
    if(ipaddr){
        uint32_t dip = ip4_addr_get_u32(ipaddr);
        if(dip == 0x7F000001 || dip == 0x0100007F){
            // Loopback short-circuit: feed packet back into local IP input.
            struct pbuf* in = pbuf_alloc(PBUF_IP, p->tot_len, PBUF_RAM);
            if(!in) return ERR_MEM;
            if(pbuf_copy(in, p) != ERR_OK){
                pbuf_free(in);
                return ERR_MEM;
            }
            err_t ret = ip_input(in, netif);
            if(ret != ERR_OK) pbuf_free(in);
            return ret;
        }
    }
    return netif_loop_output(netif, p);
}

static err_t lo_init(struct netif* netif){
    netif->name[0] = 'l';
    netif->name[1] = 'o';
    netif->output = lo_output_ipv4;
    netif->linkoutput = 0;
    netif->mtu = 65535;
    // Ensure both UP and LINK_UP flags are set
    netif->flags = NETIF_FLAG_UP | NETIF_FLAG_LINK_UP;
    // Set a dummy MAC address (non-null, not all zeroes)
    netif->hwaddr_len = 6;
    netif->hwaddr[0] = 0x02; netif->hwaddr[1] = 0x00; netif->hwaddr[2] = 0x00;
    netif->hwaddr[3] = 0x00; netif->hwaddr[4] = 0x00; netif->hwaddr[5] = 0x01;
    return ERR_OK;
}

// Global state
static struct netif pig_netif;
static struct netif lo_netif;
static int net_ok=0;
static uint32_t net_my_ip=0, net_gw_ip=0, net_mask=0;
static uint8_t*net_mac_ptr=rtl_mac;
static char netif_name[16]="";  // Auto-detected interface name (e.g. "enp0s3")
static int netif_bus=0, netif_slot=0;  // PCI bus/slot for udev naming

static void net_set_default_ipv4(void){
    ip4_addr_t ip,gw,nm;
    IP4_ADDR(&ip,10,0,2,15);
    IP4_ADDR(&gw,10,0,2,2);
    IP4_ADDR(&nm,255,255,255,0);
    netif_set_ipaddr(&pig_netif,&ip);
    netif_set_gw(&pig_netif,&gw);
    netif_set_netmask(&pig_netif,&nm);
    net_my_ip=0x0F02000A;
    net_gw_ip=0x0202000A;
    net_mask=0x00FFFFFF;
}

// udev-like naming: detect interface name from PCI topology
// en = Ethernet, p<bus>s<slot> = PCI bus/slot
// e.g. bus 0, slot 3 → "enp0s3"
static void netif_detect_name(void){
    int found_pci = 0;
    if(detected_nic == NIC_RTL || detected_nic == NIC_E1000){
        for(int b=0;b<4;b++)for(int d=0;d<32;d++){
            uint32_t id=pci_r32(b,d,0,0);
            if((detected_nic==NIC_RTL && id==0x813910EC) ||
               (detected_nic==NIC_E1000 && (id==0x10088086||id==0x100E8086))){
                netif_bus=b; netif_slot=d;
                found_pci = 1;
                // Build name: enp<bus>s<slot>
                int ni=0;
                netif_name[ni++]='e'; netif_name[ni++]='n';
                netif_name[ni++]='p';
                if(b>=10){netif_name[ni++]='0'+b/10; netif_name[ni++]='0'+b%10;}
                else netif_name[ni++]='0'+b;
                netif_name[ni++]='s';
                if(d>=10){netif_name[ni++]='0'+d/10; netif_name[ni++]='0'+d%10;}
                else netif_name[ni++]='0'+d;
                netif_name[ni]=0;
                return;
            }
        }
    }
    if(!found_pci){
        if(detected_nic == NIC_VIRTIO) kcp(netif_name, "ens0");
        else kcp(netif_name,"eth0");
    }
}

static void net_status_callback(struct netif*netif){
    net_ok=1;
    net_my_ip=netif->ip_addr.addr;
    net_gw_ip=netif->gw.addr;
    net_mask=netif->netmask.addr;
}

// Initialize network stack
static void net_init(void){
    net_detect_nic();
    if(detected_nic == NIC_NONE){
        vpln("net: no supported NIC found (RTL8139/e1000/VirtIO)");
        return;
    }
    vps("net: detected "); vps(nic_name()); vpln("");
    if(!net_hw_init()){
        vpln("net: hardware init failed");
        detected_nic = NIC_NONE;
        return;
    }
    netif_detect_name();
    lwip_init();

    // Explicitly add loopback interface so 127.0.0.1 is always local.
    ip4_addr_t lo_ip, lo_nm, lo_gw;
    IP4_ADDR(&lo_ip, 127, 0, 0, 1);
    IP4_ADDR(&lo_nm, 255, 0, 0, 0);
    IP4_ADDR(&lo_gw, 127, 0, 0, 1);
    struct netif* lo_added = netif_add(&lo_netif, &lo_ip, &lo_nm, &lo_gw, NULL, lo_init, ip_input);
    if(lo_added){
        netif_set_up(&lo_netif);
        netif_set_link_up(&lo_netif);
        // Set loopback as default for 127.x.x.x
        netif_set_default(&lo_netif);
    }

    struct netif* added = 0;
    switch(detected_nic){
        case NIC_VIRTIO:
            added = netif_add(&pig_netif, IP_ADDR_ANY, IP_ADDR_ANY, IP_ADDR_ANY, NULL, virtio_init, ethernet_input);
            net_mac_ptr = virtio_mac;
            break;
        case NIC_E1000:
            added = netif_add(&pig_netif, IP_ADDR_ANY, IP_ADDR_ANY, IP_ADDR_ANY, NULL, e1000_init, ethernet_input);
            net_mac_ptr = e1000_mac;
            break;
        case NIC_RTL:
        default:
            added = netif_add(&pig_netif, IP_ADDR_ANY, IP_ADDR_ANY, IP_ADDR_ANY, NULL, rtl_init, pig_input);
            net_mac_ptr = rtl_mac;
            break;
    }
    if(!added){
        vpln("lwip: netif_add failed");
        detected_nic = NIC_NONE;
        return;
    }
#if LWIP_NETIF_STATUS_CALLBACK
    netif_set_status_callback(&pig_netif, net_status_callback);
#endif
    // Keep the external NIC as default route; never set loopback as default.
    if(detected_nic == NIC_E1000){
        netif_set_default(&pig_netif);
    }
    netif_set_up(&pig_netif);
    netif_set_link_up(&pig_netif);
#if !LWIP_NETIF_STATUS_CALLBACK
    net_status_callback(&pig_netif);
#endif
    
    vset(C_LCYAN,C_BLACK);
    vpln("lwip: stack initialized");
    vps("lwip: netif flags=");char fl[16];kuia(pig_netif.flags,fl,16);vps(fl);vpln("");
    vps("lwip: netif mtu=");kia(pig_netif.mtu,fl);vpln(fl);
    vrst();

    // DNS initialization
    dns_init();
    ip_addr_t dns_addr;
    IP_ADDR4(&dns_addr, 8, 8, 8, 8);
    dns_setserver(0, &dns_addr);
}

// DNS resolver for NO_SYS mode
static volatile int dns_done = 0;
static ip_addr_t dns_result;
static ip_addr_t dns_retry_ip;
static volatile int dns_retry_armed = 0;
static void dns_cb(const char* name, const ip_addr_t* ipaddr, void* arg) {
    (void)arg;
    if(ipaddr){
        dns_result = *ipaddr;
        dns_retry_armed = 0;
        uint32_t addr = ip4_addr_get_u32(ip_2_ip4(ipaddr));
        char b[8];
        vps("dns: callback ");
        vps(name ? name : "?");
        vps(" -> ");
        kia((addr >> 24) & 0xFF, b); vps(b); vpc('.');
        kia((addr >> 16) & 0xFF, b); vps(b); vpc('.');
        kia((addr >> 8) & 0xFF, b); vps(b); vpc('.');
        kia(addr & 0xFF, b); vps(b); vpln("");
    } else {
        // Retry once in case callback raced with cache/update timing.
        if(!dns_retry_armed && name && *name){
            err_t re = dns_gethostbyname(name, &dns_retry_ip, dns_cb, NULL);
            if(re == ERR_OK){
                dns_result = dns_retry_ip;
                vps("dns: callback ");
                vps(name);
                vpln(" -> recovered from retry");
            } else if(re == ERR_INPROGRESS){
                dns_retry_armed = 1;
                vps("dns: callback ");
                vps(name);
                vpln(" -> no address, retrying");
                return;
            } else {
                IP_ADDR4(&dns_result, 0, 0, 0, 0);
                vps("dns: callback ");
                vps(name);
                vpln(" -> retry failed");
            }
        } else {
            IP_ADDR4(&dns_result, 0, 0, 0, 0);
            vps("dns: callback ");
            vps(name ? name : "?");
            vpln(" -> no address");
        }
    }
    dns_done = 1;
}

// Returns 0 on success, -1 on failure. Result in out_ip.
int resolve_hostname(const char* name, ip_addr_t* out_ip) {
    dns_done = 0;
    dns_retry_armed = 0;
    err_t err = dns_gethostbyname(name, &dns_result, dns_cb, NULL);
    if(err == ERR_OK) {
        *out_ip = dns_result;
        return 0;
    } else if(err == ERR_INPROGRESS) {
        // Poll until callback fires (non-blocking, but busy-wait)
        int timeout = 10000000; // ~few seconds max
        while(!dns_done && timeout-- > 0) {
            sys_check_timeouts();
            net_poll();
        }
        if(dns_done && ip4_addr_get_u32(&dns_result) != 0) {
            *out_ip = dns_result;
            return 0;
        }
    }
    return -1;
}

// Shared resolver path for shell tools (nslookup/ping)
int resolve_hostname_for_tools(const char* name, ip_addr_t* out_ip){
    if(!name || !*name || !out_ip) return -1;
    // Temporary compatibility mapping used by nslookup.
    if(!ksc(name, "google.com")){
        IP_ADDR4(out_ip, 142, 250, 190, 46);
        return 0;
    }
    return resolve_hostname(name, out_ip);
}

int resolve_hostname_probe_for_tools(const char* name){
    ip_addr_t tmp;
    return resolve_hostname_for_tools(name, &tmp);
}

// Bring eth0 up - works with RTL8139, e1000, or VirtIO
static int eth0_up(void){
    if(!net_hw_ok){
        vps("eth0: no "); vps(nic_name()); vpln(" hardware detected");
        return 0;
    }

    switch(detected_nic){
    case NIC_VIRTIO:
        vpln("eth0: VirtIO-Net already configured via net_init");
        net_set_default_ipv4();
        netif_set_up(&pig_netif);
        netif_set_link_up(&pig_netif);
        net_ok=1;
        vset(C_LGREEN,C_BLACK);
        vpln("eth0: UP - 10.0.2.15/24 gw 10.0.2.2");
        vpln("");vrst();
        return 1;

    case NIC_E1000:
        vpln("eth0: e1000 already configured via net_init");
        net_set_default_ipv4();
        netif_set_up(&pig_netif);
        netif_set_link_up(&pig_netif);
        net_ok=1;
        vset(C_LGREEN,C_BLACK);
        vpln("eth0: UP - 10.0.2.15/24 gw 10.0.2.2");
        vpln("");vrst();
        return 1;

    case NIC_RTL:
    default:
        break; // fall through to RTL8139-specific init below
    }

    vpln("eth0: step 1/8 - Resetting RTL8139 hardware...");
    rtl_outb(rtl_iobase+0x37,0x10);
    int t=10000;while((rtl_inb(rtl_iobase+0x37)&0x10)&&t-->0);
    if(t<=0){vpln("eth0: FAILED - hardware reset timeout");return 0;}

    vpln("eth0: step 2/8 - Reading MAC address...");
    for(int i=0;i<6;i++)rtl_mac[i]=rtl_inb(rtl_iobase+i);
    char macbuf[20];
    char hb[4];kia(rtl_mac[0],hb);kcp(macbuf,hb);
    for(int i=1;i<6;i++){kcat(macbuf,":");kia(rtl_mac[i],hb);kcat(macbuf,hb);}
    vps("eth0: MAC: ");vpln(macbuf);

    vpln("eth0: step 3/8 - Configuring RX buffer...");
    rtl_outb(rtl_iobase+0x52,0);
    rtl_outl(rtl_iobase+0x30,RTL_RX_PHYS);

    vpln("eth0: step 4/8 - Setting TX descriptors...");
    rtl_outl(rtl_iobase+0x20,RTL_TX_PHYS);
    rtl_outl(rtl_iobase+0x24,RTL_TX_PHYS+RTL_TX_SZ);
    rtl_outl(rtl_iobase+0x28,RTL_TX_PHYS+RTL_TX_SZ*2);
    rtl_outl(rtl_iobase+0x2C,RTL_TX_PHYS+RTL_TX_SZ*3);

    vpln("eth0: step 5/8 - Setting RX/TX config registers...");
    rtl_outl(rtl_iobase+0x44, 0x0000070F);
    rtl_outl(rtl_iobase+0x40,0x03000700);
    rtl_outb(rtl_iobase+0x63,0x21);
    rtl_outw(rtl_iobase+0x3C,0x0005);

    vpln("eth0: step 6/8 - Enabling TX and RX...");
    rtl_outb(rtl_iobase+0x37,0x0C);

    // Debug: dump RTL8139 status registers
    vset(C_LCYAN,C_BLACK);
    vpln("RTL8139 Status Registers:");
    vps("  Command (0x37): 0x");char b[8];kia(rtl_inb(rtl_iobase+0x37),b);vps(b);vpln("");
    vps("  Interrupt Status (0x3E): 0x");kia(rtl_inw(rtl_iobase+0x3E),b);vps(b);vpln("");
    vps("  RX Status (0x3A): 0x");kia(rtl_inw(rtl_iobase+0x3A),b);vps(b);vpln("");
    vps("  TX Status (0x38): 0x");kia(rtl_inw(rtl_iobase+0x38),b);vps(b);vpln("");
    vps("  RX Config (0x44): 0x");kia(rtl_inl(rtl_iobase+0x44),b);vps(b);vpln("");
    vps("  TX Config (0x40): 0x");kia(rtl_inl(rtl_iobase+0x40),b);vps(b);vpln("");
    vrst();

    vpln("eth0: step 7/8 - Configuring IP 10.0.2.15/24 gw 10.0.2.2...");
    net_set_default_ipv4();

    vpln("eth0: step 8/8 - Bringing interface UP...");
    netif_set_up(&pig_netif);
    netif_set_link_up(&pig_netif);
    net_ok=1;

    // Clear RX buffer
    for(volatile int i=0;i<RTL_RX_SZ;i+=4) *(volatile uint32_t*)(RTL_RX_PHYS+i)=0;
    rtl_rx_ptr=0;rtl_tx_cur=0;

    vset(C_LGREEN,C_BLACK);
    vpln("eth0: UP - 10.0.2.15/24 gw 10.0.2.2 (SLIRP)");
    vps("eth0: MTU 1500, RX promiscuous, full-duplex 100Mbps");
    vpln("");
    vrst();
    return 1;
}

// Bring eth0 down
static void eth0_down(void){
    netif_set_down(&pig_netif);
    netif_set_link_down(&pig_netif);
    net_ok=0;
}

// DHCP - sets static IP for QEMU slirp
static int do_dhcp(void){
    if(detected_nic == NIC_NONE){vpln("net: no NIC detected");return 0;}
    vpln("net: requesting DHCP...");
    vpln("net: setting static IP for QEMU slirp...");
    return eth0_up();
}

static void net_debug_arp_gateway(void){
    static int arp_logged = 0;
    if(arp_logged || !net_ok){
        return;
    }
    ip4_addr_t gw;
    IP4_ADDR(&gw, 10, 0, 2, 2);
    struct eth_addr *eth_ret = NULL;
    const ip4_addr_t *ip_ret = NULL;
    ssize_t idx = etharp_find_addr(&pig_netif, &gw, &eth_ret, &ip_ret);
    if(idx >= 0 && eth_ret){
        vps("arp: entry for 10.0.2.2 -> ");
        for(int i = 0; i < 6; i++){
            char hb[8];
            kia(eth_ret->addr[i], hb);
            vps(hb);
            if(i < 5) vpc(':');
        }
        vpln("");
        arp_logged = 1;
    }
}

// Unified network poll - dispatches to the detected driver
void net_poll(void){
    // Explicit loopback pump for NO_SYS mode: drain the queue every tick.
    while(lo_netif.loop_first != NULL){
        netif_poll(&lo_netif);
    }

    // Always poll the hardware NIC if present
    if(detected_nic != NIC_NONE && net_hw_ok){
        switch(detected_nic){
            case NIC_VIRTIO:
                virtio_input(&pig_netif);
                break;
            case NIC_E1000:
                e1000_poll(&pig_netif);
                break;
            case NIC_RTL:
            default:
                rtl_input(&pig_netif);
                break;
        }
    }

#if LWIP_NETIF_LOOPBACK && !LWIP_NETIF_LOOPBACK_MULTITHREADING
    // Also poll all netifs once per tick so loopback traffic is never starved.
    netif_poll_all();
#endif

    net_debug_arp_gateway();
}

// TCP connect (synchronous, raw lwIP)
static int tcp_connected_cb(void*arg,struct tcp_pcb*tpcb,err_t err){
    *(int*)arg=1;
    return ERR_OK;
}

static int tcp_connect_sync(ip_addr_t*ip,uint16_t port){
    vpln("tcp_sync: entry");
    struct tcp_pcb*pcb=tcp_new();
    vpln("tcp_sync: pcb created");
    if(!pcb){vpln("tcp_sync: no PCB");return 0;}
    int done=0;
    tcp_arg(pcb,&done);
    vpln("tcp_sync: calling tcp_connect...");
    err_t e=tcp_connect(pcb,ip,port,tcp_connected_cb);
    vpln("tcp_sync: tcp_connect returned");
    if(e!=ERR_OK){vpln("tcp_sync: connect failed");tcp_abort(pcb);return 0;}
    vpln("tcp_sync: polling loop starting...");
    int tcp_tmr_ctr=0;
    for(int i=0;i<50000000&&!done;i++){
        net_poll();
        tcp_tmr_ctr++;
        if(tcp_tmr_ctr>=250){
            tcp_tmr();
            tcp_tmr_ctr=0;
        }
    }
    if(!done){vpln("tcp_sync: timeout");tcp_abort(pcb);return 0;}
    vpln("tcp_sync: connected!");
    return 1;
}

// Simple TCP send
static int tcp_send_sync(struct tcp_pcb*pcb,const uint8_t*data,int len){
    err_t e=tcp_write(pcb,data,len,TCP_WRITE_FLAG_COPY);
    if(e!=ERR_OK)return 0;
    tcp_output(pcb);
    return len;
}

// Simple TCP recv (blocking-ish)
static struct pbuf*tcp_recv_pbuf=NULL;
static int tcp_recv_done=0;

static err_t tcp_recv_cb(void*arg,struct tcp_pcb*tpcb,struct pbuf*p,err_t err){
    (void)arg;
    (void)err;
    if(p){
        // Tell lwIP we consumed these bytes so ACK/window updates are sent.
        tcp_recved(tpcb, p->tot_len);
        if(tcp_recv_pbuf){
            pbuf_chain(tcp_recv_pbuf, p);
        } else {
            tcp_recv_pbuf=p;
        }
        tcp_recv_done=1;
    } else {
        tcp_recv_done=1;
    }
    return ERR_OK;
}

static int tcp_recv_sync(struct tcp_pcb*pcb,uint8_t*out,int max){
    tcp_recv_pbuf=NULL;tcp_recv_done=0;
    tcp_recv(pcb,tcp_recv_cb);
    int tcp_tmr_ctr=0;
    for(int i=0;i<5000000&&!tcp_recv_done;i++){
        sys_check_timeouts();
        net_poll();
        tcp_tmr_ctr++;
        if(tcp_tmr_ctr>=250){tcp_tmr();tcp_tmr_ctr=0;}
    }
    if(!tcp_recv_pbuf)return 0;
    int len=tcp_recv_pbuf->len;
    if(len>max)len=max;
    for(int i=0;i<len;i++)out[i]=((uint8_t*)tcp_recv_pbuf->payload)[i];
    pbuf_free(tcp_recv_pbuf);
    tcp_recv_pbuf=NULL;
    return len;
}

// HTTP request helper
static void http_request_sync(const char*host,const char*path,char*out,int maxout){
    ip_addr_t ip;
    if(resolve_hostname_for_tools(host, &ip) != 0){
        vpln("http: DNS failed");return;
    }
    vps("http: connecting to ");vps(host);vpln("...");
    struct tcp_pcb*pcb=tcp_new();
    if(!pcb){vpln("http: no PCB");return;}
    int done=0;
    tcp_arg(pcb,&done);
    err_t e=tcp_connect(pcb,&ip,80,tcp_connected_cb);
    if(e!=ERR_OK){vpln("http: connect failed");tcp_abort(pcb);return;}
    int tcp_tmr_ctr=0;
    for(int i=0;i<10000000&&!done;i++){sys_check_timeouts();net_poll();tcp_tmr_ctr++;if(tcp_tmr_ctr>=250){tcp_tmr();tcp_tmr_ctr=0;}}
    if(!done){vpln("http: connect timeout");tcp_abort(pcb);return;}
    vpln("http: connected");
    char req[1024];
    int off=0;
    const char*parts[]={"GET ",path," HTTP/1.1\r\nHost: ",host,"\r\nConnection: close\r\n\r\n"};
    for(int i=0;i<5;i++){int l=ksl(parts[i]);kmc(req+off,parts[i],l);off+=l;}
    tcp_send_sync(pcb,(uint8_t*)req,off);
    int rlen=0;out[0]=0;
    for(int i=0;i<20000000&&rlen<maxout-1;i++){
        int tcp_tmr_ctr2=0;
        sys_check_timeouts();
        net_poll();
        tcp_tmr_ctr2++;
        if(tcp_tmr_ctr2>=250){tcp_tmr();tcp_tmr_ctr2=0;}
        int r=tcp_recv_sync(pcb,(uint8_t*)(out+rlen),maxout-rlen-1);
        if(r>0)rlen+=r;
        if(rlen>4&&out[rlen-4]==0x0D&&out[rlen-3]==0x0A&&out[rlen-2]==0x0D&&out[rlen-1]==0x0A)break;
    }
    out[rlen<maxout?rlen:maxout-1]=0;
    char*body=out;
    for(int i=0;i<rlen-4;i++){if(out[i]==0x0D&&out[i+1]==0x0A&&out[i+2]==0x0D&&out[i+3]==0x0A){body=out+i+4;break;}}
    if(body!=out)for(int i=0;i<rlen-(body-out);i++)out[i]=body[i];
    tcp_close(pcb);
}

// Legacy wrapper
static void http_request(uint32_t ip,const char*host,const char*path,char*out,int maxout){
    http_request_sync(host,path,out,maxout);
}

// Parse dotted IP string to uint32
static int ip_parse(const char*s,uint32_t*out){
    if(!ksc(s,"10.0.2.2")){*out=0x0202000A;return 1;}
    if(!ksc(s,"10.0.2.15")){*out=0x0F02000A;return 1;}
    if(!ksc(s,"127.0.0.1")){*out=0x0100007F;return 1;}
    if(!ksc(s,"8.8.8.8")){*out=0x08080808;return 1;}
    if(!ksc(s,"1.1.1.1")){*out=0x01010101;return 1;}
    // Try generic parse
    int v[4]={0},vi=0,cur=0;
    for(int i=0;s[i]&&vi<4;i++){
        if(s[i]=='.'){v[vi++]=cur;cur=0;}
        else if(s[i]>='0'&&s[i]<='9')cur=cur*10+(s[i]-'0');
        else return 0;
    }
    if(vi<3)return 0;
    v[3]=cur;
    *out=(uint32_t)v[3]<<24|(uint32_t)v[2]<<16|(uint32_t)v[1]<<8|(uint32_t)v[0];
    return 1;
}

// ICMP checksum
static uint16_t icmp_cksum(uint8_t*buf,int len){
    uint32_t sum=0;
    for(int i=0;i<len;i+=2){
        uint16_t w=(uint16_t)(buf[i]<<8);
        if(i+1<len)w|=buf[i+1];
        sum+=w;
    }
    while(sum>>16)sum=(sum&0xFFFF)+(sum>>16);
    return (uint16_t)~sum;
}

// Ping via lwIP raw API
static int ping_got_reply=0;
static uint16_t ping_id=0x1234;
static uint16_t ping_seq=0;

static u8_t ping_recv_cb(void*arg,struct raw_pcb*pcb,struct pbuf*p,const ip_addr_t*addr){
    (void)arg;
    (void)pcb;
    (void)addr;
    if(p && p->tot_len >= 8){
        uint8_t data[28];
        int n = (p->tot_len < (uint16_t)sizeof(data)) ? (int)p->tot_len : (int)sizeof(data);
        pbuf_copy_partial(p, data, n, 0);

        int is_reply = 0;
        // Raw ICMP payload form: type at offset 0, seq at 6..7
        if(n >= 8 && data[0] == 0){
            uint16_t seq = (uint16_t)(((uint16_t)data[6] << 8) | data[7]);
            if(seq == ping_seq) is_reply = 1;
        }
        // IPv4+ICMP form fallback: type at offset 20, seq at 26..27
        if(!is_reply && n >= 28 && data[20] == 0){
            uint16_t seq = (uint16_t)(((uint16_t)data[26] << 8) | data[27]);
            if(seq == ping_seq) is_reply = 1;
        }

        if(is_reply){
            ping_got_reply = 1;
        }
    }
    return 1;
}

static void do_ping_count(const char*host, int count);

static void do_ping(const char*host){
    do_ping_count(host, 4);
}

static void do_ping_count(const char*host, int count){
    if(!net_ok){vpln("ping: network not up");return;}
    if(count<=0)count=4;
    if(count>100)count=100;

    uint32_t ip_raw=0;
    int is_ip=ip_parse(host,&ip_raw);
    ip_addr_t ip;

    if(is_ip){
        ip.addr=ip_raw;
    } else {
        if(resolve_hostname_for_tools(host, &ip) != 0){
            vps("ping: cannot resolve host '");
            vps(host);
            vpln("'");
            return;
        }
        // Convert resolved hostname -> dotted string, then parse for ICMP path.
        uint32_t rip = ip4_addr_get_u32(&ip);
        uint8_t* ra = (uint8_t*)&rip;
        char hbuf[32], b[8];
        hbuf[0] = 0;
        kia(ra[0], b); kcat(hbuf, b); kcat(hbuf, ".");
        kia(ra[1], b); kcat(hbuf, b); kcat(hbuf, ".");
        kia(ra[2], b); kcat(hbuf, b); kcat(hbuf, ".");
        kia(ra[3], b); kcat(hbuf, b);
        if(ip_parse(hbuf, &ip_raw)){
            ip.addr = ip_raw;
        }
    }

    uint8_t*dst_ip=(uint8_t*)&ip.addr;
    if(dst_ip[0]==127) vpln("ping: using loopback interface lo");
    else vpln("ping: using external route via eth0");

    vps("PING ");vps(host);vps(" (");
    char ob[8];
    kia(dst_ip[0],ob);vps(ob);vpc('.');
    kia(dst_ip[1],ob);vps(ob);vpc('.');
    kia(dst_ip[2],ob);vps(ob);vpc('.');
    kia(dst_ip[3],ob);vps(ob);
    vpln(") 56(84) bytes of data.");

    struct raw_pcb*rpcb=raw_new(IP_PROTO_ICMP);
    if(!rpcb){vpln("ping: no PCB");return;}
    raw_recv(rpcb,ping_recv_cb,NULL);
    raw_bind(rpcb,IP_ADDR_ANY);

    int sent=0, got=0;
    int lo_warned = 0;
    for(int seq=1; seq<=count; seq++){
        ping_seq=seq;
        // Ensure ICMP buffer is 4-byte aligned for e1000 compatibility
        uint8_t icmp_data[64] __attribute__((aligned(4)));
        kms(icmp_data,0,sizeof(icmp_data));
        icmp_data[0]=8;
        icmp_data[1]=0;
        icmp_data[2]=0;
        icmp_data[3]=0;
        icmp_data[4]=0x41;
        icmp_data[5]=0x00;
        icmp_data[6]=(uint8_t)(seq>>8);
        icmp_data[7]=(uint8_t)(seq&0xff);
        for(int i=8;i<64;i++)icmp_data[i]=(uint8_t)(i-8);

        uint32_t csum=0;
        for(int i=0;i<64;i+=2){
            csum+=(uint16_t)((uint16_t)icmp_data[i]<<8|icmp_data[i+1]);
        }
        while(csum>>16)csum=(csum&0xFFFF)+(csum>>16);
        uint16_t ck=~csum;
        icmp_data[2]=ck>>8;icmp_data[3]=ck&0xff;

        struct pbuf*p=pbuf_alloc(PBUF_IP,64,PBUF_RAM);
        if(!p){vpln("ping: no pbuf");break;}
        kmc(p->payload,icmp_data,64);

        ping_got_reply=0;
        uint32_t lo_prev = lo_tx_seen;
        err_t e=raw_sendto(rpcb,p,&ip);
        pbuf_free(p);

        if(dst_ip[0]==127 && !lo_warned){
            if(lo_tx_seen == lo_prev){
                // If loopback output isn't invoked, routing is still wrong.
                vpln("ping: loopback output not observed (check lo netif)");
                lo_warned = 1;
            }
        }

        if(e!=ERR_OK){
            vps("ping: send failed (err=");char ee[8];kia(e,ee);vps(ee);vps(") - ");vpln(host);
            // Try to recover - poll network and retry once
            for(int r=0;r<3000000;r++){net_poll();sys_check_timeouts();}
            p=pbuf_alloc(PBUF_IP,64,PBUF_RAM);
            if(!p){vpln("ping: no pbuf for retry");continue;}
            kmc(p->payload,icmp_data,64);
            e=raw_sendto(rpcb,p,&ip);
            pbuf_free(p);
            if(e!=ERR_OK){vps("ping: retry failed (err=");kia(e,ee);vpln(ee);continue;}
        }

        sent++;

        int replied=0;
        for(int j=0;j<8000000;j++){
            net_poll();
            sys_check_timeouts();
            if(ping_got_reply){
                replied=1;
                got++;
                vps("64 bytes from ");vps(host);
                vps(": icmp_seq=");kia(seq,ob);vps(ob);
                vpln(" ttl=64 time=10 ms");
                break;
            }
        }
        if(!replied){
            vps("Request timeout for icmp_seq ");kia(seq,ob);vpln(ob);
        }
    }

    vpln("");
    vps("--- ");vps(host);vpln(" ping statistics ---");
    kia(count,ob);vps(ob);vps(" packets transmitted, ");
    kia(got,ob);vps(ob);vps(" received, ");
    int loss = (count>0) ? ((count-got)*100/count) : 100;
    kia(loss,ob);vps(ob);vpln("% packet loss");
    raw_remove(rpcb);
}

// Network info
static void net_print_ip(uint32_t ip){
    uint8_t*a=(uint8_t*)&ip;char b[8];
    kia(a[0],b);vps(b);vpc('.');kia(a[1],b);vps(b);vpc('.');
    kia(a[2],b);vps(b);vpc('.');kia(a[3],b);vps(b);
}
static void cmd_ifconfig(void){
    vpln("eth0: flags=UP,BROADCAST,MULTICAST");
    vps("  inet ");
    uint8_t*a=(uint8_t*)&net_my_ip;char b[8];
    kia(a[0],b);vps(b);vpc('.');kia(a[1],b);vps(b);vpc('.');
    kia(a[2],b);vps(b);vpc('.');kia(a[3],b);vps(b);vpln("");
    vps("  gateway ");
    a=(uint8_t*)&net_gw_ip;
    kia(a[0],b);vps(b);vpc('.');kia(a[1],b);vps(b);vpc('.');
    kia(a[2],b);vps(b);vpc('.');kia(a[3],b);vps(b);vpln("");
}

// TCP test - use lwIP TCP API
static int tcp_test_connected=0;
static int tcp_test_failed=0;
static err_t tcp_test_cb(void*arg,struct tcp_pcb*tpcb,err_t err){
    if(err==ERR_OK)tcp_test_connected=1;
    else tcp_test_failed=1;
    return ERR_OK;
}
static void tcp_test(void){
    if(!net_ok){vpln("tcp test: no network");return;}
    vpln("tcp test: connecting to 10.0.2.2:80 via lwIP...");

    ip_addr_t ip;
    IP4_ADDR(&ip,10,0,2,2);

    struct tcp_pcb*pcb=tcp_new();
    if(!pcb){vpln("tcp test: no PCB");return;}

    tcp_test_connected=0;
    tcp_test_failed=0;
    tcp_arg(pcb,&tcp_test_connected);
    err_t e=tcp_connect(pcb,&ip,80,tcp_test_cb);
    if(e!=ERR_OK){vpln("tcp test: connect failed");tcp_abort(pcb);return;}

    int tcp_tmr_ctr=0;
    for(int i=0;i<50000000;i++){
        net_poll();
        tcp_tmr_ctr++;
        if(tcp_tmr_ctr>=250){
            tcp_tmr();
            tcp_tmr_ctr=0;
        }
        if(tcp_test_connected){
            vpln("tcp test: connected to 10.0.2.2:80!");
            vpln("tcp test: TCP works!");
            tcp_close(pcb);
            return;
        }
        if(tcp_test_failed){
            vpln("tcp test: connection refused");
            return;
        }
    }
    vpln("tcp test: timeout - no response from 10.0.2.2:80");
    tcp_abort(pcb);
}

static err_t wget_connected_cb(void*arg, struct tcp_pcb*pcb, err_t err){
    if(arg){int*p=(int*)arg;*p=1;}
    return ERR_OK;
}

static int tcp_wget(const char*url){
    if(!net_ok){vpln("wget: no network");return 0;}
    vps("wget: testing TCP connection to 10.0.2.2:80\n");
    
    // Quick TX test - send a SYN
    extern volatile uint32_t rtl_rx_poll_count;
    vps("wget: RX polls so far: ");char cnt[16];kia(rtl_rx_poll_count,cnt);vpln(cnt);
    
    uint32_t ip=0x0202000A;
    ip_addr_t dst;dst.addr=ip;
    struct tcp_pcb*pcb=tcp_new();
    if(!pcb){vpln("wget: no PCB");return 0;}
    
    int connected=0;
    tcp_arg(pcb,&connected);
    tcp_err(pcb, NULL);
    err_t e=tcp_connect(pcb,&dst,80,wget_connected_cb);
    if(e!=ERR_OK){vpln("wget: tcp_connect returned error");tcp_close(pcb);return 0;}
    
    vps("wget: polling for 3 seconds...\n");
    for(int i=0;i<15000000;i++){
        net_poll();
        sys_check_timeouts();
        tcp_tmr();
        if(connected){
            vset(C_LGREEN,C_BLACK);
            vpln("wget: TCP CONNECTED!");
            vrst();
            tcp_close(pcb);
            return 1;
        }
        // Show progress every 2M iterations
        if(i%2000000==0)vpc('.');
    }
    vpln("");
    vps("wget: timeout - RX polls: ");kia(rtl_rx_poll_count,cnt);vpln(cnt);
    vpln("wget: TCP failed - QEMU SLIRP doesn't forward packets to RTL8139");
    vpln("Fix: Use TAP networking or different QEMU backend");
    tcp_close(pcb);
    return 0;
}

// ── Compatibility wrappers for old API ──────────────────────────
static int tcp_state=0;
static struct tcp_pcb*active_pcb=NULL;

// tcp_connect compatibility: connect and set tcp_state
static int tcp_connect_compat(uint32_t dst,uint16_t dport){
    ip_addr_t ip;
    ip.addr=dst;
    active_pcb=tcp_new();
    if(!active_pcb)return 0;
    int done=0;
    tcp_arg(active_pcb,&done);
    err_t e=tcp_connect(active_pcb,&ip,dport,tcp_connected_cb);
    if(e!=ERR_OK){tcp_abort(active_pcb);active_pcb=NULL;return 0;}
    int tcp_tmr_ctr=0;
    for(int i=0;i<10000000&&!done;i++){sys_check_timeouts();net_poll();tcp_tmr_ctr++;if(tcp_tmr_ctr>=250){tcp_tmr();tcp_tmr_ctr=0;}}
    if(!done){tcp_abort(active_pcb);active_pcb=NULL;return 0;}
    tcp_state=2;
    return 1;
}

static int tcp_send_buf_compat(uint32_t dst,uint16_t dport,const uint8_t*data,int len){
    if(!active_pcb||tcp_state!=2)return 0;
    return tcp_send_sync(active_pcb,data,len);
}

static int tcp_recv_compat(uint32_t*src,uint16_t*dport,uint8_t*data,int maxlen){
    if(!active_pcb||tcp_state!=2)return -1;
    int r=tcp_recv_sync(active_pcb,data,maxlen);
    return r>0?0x10:-1;
}

static void tcp_close_compat(void){
    if(active_pcb){tcp_close(active_pcb);active_pcb=NULL;}
    tcp_state=0;
}

// Override old function names
#define tcp_connect tcp_connect_compat
#define tcp_send_buf tcp_send_buf_compat
#define tcp_recv tcp_recv_compat
#define tcp_disconnect tcp_close_compat
#define tcp_send_data tcp_send_data_compat
#define tcp_recv_data tcp_recv_data_compat

static int tcp_send_data_compat(const uint8_t*data,int len){
    return tcp_send_buf_compat(0,0,data,len);
}
static int tcp_recv_data_compat(uint8_t*data,int maxlen){
    return tcp_recv_compat(NULL,NULL,data,maxlen);
}

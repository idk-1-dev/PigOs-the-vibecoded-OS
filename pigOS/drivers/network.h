#pragma once
#include <stdint.h>
#include "../drivers/vga.h"
#include "../kernel/mem.h"

// Fake network stack skeleton - RTL8139 detection
#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

static inline uint32_t pci_read(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t off){
    uint32_t addr=(1u<<31)|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(off&0xFC);
    uint32_t v; __asm__ volatile("outl %0,%1"::"a"(addr),"Nd"((uint16_t)PCI_ADDR));
    __asm__ volatile("inl %1,%0":"=a"(v):"Nd"((uint16_t)PCI_DATA));
    return v;
}

static int net_detected=0;
static char net_mac[18]="DE:AD:BE:EF:00:01";
static char net_ip[16]="192.168.1.100";

static void net_init(){
    // Scan PCI for RTL8139 (0x10EC:0x8139) or virtio-net (0x1AF4:0x1000)
    for(int b=0;b<4&&!net_detected;b++){
        for(int d=0;d<32&&!net_detected;d++){
            uint32_t v=pci_read(b,d,0,0);
            if(v==0xFFFFFFFF)continue;
            uint16_t vid=v&0xFFFF, did=(v>>16)&0xFFFF;
            if((vid==0x10EC&&did==0x8139)||(vid==0x1AF4&&did==0x1000)){
                net_detected=1;
            }
        }
    }
}

static void cmd_ifconfig(const char*args){
    (void)args;
    if(!net_detected){ vset(C_YELLOW,C_BLACK); vpln("ifconfig: no NIC detected (no RTL8139/virtio-net found)"); vset(C_WHITE,C_BLACK); }
    vset(C_LCYAN,C_BLACK); vpln("lo:"); vset(C_WHITE,C_BLACK);
    vpln("    inet 127.0.0.1  netmask 255.0.0.0  up loopback");
    vpln("    RX: 0  TX: 0  errors: 0");
    if(net_detected){
        vset(C_LCYAN,C_BLACK); vpln("eth0:"); vset(C_WHITE,C_BLACK);
        vps("    inet "); vpln(net_ip);
        vps("    HWaddr "); vpln(net_mac);
        vpln("    RX: 0  TX: 0  errors: 0");
    }
}

static void cmd_ping(const char*host){
    if(!host||!*host){vpln("usage: ping <host>");return;}
    vps("PING "); vps(host); vpln(" (0.0.0.0) 56 bytes");
    if(!net_detected){vset(C_LRED,C_BLACK);vpln("ping: network unavailable - no NIC detected");vset(C_WHITE,C_BLACK);return;}
    for(int i=1;i<=4;i++){
        vps("64 bytes from "); vps(host); vps(": icmp_seq="); char b[8]; kia(i,b); vps(b); vpln(" ttl=64 time=<unknown>");
        for(volatile int j=0;j<20000000;j++);
    }
}

static void cmd_netstat(){
    vset(C_LCYAN,C_BLACK); vpln("Proto  Recv-Q  Send-Q  Local           Foreign         State"); vset(C_WHITE,C_BLACK);
    vpln("tcp         0       0  0.0.0.0:80      0.0.0.0:*       LISTEN");
    vpln("tcp         0       0  127.0.0.1:22    0.0.0.0:*       LISTEN");
    if(!net_detected){ vset(C_DGREY,C_BLACK); vpln("[no NIC - all sockets simulated]"); vset(C_WHITE,C_BLACK); }
}

static void cmd_wget(const char*url){
    if(!url||!*url){vpln("usage: pigget <url>");return;}
    vps("pigget: connecting to "); vpln(url);
    if(!net_detected){vset(C_LRED,C_BLACK);vpln("pigget: no network interface available");vset(C_WHITE,C_BLACK);return;}
    vpln("pigget: [network driver not fully implemented - no TCP stack yet]");
}

// Fake SSL info
static void cmd_openssl(const char*args){
    if(!args||!*args){
        vset(C_LCYAN,C_BLACK); vpln("pigssl v1.0 - pigOS SSL/TLS subsystem"); vset(C_WHITE,C_BLACK);
        vpln("Commands: pigssl version | pigssl enc | pigssl genrsa | pigssl x509");
        return;
    }
    if(!ksc(args,"version")){ vpln("pigssl 1.0.0 (pigOS built-in)"); vpln("OpenSSL-compatible API layer"); vpln("Ciphers: AES-256-GCM, ChaCha20-Poly1305 [simulated]"); return; }
    if(!ksc(args,"genrsa")){ vpln("Generating RSA-2048 key..."); vpln("......++++"); vpln("..........++++"); vpln("[pigssl: RSA generation simulated - no math coprocessor]"); return; }
    vpln("[pigssl: command queued - full TLS requires network stack]");
}

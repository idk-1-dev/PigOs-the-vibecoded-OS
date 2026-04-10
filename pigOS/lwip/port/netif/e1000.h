#pragma once
// pigOS e1000 driver for lwIP - QEMU's e1000 (82540EM) using MMIO
#include "lwip/port/arch/cc.h"
#include "lwip/port/lwipopts.h"
#include "lwip/src/include/lwip/netif.h"
#include "lwip/src/include/lwip/pbuf.h"
#include "lwip/src/include/lwip/err.h"
#include "lwip/src/include/lwip/def.h"
#include "lwip/src/include/lwip/etharp.h"
#include <stdint.h>

#define E1000_RX_SZ 8192
#define E1000_TX_SZ 4096
#define E1000_MAX_DESC 32

// Transmit descriptor (16 bytes)
struct e1000_tx_desc {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
};

// Receive descriptor (16 bytes)  
struct e1000_rx_desc {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
};

static void* e1000_mmio=0;
static uint8_t e1000_mac[6]={0x52,0x54,0x00,0x12,0x34,0x56};
static int e1000_hw_ok=0;
static int e1000_tx_idx=0;
static int e1000_rx_idx=0;

static inline uint32_t e1000_read32(uint32_t off){ return *(volatile uint32_t*)(e1000_mmio+off); }
static inline void e1000_write32(uint32_t off, uint32_t v){ *(volatile uint32_t*)(e1000_mmio+off)=v; }

static err_t e1000_output(struct netif*netif, struct pbuf*p){
    if(!e1000_hw_ok||!e1000_mmio) return ERR_IF;
    
    struct e1000_tx_desc* txd=(struct e1000_tx_desc*)(0x03020000);
    int idx=e1000_tx_idx;
    
    // Wait for descriptor to be free
    for(int wait=0; wait<100000; wait++){
        if(!(txd[idx].status & 0x01)) break;
    }
    
    // Copy packet data
    int off=0;
    for(struct pbuf*q=p; q!=NULL; q=q->next){
        int c=q->len;
        if(off+c>1514) c=1514-off;
        for(int i=0;i<c;i++) ((uint8_t*)0x03020000)[idx*16+16+off+i]=((uint8_t*)q->payload)[i];
        off+=c;
    }
    
    txd[idx].addr = 0x03020000 + idx*16 + 16;
    txd[idx].length = off;
    txd[idx].cmd = 0x09; // EOP | IFCS
    txd[idx].status = 0;
    
    e1000_write32(0x3818, idx); // TDT
    e1000_tx_idx = (e1000_tx_idx + 1) & 0x1F;
    
    return ERR_OK;
}

static err_t e1000_input(struct pbuf*p, struct netif*netif){
    if(!e1000_hw_ok||!e1000_mmio) return ERR_VAL;
    
    struct e1000_rx_desc* rxd=(struct e1000_rx_desc*)(0x03000000);
    int count=0;
    
    while(count++<16){
        int idx=e1000_rx_idx;
        if(!(rxd[idx].status & 0x01)) break;
        
        uint16_t len=rxd[idx].length;
        if(len>0 && len<=1518){
            struct pbuf* pb = pbuf_alloc(PBUF_LINK, len, PBUF_POOL);
            if(pb){
                uint8_t* data = (uint8_t*)(0x03000000 + idx*16 + 16);
                for(int i=0;i<len;i++) ((uint8_t*)pb->payload)[i]=data[i];
                netif->input(pb, netif);
            }
        }
        
        rxd[idx].status = 0;
        e1000_write32(0x3808, (e1000_rx_idx+1)&0x1F); // RDT
        e1000_rx_idx = (e1000_rx_idx + 1) & 0x1F;
    }
    return ERR_OK;
}

static err_t e1000_init(struct netif*netif){
    netif->name[0]='e'; netif->name[1]='t';
    netif->output = etharp_output;
    netif->linkoutput = e1000_output;
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    for(int i=0;i<6;i++) netif->hwaddr[i] = e1000_mac[i];
    netif->hwaddr_len = 6;
    return ERR_OK;
}

static int e1000_hw_init(void){
    // Scan PCI for e1000 (8086:1008 = 82540EM)
    for(int bus=0; bus<4; bus++){
        for(int dev=0; dev<32; dev++){
            uint32_t id = pci_r32(bus,dev,0,0);
            if(id == 0x10088086 || id == 0x10098086){  // 82540EM or 82541ER
                uint32_t bar0 = pci_r32(bus,dev,0,0x10);
                if(bar0 & 1){
                    e1000_mmio = (void*)(bar0 & ~1);
                    goto found;
                }
            }
        }
    }
    // Also check common QEMU location
    e1000_mmio = (void*)0xFE000000;
found:
    if(!e1000_mmio) return 0;
    
    // Initialize e1000 - simplified for QEMU
    e1000_write32(0x0000, 0x00000000); // Device Control - reset
    for(volatile int i=0;i<10000;i++);
    
    // Disable interrupts
    e1000_write32(0x00D0, 0);
    e1000_write32(0x00D8, 0);
    
    // Setup transmit
    e1000_write32(0x3800, 0x02020000);  // TXRAL, TXRAH (MAC)
    e1000_write32(0x3804, 0x80000000);
    e1000_write32(0x3808, 0x03020000); // TX descriptor base
    e1000_write32(0x3810, E1000_MAX_DESC * 16); // TDLEN
    e1000_write32(0x3818, 0); // TDH, TDT
    e1000_write32(0x0400, 0x00040000); // TCTL: enable, pad short packets
    e1000_write32(0x0404, 0x00000010); // TIPG
    
    // Setup receive
    e1000_write32(0x2800, 0x02020000); // RXRAL, RXRAH (MAC)
    e1000_write32(0x2804, 0x80000000);
    e1000_write32(0x2808, 0x03000000); // RX descriptor base
    e1000_write32(0x2810, E1000_MAX_DESC * 16); // RDLEN
    e1000_write32(0x2814, 0); // RDH
    e1000_write32(0x2818, E1000_MAX_DESC - 1); // RDT
    e1000_write32(0x2600, 0x00000001); // RCTL: enable
    e1000_write32(0x2604, 0x00000000); // RFCTL
    
    // Enable receiver and transmitter
    e1000_write32(0x0000, 0x00000004 | 0x00000002); // RST | LR | ME | SLU
    
    e1000_hw_ok = 1;
    e1000_tx_idx = 0;
    e1000_rx_idx = 0;
    
    return 1;
}

static int e1000_detected(void){ return e1000_hw_ok; }
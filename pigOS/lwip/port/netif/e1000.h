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

#define E1000_MAX_DESC 32
#define E1000_BUF_SZ   2048

#define E1000_TX_DESC_PHYS 0x03000000
#define E1000_RX_DESC_PHYS 0x03001000
#define E1000_TX_BUF_PHYS  0x03010000
#define E1000_RX_BUF_PHYS  0x03020000

#define E1000_REG_CTRL   0x0000
#define E1000_REG_STATUS 0x0008
#define E1000_REG_IMC    0x00D8

#define E1000_STATUS_LU  (1u<<1)

#define E1000_REG_RCTL   0x0100
#define E1000_REG_TCTL   0x0400
#define E1000_REG_TIPG   0x0410

#define E1000_REG_RDBAL  0x2800
#define E1000_REG_RDBAH  0x2804
#define E1000_REG_RDLEN  0x2808
#define E1000_REG_RDH    0x2810
#define E1000_REG_RDT    0x2818

#define E1000_REG_TDBAL  0x3800
#define E1000_REG_TDBAH  0x3804
#define E1000_REG_TDLEN  0x3808
#define E1000_REG_TDH    0x3810
#define E1000_REG_TDT    0x3818

#define E1000_TCTL_EN    (1u<<1)
#define E1000_TCTL_PSP   (1u<<3)
#define E1000_RCTL_EN    (1u<<1)
#define E1000_RCTL_BAM   (1u<<15)
#define E1000_RCTL_SECRC (1u<<26)

#define E1000_TX_CMD_EOP  (1u<<0)
#define E1000_TX_CMD_IFCS (1u<<1)
#define E1000_TX_CMD_RS   (1u<<3)

#define E1000_TX_ST_DD    (1u<<0)
#define E1000_RX_ST_DD    (1u<<0)

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

static volatile uint8_t* e1000_mmio=0;
static uint8_t e1000_mac[6]={0x52,0x54,0x00,0x12,0x34,0x56};
static int e1000_hw_ok=0;
static int e1000_tx_idx=0;
static int e1000_rx_idx=0;

static inline uint32_t e1000_read32(uint32_t off){ return *(volatile uint32_t*)(e1000_mmio+off); }
static inline void e1000_write32(uint32_t off, uint32_t v){ *(volatile uint32_t*)(e1000_mmio+off)=v; }

static inline struct e1000_tx_desc* e1000_tx_desc_ring(void){ return (struct e1000_tx_desc*)(uintptr_t)E1000_TX_DESC_PHYS; }
static inline struct e1000_rx_desc* e1000_rx_desc_ring(void){ return (struct e1000_rx_desc*)(uintptr_t)E1000_RX_DESC_PHYS; }
static inline uint8_t* e1000_tx_buf(int idx){ return (uint8_t*)(uintptr_t)(E1000_TX_BUF_PHYS + (idx * E1000_BUF_SZ)); }
static inline uint8_t* e1000_rx_buf(int idx){ return (uint8_t*)(uintptr_t)(E1000_RX_BUF_PHYS + (idx * E1000_BUF_SZ)); }

static err_t e1000_output(struct netif*netif, struct pbuf*p){
    (void)netif;
    if(!e1000_hw_ok||!e1000_mmio) return ERR_IF;

    struct e1000_tx_desc* txd=e1000_tx_desc_ring();
    int idx=e1000_tx_idx;

    if(!(txd[idx].status & E1000_TX_ST_DD)) return ERR_BUF;

    int off=0;
    for(struct pbuf*q=p; q!=NULL; q=q->next){
        int c=q->len;
        if(off + c > 1518) c = 1518 - off;
        for(int i=0;i<c;i++) e1000_tx_buf(idx)[off+i] = ((uint8_t*)q->payload)[i];
        off+=c;
        if(off >= 1518) break;
    }

    txd[idx].addr = (uint64_t)(uintptr_t)e1000_tx_buf(idx);
    txd[idx].length = off;
    txd[idx].cmd = E1000_TX_CMD_EOP | E1000_TX_CMD_IFCS | E1000_TX_CMD_RS;
    txd[idx].status = 0;

    e1000_tx_idx = (e1000_tx_idx + 1) % E1000_MAX_DESC;
    e1000_write32(E1000_REG_TDT, (uint32_t)e1000_tx_idx);

    return ERR_OK;
}

static void e1000_poll(struct netif*netif){
    if(!e1000_hw_ok || !e1000_mmio) return;

    struct e1000_rx_desc* rxd=e1000_rx_desc_ring();
    for(int count=0; count<16; count++){
        int idx=e1000_rx_idx;
        if(!(rxd[idx].status & E1000_RX_ST_DD)) break;

        uint16_t len=rxd[idx].length;
        if(len>0 && len<=1518){
            struct pbuf* pb = pbuf_alloc(PBUF_LINK, len, PBUF_POOL);
            if(pb){
                int off=0;
                for(struct pbuf* q=pb; q!=NULL && off<len; q=q->next){
                    int c=q->len;
                    if(off + c > len) c = len - off;
                    for(int i=0;i<c;i++) ((uint8_t*)q->payload)[i] = e1000_rx_buf(idx)[off+i];
                    off += c;
                }
                if(netif->input(pb, netif) != ERR_OK) pbuf_free(pb);
            }
        }

        rxd[idx].status = 0;
        e1000_write32(E1000_REG_RDT, (uint32_t)idx);
        e1000_rx_idx = (e1000_rx_idx + 1) % E1000_MAX_DESC;
    }
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
    // Scan PCI for e1000 family (QEMU default: 8086:100e)
    uint8_t found_bus=0, found_dev=0;
    int found=0;
    for(int bus=0; bus<4; bus++){
        for(int dev=0; dev<32; dev++){
            uint32_t id = pci_r32(bus,dev,0,0);
            if(id == 0x10088086 || id == 0x10098086 || id == 0x100E8086){
                uint32_t bar0 = pci_r32(bus,dev,0,0x10);
                if((bar0 & 1u) == 0){
                    e1000_mmio = (volatile uint8_t*)(uintptr_t)(bar0 & ~0x0Fu);
                    found_bus=(uint8_t)bus;
                    found_dev=(uint8_t)dev;
                    found=1;
                    goto found;
                }
            }
        }
    }
    return 0;
found:
    if(!e1000_mmio) return 0;

    if(found){
        uint32_t cmd = pci_r32(found_bus, found_dev, 0, 0x04);
        cmd |= (1u<<1) | (1u<<2);
        pci_w32(found_bus, found_dev, 0, 0x04, cmd);
    }

    e1000_write32(E1000_REG_IMC, 0xFFFFFFFFu);
    for(volatile int i=0;i<10000;i++);

    struct e1000_tx_desc* txd = e1000_tx_desc_ring();
    struct e1000_rx_desc* rxd = e1000_rx_desc_ring();

    for(int i=0;i<E1000_MAX_DESC;i++){
        txd[i].addr = (uint64_t)(uintptr_t)e1000_tx_buf(i);
        txd[i].length = 0;
        txd[i].cmd = 0;
        txd[i].status = E1000_TX_ST_DD;

        rxd[i].addr = (uint64_t)(uintptr_t)e1000_rx_buf(i);
        rxd[i].length = 0;
        rxd[i].status = 0;
        rxd[i].errors = 0;
    }

    e1000_write32(E1000_REG_TDBAL, E1000_TX_DESC_PHYS);
    e1000_write32(E1000_REG_TDBAH, 0);
    e1000_write32(E1000_REG_TDLEN, E1000_MAX_DESC * (uint32_t)sizeof(struct e1000_tx_desc));
    e1000_write32(E1000_REG_TDH, 0);
    e1000_write32(E1000_REG_TDT, 0);

    e1000_write32(E1000_REG_RDBAL, E1000_RX_DESC_PHYS);
    e1000_write32(E1000_REG_RDBAH, 0);
    e1000_write32(E1000_REG_RDLEN, E1000_MAX_DESC * (uint32_t)sizeof(struct e1000_rx_desc));
    e1000_write32(E1000_REG_RDH, 0);
    e1000_write32(E1000_REG_RDT, E1000_MAX_DESC - 1);

    e1000_write32(E1000_REG_TIPG, 0x0060200A);
    e1000_write32(E1000_REG_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP | (0x10u << 4) | (0x40u << 12));
    e1000_write32(E1000_REG_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC);

    e1000_write32(E1000_REG_CTRL, e1000_read32(E1000_REG_CTRL) | (1u<<6));

    // Validate that TX/RX are really enabled after BAR0/MMIO setup.
    if((e1000_read32(E1000_REG_TCTL) & E1000_TCTL_EN) == 0) return 0;
    if((e1000_read32(E1000_REG_RCTL) & E1000_RCTL_EN) == 0) return 0;

    // Wait for Link Up before returning success.
    int link_wait = 2000000;
    while(link_wait-- > 0){
        if(e1000_read32(E1000_REG_STATUS) & E1000_STATUS_LU) break;
    }
    if(link_wait <= 0) return 0;

    e1000_hw_ok = 1;
    e1000_tx_idx = 0;
    e1000_rx_idx = 0;

    return 1;
}

static int e1000_detected(void){ return e1000_hw_ok; }
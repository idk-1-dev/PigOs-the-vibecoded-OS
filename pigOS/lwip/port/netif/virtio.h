#pragma once
// pigOS VirtIO Network Driver - Based on working VibeOS implementation
#include "lwip/port/arch/cc.h"
#include "lwip/port/lwipopts.h"
#include "lwip/src/include/lwip/netif.h"
#include "lwip/src/include/lwip/pbuf.h"
#include "lwip/src/include/lwip/err.h"
#include "lwip/src/include/lwip/etharp.h"
#include <stdint.h>

// VirtIO MMIO (fixed address - standard for QEMU)
#define VIRTIO_MMIO_BASE 0x0a000000
#define VIRTIO_MMIO_STRIDE 0x200

// MMIO Registers
#define VIRTIO_MMIO_MAGIC 0x000
#define VIRTIO_MMIO_VERSION 0x004
#define VIRTIO_MMIO_DEVICE_ID 0x008
#define VIRTIO_MMIO_VENDOR_ID 0x00C
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_MMIO_QUEUE_SEL 0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034
#define VIRTIO_MMIO_QUEUE_NUM 0x038
#define VIRTIO_MMIO_QUEUE_READY 0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064
#define VIRTIO_MMIO_STATUS 0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW 0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH 0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW 0x0a0
#define VIRTIO_MMIO_QUEUE_USED_HIGH 0x0a4
#define VIRTIO_MMIO_CONFIG 0x100

// Status bits
#define VIRTIO_STATUS_ACK 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEATURES_OK 8

// Device types
#define VIRTIO_DEV_NET 1

// Net features
#define VIRTIO_NET_F_MAC (1 << 5)

// VirtIO net header
typedef struct __attribute__((packed)) {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
} virtio_net_hdr_t;

// Virtqueue structures
typedef struct __attribute__((packed)) {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} virtq_desc_t;

typedef struct __attribute__((packed)) {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[16];
} virtq_avail_t;

typedef struct __attribute__((packed)) {
    uint32_t id;
    uint32_t len;
} virtq_used_elem_t;

typedef struct __attribute__((packed)) {
    uint16_t flags;
    uint16_t idx;
    virtq_used_elem_t ring[16];
} virtq_used_t;

#define QUEUE_SIZE 16
#define DESC_F_NEXT 1
#define DESC_F_WRITE 2

// Driver state
static volatile uint32_t *virtio_mmio = NULL;
static int virtio_device_index = -1;
static uint8_t virtio_mac[6] = {0};
static int virtio_hw_ok = 0;

// RX queue
static virtq_desc_t *rx_desc = NULL;
static virtq_avail_t *rx_avail = NULL;
static virtq_used_t *rx_used = NULL;
static uint16_t rx_last_used_idx = 0;

// TX queue
static virtq_desc_t *tx_desc = NULL;
static virtq_avail_t *tx_avail = NULL;
static virtq_used_t *tx_used = NULL;

// Buffers
static uint8_t rx_queue_mem[4096] __attribute__((aligned(4096)));
static uint8_t tx_queue_mem[4096] __attribute__((aligned(4096)));

typedef struct __attribute__((aligned(16))) {
    virtio_net_hdr_t hdr;
    uint8_t data[1514];
} rx_buffer_t;

static rx_buffer_t rx_buffers[QUEUE_SIZE];

typedef struct __attribute__((aligned(16))) {
    virtio_net_hdr_t hdr;
    uint8_t data[1514];
} tx_buffer_t;

static tx_buffer_t tx_buffer;

static void mb(void) {
    __asm__ volatile("" ::: "memory");
}

static uint32_t vread32(volatile uint32_t *addr) {
    uint32_t val = *addr;
    mb();
    return val;
}

static void vwrite32(volatile uint32_t *addr, uint32_t val) {
    mb();
    *addr = val;
    mb();
}

static volatile uint32_t* find_virtio_net(void) {
    for(int i = 0; i < 32; i++) {
        volatile uint32_t *base = (volatile uint32_t *)(VIRTIO_MMIO_BASE + i * VIRTIO_MMIO_STRIDE);
        
        uint32_t magic = vread32(base + VIRTIO_MMIO_MAGIC/4);
        uint32_t device_id = vread32(base + VIRTIO_MMIO_DEVICE_ID/4);
        
        if(magic == 0x74726976 && device_id == VIRTIO_DEV_NET) {
            virtio_device_index = i;
            return base;
        }
    }
    return NULL;
}

static int setup_queue(int queue_idx, uint8_t *queue_mem,
                       virtq_desc_t **desc_out, virtq_avail_t **avail_out, virtq_used_t **used_out) {
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_SEL/4, queue_idx);
    
    uint32_t max_queue = vread32(virtio_mmio + VIRTIO_MMIO_QUEUE_NUM_MAX/4);
    if(max_queue < QUEUE_SIZE) return -1;
    
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_NUM/4, QUEUE_SIZE);
    
    *desc_out = (virtq_desc_t *)queue_mem;
    *avail_out = (virtq_avail_t *)(queue_mem + QUEUE_SIZE * sizeof(virtq_desc_t));
    *used_out = (virtq_used_t *)(queue_mem + 2048);
    
    uint64_t desc_addr = (uint64_t)*desc_out;
    uint64_t avail_addr = (uint64_t)*avail_out;
    uint64_t used_addr = (uint64_t)*used_out;
    
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_DESC_LOW/4, (uint32_t)desc_addr);
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_DESC_HIGH/4, (uint32_t)(desc_addr >> 32));
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_AVAIL_LOW/4, (uint32_t)avail_addr);
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_AVAIL_HIGH/4, (uint32_t)(avail_addr >> 32));
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_USED_LOW/4, (uint32_t)used_addr);
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_USED_HIGH/4, (uint32_t)(used_addr >> 32));
    
    (*avail_out)->flags = 0;
    (*avail_out)->idx = 0;
    
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_READY/4, 1);
    
    return 0;
}

static int virtio_hw_init(void) {
    virtio_mmio = find_virtio_net();
    if(!virtio_mmio) {
        return 0;
    }
    
    // Reset device
    vwrite32(virtio_mmio + VIRTIO_MMIO_STATUS/4, 0);
    for(volatile int i = 0; i < 100000; i++);
    
    // Acknowledge and set driver
    vwrite32(virtio_mmio + VIRTIO_MMIO_STATUS/4, VIRTIO_STATUS_ACK);
    vwrite32(virtio_mmio + VIRTIO_MMIO_STATUS/4, VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER);
    
    // Read device features
    vwrite32(virtio_mmio + VIRTIO_MMIO_DEVICE_FEATURES_SEL/4, 0);
    uint32_t features = vread32(virtio_mmio + VIRTIO_MMIO_DEVICE_FEATURES/4);
    (void)features;
    
    // Accept MAC feature only
    vwrite32(virtio_mmio + VIRTIO_MMIO_DRIVER_FEATURES_SEL/4, 0);
    vwrite32(virtio_mmio + VIRTIO_MMIO_DRIVER_FEATURES/4, VIRTIO_NET_F_MAC);
    
    vwrite32(virtio_mmio + VIRTIO_MMIO_STATUS/4,
            VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);
    
    uint32_t status = vread32(virtio_mmio + VIRTIO_MMIO_STATUS/4);
    if(!(status & VIRTIO_STATUS_FEATURES_OK)) {
        return 0;
    }
    
    // Read MAC
    volatile uint8_t *config = (volatile uint8_t *)virtio_mmio + VIRTIO_MMIO_CONFIG;
    for(int i = 0; i < 6; i++) {
        virtio_mac[i] = config[i];
    }
    
    // Setup RX queue
    if(setup_queue(0, rx_queue_mem, &rx_desc, &rx_avail, &rx_used) < 0) {
        return 0;
    }
    
    // Setup TX queue
    if(setup_queue(1, tx_queue_mem, &tx_desc, &tx_avail, &tx_used) < 0) {
        return 0;
    }
    
    // Pre-populate RX queue
    for(int i = 0; i < QUEUE_SIZE; i++) {
        rx_desc[i].addr = (uint64_t)&rx_buffers[i];
        rx_desc[i].len = sizeof(rx_buffer_t);
        rx_desc[i].flags = DESC_F_WRITE;
        rx_desc[i].next = 0;
        rx_avail->ring[i] = i;
    }
    rx_avail->idx = QUEUE_SIZE;
    mb();
    
    // Set driver OK
    vwrite32(virtio_mmio + VIRTIO_MMIO_STATUS/4,
            VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER |
            VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);
    
    status = vread32(virtio_mmio + VIRTIO_MMIO_STATUS/4);
    if(status & 0x40) {
        return 0;
    }
    
    // Notify device that RX buffers available
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_SEL/4, 0);
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_NOTIFY/4, 0);
    
    virtio_hw_ok = 1;
    return 1;
}

static err_t virtio_output(struct netif*netif, struct pbuf*p){
    (void)netif;
    if(!virtio_hw_ok) return ERR_IF;
    
    extern void*pig_memcpy(void*,const void*,unsigned long);
    extern void*pig_memset(void*,int,unsigned long);
    pig_memset(&tx_buffer.hdr, 0, sizeof(virtio_net_hdr_t));
    
    int off = 0;
    for(struct pbuf* q = p; q != NULL && off < 1514; q = q->next){
        int c = q->len;
        if(off + c > 1514) c = 1514 - off;
        for(int i = 0; i < c; i++) tx_buffer.data[off + i] = ((uint8_t*)q->payload)[i];
        off += c;
    }
    
    tx_desc[0].addr = (uint64_t)&tx_buffer;
    tx_desc[0].len = sizeof(virtio_net_hdr_t) + (uint32_t)off;
    tx_desc[0].flags = 0;
    tx_desc[0].next = 0;
    
    mb();
    uint16_t avail_idx = tx_avail->idx % QUEUE_SIZE;
    tx_avail->ring[avail_idx] = 0;
    mb();
    tx_avail->idx++;
    mb();
    
    uint16_t old_used = tx_used->idx;
    
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_SEL/4, 1);
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_NOTIFY/4, 1);
    
    int timeout = 1000000;
    while(tx_used->idx == old_used && timeout > 0) {
        mb();
        timeout--;
    }
    
    if(timeout == 0) {
        return ERR_BUF;
    }
    
    vwrite32(virtio_mmio + VIRTIO_MMIO_INTERRUPT_ACK/4,
            vread32(virtio_mmio + VIRTIO_MMIO_INTERRUPT_STATUS/4));
    
    return ERR_OK;
}

static void virtio_input(struct netif*netif){
    if(!virtio_hw_ok) return;
    
    mb();
    if(rx_used->idx == rx_last_used_idx) {
        return;
    }
    
    uint16_t used_idx = rx_last_used_idx % QUEUE_SIZE;
    uint32_t desc_idx = rx_used->ring[used_idx].id;
    uint32_t total_len = rx_used->ring[used_idx].len;
    
    rx_last_used_idx++;
    
    rx_buffer_t *rxbuf = &rx_buffers[desc_idx];
    uint32_t frame_len = total_len - sizeof(virtio_net_hdr_t);
    
    if(frame_len > 0 && frame_len <= 1514) {
        struct pbuf *pb = pbuf_alloc(PBUF_LINK, frame_len, PBUF_POOL);
        if(pb) {
            for(int i = 0; i < frame_len; i++) {
                ((uint8_t*)pb->payload)[i] = rxbuf->data[i];
            }
            netif->input(pb, netif);
        }
    }
    
    uint16_t avail_idx = rx_avail->idx % QUEUE_SIZE;
    rx_avail->ring[avail_idx] = desc_idx;
    mb();
    rx_avail->idx++;
    mb();
    
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_SEL/4, 0);
    vwrite32(virtio_mmio + VIRTIO_MMIO_QUEUE_NOTIFY/4, 0);
    
    vwrite32(virtio_mmio + VIRTIO_MMIO_INTERRUPT_ACK/4,
            vread32(virtio_mmio + VIRTIO_MMIO_INTERRUPT_STATUS/4));
}

static err_t virtio_init(struct netif*netif){
    netif->name[0] = 'e'; netif->name[1] = 't';
    netif->output = etharp_output;
    netif->linkoutput = virtio_output;
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    for(int i = 0; i < 6; i++) netif->hwaddr[i] = virtio_mac[i];
    netif->hwaddr_len = 6;
    return ERR_OK;
}
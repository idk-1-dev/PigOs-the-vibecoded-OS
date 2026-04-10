#pragma once
// pigOS RTL8139 netif driver for lwIP
// Freestanding, no libc
#include "lwip/port/arch/cc.h"

#include "lwip/port/lwipopts.h"
#include "lwip/src/include/lwip/netif.h"
#include "lwip/src/include/lwip/pbuf.h"
#include "lwip/src/include/lwip/err.h"
#include "lwip/src/include/lwip/def.h"
#include "lwip/src/include/lwip/etharp.h"
#include <stdint.h>

// Standalone network logging (no kernel dependencies)
#define NET_LOG_MAX 64
typedef struct {
    int tick;
    char event[16];
    char detail[48];
} NetLogEntry;
static NetLogEntry net_log[NET_LOG_MAX];
static int net_log_idx = 0;
static int net_log_count = 0;
static int net_tick = 0;
static void net_log_event(const char* event, const char* detail){
    if(!net_log_count)net_log_count=1;
    int idx = net_log_idx % NET_LOG_MAX;
    net_log_idx++; net_tick++;
    if(net_log_count < NET_LOG_MAX) net_log_count++;
    net_log[idx].tick = net_tick;
    int i=0; while(event[i] && i<15){net_log[idx].event[i]=event[i];i++;}
    net_log[idx].event[i]=0;
    i=0; while(detail[i] && i<47){net_log[idx].detail[i]=detail[i];i++;}
    net_log[idx].detail[i]=0;
}
static void net_log_dump(void){
    extern void vpln(const char*);
    extern void vps(const char*);
    if(net_log_count==0){vpln("[no network events logged yet]");return;}
    for(int i=0;i<net_log_count;i++){
        int idx = (net_log_idx - net_log_count + i) % NET_LOG_MAX;
        if(net_log[idx].event[0]){
            vps("[");vps(net_log[idx].event);vps("] ");vpln(net_log[idx].detail);
        }
    }
}
void rtl_net_log_get(char*out,int max){
    int pos=0;
    for(int i=0;i<net_log_count && pos<max-1;i++){
        int idx = (net_log_idx - net_log_count + i) % NET_LOG_MAX;
        if(net_log[idx].event[0]){
            int j=0;while(net_log[idx].event[j] && pos<max-1){out[pos++]=net_log[idx].event[j++];}
            if(pos<max-1)out[pos++]=' ';
            j=0;while(net_log[idx].detail[j] && pos<max-1){out[pos++]=net_log[idx].detail[j++];}
            if(pos<max-1)out[pos++]='\n';
        }
    }
    out[pos]=0;
}
#define NET_LOG(e,d) net_log_event(e,d)

// RTL8139 port I/O
static inline void rtl_outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline void rtl_outw(uint16_t p,uint16_t v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static inline void rtl_outl(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t  rtl_inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint16_t rtl_inw(uint16_t p){uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint32_t rtl_inl(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint32_t pci_r32(uint8_t b,uint8_t d,uint8_t f,uint8_t r){
    uint32_t a=(1u<<31)|((uint32_t)b<<16)|((uint32_t)d<<11)|((uint32_t)f<<8)|(r&0xFC);
    __asm__ volatile("outl %0,%1"::"a"(a),"Nd"((uint16_t)0xCF8));
    uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"((uint16_t)0xCFC));return v;
}
static inline void pci_w32(uint8_t b,uint8_t d,uint8_t f,uint8_t r,uint32_t v){
    uint32_t a=(1u<<31)|((uint32_t)b<<16)|((uint32_t)d<<11)|((uint32_t)f<<8)|(r&0xFC);
    __asm__ volatile("outl %0,%1"::"a"(a),"Nd"((uint16_t)0xCF8));
    __asm__ volatile("outl %0,%1"::"a"(v),"Nd"((uint16_t)0xCFC));
}

#define RTL_RX_SZ (8*1024)  // 8KB ring buffer (standard RTL8139 C mode)
#define RTL_TX_SZ 1792
// RTL8139 is a 32-bit PCI device - DMA buffers MUST be below 4GB
// Use fixed low physical addresses to avoid DMA truncation
#define RTL_RX_PHYS 0x02000000  // 32MB - safely in low memory
#define RTL_TX_PHYS 0x02010000  // 32MB + 64KB
static inline uint8_t*rtl_rx_buf(void){ return (uint8_t*)RTL_RX_PHYS; }
static inline uint8_t*rtl_tx_buf(int slot){ return (uint8_t*)(RTL_TX_PHYS + slot*RTL_TX_SZ); }
static uint32_t rtl_iobase=0;
static uint8_t rtl_mac[6]={0};
static int rtl_hw_ok=0;
static int rtl_tx_cur=0;
static uint16_t rtl_rx_ptr=0;
static volatile uint32_t rtl_rx_count=0;
static volatile uint32_t rtl_tx_count=0;
static volatile uint32_t rtl_rx_poll_count=0;

static err_t rtl_output(struct netif*netif,struct pbuf*p){
    if(!rtl_hw_ok){NET_LOG("TX","RTL not initialized");return ERR_IF;}
    NET_LOG("TX","starting packet send");
    struct pbuf*q;
    uint8_t buf[1600];
    int off=0;
    for(q=p;q!=NULL;q=q->next){
        int c=q->len;
        if(off+c>1514)c=1514-off;
        for(int i=0;i<c;i++)buf[off+i]=((uint8_t*)q->payload)[i];
        off+=c;
    }
    for(int i=0;i<off;i++)rtl_tx_buf(rtl_tx_cur)[i]=buf[i];
    rtl_outl(rtl_iobase+0x20+rtl_tx_cur*4,(uint32_t)(uint64_t)rtl_tx_buf(rtl_tx_cur));
    rtl_outl(rtl_iobase+0x10+rtl_tx_cur*4,(off&0x1FFF)|0x80000000);
    NET_LOG("TX","waiting for TX completion");
    int t=1000000;
    while(t-->0){uint32_t tsd=rtl_inl(rtl_iobase+0x10+rtl_tx_cur*4);if(!(tsd&0x80000000))break;}
    if(t<=0){NET_LOG("TX","timeout waiting for TX");return ERR_BUF;}
    NET_LOG("TX","packet sent successfully");
    rtl_tx_cur=(rtl_tx_cur+1)&3;
    rtl_tx_count++;
    return ERR_OK;
}

static void rtl_input(struct netif*netif){
    if(!rtl_hw_ok){NET_LOG("RX","RTL not initialized");return;}
    rtl_rx_poll_count++;
    NET_LOG("RX","polling for packets...");
    for(int pkt=0;pkt<16;pkt++){
        uint16_t pos=rtl_rx_ptr;
        uint16_t status=*(volatile uint16_t*)(rtl_rx_buf()+pos);
        if(!(status&0x0001)){
            if(pkt==0)NET_LOG("RX","NO PACKETS - rx status=0x0000 (buffer empty)");
            break;
        }
        NET_LOG("RX","packet found! status=0x0001");
        uint16_t raw_len=*(volatile uint16_t*)(rtl_rx_buf()+pos+2);
        int len=raw_len-4;
        if(len<=0||len>1518){NET_LOG("RX","bad length");break;}
        rtl_rx_count++;
        NET_LOG("RX","processing packet");
        struct pbuf*p=pbuf_alloc(PBUF_LINK,len,PBUF_POOL);
        if(p){
            uint16_t dp=(uint16_t)((pos+4)%RTL_RX_SZ);
            int off=0;
            for(int i=0;i<len;i++){
                ((uint8_t*)p->payload)[off++]=rtl_rx_buf()[(dp+i)%RTL_RX_SZ];
            }
            netif->input(p,netif);
            NET_LOG("RX","packet delivered to lwIP");
        }
        uint16_t next_ptr=(uint16_t)((pos+4+raw_len+3)&~3);
        if(next_ptr>=RTL_RX_SZ)next_ptr-=(uint16_t)RTL_RX_SZ;
        rtl_outw(rtl_iobase+0x38,(uint16_t)(next_ptr-0x10));
        rtl_rx_ptr=next_ptr;
    }
    if(rtl_rx_poll_count%1000==0){
        NET_LOG("RX","poll check");
    }
}

static err_t rtl_init(struct netif*netif){
    netif->name[0]='e';netif->name[1]='t';
    netif->output=etharp_output;
    netif->linkoutput=rtl_output;
    netif->mtu=1500;
    netif->flags=NETIF_FLAG_BROADCAST|NETIF_FLAG_ETHARP|NETIF_FLAG_LINK_UP;
    for(int i=0;i<6;i++)netif->hwaddr[i]=rtl_mac[i];
    netif->hwaddr_len=6;
    return ERR_OK;
}

static int rtl_hw_init(void){
    NET_LOG("RTL","Starting RTL8139 initialization...");
    for(int b=0;b<4;b++)for(int d=0;d<32;d++){
        uint32_t id=pci_r32(b,d,0,0);
        NET_LOG("RTL", "PCI scan");
        if(id==0x813910EC){
            NET_LOG("RTL","Found RTL8139!");
            uint32_t bar=pci_r32(b,d,0,0x10);rtl_iobase=bar&~3;
            NET_LOG("RTL","Enabled bus master");
            pci_w32(b,d,0,4,pci_r32(b,d,0,4)|5);
            goto found;
        }
    }
    NET_LOG("RTL","FAILED: No RTL8139 found");
    return 0;
found:
    NET_LOG("RTL","Resetting chip...");
    // Soft reset
    rtl_outb(rtl_iobase+0x37,0x10);
    int t=10000;while((rtl_inb(rtl_iobase+0x37)&0x10)&&t-->0);
    if(t<=0){NET_LOG("RTL","FAILED: Reset timeout");return 0;}
    NET_LOG("RTL","Reading MAC address...");
    // Read MAC
    for(int i=0;i<6;i++)rtl_mac[i]=rtl_inb(rtl_iobase+i);
    // Clear bit 0 of 0x52 (enable RX/TX)
    rtl_outb(rtl_iobase+0x52,0);
    // Set RX buffer address - use fixed low physical address for 32-bit DMA
    rtl_outl(rtl_iobase+0x30,RTL_RX_PHYS);
    // Set TX descriptors - use fixed low physical addresses
    rtl_outl(rtl_iobase+0x20,RTL_TX_PHYS);
    rtl_outl(rtl_iobase+0x24,RTL_TX_PHYS+RTL_TX_SZ);
    rtl_outl(rtl_iobase+0x28,RTL_TX_PHYS+RTL_TX_SZ*2);
    rtl_outl(rtl_iobase+0x2C,RTL_TX_PHYS+RTL_TX_SZ*3);
    // Set RX config: FULL PROMISCUOUS MODE for SLIRP
    // 0x0000000F = AB(3) + AM(2) + APM(1) + AAP(0) = accept all
    // Add 0x00000800 to enable Accept All Packets (AAP) explicitly
    rtl_outl(rtl_iobase+0x44, 0x00008F0F);  // Full promiscuous - accept EVERYTHING
    // RX config: enable Accept All Packets (bit 8), accept broadcast/multicast/unicast
    rtl_outl(rtl_iobase+0x40,0x03000700);
    // Enable full duplex 100Mbps
    rtl_outb(rtl_iobase+0x63,0x21);
    // Enable interrupts: ROK, TOK
    rtl_outw(rtl_iobase+0x3C,0x0005);
    // Enable TX and RX LAST (after config is set)
    rtl_outb(rtl_iobase+0x37,0x0C);
    // Zero out RX buffer
    for(volatile int i=0;i<RTL_RX_SZ;i+=4) *(volatile uint32_t*)(RTL_RX_PHYS+i)=0;
    NET_LOG("RTL","Hardware initialized OK");
    // Debug
    extern void vps(const char*);extern void vpln(const char*);
    extern void kuia(uint64_t,char*,int);
    char b[16];kuia(RTL_RX_PHYS,b,16);vps("rtl rx_buf=0x");vps(b);vpln("");
    rtl_hw_ok=1;rtl_tx_cur=0;rtl_rx_ptr=0;
    NET_LOG("RTL","RTL8139 ready");
    return 1;
}

// Raw send helper - send a pre-built Ethernet frame
static void rtl_send(const uint8_t*data,int len){
    if(!rtl_hw_ok||len>1514)return;
    int slot=rtl_tx_cur;
    for(int i=0;i<len;i++)rtl_tx_buf(slot)[i]=data[i];
    rtl_outl(rtl_iobase+0x20+slot*4,(uint32_t)RTL_TX_PHYS+slot*RTL_TX_SZ);
    rtl_outl(rtl_iobase+0x10+slot*4,(uint32_t)len&0x1FFF);
    int t=1000000;
    while(t-->0){uint32_t tsd=rtl_inl(rtl_iobase+0x10+slot*4);if(!(tsd&0x2000))break;}
    rtl_tx_cur=(rtl_tx_cur+1)&3;
}

// Raw recv helper - read one pending frame
static int rtl_recv(uint8_t*buf,int max){
    if(!rtl_hw_ok)return 0;
    uint16_t pos=rtl_rx_ptr;
    uint16_t status=*(volatile uint16_t*)(RTL_RX_PHYS+pos);
    if(!(status&0x0001))return 0;
    uint16_t len=*(volatile uint16_t*)(RTL_RX_PHYS+pos+2);
    len-=4;
    if(len<=0||len>1518||len>max)return 0;
    uint16_t dp=(uint16_t)((pos+4)%RTL_RX_SZ);
    for(int i=0;i<len;i++)buf[i]=rtl_rx_buf()[(dp+i)%RTL_RX_SZ];
    uint16_t next_ptr=(uint16_t)((pos+len+4+3)&~3);
    if(next_ptr>=RTL_RX_SZ)next_ptr-=(uint16_t)RTL_RX_SZ;
    rtl_outw(rtl_iobase+0x38,next_ptr-0x10);
    rtl_rx_ptr=next_ptr;
    return len;
}

#pragma once
// pigOS lwIP configuration - minimal freestanding setup
// No RTOS, no sockets, raw API only

#define NO_SYS                          1
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0

#define LWIP_IPV6                       0
#define LWIP_ARP                        1
#define ETHARP_SUPPORT_STATIC_ENTRIES   0

#define IP_REASSEMBLY                   0
#define IP_FRAG                         0

#define LWIP_UDP                        1
#define LWIP_TCP                        1
#define LWIP_RAW                        1
#define TCP_MSS                         1460
#define TCP_SND_BUF                     (4*TCP_MSS)
#define TCP_WND                         (4*TCP_MSS)
#define TCP_QUEUE_OOSEQ                 0

#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        16384
#define MEMP_NUM_PBUF                   32
#define MEMP_NUM_UDP_PCB                4
#define MEMP_NUM_TCP_PCB                8
#define MEMP_NUM_TCP_PCB_LISTEN         4
#define MEMP_NUM_TCP_SEG                32
#define PBUF_POOL_SIZE                  16
#define PBUF_POOL_BUFSIZE               1536

#define LWIP_DHCP                       1
#define IP_ACCEPT_LINK_LAYER_ADDRESSING  1
#define LWIP_DNS                        1
#define LWIP_ICMP                       1

#define LWIP_NETIF_STATUS_CALLBACK      0
#define LWIP_NETIF_LINK_CALLBACK        0

#define LWIP_STATS                      0
#define LWIP_PROVIDE_ERRNO              0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_GEN_UDP                0
#define CHECKSUM_GEN_IP                 0

#define LWIP_DEBUG                      0

#define IP_ACCEPT_LINK_LAYER_ADDRESSING  1
#define LWIP_IP_ACCEPT_UDP_PORT(dst_port) ((dst_port) == PP_NTOHS(68))

#define LWIP_RAND()                     ({ static uint32_t _s=0xDEAD1337; _s^=_s<<13; _s^=_s>>17; _s^=_s<<5; _s; })

#pragma once
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS 1
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
#define MEM_SIZE                        (32 * 1024)
#define MEMP_NUM_PBUF                   32
#define MEMP_NUM_UDP_PCB                4
#define MEMP_NUM_TCP_PCB                8
#define MEMP_NUM_TCP_PCB_LISTEN         4
#define MEMP_NUM_TCP_SEG                32
#define PBUF_POOL_SIZE                  32
#define PBUF_POOL_BUFSIZE               1536

#define LWIP_DHCP                       1
#define IP_ACCEPT_LINK_LAYER_ADDRESSING  1
#define LWIP_DNS                        1
#define LWIP_ICMP                       1

// Enable 127.0.0.1 support and use polling loopback mode for NO_SYS.
#define LWIP_NETIF_LOOPBACK             1
#define LWIP_HAVE_LOOPIF                0
#define LWIP_NETIF_LOOPBACK_MULTITHREADING 0

#define LWIP_NETIF_STATUS_CALLBACK      0
#define LWIP_NETIF_LINK_CALLBACK        0

#define LWIP_STATS                      0
#define LWIP_PROVIDE_ERRNO              0
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_GEN_ICMP6              1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1
#define CHECKSUM_CHECK_ICMP6            1

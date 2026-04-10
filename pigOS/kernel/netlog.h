#pragma once
// pigOS Network Debug Logger - Records all network events to file
#include "kernel/mem.h"
#include "kernel/pigfs.h"

#define NET_LOG_MAX 256
#define NET_LOG_FILE "/var/log/netdebug.log"

typedef struct {
    int tick;
    char event[64];
    char detail[96];
} NetLogEntry;

static NetLogEntry net_log[NET_LOG_MAX];
static int net_log_idx = 0;
static int net_log_count = 0;

static void net_log_init(void){
    net_log_idx = 0;
    net_log_count = 0;
    for(int i=0;i<NET_LOG_MAX;i++)net_log[i].event[0]=0;
}

static void net_log_event(const char* event, const char* detail){
    int idx = net_log_idx % NET_LOG_MAX;
    net_log_idx++;
    if(net_log_count < NET_LOG_MAX) net_log_count++;
    
    net_log[idx].tick = net_log_idx;
    
    int i=0;
    while(event[i] && i<63){net_log[idx].event[i]=event[i];i++;}
    net_log[idx].event[i]=0;
    
    i=0;
    while(detail[i] && i<95){net_log[idx].detail[i]=detail[i];i++;}
    net_log[idx].detail[i]=0;
}

static void net_log_dump(void){
    vpln("=== Network Debug Log ===");
    for(int i=0;i<net_log_count;i++){
        int idx = (net_log_idx - net_log_count + i) % NET_LOG_MAX;
        if(net_log[idx].event[0]){
            vps("[");vps(net_log[idx].event);vps("] ");
            vpln(net_log[idx].detail);
        }
    }
}

static void net_log_save(void){
    extern void memfs_set_content(const char*path,const char*content);
    char buffer[4096];
    int pos=0;
    
    kcp(buffer+pos,"=== pigOS Network Debug Log ===\n");
    pos=kcp(buffer,"")-buffer;
    
    for(int i=0;i<net_log_count && pos<4000;i++){
        int idx = (net_log_idx - net_log_count + i) % NET_LOG_MAX;
        if(net_log[idx].event[0]){
            kcat(buffer+pos,"[");
            kcat(buffer+pos,net_log[idx].event);
            kcat(buffer+pos,"] ");
            kcat(buffer+pos,net_log[idx].detail);
            kcat(buffer+pos,"\n");
        }
    }
    
    memfs_set_content(NET_LOG_FILE, buffer);
}

#define NET_LOG(e,d) net_log_event(e,d)
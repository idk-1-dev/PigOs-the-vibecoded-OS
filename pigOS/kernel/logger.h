#pragma once
// pigOS Runtime Logging System - No VGA dependencies
#include <stdint.h>
#include <stddef.h>

#define LOG_BUFFER_SIZE 8192
#define LOG_ENTRIES 64

typedef struct {
    int seq;
    char level[8];
    char msg[128];
} LogEntry;

static LogEntry log_buffer[LOG_ENTRIES];
static int log_count = 0;
static int log_index = 0;
static int log_initialized = 0;

static void log_init(void){
    log_count = 0;
    log_index = 0;
    log_initialized = 1;
    for(int i=0;i<LOG_ENTRIES;i++){
        log_buffer[i].msg[0]=0;
    }
}

static void log_add(const char* level, const char* msg){
    if(!log_initialized) log_init();
    
    int idx = log_index % LOG_ENTRIES;
    log_index++;
    if(log_count < LOG_ENTRIES) log_count++;
    
    log_buffer[idx].seq = log_index;
    
    int i=0;
    while(level[i] && i<7){log_buffer[idx].level[i]=level[i];i++;}
    log_buffer[idx].level[i]=0;
    
    i=0;
    while(msg[i] && i<127){log_buffer[idx].msg[i]=msg[i];i++;}
    log_buffer[idx].msg[i]=0;
}

static int log_get_count(void){ return log_count; }

static void log_get_entry(int n, char* out, int max){
    if(n < 0 || n >= log_count) {out[0]=0;return;}
    int idx = (log_index - log_count + n) % LOG_ENTRIES;
    int pos=0;
    // seq
    char sb[8];
    // level
    for(int i=0;log_buffer[idx].level[i] && pos<max-1;){out[pos++]=log_buffer[idx].level[i++];}
    if(pos<max-1)out[pos++]=' ';
    // msg
    for(int i=0;log_buffer[idx].msg[i] && pos<max-1;){out[pos++]=log_buffer[idx].msg[i++];}
    out[pos]=0;
}

static void log_clear(void){
    log_count=0;
    log_index=0;
}

#define LOG_INFO(msg) log_add("INFO", msg)
#define LOG_ERROR(msg) log_add("ERROR", msg)
#define LOG_DEBUG(msg) log_add("DEBUG", msg)
#define LOG_NET(msg) log_add("NET", msg)
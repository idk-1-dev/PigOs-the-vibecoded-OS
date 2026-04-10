#pragma once
// pigOS Memory Allocator - mem.h
#include <stddef.h>
#include <stdint.h>
#define HEAP_BASE 0x400000UL
#define HEAP_SIZE (4*1024*1024)  // 4MB heap
#define HEAP_LIMIT (HEAP_BASE + HEAP_SIZE)
static uint8_t* _hp=(uint8_t*)HEAP_BASE;
static void* km(size_t n){
    n=(n+15)&~15u;
    if(_hp + n > (uint8_t*)HEAP_LIMIT) return NULL;
    void*p=(void*)_hp;_hp+=n;return p;
}
static void kf(void*p){(void)p;}
static void* kms(void*d,int v,size_t n){uint8_t*p=d;while(n--)*p++=(uint8_t)v;return d;}
static void* kmc(void*d,const void*s,size_t n){uint8_t*p=d;const uint8_t*q=s;while(n--)*p++=*q++;return d;}
static size_t ksl(const char*s){size_t n=0;while(*s++)n++;return n;}
static int ksc(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return(uint8_t)*a-(uint8_t)*b;}
static int ksnc(const char*a,const char*b,size_t n){while(n--&&*a&&*a==*b){a++;b++;}if(n==(size_t)-1)return 0;return(uint8_t)*a-(uint8_t)*b;}
static char* kcp(char*d,const char*s){char*r=d;while((*d++=*s++));return r;}
static char* kcat(char*d,const char*s){char*r=d;while(*d)d++;while((*d++=*s++));return r;}
static void kia(int n,char*b){if(!n){b[0]='0';b[1]=0;return;}char t[20];int i=0,neg=0;if(n<0){neg=1;n=-n;}while(n){t[i++]='0'+n%10;n/=10;}if(neg)t[i++]='-';for(int j=0;j<i;j++)b[j]=t[i-1-j];b[i]=0;}
static void kuia(uint64_t n,char*b,int base){if(!n){b[0]='0';b[1]=0;return;}const char*hx="0123456789abcdef";char t[20];int i=0;while(n){t[i++]=hx[n%base];n/=base;}for(int j=0;j<i;j++)b[j]=t[i-1-j];b[i]=0;}

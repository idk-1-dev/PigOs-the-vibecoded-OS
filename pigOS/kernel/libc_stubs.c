// pigOS libc stubs for lwIP
#include <stdint.h>
#include <stddef.h>
#include "libc_stubs.h"

void*pig_memcpy(void*dst,const void*src,unsigned long n){
    unsigned char*d=(unsigned char*)dst;const unsigned char*s=(const unsigned char*)src;
    for(unsigned long i=0;i<n;i++)d[i]=s[i];
    return dst;
}

void*pig_memmove(void*dst,const void*src,unsigned long n){
    unsigned char*d=(unsigned char*)dst;const unsigned char*s=(const unsigned char*)src;
    if(d<s){for(unsigned long i=0;i<n;i++)d[i]=s[i];}
    else{for(unsigned long i=n;i>0;i--)d[i-1]=s[i-1];}
    return dst;
}

void*pig_memset(void*s,int c,unsigned long n){
    unsigned char*p=(unsigned char*)s;
    for(unsigned long i=0;i<n;i++)p[i]=(unsigned char)c;
    return s;
}

int pig_memcmp(const void*s1,const void*s2,unsigned long n){
    const unsigned char*p1=(const unsigned char*)s1;
    const unsigned char*p2=(const unsigned char*)s2;
    for(unsigned long i=0;i<n;i++){
        if(p1[i]!=p2[i])return p1[i]<p2[i]?-1:1;
    }
    return 0;
}

unsigned long pig_strlen(const char*s){
    unsigned long n=0;while(s[n])n++;return n;
}

int pig_strncmp(const char*s1,const char*s2,unsigned long n){
    for(unsigned long i=0;i<n;i++){
        if(s1[i]!=s2[i])return(unsigned char)s1[i]<(unsigned char)s2[i]?-1:1;
        if(s1[i]==0)return 0;
    }
    return 0;
}

int pig_strcmp(const char*s1,const char*s2){
    while(*s1 && (*s1 == *s2)){ s1++; s2++; }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

char* pig_strcpy(char*dst,const char*src){
    char*ret=dst;
    while((*dst++=*src++));
    return ret;
}

long pig_strtol(const char*nptr,char**endptr,int base){
    long val=0;int neg=0;const char*p=nptr;
    while(*p==' '||*p=='\t')p++;
    if(*p=='-'){neg=1;p++;}else if(*p=='+')p++;
    while(*p>='0'&&*p<='9'){val=val*base+(*p-'0');p++;}
    if(endptr)*endptr=(char*)p;
    return neg?-val:val;
}

uint32_t sys_now(void){
    static uint64_t tsc_start=0;
    static int init=0;
    if(!init){
        uint32_t lo,hi;
        __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
        tsc_start = lo | ((uint64_t)hi << 32);
        init=1;
        return 0;
    }
    uint32_t lo,hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    uint64_t tsc = lo | ((uint64_t)hi << 32);
    uint64_t diff = tsc - tsc_start;
    return (uint32_t)(diff / 2400000);
}

static const unsigned short _pig_ctype_b[256]={0};
static const unsigned short*_pig_ctype_b_ptr=_pig_ctype_b;
const unsigned short**__ctype_b_loc(void){return &_pig_ctype_b_ptr;}

static const int _pig_ctype_tolower[256]={0};
static const int*_pig_ctype_tolower_ptr=_pig_ctype_tolower;
const int**__ctype_tolower_loc(void){return &_pig_ctype_tolower_ptr;}

// Standard libc symbol wrappers used by lwIP and other objects.
void* memcpy(void*dst,const void*src,size_t n){ return pig_memcpy(dst,src,(unsigned long)n); }
void* memmove(void*dst,const void*src,size_t n){ return pig_memmove(dst,src,(unsigned long)n); }
void* memset(void*s,int c,size_t n){ return pig_memset(s,c,(unsigned long)n); }
int memcmp(const void*s1,const void*s2,size_t n){ return pig_memcmp(s1,s2,(unsigned long)n); }
size_t strlen(const char*s){ return (size_t)pig_strlen(s); }
int strncmp(const char*s1,const char*s2,size_t n){ return pig_strncmp(s1,s2,(unsigned long)n); }
int strcmp(const char*s1,const char*s2){ return pig_strcmp(s1,s2); }
char* strcpy(char*dst,const char*src){ return pig_strcpy(dst,src); }
long strtol(const char*nptr,char**endptr,int base){ return pig_strtol(nptr,endptr,base); }

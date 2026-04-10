#pragma once
// pigOS libc stubs for lwIP
// These provide the minimal C library functions that lwIP needs
#include <stdint.h>

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

long pig_strtol(const char*nptr,char**endptr,int base){
    long val=0;int neg=0;const char*p=nptr;
    while(*p==' '||*p=='\t')p++;
    if(*p=='-'){neg=1;p++;}else if(*p=='+')p++;
    while(*p>='0'&&*p<='9'){val=val*base+(*p-'0');p++;}
    if(endptr)*endptr=(char*)p;
    return neg?-val:val;
}

// sys_now - return milliseconds (dummy for bare metal)
uint32_t sys_now(void){
    static uint32_t t=0;
    t+=100;
    return t;
}

// ctype stubs
static const unsigned short _pig_ctype_b[256]={0};
static const unsigned short*_pig_ctype_b_ptr=_pig_ctype_b;
const unsigned short**__ctype_b_loc(void){return &_pig_ctype_b_ptr;}

static const int _pig_ctype_tolower[256]={0};
static const int*_pig_ctype_tolower_ptr=_pig_ctype_tolower;
const int**__ctype_tolower_loc(void){return &_pig_ctype_tolower_ptr;}

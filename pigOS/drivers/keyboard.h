#pragma once
#include <stdint.h>

static inline uint8_t inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}

#define KEY_UP    (-1)
#define KEY_DOWN  (-2)
#define KEY_LEFT  (-3)
#define KEY_RIGHT (-4)
#define KEY_F1    (-5)
#define KEY_F2    (-6)
#define KEY_F3    (-7)
#define KEY_F4    (-8)
#define KEY_F5    (-9)
#define KEY_F6    (-10)
#define KEY_F7    (-11)
#define KEY_F8    (-12)
#define KEY_DEL   (-13)
#define KEY_HOME  (-14)
#define KEY_END   (-15)
#define KEY_PGUP  (-16)
#define KEY_PGDN  (-17)

static const char sc_lo[]="\0\x1b""1234567890-=\b\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 ";
static const char sc_hi[]="\0\x1b""!@#$%^&*()_+\b\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0|ZXCVBNM<>?\0*\0 ";

#define KB_BUF 512
static int kb_buf[KB_BUF];
static volatile int kb_h=0,kb_t=0;
static int kb_shift=0,kb_ctrl=0,kb_caps=0,kb_ext=0;

static void kb_push(int c){int n=(kb_h+1)%KB_BUF;if(n!=kb_t){kb_buf[kb_h]=c;kb_h=n;}}

static void kb_poll(){
    while(inb(0x64)&1){
        uint8_t sc=inb(0x60);
        if(sc==0xE0){kb_ext=1;continue;}
        if(kb_ext){
            kb_ext=0;
            if(!(sc&0x80)){
                if(sc==0x48)kb_push(KEY_UP);
                else if(sc==0x50)kb_push(KEY_DOWN);
                else if(sc==0x4B)kb_push(KEY_LEFT);
                else if(sc==0x4D)kb_push(KEY_RIGHT);
                else if(sc==0x53)kb_push(KEY_DEL);
                else if(sc==0x47)kb_push(KEY_HOME);
                else if(sc==0x4F)kb_push(KEY_END);
                else if(sc==0x49)kb_push(KEY_PGUP);
                else if(sc==0x51)kb_push(KEY_PGDN);
            }
            continue;
        }
        if(sc==0x2A||sc==0x36){kb_shift=1;continue;}
        if(sc==0xAA||sc==0xB6){kb_shift=0;continue;}
        if(sc==0x1D){kb_ctrl=1;continue;}
        if(sc==0x9D){kb_ctrl=0;continue;}
        if(sc==0x3A){kb_caps=!kb_caps;continue;}
        if(sc==0x3B){kb_push(KEY_F1);continue;}
        if(sc==0x3C){kb_push(KEY_F2);continue;}
        if(sc==0x3D){kb_push(KEY_F3);continue;}
        if(sc==0x3E){kb_push(KEY_F4);continue;}
        if(sc==0x3F){kb_push(KEY_F5);continue;}
        if(sc==0x40){kb_push(KEY_F6);continue;}
        if(sc==0x41){kb_push(KEY_F7);continue;}
        if(sc==0x42){kb_push(KEY_F8);continue;}
        if(sc&0x80)continue;
        if(sc<(int)sizeof(sc_lo)){
            int sh=kb_shift^(kb_caps&&sc_lo[sc]>='a'&&sc_lo[sc]<='z');
            char c=sh?sc_hi[sc]:sc_lo[sc];
            if(kb_ctrl&&c>='a'&&c<='z')c=c-'a'+1;
            if(c)kb_push((int)(uint8_t)c);
        }
    }
}
static int kb_avail(){kb_poll();return kb_h!=kb_t;}
static int kb_get(){while(!kb_avail());int c=kb_buf[kb_t];kb_t=(kb_t+1)%KB_BUF;return c;}

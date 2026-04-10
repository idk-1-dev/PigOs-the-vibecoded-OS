#pragma once
// pigOS PS/2 Controller - Mouse ENABLED
#include <stdint.h>
#include "../vga/vga.h"

static inline uint8_t ps2_inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void ps2_outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}

#define KEY_UP    (-1)
#define KEY_DOWN  (-2)
#define KEY_LEFT  (-3)
#define KEY_RIGHT (-4)
#define KEY_F1 (-5)
#define KEY_F2 (-6)
#define KEY_F3 (-7)
#define KEY_F4 (-8)
#define KEY_F5 (-9)
#define KEY_F6 (-10)
#define KEY_F7 (-11)
#define KEY_F8 (-12)
#define KEY_F10 (-13)
#define KEY_DEL (-14)
#define KEY_HOME (-15)
#define KEY_END (-16)
#define KEY_PGUP (-17)
#define KEY_PGDN (-18)

static const char sc_lo[]="\0\x1b""1234567890-=\b\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 ";
static const char sc_hi[]="\0\x1b""!@#$%^&*()_+\b\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0|ZXCVBNM<>?\0*\0 ";

static int kb_shift=0,kb_ctrl=0,kb_alt=0,kb_caps=0,kb_ext=0;
#define KB_BUF 256
static int kb_buf[KB_BUF];
static volatile int kb_h=0,kb_t=0;
static void kb_push(int c){int n=(kb_h+1)%KB_BUF;if(n!=kb_t){kb_buf[kb_h]=c;kb_h=n;}}

// Mouse state - initialized
static int mouse_x=40,mouse_y=12;
static int mouse_btn_l=0,mouse_btn_r=0;
static int mouse_btn_l_prev=0;
static int mouse_enabled=0;
static int mouse_wm_mode=0;

// Save/restore cursor background (like VibeOS)
#define CUR_W 12
#define CUR_H 12
static uint16_t mouse_bg[CUR_H][CUR_W];
static int mouse_drawn=0;

static void mouse_init(){
    mouse_x = 40;
    mouse_y = 12;
    mouse_btn_l = 0;
    mouse_btn_r = 0;
    mouse_btn_l_prev = 0;
    mouse_drawn = 0;
    
    ps2_outb(0x64, 0xA8);
    ps2_outb(0x64, 0x20);
    uint8_t ctrl = (ps2_inb(0x60) | 0x02) & 0xF7;
    ps2_outb(0x64, 0x60);
    ps2_outb(0x60, ctrl);
    
    ps2_outb(0x64, 0xD4);
    ps2_outb(0x60, 0xF4);
    (void)ps2_inb(0x60);
    
    for(int i = 0; i < 10; i++) ps2_inb(0x60);
    
    mouse_enabled = 1;
}

static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[3];

static void mouse_poll(){
    if(!mouse_enabled) return;
    
    while(ps2_inb(0x64) & 0x01){
        uint8_t b = ps2_inb(0x60);
        
        if(!(b & 0x08)){
            mouse_cycle = 0;
            continue;
        }
        
        mouse_packet[mouse_cycle++] = b;
        
        if(mouse_cycle >= 3){
            mouse_cycle = 0;
            
            int8_t dx = (int8_t)mouse_packet[1];
            int8_t dy = (int8_t)mouse_packet[2];
            
            mouse_btn_l_prev = mouse_btn_l;
            mouse_btn_l = (mouse_packet[0] & 0x01) ? 1 : 0;
            mouse_btn_r = (mouse_packet[0] & 0x02) ? 1 : 0;
            
            mouse_x += dx;
            mouse_y -= dy;
            
            // Mouse wheel scrolling
            // Check for wheel event (buttons 4/5 = wheel up/down)
            int wheel = 0;
            if(mouse_packet[0] & 0x10) wheel = 1;   // Wheel up
            if(mouse_packet[0] & 0x20) wheel = -1;  // Wheel down
            
            if(wheel != 0){
                extern void vscroll_up();
                extern void vscroll_down();
                if(wheel > 0) vscroll_up();
                else vscroll_down();
            }
            
            if(mouse_x < 1) mouse_x = 1;
            if(mouse_y < 1) mouse_y = 1;
            if(mouse_x >= VGA_W-1) mouse_x = VGA_W-2;
            if(mouse_y >= VGA_H-1) mouse_y = VGA_H-2;
        }
    }
}

static inline void mouse_draw(){
    if(!mouse_enabled) return;
    if(mouse_x < 1 || mouse_y < 1 || mouse_x >= VGA_W-1 || mouse_y >= VGA_H-1) return;
    
    // Simple block cursor - very visible white block
    int mx = mouse_x;
    int my = mouse_y;
    
    // Save background
    mouse_bg[0][0] = VGA_MEM[my * VGA_W + mx];
    mouse_drawn = 1;
    
    // Draw white block cursor with black text
    VGA_MEM[my * VGA_W + mx] = 0x0F20;  // White bg, black ' ' character
}

static inline void mouse_erase(){
    if(!mouse_enabled || !mouse_drawn) return;
    int mx = mouse_x;
    int my = mouse_y;
    VGA_MEM[my * VGA_W + mx] = mouse_bg[0][0];
    mouse_drawn = 0;
}

static inline void mouse_draw_cursor(){
    mouse_erase();
    mouse_draw();
}

static void ps2_poll(){
    uint8_t st;
    while((st=ps2_inb(0x64))&1){
        uint8_t dat=ps2_inb(0x60);
        
        // Mouse data?
        if(st&0x20){
            // Mouse already handled above
            continue;
        }
        
        // Keyboard data
        uint8_t sc=dat;
        if(sc==0xE0){kb_ext=1;continue;}
        if(kb_ext){
            kb_ext=0;
            if(!(sc&0x80)){
                switch(sc){
                    case 0x48:kb_push(KEY_UP);break;
                    case 0x50:kb_push(KEY_DOWN);break;
                    case 0x4B:kb_push(KEY_LEFT);break;
                    case 0x4D:kb_push(KEY_RIGHT);break;
                    case 0x53:kb_push(KEY_DEL);break;
                    case 0x47:kb_push(KEY_HOME);break;
                    case 0x4F:kb_push(KEY_END);break;
                    case 0x49:kb_push(KEY_PGUP);break;
                    case 0x51:kb_push(KEY_PGDN);break;
                }
            }
            continue;
        }
        if(sc==0x2A||sc==0x36){kb_shift=1;continue;}
        if(sc==0xAA||sc==0xB6){kb_shift=0;continue;}
        if(sc==0x1D){kb_ctrl=1;continue;}
        if(sc==0x9D){kb_ctrl=0;continue;}
        if(sc==0x38){kb_alt=1;continue;}
        if(sc==0xB8){kb_alt=0;continue;}
        if(sc==0x3A){kb_caps=!kb_caps;continue;}
        if(sc==0x3B){kb_push(KEY_F1);continue;}
        if(sc==0x3C){kb_push(KEY_F2);continue;}
        if(sc==0x3D){kb_push(KEY_F3);continue;}
        if(sc==0x3E){kb_push(KEY_F4);continue;}
        if(sc==0x3F){kb_push(KEY_F5);continue;}
        if(sc==0x40){kb_push(KEY_F6);continue;}
        if(sc==0x41){kb_push(KEY_F7);continue;}
        if(sc==0x44){kb_push(KEY_F10);continue;}
        if(sc&0x80)continue;
        if(sc<(int)sizeof(sc_lo)){
            int sh=kb_shift^(kb_caps&&sc_lo[sc]>='a'&&sc_lo[sc]<='z');
            char c=sh?sc_hi[sc]:sc_lo[sc];
            if(kb_ctrl&&c>='a'&&c<='z')c=(char)(c-'a'+1);
            if(c)kb_push((int)(uint8_t)c);
        }
    }
}

static int kb_avail(){ps2_poll();return kb_h!=kb_t;}
static int kb_get(){while(!kb_avail());int c=kb_buf[kb_t];kb_t=(kb_t+1)%KB_BUF;return c;}

#pragma once
// ================================================================
// pigOS VGA Driver - vga.h
// 80x25 text mode, CP437, hardware cursor, color support
// ================================================================
#include <stdint.h>
#include <stddef.h>

#define VGA_W    80
#define VGA_H    25
#define VGA_MEM  ((volatile uint16_t*)0xB8000)
#define VGA_PORT_IDX  0x3D4
#define VGA_PORT_DATA 0x3D5

// ── CP437 box drawing characters ─────────────────────────────
#define CH_TL    0xC9u  // ╔
#define CH_TR    0xBBu  // ╗
#define CH_BL    0xC8u  // ╚
#define CH_BR    0xBCu  // ╝
#define CH_H     0xCDu  // ═
#define CH_V     0xBAu  // ║
#define CH_ML    0xCCu  // ╠
#define CH_MR    0xB9u  // ╣
#define CH_STL   0xDAu  // ┌
#define CH_STR   0xBFu  // ┐
#define CH_SBL   0xC0u  // └
#define CH_SBR   0xD9u  // ┘
#define CH_SH    0xC4u  // ─
#define CH_SV    0xB3u  // │
#define CH_BLOCK 0xDBu  // █
#define CH_DARK  0xB2u  // ▓
#define CH_MED   0xB1u  // ▒
#define CH_LGT   0xB0u  // ░
#define CH_HALF_U 0xDFu // ▀
#define CH_HALF_L 0xDCu // ▄
#define CH_DOT   0xF9u  // ·
#define CH_DIAM  0x04u  // ♦
#define CH_BULL  0x07u  // •
#define CH_SMILE 0x01u  // ☺
#define CH_SUN   0x0Fu  // ✾
#define CH_CHECK 0xFBu  // √
#define CH_ARROW_R 0x10u // ►
#define CH_ARROW_L 0x11u // ◄
#define CH_ARROW_U 0x1Eu // ▲
#define CH_ARROW_D 0x1Fu // ▼
#define CH_SPADE 0x06u  // ♠
#define CH_CLUB  0x05u  // ♣

// ── Colors ───────────────────────────────────────────────────
typedef enum {
    C_BLACK=0,C_BLUE=1,C_GREEN=2,C_CYAN=3,
    C_RED=4,C_MAG=5,C_BROWN=6,C_LGREY=7,
    C_DGREY=8,C_LBLUE=9,C_LGREEN=10,C_LCYAN=11,
    C_LRED=12,C_LMAG=13,C_YELLOW=14,C_WHITE=15,
} color_t;

#define COL(fg,bg)  ((uint8_t)((fg)|((bg)<<4)))
#define ENT(c,col)  ((uint16_t)(uint8_t)(c)|((uint16_t)(uint8_t)(col)<<8))

// ── True color support (24-bit) ─────────────────────────────
static char vga_truecolor_buf[32];
static void vset_truecolor(uint8_t r, uint8_t g, uint8_t b){
    int n = 0;
    vga_truecolor_buf[n++] = '\x1b';
    vga_truecolor_buf[n++] = '[';
    vga_truecolor_buf[n++] = '3';
    vga_truecolor_buf[n++] = '8';
    vga_truecolor_buf[n++] = ';';
    vga_truecolor_buf[n++] = '2';
    vga_truecolor_buf[n++] = ';';
    // R
    vga_truecolor_buf[n++] = '0' + (r / 100);
    vga_truecolor_buf[n++] = '0' + ((r / 10) % 10);
    vga_truecolor_buf[n++] = '0' + (r % 10);
    vga_truecolor_buf[n++] = ';';
    // G
    vga_truecolor_buf[n++] = '0' + (g / 100);
    vga_truecolor_buf[n++] = '0' + ((g / 10) % 10);
    vga_truecolor_buf[n++] = '0' + (g % 10);
    vga_truecolor_buf[n++] = ';';
    // B
    vga_truecolor_buf[n++] = '0' + (b / 100);
    vga_truecolor_buf[n++] = '0' + ((b / 10) % 10);
    vga_truecolor_buf[n++] = '0' + (b % 10);
    vga_truecolor_buf[n++] = 'm';
    vga_truecolor_buf[n++] = 0;
}
static void vrst_truecolor(){
    vga_truecolor_buf[0] = '\x1b';
    vga_truecolor_buf[1] = '[';
    vga_truecolor_buf[2] = '0';
    vga_truecolor_buf[3] = 'm';
    vga_truecolor_buf[4] = 0;
}

// True color constants matching your HEX colors
#define BODY_COLOR_R 252
#define BODY_COLOR_G 205
#define BODY_COLOR_B 176
// Close VGA equivalents (fallback to if no true color)
// #FCCDB0 = closest to C_WHITE or C_LGREY
// For pig body, use C_LGREY as closest 16-color VGA match

// ── State ────────────────────────────────────────────────────
static int vx=0, vy=0;
static uint8_t vc=COL(C_WHITE,C_BLACK);
static int vga_cursor_enabled=1;

// Scrollback buffer - stores scrolled-off lines
#define SCROLLBACK_LINES 100
static uint16_t scrollback[SCROLLBACK_LINES][VGA_W];
static int scrollback_pos = 0;
static int scrollback_count = 0;

// ── I/O ──────────────────────────────────────────────────────
static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline void _outb(uint16_t p,uint8_t v){
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}

static inline uint8_t _inb_vga(uint16_t p){
    uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v;
}


// ── Hardware cursor (CRITICAL for Hyprland/QEMU refresh) ─────
static void vga_update_hw_cursor(){
    if(!vga_cursor_enabled)return;
    uint16_t pos=(uint16_t)(vy*VGA_W+vx);
    _outb(VGA_PORT_IDX,0x0F); _outb(VGA_PORT_DATA,(uint8_t)(pos&0xFF));
    _outb(VGA_PORT_IDX,0x0E); _outb(VGA_PORT_DATA,(uint8_t)(pos>>8));
}

static void vga_enable_cursor(uint8_t start,uint8_t end){
    _outb(VGA_PORT_IDX,0x0A); _outb(VGA_PORT_DATA,(uint8_t)((_inb_vga(VGA_PORT_DATA)&0xC0)|start));
    _outb(VGA_PORT_IDX,0x0B); _outb(VGA_PORT_DATA,(uint8_t)((_inb_vga(VGA_PORT_DATA)&0xE0)|end));
    vga_cursor_enabled=1;
}

// inb helper just for VGA

static void vga_disable_cursor(){
    _outb(VGA_PORT_IDX,0x0A); _outb(VGA_PORT_DATA,0x20);
    vga_cursor_enabled=0;
}

// ── Core ops ─────────────────────────────────────────────────
static void vga_init(){
    vx=0; vy=0; vc=COL(C_WHITE,C_BLACK);
    for(int r=0;r<VGA_H;r++)
        for(int c=0;c<VGA_W;c++)
            VGA_MEM[r*VGA_W+c]=ENT(' ',COL(C_WHITE,C_BLACK));
    vga_enable_cursor(13,15);
    vga_update_hw_cursor();
}

static void vset(color_t fg,color_t bg){ vc=COL(fg,bg); }
static void vrst(){ vc=COL(C_WHITE,C_BLACK); }

static void vat(uint8_t c,uint8_t col,int x,int y){
    if(x<0||x>=VGA_W||y<0||y>=VGA_H)return;
    VGA_MEM[y*VGA_W+x]=ENT(c,col);
}

static uint16_t vget(int x,int y){
    if(x<0||x>=VGA_W||y<0||y>=VGA_H)return 0;
    return VGA_MEM[y*VGA_W+x];
}

static void vfill(int x,int y,int w,int h,uint8_t ch,uint8_t col){
    for(int r=0;r<h;r++)
        for(int c=0;c<w;c++)
            vat(ch,col,x+c,y+r);
}

static void vscroll(){
    // Save line that scrolled off to scrollback buffer
    for(int c=0;c<VGA_W;c++){
        scrollback[scrollback_pos][c] = VGA_MEM[(0)*VGA_W+c];
    }
    scrollback_pos = (scrollback_pos + 1) % SCROLLBACK_LINES;
    if(scrollback_count < SCROLLBACK_LINES) scrollback_count++;
    
    for(int r=1;r<VGA_H;r++)
        for(int c=0;c<VGA_W;c++)
            VGA_MEM[(r-1)*VGA_W+c]=VGA_MEM[r*VGA_W+c];
    for(int c=0;c<VGA_W;c++)
        VGA_MEM[(VGA_H-1)*VGA_W+c]=ENT(' ',vc);
    vy=VGA_H-1;
}

// Scroll up - show previous line from scrollback
static void vscroll_up(){
    if(scrollback_count == 0) return;
    // Shift all lines down
    for(int r=VGA_H-1;r>0;r--)
        for(int c=0;c<VGA_W;c++)
            VGA_MEM[r*VGA_W+c]=VGA_MEM[(r-1)*VGA_W+c];
    // Get oldest line from scrollback
    int sb_idx = (scrollback_pos - scrollback_count + SCROLLBACK_LINES) % SCROLLBACK_LINES;
    if(sb_idx < 0) sb_idx += SCROLLBACK_LINES;
    for(int c=0;c<VGA_W;c++)
        VGA_MEM[0*VGA_W+c] = scrollback[sb_idx][c];
    vy = 0;
    vga_update_hw_cursor();
}

// Scroll down - discard current top line  
static void vscroll_down(){
    if(scrollback_count == 0) return;
    // Shift lines up
    for(int r=0;r<VGA_H-1;r++)
        for(int c=0;c<VGA_W;c++)
            VGA_MEM[r*VGA_W+c]=VGA_MEM[(r+1)*VGA_W+c];
    // Clear bottom line
    for(int c=0;c<VGA_W;c++)
        VGA_MEM[(VGA_H-1)*VGA_W+c]=ENT(' ',vc);
    vy = VGA_H-1;
    vga_update_hw_cursor();
}

// Keyboard scroll: PageUp/PageDown
static void vscroll_pageup(){
    for(int s=0; s<10 && scrollback_count>0; s++) vscroll_up();
}

static void vscroll_pagedown(){
    for(int s=0; s<10 && scrollback_count>0; s++) vscroll_down();
}

// Move cursor without writing (for arrow key navigation)
static inline void vcursor_left(){
    if(vx>0){vx--;}
    else if(vy>0){vy--;vx=VGA_W-1;}
    vga_update_hw_cursor();
}
static inline void vcursor_right(){
    if(vx<VGA_W-1){vx++;}
    else{vx=0;if(vy<VGA_H-1)vy++;}
    vga_update_hw_cursor();
}
// Write char at current position without advancing cursor (for overlay)
static inline void vat_cur(uint8_t c){ vat(c,vc,vx,vy); }

static void vpc(uint8_t c){
    if(c=='\n'){ vx=0; if(++vy>=VGA_H)vscroll(); }
    else if(c=='\r'){ vx=0; }
    else if(c=='\b'){ if(vx>0){vx--;vat(' ',vc,vx,vy);} }
    else if(c=='\t'){ vx=(vx+8)&~7; if(vx>=VGA_W){vx=0;if(++vy>=VGA_H)vscroll();} }
    else {
        vat(c,vc,vx,vy);
        if(++vx>=VGA_W){vx=0;if(++vy>=VGA_H)vscroll();}
    }
    vga_update_hw_cursor(); // always update after each char
}

// Pipe support globals
static char pipe_buf[4096];
static int pipe_buf_len=0;
static int pipe_active=0;

static void vps(const char*s){
    if(pipe_active){
        while(*s&&pipe_buf_len<4095){
            pipe_buf[pipe_buf_len++]=*s;
            s++;
        }
        return;
    }
    while(*s)vpc((uint8_t)*s++);
}
static void vpln(const char*s){
    vps(s);
    if(!pipe_active) vpc('\n');
    else if(pipe_buf_len<4095) pipe_buf[pipe_buf_len++]='\n';
}
static void vclear(){ vga_init(); }

static void vstr(int x,int y,uint8_t col,const char*s){
    while(*s){ vat((uint8_t)*s,col,x,y); s++; x++; }
}

static int vstrlen(const char*s){ int n=0; while(s[n])n++; return n; }

// ── Box drawing ───────────────────────────────────────────────
// Windows 10-style window with gradient title bar
static void vbox_win10(int x,int y,int w,int h,uint8_t title_bg,const char*title,int focused){
    // Shadow (1px offset right+bottom)
    uint8_t shadow=COL(C_DGREY,C_DGREY);
    for(int r=1;r<h;r++) vat(CH_BLOCK,shadow,x+w,y+r);
    for(int c=1;c<w+1;c++) vat(CH_BLOCK,shadow,x+c,y+h);

    // Clear interior
    vfill(x,y,w,h,' ',COL(C_WHITE,C_BLACK));

    // Title bar (3 rows tall, gradient effect)
    uint8_t tb1=focused?COL(C_WHITE,C_BLUE):COL(C_LGREY,C_DGREY);
    uint8_t tb2=focused?COL(C_LCYAN,C_BLUE):COL(C_DGREY,C_DGREY);
    vfill(x,y,w,1,' ',tb1);      // top stripe
    vfill(x,y+1,w,1,' ',tb1);    // title row
    vfill(x,y+2,w,1,' ',tb2);    // bottom stripe of titlebar

    // Title text
    if(title){
        int tl=vstrlen(title);
        int tx=x+1;
        for(int i=0;i<tl&&tx+i<x+w-4;i++)
            vat((uint8_t)title[i],tb1,tx+i,y+1);
    }

    // Win10 control buttons: [─] [□] [✕]
    if(w>=8){
        vat(CH_SH,  COL(C_WHITE,C_DGREY), x+w-7, y+1); // minimize
        vat(' ',    COL(C_WHITE,C_DGREY), x+w-6, y+1);
        vat(CH_STL, COL(C_WHITE,C_DGREY), x+w-5, y+1); // maximize
        vat(CH_STR, COL(C_WHITE,C_DGREY), x+w-4, y+1);
        vat(' ',    COL(C_WHITE,C_DGREY), x+w-3, y+1);
        vat('X',    COL(C_WHITE,C_LRED),  x+w-2, y+1); // close
        vat(' ',    COL(C_WHITE,C_LRED),  x+w-1, y+1);
    }

    // Border (very thin, Win10-style)
    uint8_t bc=focused?COL(C_LBLUE,C_LBLUE):COL(C_DGREY,C_DGREY);
    // Left
    for(int r=y;r<y+h;r++) vat(' ',bc,x-1,r);
    // Right
    for(int r=y;r<y+h;r++) vat(' ',bc,x+w,r);
    // Bottom
    for(int c=x;c<x+w;c++) vat(' ',bc,c,y+h);
}

static void vbox(int x,int y,int w,int h,uint8_t bc,uint8_t tc,const char*title){
    vfill(x+1,y+1,w-2,h-2,' ',COL(C_WHITE,C_BLACK));
    vat(CH_TL,bc,x,y);
    for(int i=1;i<w-1;i++) vat(CH_H,bc,x+i,y);
    vat(CH_TR,bc,x+w-1,y);
    vfill(x+1,y+1,w-2,1,' ',tc);
    if(title){ int tl=vstrlen(title); int tx=x+1+(w-2-tl)/2; vstr(tx,y+1,tc,title); }
    vat('[',tc,x+w-4,y+1); vat('X',COL(C_WHITE,C_RED),x+w-3,y+1); vat(']',tc,x+w-2,y+1);
    vat(CH_ML,bc,x,y+2);
    for(int i=1;i<w-1;i++) vat(CH_H,bc,x+i,y+2);
    vat(CH_MR,bc,x+w-1,y+2);
    for(int r=3;r<h-1;r++){ vat(CH_V,bc,x,y+r); vat(CH_V,bc,x+w-1,y+r); }
    vat(CH_BL,bc,x,y+h-1);
    for(int i=1;i<w-1;i++) vat(CH_H,bc,x+i,y+h-1);
    vat(CH_BR,bc,x+w-1,y+h-1);
}

static int vitoa(int n,char*buf){
    if(!n){buf[0]='0';buf[1]=0;return 1;}
    char t[16];int i=0,neg=0;
    if(n<0){neg=1;n=-n;}
    while(n){t[i++]='0'+n%10;n/=10;}
    if(neg)t[i++]='-';
    for(int j=0;j<i;j++)buf[j]=t[i-1-j];buf[i]=0;return i;
}

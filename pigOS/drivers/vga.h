#pragma once
#include <stdint.h>
#include <stddef.h>

#define VGA_W 80
#define VGA_H 25
#define VGA_MEM ((volatile uint16_t*)0xB8000)

// CP437 box drawing
#define CH_TL  0xC9u  // ╔
#define CH_TR  0xBBu  // ╗
#define CH_BL  0xC8u  // ╚
#define CH_BR  0xBCu  // ╝
#define CH_H   0xCDu  // ═
#define CH_V   0xBAu  // ║
#define CH_ML  0xCCu  // ╠
#define CH_MR  0xB9u  // ╣
#define CH_STL 0xDAu  // ┌
#define CH_STR 0xBFu  // ┐
#define CH_SBL 0xC0u  // └
#define CH_SBR 0xD9u  // ┘
#define CH_SH  0xC4u  // ─
#define CH_SV  0xB3u  // │
#define CH_FULL 0xDBu // █
#define CH_DARK 0xB2u // ▓
#define CH_MED  0xB1u // ▒
#define CH_LGT  0xB0u // ░
#define CH_ARROW_R 0x10u
#define CH_ARROW_L 0x11u
#define CH_DIAMOND 0x04u
#define CH_BULLET  0x07u
#define CH_SMILE   0x01u
#define CH_SPADE   0x06u
#define CH_CHECK   0xFBu
#define CH_DEGREE  0xF8u

typedef enum {
    C_BLACK=0,C_BLUE=1,C_GREEN=2,C_CYAN=3,
    C_RED=4,C_MAG=5,C_BROWN=6,C_LGREY=7,
    C_DGREY=8,C_LBLUE=9,C_LGREEN=10,C_LCYAN=11,
    C_LRED=12,C_LMAG=13,C_YELLOW=14,C_WHITE=15,
} color_t;

#define COL(fg,bg) ((uint8_t)((fg)|((bg)<<4)))
#define ENT(c,col) ((uint16_t)(uint8_t)(c)|((uint16_t)(uint8_t)(col)<<8))

static size_t vx=0,vy=0;
static uint8_t vc=COL(C_LGREEN,C_BLACK);

static void vga_init(){
    vx=0;vy=0;vc=COL(C_WHITE,C_BLACK);
    for(size_t r=0;r<VGA_H;r++)
        for(size_t c=0;c<VGA_W;c++)
            VGA_MEM[r*VGA_W+c]=ENT(' ',COL(C_WHITE,C_BLACK));
}
static void vset(color_t fg,color_t bg){vc=COL(fg,bg);}
static void vat(uint8_t c,uint8_t col,int x,int y){
    if(x<0||x>=VGA_W||y<0||y>=VGA_H)return;
    VGA_MEM[y*VGA_W+x]=ENT(c,col);
}
static void vfill(int x,int y,int w,int h,uint8_t ch,uint8_t col){
    for(int r=0;r<h;r++)for(int c=0;c<w;c++)vat(ch,col,x+c,y+r);
}
static void vscroll(){
    for(size_t r=1;r<VGA_H;r++)
        for(size_t c=0;c<VGA_W;c++)
            VGA_MEM[(r-1)*VGA_W+c]=VGA_MEM[r*VGA_W+c];
    for(size_t c=0;c<VGA_W;c++)
        VGA_MEM[(VGA_H-1)*VGA_W+c]=ENT(' ',vc);
    vy=VGA_H-1;
}
static void vpc(uint8_t c){
    if(c=='\n'){vx=0;if(++vy==VGA_H)vscroll();return;}
    if(c=='\r'){vx=0;return;}
    if(c=='\b'){if(vx>0){vx--;vat(' ',vc,vx,vy);}return;}
    if(c=='\t'){vx=(vx+8)&~7u;if(vx>=VGA_W){vx=0;if(++vy==VGA_H)vscroll();}return;}
    vat(c,vc,vx,vy);
    if(++vx==VGA_W){vx=0;if(++vy==VGA_H)vscroll();}
}
static void vps(const char*s){while(*s)vpc((uint8_t)*s++);}
static void vpln(const char*s){vps(s);vpc('\n');}
static void vclear(){vga_init();vx=0;vy=0;}
static void vstr(int x,int y,uint8_t col,const char*s){while(*s){vat((uint8_t)*s,col,x,y);s++;x++;}}
static int vitoa(int n,char*buf){
    if(!n){buf[0]='0';buf[1]=0;return 1;}
    char t[16];int i=0,neg=0;
    if(n<0){neg=1;n=-n;}
    while(n){t[i++]='0'+n%10;n/=10;}
    if(neg)t[i++]='-';
    for(int j=0;j<i;j++)buf[j]=t[i-1-j];buf[i]=0;return i;
}
static void vbox(int x,int y,int w,int h,uint8_t bc,uint8_t tc,const char*title){
    vfill(x+1,y+1,w-2,h-2,' ',COL(C_WHITE,C_BLACK));
    vat(CH_TL,bc,x,y);
    for(int i=1;i<w-1;i++)vat(CH_H,bc,x+i,y);
    vat(CH_TR,bc,x+w-1,y);
    vfill(x+1,y+1,w-2,1,' ',tc);
    if(title){int tl=0;const char*t=title;while(t[tl])tl++;int tx=x+1+(w-2-tl)/2;vstr(tx,y+1,tc,title);}
    vat('[',tc,x+w-4,y+1);
    vat('X',COL(C_WHITE,C_RED),x+w-3,y+1);
    vat(']',tc,x+w-2,y+1);
    vat(CH_ML,bc,x,y+2);
    for(int i=1;i<w-1;i++)vat(CH_H,bc,x+i,y+2);
    vat(CH_MR,bc,x+w-1,y+2);
    for(int r=3;r<h-1;r++){vat(CH_V,bc,x,y+r);vat(CH_V,bc,x+w-1,y+r);}
    vat(CH_BL,bc,x,y+h-1);
    for(int i=1;i<w-1;i++)vat(CH_H,bc,x+i,y+h-1);
    vat(CH_BR,bc,x+w-1,y+h-1);
}

#pragma once
// pigOS Tetris
#include "../../drivers/vga/vga.h"
#include "../../drivers/kbd/keyboard.h"
#include "../../kernel/mem.h"

#define TET_W  10
#define TET_H  20
#define TET_OX 2   // board x offset
#define TET_OY 2   // board y offset

static const int PIECES[7][4][4][2] = {
    // I
    {{{0,1},{1,1},{2,1},{3,1}},{{2,0},{2,1},{2,2},{2,3}},{{0,2},{1,2},{2,2},{3,2}},{{1,0},{1,1},{1,2},{1,3}}},
    // O
    {{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}}},
    // T
    {{{1,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{2,1},{1,2}},{{1,0},{0,1},{1,1},{1,2}}},
    // S
    {{{1,0},{2,0},{0,1},{1,1}},{{1,0},{1,1},{2,1},{2,2}},{{1,1},{2,1},{0,2},{1,2}},{{0,0},{0,1},{1,1},{1,2}}},
    // Z
    {{{0,0},{1,0},{1,1},{2,1}},{{2,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{1,2},{2,2}},{{1,0},{0,1},{1,1},{0,2}}},
    // J
    {{{0,0},{0,1},{1,1},{2,1}},{{1,0},{2,0},{1,1},{1,2}},{{0,1},{1,1},{2,1},{2,2}},{{1,0},{1,1},{0,2},{1,2}}},
    // L
    {{{2,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{1,2},{2,2}},{{0,1},{1,1},{2,1},{0,2}},{{0,0},{1,0},{1,1},{1,2}}},
};

static const uint8_t PIECE_COLORS[7]={
    COL(C_LCYAN,C_BLACK),COL(C_YELLOW,C_BLACK),COL(C_LMAG,C_BLACK),
    COL(C_LGREEN,C_BLACK),COL(C_LRED,C_BLACK),COL(C_LBLUE,C_BLACK),COL(C_BROWN,C_BLACK)
};

static uint8_t tet_board[TET_H][TET_W]; // 0=empty, 1-7=piece color
static int tet_score=0, tet_lines=0, tet_level=1;
static int tet_px=4, tet_py=0, tet_rot=0, tet_piece=0, tet_next=0;

static int tet_rng_seed=12345;
static int tet_rand(){ tet_rng_seed=tet_rng_seed*1103515245+12345; return (tet_rng_seed>>16)&7; }

static int tet_valid(int px,int py,int rot,int piece){
    for(int i=0;i<4;i++){
        int x=px+PIECES[piece][rot][i][0];
        int y=py+PIECES[piece][rot][i][1];
        if(x<0||x>=TET_W||y>=TET_H) return 0;
        if(y>=0&&tet_board[y][x]) return 0;
    }
    return 1;
}

static void tet_place(){
    for(int i=0;i<4;i++){
        int x=tet_px+PIECES[tet_piece][tet_rot][i][0];
        int y=tet_py+PIECES[tet_piece][tet_rot][i][1];
        if(y>=0&&y<TET_H&&x>=0&&x<TET_W)
            tet_board[y][x]=(uint8_t)(tet_piece+1);
    }
}

static int tet_clear_lines(){
    int cleared=0;
    for(int r=TET_H-1;r>=0;r--){
        int full=1;
        for(int c=0;c<TET_W;c++) if(!tet_board[r][c]){full=0;break;}
        if(full){
            for(int rr=r;rr>0;rr--)
                for(int c=0;c<TET_W;c++) tet_board[rr][c]=tet_board[rr-1][c];
            for(int c=0;c<TET_W;c++) tet_board[0][c]=0;
            cleared++; r++; // recheck same row
        }
    }
    return cleared;
}

static void tet_draw_board(){
    // Border
    uint8_t bc=COL(C_WHITE,C_BLACK);
    vat(CH_TL,bc,TET_OX-1,TET_OY-1);
    for(int c=0;c<TET_W;c++) vat(CH_H,bc,TET_OX+c,TET_OY-1);
    vat(CH_TR,bc,TET_OX+TET_W,TET_OY-1);
    for(int r=0;r<TET_H;r++){
        vat(CH_V,bc,TET_OX-1,TET_OY+r);
        vat(CH_V,bc,TET_OX+TET_W,TET_OY+r);
    }
    vat(CH_BL,bc,TET_OX-1,TET_OY+TET_H);
    for(int c=0;c<TET_W;c++) vat(CH_H,bc,TET_OX+c,TET_OY+TET_H);
    vat(CH_BR,bc,TET_OX+TET_W,TET_OY+TET_H);

    // Board
    for(int r=0;r<TET_H;r++){
        for(int c=0;c<TET_W;c++){
            if(tet_board[r][c]){
                uint8_t col=PIECE_COLORS[tet_board[r][c]-1];
                vat(CH_BLOCK,col,TET_OX+c,TET_OY+r);
            } else {
                vat('.',COL(C_DGREY,C_BLACK),TET_OX+c,TET_OY+r);
            }
        }
    }
    // Current piece
    for(int i=0;i<4;i++){
        int x=tet_px+PIECES[tet_piece][tet_rot][i][0];
        int y=tet_py+PIECES[tet_piece][tet_rot][i][1];
        if(y>=0) vat(CH_BLOCK,PIECE_COLORS[tet_piece],TET_OX+x,TET_OY+y);
    }

    // Sidebar
    int sx=TET_OX+TET_W+3;
    vset(C_YELLOW,C_BLACK);
    vstr(sx,TET_OY,COL(C_YELLOW,C_BLACK),"TETRIS");
    vstr(sx,TET_OY+1,COL(C_DGREY,C_BLACK),"------");
    char sb[16];
    vstr(sx,TET_OY+3,COL(C_WHITE,C_BLACK),"Score:");
    kia(tet_score,sb); vstr(sx,TET_OY+4,COL(C_LGREEN,C_BLACK),sb);
    vstr(sx,TET_OY+6,COL(C_WHITE,C_BLACK),"Lines:");
    kia(tet_lines,sb); vstr(sx,TET_OY+7,COL(C_LGREEN,C_BLACK),sb);
    vstr(sx,TET_OY+9,COL(C_WHITE,C_BLACK),"Level:");
    kia(tet_level,sb); vstr(sx,TET_OY+10,COL(C_LGREEN,C_BLACK),sb);
    vstr(sx,TET_OY+12,COL(C_LCYAN,C_BLACK),"Next:");
    // Clear next piece area first, then draw
    for(int r=0;r<4;r++) for(int c=0;c<6;c++) vat(' ',COL(C_BLACK,C_BLACK),sx+c,TET_OY+13+r);
    for(int i=0;i<4;i++){
        int x=PIECES[tet_next][0][i][0];
        int y=PIECES[tet_next][0][i][1];
        if(x>=0&&x<6&&y>=0&&y<4)
            vat(CH_BLOCK,PIECE_COLORS[tet_next],sx+x,TET_OY+13+y);
    }
    vstr(sx,TET_OY+17,COL(C_LGREY,C_BLACK),"Controls:");
    vstr(sx,TET_OY+18,COL(C_LGREY,C_BLACK),"WASD/Arrow");
    vstr(sx,TET_OY+19,COL(C_LGREY,C_BLACK),"Q=quit");
    vrst();
}

static void cmd_tetris(){
    vclear();
    // Init
    for(int r=0;r<TET_H;r++) for(int c=0;c<TET_W;c++) tet_board[r][c]=0;
    tet_score=0; tet_lines=0; tet_level=1;
    tet_piece=tet_rand()%7; tet_next=tet_rand()%7;
    tet_px=4; tet_py=0; tet_rot=0;
    tet_rng_seed=12345;

    int drop_timer=0;
    int drop_speed=40; // frames before auto-drop - slower

    while(1){
        // Input (non-blocking)
        if(kb_avail()){
            int c=kb_get();
            if(c=='q'||c==27) goto tet_done;
            int nr=tet_rot, nx=tet_px, ny=tet_py;
            if(c=='a'||c==KEY_LEFT)  nx--;
            if(c=='d'||c==KEY_RIGHT) nx++;
            if(c=='w'||c==KEY_UP)    nr=(tet_rot+1)%4;
            if(c=='s'||c==KEY_DOWN)  ny++;
            if(c==' '){
                // Hard drop
                while(tet_valid(tet_px,ny+1,tet_rot,tet_piece)) ny++;
                tet_py=ny;
                goto tet_lock;
            }
            if(nx!=tet_px&&tet_valid(nx,tet_py,tet_rot,tet_piece)) tet_px=nx;
            if(nr!=tet_rot&&tet_valid(tet_px,tet_py,nr,tet_piece)) tet_rot=nr;
            if(ny!=tet_py&&tet_valid(tet_px,ny,tet_rot,tet_piece)) tet_py=ny;
        }

        // Auto-drop
        drop_timer++;
        if(drop_timer>=drop_speed){
            drop_timer=0;
            if(tet_valid(tet_px,tet_py+1,tet_rot,tet_piece)){
                tet_py++;
            } else {
tet_lock:
                tet_place();
                int cl=tet_clear_lines();
                tet_lines+=cl;
                tet_score+=cl*cl*100*tet_level;
                tet_level=1+tet_lines/10;
                drop_speed=40-tet_level*3; if(drop_speed<4)drop_speed=4;
                // New piece
                tet_piece=tet_next; tet_next=tet_rand()%7;
                tet_px=4; tet_py=0; tet_rot=0;
                if(!tet_valid(tet_px,tet_py,tet_rot,tet_piece)){
                    tet_draw_board();
                    vset(C_LRED,C_BLACK);
                    vstr(TET_OX,TET_OY+TET_H/2,COL(C_WHITE,C_LRED)," GAME OVER ");
                    vrst();
                    kb_get(); goto tet_done;
                }
            }
        }

        tet_draw_board();
        for(volatile int d=0;d<2000000;d++);
    }
tet_done:
    vclear();
    vset(C_YELLOW,C_BLACK); vps("Tetris done! Score: ");
    char sb[16]; kia(tet_score,sb); vps(sb);
    vps("  Lines: "); kia(tet_lines,sb); vps(sb);
    vps("  Level: "); kia(tet_level,sb); vpln(sb);
    vrst();
}

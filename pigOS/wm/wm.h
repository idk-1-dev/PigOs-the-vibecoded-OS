#pragma once
// ================================================================
// PigWM - pigOS Window Manager
// VibeOS-inspired, modern text-mode compositor
// ================================================================
#include "../drivers/vga/vga.h"
#include "../drivers/kbd/keyboard.h"
#include "../drivers/ps2/ps2.h"
#include "../drivers/mouse/mouse.h"
#include "../kernel/mem.h"

// Forward declarations
static void wm_power_menu(void);
static void wm_robin(void);
static void wm_terminal(void);

void sh_dispatch(const char*);
extern char sh_user[32];
extern char sh_cwd[256];
extern const char* CMDS[];
extern void sh_dispatch(const char*);

// ── Windows ───────────────────────────────────────────────────
#define WM_MAX    8
#define WIN_LINES 20
#define WIN_COLS  76

typedef struct {
    int alive,focused,minimized,maximized;
    int x,y,w,h;
    int ox,oy,ow,oh;
    char title[48];
    char lines[WIN_LINES][WIN_COLS];
    int nlines;
    int scroll;
} PigWin;

static PigWin wins[WM_MAX];
static int wfocus=-1, wcount=0;
static int wm_running=0;
static uint32_t wm_tick=0;

// ── Dock icons ───────────────────────────────────────────────
typedef struct { int x; const char*label; uint8_t icon_ch; uint8_t col; void (*action)(void); } DockIcon;
#define DOCK_ICONS 6
static void dock_terminal(){ /* handled by F2 */ }
static void dock_robin(){ /* handled by F3 */ }
static void dock_power(){ /* handled by F5 */ }

static DockIcon dock[]={
    {3,  "Term",     0x01, COL(C_LGREEN,C_LGREEN)},
    {12, "Files",    0x02, COL(C_LBLUE,C_LBLUE)},
    {21, "Browser",  0x03, COL(C_LCYAN,C_LCYAN)},
    {30, "Settings", 0x04, COL(C_LMAG,C_LMAG)},
    {39, "Browser2", 0x05, COL(C_YELLOW,C_YELLOW)},
    {48, "Power",    0x06, COL(C_LRED,C_LRED)},
};
static int dock_selected=-1;

// ── pigOS Wallpaper ──────────────────────────────────
static void wm_wallpaper(){
    vfill(0,0,VGA_W,VGA_H,' ',COL(C_WHITE,C_LGREY));
    
    for(int y=1;y<VGA_H-1;y++){
        uint8_t bg = (y < 15) ? COL(C_WHITE,C_LGREY) : 
                     (y < 20) ? COL(C_DGREY,C_LGREY) : COL(C_DGREY,C_LGREY);
        vfill(0,y,VGA_W,1,' ',bg);
    }
    
    vstr(32,9,COL(C_LCYAN,C_LGREY),"   ____  _       ");
    vstr(32,10,COL(C_LCYAN,C_LGREY),"|  _ \\(_) ___   ");
    vstr(32,11,COL(C_LCYAN,C_LGREY),"| |_) | |/ _ \\  ");
    vstr(32,12,COL(C_LCYAN,C_LGREY),"|  __/| | (_) | ");
    vstr(32,13,COL(C_LCYAN,C_LGREY),"|_|   |_|\\___/  ");
    
    vstr(34,15,COL(C_LCYAN,C_LGREY),"pigOS v1.0");
    vstr(30,17,COL(C_DGREY,C_LGREY),"Powered by larpshell");
}

// ── Dock (macOS/VibeOS style) ─────────────────────────────────
static void wm_dock(){
    // Dock background with rounded feel
    int dock_y = VGA_H - 6;
    vfill(0,dock_y,VGA_W,6,' ',COL(C_LGREY,C_LGREY));
    vfill(0,dock_y,80,1,' ',COL(C_WHITE,C_WHITE));
    
    // Dock icons with hover effect
    for(int i=0;i<DOCK_ICONS;i++){
        int dx = dock[i].x;
        uint8_t icon_bg = COL(C_WHITE,C_WHITE);
        uint8_t icon_col = dock[i].col;
        
        if(dock_selected==i){
            // Highlighted state
            icon_bg = COL(C_LCYAN,C_LCYAN);
            icon_col = COL(C_BLACK,C_LCYAN);
        }
        
        // Icon background with shadow
        vfill(dx,dock_y+1,8,4,' ',COL(C_DGREY,C_LGREY));
        vfill(dx+1,dock_y+2,7,3,' ',icon_bg);
        
        // Icon symbol
        vfill(dx+3,dock_y+3,2,1,' ',icon_col);
        vat(dock[i].icon_ch,icon_col,dx+3,dock_y+2);
        
        // Label with shadow
        vstr(dx,dock_y+6,COL(C_DGREY,C_LGREY),dock[i].label);
        vstr(dx,dock_y+5,COL(C_BLACK,C_LGREY),dock[i].label);
    }
    
    // Clock/time
    vstr(68,dock_y+2,COL(C_WHITE,C_LGREY),"10:00 AM");
    vstr(68,dock_y+4,COL(C_DGREY,C_LGREY),"Jan 1 1970");
}

// ── Window buttons (modern style) ──────────────────────────────
static void wm_draw_win(int id){
    if(id<0||id>=WM_MAX||!wins[id].alive||wins[id].minimized)return;
    PigWin*W=&wins[id];
    int x=W->x,y=W->y,w=W->w,h=W->h;

    if(x<0)x=0; if(y<1)y=1;
    if(x+w>VGA_W-1)w=VGA_W-1-x;
    if(y+h>VGA_H-1)h=VGA_H-1-y;

    // Shadow
    uint8_t shadow_col=COL(C_BLACK,C_LGREY);
    for(int r=0;r<h;r++) vat(CH_BLOCK,shadow_col,x+w,y+r+1);
    vat(CH_BLOCK,shadow_col,x+w,y+h+1);
    
    // Window body
    uint8_t body_col = W->focused ? COL(C_WHITE,C_WHITE) : COL(C_DGREY,C_LGREY);
    vfill(x,y,w,h+1,' ',body_col);

    // Title bar
    uint8_t tb = W->focused ? COL(C_WHITE,C_LCYAN) : COL(C_WHITE,C_LGREY);
    vfill(x,y,w,3,' ',tb);

    // Title text
    int tl=ksl(W->title); if(tl>w-10)tl=w-10;
    for(int i=0;i<tl;i++) vat((uint8_t)W->title[i],tb,x+2+i,y+1);

    // Window buttons
    if(w>8){
        // Close button (red)
        uint8_t cb_close = W->focused ? COL(C_LRED,C_LRED) : COL(C_DGREY,C_LGREY);
        vat('X', cb_close, x+w-4, y+1);
        
        // Minimize button (yellow)
        uint8_t cb_min = W->focused ? COL(C_YELLOW,C_YELLOW) : COL(C_DGREY,C_LGREY);
        vat('_', cb_min, x+w-6, y+1);
        
        // Maximize button (green)
        uint8_t cb_max = W->focused ? COL(C_LGREEN,C_LGREEN) : COL(C_DGREY,C_LGREY);
        vat('O', cb_max, x+w-8, y+1);
    }

    // Border
    uint8_t border_col = W->focused ? COL(C_LGREY,C_LGREY) : COL(C_DGREY,C_LGREY);
    for(int c=x;c<x+w;c++) vat(CH_H,border_col,c,y+2);
    vat(CH_TL,border_col,x,y+2); vat(CH_TR,border_col,x+w-1,y+2);
    for(int r=y+3;r<y+h+1;r++){
        vat(CH_V,border_col,x,r);
        vat(CH_V,border_col,x+w-1,r);
    }

    // Content area
    uint8_t cc=COL(C_BLACK,C_WHITE);
    int maxr=h-4; if(maxr>WIN_LINES)maxr=WIN_LINES;
    for(int r=0;r<maxr;r++){
        int li=r+W->scroll;
        if(li<W->nlines)
            vstr(x+1,y+3+r,cc,W->lines[li]);
    }

    // Scrollbar
    if(W->nlines>maxr&&w>3){
        uint8_t sc=COL(C_LGREY,C_LGREY);
        for(int r=0;r<maxr;r++) vat(CH_V,sc,x+w-1,y+3+r);
        int spos=W->scroll*maxr/W->nlines;
        vat(CH_BLOCK,COL(C_BLUE,C_LGREY),x+w-1,y+3+spos);
    }
}

// ── Window management ──────────────────────────────────────────
static int wm_find_win(void){
    for(int i=0;i<WM_MAX;i++) if(!wins[i].alive) return i;
    return -1;
}

static int wm_new(int x,int y,int w,int h,const char*t){
    for(int i=0;i<WM_MAX;i++){
        if(!wins[i].alive){
            PigWin*W=&wins[i];
            W->alive=1; W->focused=0; W->minimized=0; W->maximized=0;
            W->x=x; W->y=y; W->w=w; W->h=h;
            W->ox=x; W->oy=y; W->ow=w; W->oh=h;
            W->nlines=0; W->scroll=0;
            int tl=ksl(t); if(tl>47)tl=47;
            for(int j=0;j<tl;j++) W->title[j]=t[j]; W->title[tl]=0;
            wcount++;
            if(wfocus<0){ wfocus=i; W->focused=1; }
            return i;
        }
    }
    return -1;
}

static void wm_create_win(const char*t,int x,int y,int w,int h){
    wm_new(x,y,w,h,t);
}

static void wm_write(int id,const char*s){
    if(id<0||id>=WM_MAX||!wins[id].alive)return;
    PigWin*W=&wins[id];
    if(W->nlines<WIN_LINES){
        int i=0;
        while(s[i]&&i<WIN_COLS-1){ W->lines[W->nlines][i]=s[i]; i++; }
        W->lines[W->nlines][i]=0; W->nlines++;
    }
}

static void wm_writeln(int id,const char*s){
    wm_write(id,s);
    if(id>=0&&id<WM_MAX&&wins[id].alive){
        wins[id].scroll=0;
    }
}

static void wm_focus(int id){
    if(wfocus>=0&&wfocus<WM_MAX) wins[wfocus].focused=0;
    wfocus=id;
    if(id>=0&&id<WM_MAX) wins[id].focused=1;
}

static void wm_close(int id){
    if(id>=0&&id<WM_MAX&&wins[id].alive){
        wins[id].alive=0;
        if(wfocus==id){
            wfocus=-1;
            for(int j=0;j<WM_MAX;j++) if(wins[j].alive){ wm_focus(j); break; }
        }
    }
}

static void wm_minimize(int id){
    if(id>=0&&id<WM_MAX&&wins[id].alive){
        wins[id].minimized=1;
        if(wfocus==id){
            wfocus=-1;
            for(int j=0;j<WM_MAX;j++) if(wins[j].alive&&!wins[j].minimized){ wm_focus(j); break; }
        }
    }
}

static void wm_render(){
    mouse_erase();
    wm_wallpaper();
    for(int i=0;i<WM_MAX;i++)
        if(wins[i].alive&&!wins[i].focused&&!wins[i].minimized) wm_draw_win(i);
    if(wfocus>=0&&wins[wfocus].alive) wm_draw_win(wfocus);
    wm_dock();
    mouse_poll();
    if(mouse_enabled){mouse_draw_cursor();}
    wm_tick++;
}

// ── Main desktop loop ─────────────────────────────────────────
static void wm_init(){
    for(int i=0;i<WM_MAX;i++) wins[i].alive=0;
    wfocus=-1; wcount=0; wm_running=1;
    mouse_init();
    mouse_wm_mode=1;
}

static void wm_desktop(){
    vclear();
    vga_disable_cursor();
    wm_init();

    // Welcome window
    int wa=wm_new(25,2,30,12," Welcome ");
    wm_write(wa,"  Welcome to pigOS!");
    wm_write(wa,"  =================");
    wm_write(wa,"");
    wm_write(wa,"  F2 = Terminal");
    wm_write(wa,"  F3 = File Manager");
    wm_write(wa,"  F5 = Power Menu");
    wm_write(wa,"  ESC = Exit WM");
    wm_write(wa,"");
    wm_write(wa,"  Click buttons to");
    wm_write(wa,"  minimize/close");

    wm_render();

    while(1){
        mouse_poll();
        ps2_poll();
        
        // Process any key presses
        while(kb_avail()){
            int c = kb_get();
            if(c == KEY_F5){ wm_power_menu(); wm_render(); }
            else if(c == KEY_F3){ wm_robin(); wm_render(); }
            else if(c == KEY_F2){ wm_terminal(); wm_render(); }
            else if(c == KEY_F7){
                wm_close(wfocus);
                wm_render();
            }
            else if(c == 27){ vga_enable_cursor(13,15); vclear(); return; }
            else if(c == '\t'){
                int nf=wfocus;
                for(int i=1;i<=WM_MAX;i++){
                    int idx=(wfocus+i)%WM_MAX;
                    if(wins[idx].alive&&!wins[idx].minimized){ nf=idx; break; }
                }
                wm_focus(nf);
                wm_render();
            }
        }
        
        // Mouse button click detection
        if(mouse_btn_l && !mouse_btn_l_prev){
            int clicked=0;
            
            // Check dock icons
            int dock_y = VGA_H - 6;
            if(mouse_y >= dock_y){
                for(int i=0;i<DOCK_ICONS;i++){
                    if(mouse_x >= dock[i].x && mouse_x <= dock[i].x+7){
                        dock_selected = i;
                        clicked = 1;
                        break;
                    }
                }
            }
            
            // Check window buttons
            if(!clicked){
                for(int i=0;i<WM_MAX;i++){
                    if(!wins[i].alive || wins[i].minimized) continue;
                    PigWin*W=&wins[i];
                    int x=W->x, y=W->y, w=W->w;
                    
                    // Close button
                    if(mouse_y==y+1 && mouse_x==x+w-4){
                        wm_close(i);
                        clicked=1;
                        break;
                    }
                    // Minimize button
                    if(mouse_y==y+1 && mouse_x==x+w-6){
                        wm_minimize(i);
                        clicked=1;
                        break;
                    }
                    
                    // Click on title bar to focus
                    if(mouse_y==y+1 && mouse_x>=x && mouse_x<x+w-8){
                        wm_focus(i);
                        clicked=1;
                        break;
                    }
                    
                    // Click anywhere in window to focus
                    if(mouse_x>=x && mouse_x<x+w && mouse_y>=y && mouse_y<=y+W->h){
                        wm_focus(i);
                        clicked=1;
                        break;
                    }
                }
            }
            
            if(clicked){
                wm_render();
            }
        }
        mouse_btn_l_prev = mouse_btn_l;
    }

    mouse_wm_mode=0;
    vga_enable_cursor(13,15);
    vclear();
}

// ── Power menu (VibeOS style) ─────────────────────────────────
static void wm_power_menu(){
    int px=25, py=8, pw=30, ph=8;
    vfill(px-1,py-1,pw+2,ph+2,' ',COL(C_DGREY,C_DGREY));
    vfill(px,py,pw,ph,' ',COL(C_WHITE,C_BLACK));
    
    uint8_t tb=COL(C_WHITE,C_LCYAN);
    vfill(px,py,pw,2,' ',tb);
    vstr(px+1,py,tb," Power Options");
    
    uint8_t oc=COL(C_BLACK,C_WHITE);
    vstr(px+2,py+3,oc,"[1] Sleep");
    vstr(px+2,py+4,oc,"[2] Restart");
    vstr(px+2,py+5,oc,"[3] Shutdown");
    vstr(px+2,py+6,oc,"[ESC] Cancel");
    
    int c=kb_get();
    if(c=='3'){
        vclear(); vset(C_LRED,C_BLACK);
        vpln("Shutting down..."); for(volatile int i=0;i<100000000;i++);
        outb(0xF4,0x00); __asm__("cli;hlt");
    }
    if(c=='2'){
        vclear(); for(volatile int i=0;i<100000000;i++);
        outb(0x64,0xFE);
    }
}

// ── Robin file explorer ────────────────────────────────────────
static void wm_robin(){
    vclear();
    uint8_t tb=COL(C_WHITE,C_LCYAN);
    uint8_t sb=COL(C_WHITE,C_LGREY);
    
    vfill(0,0,VGA_W,1,' ',tb);
    vstr(1,0,tb," Files - pigOS File Manager ");
    vstr(60,0,COL(C_WHITE,C_LGREY),"ESC=close");
    
    vfill(0,1,VGA_W,VGA_H-2,' ',sb);
    
    vstr(2,3,COL(C_LCYAN,C_LGREY),"/ pigfs virtual filesystem");
    
    extern const char*pigfs_entries[];
    extern int fs_count;
    int row = 5;
    for(int i = 0; i < 10 && row < VGA_H-4; i++){
        vstr(2,row++,COL(C_YELLOW,C_LGREY),"  [dir]  virtual_entry");
    }
    
    vstr(2,VGA_H-3,COL(C_DGREY,C_LGREY),"Use 'ls' command for full listing");
    
    while(1){
        if(kb_avail()){
            int c = kb_get();
            if(c == 27) break;
        }
    }
    vclear();
}

static void wm_terminal(){
    int tx=3, ty=3, tw=74, th=17;
    
    extern void sh_dispatch(const char*);
    extern char sh_user[32];
    extern char sh_cwd[256];
    extern int sh_hn;
    extern char sh_hist[64][512];
    extern int ksc(const char*,const char*);
    
    int cur_y=ty+2;
    
    vfill(tx,ty,tw,th,' ',COL(C_WHITE,C_BLACK));
    for(int i=tx;i<tx+tw;i++){
        vat(0xCD,i,ty,COL(C_LCYAN,C_BLACK));
        vat(0xCD,i,ty+th-1,COL(C_LCYAN,C_BLACK));
    }
    for(int i=ty;i<ty+th;i++){
        vat(0xBA,tx,i,COL(C_LCYAN,C_BLACK));
        vat(0xBA,tx+tw-1,i,COL(C_LCYAN,C_BLACK));
    }
    vat(0xC9,tx,ty,COL(C_LCYAN,C_BLACK));
    vat(0xBB,tx+tw-1,ty,COL(C_LCYAN,C_BLACK));
    vat(0xC8,tx,ty+th-1,COL(C_LCYAN,C_BLACK));
    vat(0xBC,tx+tw-1,ty+th-1,COL(C_LCYAN,C_BLACK));
    
    vfill(tx+1,ty,tw-2,1,' ',COL(C_BLACK,C_LCYAN));
    vstr(tx+2,ty,COL(C_BLACK,C_LCYAN)," larpshell ");
    vstr(tx+tw-8,ty,COL(C_WHITE,C_LRED)," [X]");
    
    vset(C_LGREEN,C_BLACK);
    vstr(tx+2,cur_y++,COL(C_LGREEN,C_BLACK),"> larpshell v5.9.3 ready");
    cur_y++;
    
    char input[128];
    int ilen=0;
    int hist_idx=-1;
    
    while(1){
        vset(C_LGREEN,C_BLACK);
        vstr(tx+2,cur_y,COL(C_LGREEN,C_BLACK),"> ");
        
        ilen=0;
        input[0]=0;
        hist_idx=-1;
        
        while(1){
            ps2_poll();
            mouse_poll();
            
            if(!kb_avail()) continue;
            
            int c=kb_get();
            
            if(c=='\n'||c=='\r'){
                vpc('\n');
                input[ilen]=0;
                break;
            }
            
            if(c==3){
                vps("^C");
                vpc('\n');
                ilen=0;
                hist_idx=-1;
                break;
            }
            
            if(c=='\b'||c==127){
                if(ilen>0){
                    ilen--;
                    vpc('\b');
                    vpc(' ');
                    vpc('\b');
                }
                continue;
            }
            
            if(c==KEY_UP){
                if(sh_hn>0&&hist_idx<sh_hn-1){
                    hist_idx++;
                    while(ilen>0){ilen--;vpc('\b');vpc(' ');vpc('\b');}
                    kcp(input,sh_hist[sh_hn-1-hist_idx]);
                    ilen=ksl(input);
                    for(int i=0;i<ilen;i++) vpc((uint8_t)input[i]);
                }
                continue;
            }
            if(c==KEY_DOWN){
                if(hist_idx>0){
                    hist_idx--;
                    while(ilen>0){ilen--;vpc('\b');vpc(' ');vpc('\b');}
                    kcp(input,sh_hist[sh_hn-1-hist_idx]);
                    ilen=ksl(input);
                    for(int i=0;i<ilen;i++) vpc((uint8_t)input[i]);
                } else if(hist_idx==0){
                    hist_idx=-1;
                    while(ilen>0){ilen--;vpc('\b');vpc(' ');vpc('\b');}
                    ilen=0;
                }
                continue;
            }
            
            if(c==27){
                vga_disable_cursor();
                wm_render();
                return;
            }
            
            if(c>=32&&c<127&&ilen<127){
                input[ilen++]=(char)c;
                vpc((uint8_t)c);
            }
        }
        
        if(ilen>0){
            extern void sh_push_hist(const char*);
            sh_push_hist(input);
            
            sh_dispatch(input);
            
            cur_y++;
            if(cur_y>=ty+th-2){
                for(int y=ty+2;y<ty+th-1;y++)
                    for(int x=tx+2;x<tx+tw-2;x++)
                        vat(' ',COL(C_WHITE,C_BLACK),x,y);
                cur_y=ty+2;
                vfill(tx,ty,tw,th,' ',COL(C_WHITE,C_BLACK));
                for(int i=tx;i<tx+tw;i++){
                    vat(0xCD,i,ty,COL(C_LCYAN,C_BLACK));
                    vat(0xCD,i,ty+th-1,COL(C_LCYAN,C_BLACK));
                }
                for(int i=ty;i<ty+th;i++){
                    vat(0xBA,tx,i,COL(C_LCYAN,C_BLACK));
                    vat(0xBA,tx+tw-1,i,COL(C_LCYAN,C_BLACK));
                }
                vat(0xC9,tx,ty,COL(C_LCYAN,C_BLACK));
                vat(0xBB,tx+tw-1,ty,COL(C_LCYAN,C_BLACK));
                vat(0xC8,tx,ty+th-1,COL(C_LCYAN,C_BLACK));
                vat(0xBC,tx+tw-1,ty+th-1,COL(C_LCYAN,C_BLACK));
                vfill(tx+1,ty,tw-2,1,' ',COL(C_BLACK,C_LCYAN));
                vstr(tx+2,ty,COL(C_BLACK,C_LCYAN)," larpshell ");
                vstr(tx+tw-8,ty,COL(C_WHITE,C_LRED)," [X]");
            }
        }
    }
}
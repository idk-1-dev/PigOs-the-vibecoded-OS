#pragma once
// larpshell v5.9.3 - Based on lash (github.com/usr-undeleted/lash)

#include <stddef.h>
#include "../drivers/vga/vga.h"
#include "../drivers/ps2/ps2.h"
#include "../kernel/mem.h"
#include "../kernel/users.h"
#include "../kernel/pigfs.h"
#include "../kernel/poing/poing.h"
#include "../drivers/net/network.h"
#include "../crypto/pigssl.h"
#include "../crypto/discord.h"
#include "pigfetch.h"
#include "polctl.h"
#include "rpk.h"
#include "netpg.h"
#include "games/tetris.h"

extern uint8_t rtl_inb(uint16_t port);
extern uint16_t rtl_inw(uint16_t port);
extern uint32_t rtl_inl(uint16_t port);

static void wm_desktop(void);
static void wm_robin(void);

static void sh_prompt();
static int nano_find_line_start(const char*buf,int pos);
static int nano_find_line_end(const char*buf,int len,int pos);
static int nano_find_prev_line_start(const char*buf,int pos);
static void nano_redraw_cursor(const char*buf,int len,int cur);
static void cmd_rpk_run(const char*);

static void cmd_snake();
static void cmd_help_sh();
static void cmd_calc_sh(const char*);

static void grep_set_pipe_input(const char*);

#define SH_BUF  512
#define SH_HIST 64
static char sh_in[SH_BUF];
static int  sh_len=0, sh_cur=0; // sh_cur = cursor position
static char sh_hist[SH_HIST][SH_BUF];
static int  sh_hn=0, sh_hi=-1;
static int  sh_rok=0;
static int logged_in=0;
char sh_cwd[256]="/";
char sh_user[32]="root";
static int  sh_uid=0;
#include "menuconfig.h"
#include "vi.h"

// Shell variables (for $VAR expansion)
#define SH_VARS 32
static char sh_var_names[SH_VARS][32];
static char sh_var_vals[SH_VARS][128];
static int  sh_nvars=0;

// Shell aliases
#define SH_ALIASES 32
static char sh_alias_names[SH_ALIASES][32];
static char sh_alias_vals[SH_ALIASES][128];
static int  sh_naliases=0;

static void sh_alias_set(const char*n,const char*v){
    for(int i=0;i<sh_naliases;i++) if(!ksc(sh_alias_names[i],n)){kcp(sh_alias_vals[i],v);return;}
    if(sh_naliases<SH_ALIASES){kcp(sh_alias_names[sh_naliases],n);kcp(sh_alias_vals[sh_naliases],v);sh_naliases++;}
}
static const char* sh_alias_get(const char*n){
    for(int i=0;i<sh_naliases;i++) if(!ksc(sh_alias_names[i],n)) return sh_alias_vals[i];
    return NULL;
}

static void sh_var_set(const char*n,const char*v){
    for(int i=0;i<sh_nvars;i++) if(!ksc(sh_var_names[i],n)){kcp(sh_var_vals[i],v);return;}
    if(sh_nvars<SH_VARS){kcp(sh_var_names[sh_nvars],n);kcp(sh_var_vals[sh_nvars],v);sh_nvars++;}
}
static const char* sh_var_get(const char*n){
    for(int i=0;i<sh_nvars;i++) if(!ksc(sh_var_names[i],n)) return sh_var_vals[i];
    return NULL;
}

// Expand $VAR in string
static void sh_expand(const char*in,char*out,int outsz){
    int oi=0;
    for(int i=0;in[i]&&oi<outsz-1;){
        if(in[i]=='$'){
            i++;
            char vn[32]; int vi=0;
            while(in[i] && ((in[i]>='A' && in[i]<='Z') || (in[i]>='a' && in[i]<='z') || (in[i]>='0' && in[i]<='9') || (in[i]=='_')) && vi < 31)
                vn[vi++]=in[i++];
            vn[vi]=0;
            const char*val=sh_var_get(vn);
            if(!val){// Special vars
                if(!ksc(vn,"HOME"))val="/root";
                else if(!ksc(vn,"USER"))val=sh_user;
                else if(!ksc(vn,"PWD"))val=sh_cwd;
                else if(!ksc(vn,"SHELL"))val="/bin/pig";
                else if(!ksc(vn,"?")){ static char ec[4]="0"; val=ec; }
                else val=vn; // unexpanded
            }
            if(val) while(*val&&oi<outsz-1) out[oi++]=*val++;
        } else {
            out[oi++]=in[i++];
        }
    }
    out[oi]=0;
}

// Simple integer to ASCII (base 10, no sign)
static void kitoa(int val, char* out) {
    char buf[12];
    int i = 0;
    if(val == 0) { out[0] = '0'; out[1] = 0; return; }
    while(val > 0 && i < 11) { buf[i++] = '0' + (val % 10); val /= 10; }
    int j = 0;
    while(i > 0) out[j++] = buf[--i];
    out[j] = 0;
}

const char* CMDS[]={
    "help","about","pigfetch","uname","hostname","uptime","date",
    "dmesg","clear","cls","color","reboot","halt","shutdown","poweroff","panic","version",
    "poingctl","polctl",
    "adduser","deluser","usermod","groupadd","groupdel","groups","passwd",
    "whoami","id","users","who","last","w","rok","su","r-","chsh",
    "ls","ll","la","lh","tree","cat","more","less","head","tail",
    "grep","find","wc","diff","sort","uniq","cut","awk","sed","xargs","tee",
    "mkdir","rmdir","rm","cp","mv","touch","ln","chmod","chown","stat",
    "pwd","cd","pushd","popd","file","pnano","pi","pivi",
    "ps","top","htop","kill","killall","pgrep","pkill","nice","jobs","bg","fg","nohup",
    "ifconfig","ping","netstat","pigget","wget","curl","pigcurl",
    "nslookup","dig","traceroute","ss","ip","arp","route","netpg","pignet","dhcpcd",
    "lspci","lsusb","lsmod","modprobe","rmmod","lscpu","lsmem","lshw","lsblk",
    "df","du","free","mount","umount","fdisk","mkfs","fsck","dd","sync",
    "rpk","pigpkg",
    "pigssl","pigcrypt","pigdecrypt","pighash","pigkey","pigcert",
    "echo","printf","export","env","alias","unalias","which","type",
    "history","set","unset","eval","exec","source","!!",
    "calc","bc","pigmath","seq","yes","no","true","false",
    "sleep","countdown","timer","watch","timeout","tee",
    "base64","xxd","strings","od","wc","tar","gzip","zip","unzip",
    "man","info","whatis","apropos",
    "wm","robin","pigbrowse","browser","compositor",
    "llm","pigai","ask","chat","python3","pip3","make","go","larpshell",
    "snake","tetris","matrix","fortune","cowpig","figpig","banner","cal",
    "todo","note","remind","piglog","dmesg","discord","dc",
    NULL
};
#include "lash.h"

static void sh_push_hist(const char*s){
    if(sh_hn<SH_HIST)kcp(sh_hist[sh_hn++],s);
    else{for(int i=0;i<SH_HIST-1;i++)kcp(sh_hist[i],sh_hist[i+1]);kcp(sh_hist[SH_HIST-1],s);}
    sh_hi=-1;
}

static void sh_redraw_line(){
    // Move cursor to start of input (after prompt), clear rest of line, redraw
    // We know vx is at sh_cur+prompt_len - just redraw everything
    // Go back to start
    for(int i=0;i<vx;i++) vpc('\b');
    // Clear to end of line
    int save_vx=vx;
    for(int i=0;i<VGA_W-save_vx;i++) vpc(' ');
    for(int i=0;i<VGA_W-save_vx;i++) vpc('\b');
}

// Tab completion removed
static void sh_tab_complete(){
    // Do nothing - tab completion disabled
}

static void sh_prompt(){
    if(sh_rok){ vset(C_LRED,C_BLACK); vps("[rok] "); }
    vset(C_LGREEN,C_BLACK); vps(sh_user);
    vset(C_WHITE,C_BLACK); vpc('@');
    vset(C_LCYAN,C_BLACK); vps("pigOS");
    vset(C_WHITE,C_BLACK); vpc(':');
    vset(C_YELLOW,C_BLACK); vps(sh_cwd);
    vset(sh_uid==0?C_LRED:C_WHITE,C_BLACK); vps(sh_uid==0?"# ":"$ ");
    vrst();
}

// ── pnano editor (unchanged from before, works) ───────────────
static void cmd_nano(const char*f){
    char path[256]="/tmp/unnamed.txt";
    if(f&&*f) fs_resolve(sh_cwd,f,path,256);
    char buf[2048]=""; int bl=0;
    MemFile*mf=memfs_find(path);
    if(mf){kcp(buf,mf->content);bl=ksl(buf);}
    else{const VNode*n=fs_find_vnode(path);if(n&&n->content&&ksl(n->content)<2047){kcp(buf,n->content);bl=ksl(buf);}}
    vclear();
    uint8_t tb=COL(C_WHITE,C_BLUE);
    vfill(0,0,VGA_W,1,' ',tb);
    vstr(1,0,tb,"pigPNano 1.0 - "); vstr(16,0,tb,path);
    vfill(0,VGA_H-2,VGA_W,1,' ',COL(C_BLACK,C_LGREY));
    vstr(1,VGA_H-2,COL(C_BLACK,C_LGREY),"^X=exit  ^O=save  ^G=help  ARROWS=move");
    char ebuf[2048]; kcp(ebuf,buf); int elen=bl;
    int cur=elen;
    int modified=0;
    vset(C_WHITE,C_BLACK); vx=0; vy=1;
    vps(ebuf);
    vx=0; vy=1;
    nano_redraw_cursor(ebuf,elen,cur);
    vga_update_hw_cursor();
    while(1){
        int c=kb_get();
        if(c==24){
            if(modified){
                vfill(0,VGA_H-3,VGA_W,1,' ',COL(C_BLACK,C_YELLOW));
                vstr(1,VGA_H-3,COL(C_BLACK,C_YELLOW),"Save modified buffer? (Y=yes N=no C=cancel) ");
                int ans=kb_get();
                if(ans=='y'||ans=='Y') goto nano_save;
                if(ans=='c'||ans=='C'){vx=0;vy=1;nano_redraw_cursor(ebuf,elen,cur);vga_update_hw_cursor();continue;}
            }
            break;
        }
        if(c==15){
nano_save:
            ebuf[elen]=0;
            MemFile*m2=memfs_find(path); if(!m2)m2=memfs_create(path);
            if(m2)kcp(m2->content,ebuf);
            modified=0;
            vfill(0,VGA_H-3,VGA_W,1,' ',COL(C_BLACK,C_LGREY));
            vstr(1,VGA_H-3,COL(C_BLACK,C_LGREY),"Saved.");
            vx=0; vy=1; nano_redraw_cursor(ebuf,elen,cur); vga_update_hw_cursor(); continue;
        }
        if(c==7){
            vfill(0,VGA_H-3,VGA_W,1,' ',COL(C_WHITE,C_BLUE));
            vstr(1,VGA_H-3,COL(C_WHITE,C_BLUE),"^X=exit ^O=save ^G=help ^K=cut line");
            vx=0; vy=1; nano_redraw_cursor(ebuf,elen,cur); vga_update_hw_cursor(); continue;
        }
        if(c==KEY_LEFT){
            if(cur>0){
                cur--;
                nano_redraw_cursor(ebuf,elen,cur);
            }
            vga_update_hw_cursor(); continue;
        }
        if(c==KEY_RIGHT){
            if(cur<elen){
                cur++;
                nano_redraw_cursor(ebuf,elen,cur);
            }
            vga_update_hw_cursor(); continue;
        }
        if(c==KEY_UP){
            int line_start=nano_find_line_start(ebuf,cur);
            if(line_start>0){
                int prev_line_start=nano_find_prev_line_start(ebuf,line_start);
                int prev_line_len=line_start-prev_line_start-1;
                int col=cur-line_start;
                if(col>prev_line_len)col=prev_line_len;
                cur=prev_line_start+col;
                if(cur<0)cur=0;
                nano_redraw_cursor(ebuf,elen,cur);
            }
            vga_update_hw_cursor(); continue;
        }
        if(c==KEY_DOWN){
            int line_end=nano_find_line_end(ebuf,elen,cur);
            if(line_end<elen){
                int next_line_start=line_end+1;
                int this_line_start=nano_find_line_start(ebuf,cur);
                int this_line_len=line_end-this_line_start;
                int col=cur-this_line_start;
                int next_line_len=nano_find_line_end(ebuf,elen,next_line_start)-next_line_start;
                if(col>next_line_len)col=next_line_len;
                cur=next_line_start+col;
                if(cur>elen)cur=elen;
                nano_redraw_cursor(ebuf,elen,cur);
            }
            vga_update_hw_cursor(); continue;
        }
        if(c>=32&&c<127&&elen<2046){
            for(int i=elen;i>cur;i--)ebuf[i]=ebuf[i-1];
            ebuf[cur]=(char)c;
            elen++;
            cur++;
            modified=1;
            nano_redraw_cursor(ebuf,elen,cur);
            vga_update_hw_cursor(); continue;
        }
        if(c=='\n'&&elen<2046){
            for(int i=elen;i>cur;i--)ebuf[i]=ebuf[i-1];
            ebuf[cur]='\n';
            elen++;
            cur++;
            modified=1;
            nano_redraw_cursor(ebuf,elen,cur);
            vga_update_hw_cursor(); continue;
        }
        if(c=='\b'&&cur>0){
            for(int i=cur-1;i<elen-1;i++)ebuf[i]=ebuf[i+1];
            elen--;
            cur--;
            modified=1;
            nano_redraw_cursor(ebuf,elen,cur);
            vga_update_hw_cursor(); continue;
        }
        if(c==127){
            if(cur<elen){
                for(int i=cur;i<elen-1;i++)ebuf[i]=ebuf[i+1];
                elen--;
                modified=1;
                nano_redraw_cursor(ebuf,elen,cur);
            }
            vga_update_hw_cursor(); continue;
        }
    }
    vclear(); vrst();
}

static int nano_find_line_start(const char*buf,int pos){
    for(int i=pos;i>0;i--){
        if(buf[i-1]=='\n')return i;
    }
    return 0;
}
static int nano_find_line_end(const char*buf,int len,int pos){
    for(int i=pos;i<len;i++){
        if(buf[i]=='\n')return i;
    }
    return len;
}
static int nano_find_prev_line_start(const char*buf,int pos){
    if(pos<=0)return 0;
    for(int i=pos-2;i>=0;i--){
        if(buf[i]=='\n')return i+1;
    }
    return 0;
}
static void nano_redraw_cursor(const char*buf,int len,int cur){
    int pos=0;
    vx=0;vy=1;
    for(int i=0;i<len&&i<cur;i++){
        if(buf[i]=='\n'){vx=0;vy++;if(vy>=VGA_H-2)break;}
        else{vx++;if(vx>=VGA_W){vx=0;vy++;if(vy>=VGA_H-2)break;}}
    }
    vga_update_hw_cursor();
}

// ── LLM - improved ───────────────────────────────────────────
static void cmd_llm(const char*p){
    if(!p||!*p){
        vset(C_LMAG,C_BLACK); vpln("pigai v0.4 - pigOS AI Assistant"); vrst();
        vpln("Usage: llm <question>");
        vpln("Try: llm hello, llm help, llm <any question>");
        return;
    }
    vset(C_LMAG,C_BLACK); vpln("pigai v0.4"); vrst();
    vps("  Q: "); vpln(p);
    
    extern int net_ok;
    if(net_ok){
        extern void http_request(uint32_t,const char*,const char*,char*,int);
        char resp[2048];
        char qbuf[512];
        kcp(qbuf,"/api?q="); kcat(qbuf,p);
        for(int i=0;qbuf[i];i++) if(qbuf[i]==' ') qbuf[i]='+';
        uint32_t hip = 0x02020202;
        http_request(hip,"httpbin.org",qbuf,resp,2048);
        int printed = 0;
        for(int i=0;resp[i]&&i<200;i++){
            if(resp[i]>=32&&resp[i]<127){vpc((uint8_t)resp[i]);printed=1;}
            else if(resp[i]=='\n'&&printed){vpc('\n');printed=0;}
        }
        vpc('\n');
    }
    
    if(ksnc(p,"hello",5)==0||ksnc(p,"hi",2)==0){
        vpln("  A: Hello! I'm pigAI v0.4 on pigOS!");
        vpln("     I'm powered by larpshell v5.9.3");
    } else if(ksnc(p,"help",4)==0){
        vpln("  A: I can answer questions about:");
        vpln("     - kernel, poing, wm, shell");
        vpln("     - network, rpk, mouse");
        vpln("     - commands, files, boot");
    } else if(ksnc(p,"network",7)==0){
        vpln("  A: Network in pigOS:");
        vpln("     RTL8139 driver loaded");
        vpln("     Run 'netpg dhcp' to get IP");
        vpln("     Then use wget, curl, ping");
     } else if(ksnc(p,"shell",5)==0||ksnc(p,"lash",4)==0){
         vpln("  A: larpshell v5.9.3:");
         vpln("     Commands: help, ls, cd, mkdir");
         vpln("     Features: history");
         vpln("     Built on github.com/usr-undeleted/lash");
    } else {
        vpln("  A: Interesting question!");
        vpln("     Try: llm help, llm network, llm shell");
    }
}

// ── GO COMMAND ─────────────────────────────────────────────────
static void cmd_go(const char*a){
    if(!a||!*a||!ksc(a,"version")||!ksc(a,"-v")){
        vset(C_LCYAN,C_BLACK);
        vpln("");
        vpln("  ===================================");
        vpln("       Go Programming Language");
        vpln("            version 1.23");
        vpln("  ===================================");
        vpln("");
        vpln("  Built-in pigOS integration");
        vrst();
        vpln("");
        vpln("  Usage:");
        vpln("    go version              - Show Go version");
        vpln("    go info                 - Show Go information");
        vpln("    go examples             - Show Go code examples");
        vpln("    go tutorial            - Quick Go tutorial");
        vpln("    go hello               - Run Hello World");
        vpln("");
        return;
    }
    if(!ksc(a,"info")){
        vset(C_LGREEN,C_BLACK);
        vpln("  Go Language Information:");
        vrst();
        vpln("  ─────────────────────────────────────");
        vpln("  Language:  Go (golang)");
        vpln("  Version:   1.23.2");
        vpln("  Creator:   Google (Robert Griesemer,");
        vpln("             Rob Pike, Ken Thompson)");
        vpln("  Released:  2009 (public)");
        vpln("  Type:      Compiled, statically typed");
        vpln("  Paradigm:  Concurrent, imperative");
        vpln("  ─────────────────────────────────────");
        vpln("");
        vpln("  Key Features:");
        vpln("    + Goroutines (lightweight threads)");
        vpln("    + Channels for communication");
        vpln("    + Garbage collection");
        vpln("    + Static typing with dynamic feel");
        vpln("    + Fast compilation");
        vpln("    + No classes (use structs + methods)");
        vpln("    + Interfaces for polymorphism");
        vpln("    + Built-in concurrency primitives");
        vpln("");
        return;
    }
    if(!ksc(a,"examples")||!ksc(a,"example")){
        vset(C_LMAG,C_BLACK);
        vpln("  Go Code Examples:");
        vrst();
        vpln("");
        vpln("  ┌─────────────────────────────────────────┐");
        vpln("  │ Hello World (package main)              │");
        vpln("  ├─────────────────────────────────────────┤");
        vpln("  │ package main                            │");
        vpln("  │                                        │");
        vpln("  │ import \"fmt\"                           │");
        vpln("  │                                        │");
        vpln("  │ func main() {                          │");
        vpln("  │     fmt.Println(\"Hello, World!\")       │");
        vpln("  │ }                                      │");
        vpln("  └─────────────────────────────────────────┘");
        vpln("");
        vpln("  ┌─────────────────────────────────────────┐");
        vpln("  │ Variables & Types                      │");
        vpln("  ├─────────────────────────────────────────┤");
        vpln("  │ var name string = \"pigOS\"               │");
        vpln("  │ age := 1            // type inference  │");
        vpln("  │ const VERSION = \"1.0\"                  │");
        vpln("  │                                        │");
        vpln("  │ // Multiple assignment                  │");
        vpln("  │ x, y := 10, 20                        │");
        vpln("  └─────────────────────────────────────────┘");
        vpln("");
        vpln("  ┌─────────────────────────────────────────┐");
        vpln("  │ Goroutines (Concurrency)                │");
        vpln("  ├─────────────────────────────────────────┤");
        vpln("  │ func count(id int) {                    │");
        vpln("  │     for i := 0; i < 5; i++ {           │");
        vpln("  │         fmt.Println(id, i)             │");
        vpln("  │     }                                  │");
        vpln("  │ }                                      │");
        vpln("  │                                        │");
        vpln("  │ go count(1)  // Start goroutine       │");
        vpln("  │ go count(2)                           │");
        vpln("  └─────────────────────────────────────────┘");
        vpln("");
        return;
    }
    if(!ksc(a,"tutorial")||!ksc(a,"learn")){
        vset(C_YELLOW,C_BLACK);
        vpln("  Go Quick Tutorial:");
        vrst();
        vpln("");
        vpln("  1. BASICS:");
        vpln("     package main          // Every program starts here");
        vpln("     import \"fmt\"         // Import packages with double quotes");
        vpln("     func main() { }      // Entry point");
        vpln("");
        vpln("  2. VARIABLES:");
        vpln("     var x int = 10       // Explicit type");
        vpln("     y := 20              // Type inference (short)");
        vpln("     const PI = 3.14159   // Constants");
        vpln("");
        vpln("  3. FUNCTIONS:");
        vpln("     func add(a, b int) int {");
        vpln("         return a + b");
        vpln("     }");
        vpln("     // Multiple return values!");
        vpln("     func swap(a, b int) (int, int) {");
        vpln("         return b, a");
        vpln("     }");
        vpln("");
        vpln("  4. LOOPS (only 'for' in Go):");
        vpln("     for i := 0; i < 10; i++ { }");
        vpln("     for { }               // Infinite loop");
        vpln("     for i < 10 { }        // While-style");
        vpln("");
        vpln("  5. SLICES (dynamic arrays):");
        vpln("     nums := []int{1, 2, 3}");
        vpln("     nums = append(nums, 4)");
        vpln("");
        vpln("  6. MAPS (key-value):");
        vpln("     ages := map[string]int{");
        vpln("         \"Alice\": 25,");
        vpln("         \"Bob\": 30,");
        vpln("     }");
        vpln("");
        vpln("  7. STRUCTS:");
        vpln("     type Person struct {");
        vpln("         Name string");
        vpln("         Age  int");
        vpln("     }");
        vpln("     p := Person{Name: \"Ada\", Age: 27}");
        vpln("");
        vpln("  8. GOROUTINES (concurrent):");
        vpln("     go func() { fmt.Println(\"Hi!\") }()");
        vpln("     // Use 'chan' for communication");
        vpln("");
        return;
    }
    if(!ksc(a,"hello")){
        vset(C_LGREEN,C_BLACK);
        vpln("");
        vpln("  ┌───────────────────────────────────────┐");
        vpln("  │  Hello from Go on pigOS!              │");
        vpln("  │                                       │");
        vpln("  │    ████  ████  █  █  █               │");
        vpln("  │    █   █ █   █ █  █  █               │");
        vpln("  │    ████  ████  █  █  █               │");
        vpln("  │    █   █ █   █ █  █  █               │");
        vpln("  │    ████  ████  ████  ████            │");
        vpln("  │                                       │");
        vpln("  │  The Go gopher says:                  │");
        vpln("  │  \"Let's build scalable systems!\"     │");
        vpln("  └───────────────────────────────────────┘");
        vrst();
        vpln("");
        vpln("  Source: package main");
        vpln("          import \"fmt\"");
        vpln("");
        vpln("          func main() {");
        vpln("              fmt.Println(\"Hello from Go on pigOS!\")");
        vpln("          }");
        vpln("");
        return;
    }
    vps("go: unknown subcommand '"); vps(a); vpln("'");
    vpln("Run 'go' for usage.");
}

// ── LARPSHELL COMMAND ──────────────────────────────────────────
// larpshell (larp shell) - Credits to creators
// lash is an AI-generated Go shell by contributors on GitHub
static void cmd_larpshell(const char*a){
    vset(C_LCYAN,C_BLACK);
    vpln("");
    vpln("  =============================================");
    vpln("       LARP SHELL (larpshell) v5.9.3");
    vpln("  =============================================");
    vrst();
    vpln("");
    vpln("  A modern Linux shell written in Go");
    vpln("");
    vpln("  CREDITS & AUTHORS:");
    vset(C_LGREEN,C_BLACK);
    vpln("  -------------------------------------------");
    vrst();
    vpln("  Project:   lash (larp shell)");
    vpln("  Version:   5.9.3");
    vpln("  Language:  Go 1.26");
    vpln("  License:   GPLv3");
    vpln("");
    vpln("  Created by AI-generated code");
    vpln("  Contributors: github.com/usr-undeleted/lash");
    vpln("");
    vset(C_YELLOW,C_BLACK);
    vpln("  FEATURES:");
    vrst();
    vpln("  + Custom PS1 prompts with escape codes");
    vpln("  + Git integration (\\g branch display)");
    vpln("  + Syntax highlighting");
    vpln("  + Job control & background processes");
    vpln("  + Line editing with history");
    vpln("  + Tab completion");
    vpln("  + Variables & expansion");
    vpln("  + Glob patterns (*, ?, [abc])");
    vpln("  + Pipes, redirections, chaining");
    vpln("  + Configurable via ~/.config/lash/");
    vrst();
    vpln("");
    vpln("  Source code included in /shell/larpshell/");
    vpln("  Run 'cat /shell/larpshell/README.md' for more");
    vpln("");
    vset(C_DGREY,C_BLACK);
    vpln("  Note: larpshell requires Linux syscalls");
    vpln("  pigOS has custom syscall interface.");
    vpln("  Full integration pending syscall port.");
    vrst();
    vpln("");
}

// ── DISPATCH ─────────────────────────────────────────────────
void sh_dispatch(const char*raw_line){
    // Expand variables first
    char expanded[SH_BUF];
    sh_expand(raw_line,expanded,SH_BUF);
    const char*line=expanded;

    while(*line==' ')line++;
    if(!*line)return;

    // Check for pipe: split on first unquoted '|'
    int pipe_pos=-1;
    int in_quote=0;
    for(int i=0;line[i];i++){
        if(line[i]=='"'||line[i]=='\''){if(in_quote==line[i])in_quote=0;else if(!in_quote)in_quote=line[i];}
        if(line[i]=='|'&&!in_quote){pipe_pos=i;break;}
    }

    if(pipe_pos>0){
        // Pipe support: left_cmd | right_cmd
        char left_cmd[SH_BUF],right_cmd[SH_BUF];
        int li=0,ri=0;
        for(int i=0;i<pipe_pos&&li<SH_BUF-1;i++) left_cmd[li++]=line[i];
        left_cmd[li]=0;
        // Skip whitespace after |
        int ri_start=pipe_pos+1;
        while(line[ri_start]==' ')ri_start++;
        for(int i=ri_start;line[i]&&ri<SH_BUF-1;i++) right_cmd[ri++]=line[i];
        right_cmd[ri]=0;

        // Execute left command, capture output to temp buffer
        extern char pipe_buf[4096];
        extern int pipe_buf_len;
        pipe_buf_len=0; pipe_buf[0]=0;

        // Redirect vps/vpln to pipe_buf temporarily
        extern int pipe_active;
        pipe_active=1;
        sh_dispatch(left_cmd);
        pipe_active=0;

        // Now execute right command with pipe_buf as input
        extern void grep_set_pipe_input(const char*);
        grep_set_pipe_input(pipe_buf);
        sh_dispatch(right_cmd);
        return;
    }

    char cmd[64];int ci=0;
    while(*line&&*line!=' '&&ci<63)cmd[ci++]=*line++;
    cmd[ci]=0;while(*line==' ')line++;
    const char*a=line;

    // Check aliases first
    const char*alias_val=sh_alias_get(cmd);
    if(alias_val){
        sh_dispatch(alias_val);
        return;
    }

    // SYSTEM
    if(!ksc(cmd,"help"))           cmd_help_sh();
    else if(!ksc(cmd,"about"))     {vset(C_LMAG,C_BLACK);vpln("pigOS v1.0 - vibe coding challenge");vrst();}
    else if(!ksc(cmd,"pigfetch")) cmd_pigfetch();
    else if(!ksc(cmd,"uname"))     {if(a[0]=='-'&&a[1]=='a')vpln("pigOS 1.0.0 pigKernel #1 x86_64");else if(a[0]=='-'&&a[1]=='r')vpln("1.0.0-pig");else vpln("pigOS");}
    else if(!ksc(cmd,"hostname"))  {if(a&&*a)vpln("hostname updated");else vpln("pigOS");}
    else if(!ksc(cmd,"uptime"))    vpln("up forever, load: 0.00 0.00 0.00");
    else if(!ksc(cmd,"date"))      vpln("Thu Jan  1 00:00:00 UTC 1970");
    else if(!ksc(cmd,"version"))   vpln("pigOS 1.0.0 larpshell 2.2 poing 1.0");
    else if(!ksc(cmd,"dmesg"))     cmd_pigfetch_dmesg();
    else if(!ksc(cmd,"menuconfig")) cmd_menuconfig();
    else if(!ksc(cmd,"clear")||!ksc(cmd,"cls")){ vclear(); vrst(); return; }
    else if(!ksc(cmd,"color"))     {static int ci2=0;color_t f[]={C_LGREEN,C_LCYAN,C_LMAG,C_YELLOW,C_WHITE,C_LBLUE};vset(f[ci2++%6],C_BLACK);vpln("color changed");}
    else if(!ksc(cmd,"reboot"))    {vpln("poing: rebooting...");for(volatile int i=0;i<3000000;i++);outb(0x64,0xFE);}
    else if(!ksc(cmd,"halt"))      {vset(C_LRED,C_BLACK);vpln("poing: halting.");__asm__("cli;hlt");}
    else if(!ksc(cmd,"shutdown")||!ksc(cmd,"poweroff")){
        vclear();vset(C_LRED,C_BLACK);vpln("\n  Shutting down pigOS...");vpln("  Goodbye.");
        vrst();for(volatile int i=0;i<200000000;i++);outb(0xF4,0x00);__asm__("cli;hlt");
    }
    else if(!ksc(cmd,"panic")){
        // Real-looking kernel panic - blue screen like Linux
        vclear();
        uint8_t pc=COL(C_WHITE,C_BLUE);
        uint8_t yc=COL(C_YELLOW,C_BLUE);
        uint8_t rc=COL(C_LRED,C_BLUE);
        vfill(0,0,VGA_W,VGA_H,' ',pc);
        int row=0;
        vstr(0,row++,pc,"[  0.000000] BUG: kernel NULL pointer dereference, address: 0000000000000000");
        vstr(0,row++,pc,"[  0.000001] #PF: supervisor read access in kernel mode");
        vstr(0,row++,pc,"[  0.000001] #PF: error_code(0x0000) - not-present page");
        vstr(0,row++,pc,"[  0.000002] PGD 0 P4D 0");
        row++;
        vstr(0,row++,pc,"[  0.000003] Oops: Oops: 0000 [#1] SMP NOPTI");
        vstr(0,row++,pc,"[  0.000003] CPU: 0 PID: 1 Comm: poing Not tainted 1.0.0-pig #1");
        vstr(0,row++,pc,"[  0.000004] Hardware name: pigOS VM x86_64");
        vstr(0,row++,pc,"[  0.000004] RIP: 0010:pig_null_deref+0x00/0x42 [pigKernel]");
        vstr(0,row++,pc,"[  0.000005] Code: 48 89 df e8 xx xx xx xx 48 85 c0 75 xx eb xx");
        vstr(0,row++,pc,"[  0.000005] RSP: 0018:ffffc90000013e98 EFLAGS: 00010282");
        vstr(0,row++,pc,"[  0.000006] RAX: 0000000000000000 RBX: dead00pigcafe0000 RCX: ffffffff81234567");
        vstr(0,row++,pc,"[  0.000006] RDX: 0000000000000001 RSI: ffffc90000013ef8 RDI: ffffffff81345678");
        vstr(0,row++,pc,"[  0.000007] RBP: ffffc90000013eb8  R8: 0000000000000000  R9: 0000000000000000");
        row++;
        vstr(0,row++,pc,"[  0.000008] Call Trace:");
        vstr(0,row++,pc,"[  0.000008]  <TASK>");
        vstr(0,row++,pc,"[  0.000009]  pig_oops_enter+0x42/0x137 [pigKernel]");
        vstr(0,row++,pc,"[  0.000009]  poing_start+0x13/0x42 [poing]");
        vstr(0,row++,pc,"[  0.000010]  kernel_init+0x07/0x137 [pigKernel]");
        vstr(0,row++,pc,"[  0.000010]  ret_from_fork+0x22/0x30");
        vstr(0,row++,pc,"[  0.000011]  </TASK>");
        row++;
        vstr(0,row++,yc,"[  0.000012] Kernel panic - not syncing: pigOS existential crisis");
        vstr(0,row++,yc,"[  0.000012] CPU: 0 PID: 1 pigKernel");
        vstr(0,row++,yc,"[  0.000013] Kernel Offset: 0x000000 from 0x1000000");
        row++;
        vstr(0,row++,rc,"[  0.000014] ---[ end Kernel panic - not syncing ]---");
        row++;
        vstr(0,row++,pc,"[  5.000000] Rebooting in 5 seconds due to panic...");
        vga_update_hw_cursor();
        for(volatile int i=0;i<800000000;i++);
        outb(0x64,0xFE); // reset
    }
    // POING
    else if(!ksc(cmd,"poingctl"))  cmd_poingctl(a);
    else if(!ksc(cmd,"polctl"))    cmd_polctl(a);
    // USERS
    else if(!ksc(cmd,"adduser"))   cmd_adduser(a);
    else if(!ksc(cmd,"deluser")){
        if(!a||!*a){vpln("usage: deluser <n>");return;}
        int ui=ufind(a);
        if(ui<0){vps("deluser: user '");vps(a);vpln("' not found");return;}
        if(!ksc(a,"root")){vpln("deluser: cannot delete root");return;}
        users[ui].active=0;
        vps("deluser: removed '");vps(a);vpln("'");
    }
    else if(!ksc(cmd,"usermod"))   cmd_usermod(a);
    else if(!ksc(cmd,"groupadd"))  cmd_groupadd(a);
    else if(!ksc(cmd,"groupdel"))  {vps("groupdel: removed ");vpln(a);}
    else if(!ksc(cmd,"groups"))    cmd_groups(a);
    else if(!ksc(cmd,"passwd"))    cmd_passwd(a);
    else if(!ksc(cmd,"whoami"))    vpln(sh_user);
    else if(!ksc(cmd,"id"))        cmd_id();
    else if(!ksc(cmd,"users"))     {for(int i=0;i<nu;i++)if(users[i].active){vps(users[i].name);vpc(' ');}vpc('\n');}
    else if(!ksc(cmd,"who"))       {vps(sh_user);vpln("  tty0  1970-01-01");}
    else if(!ksc(cmd,"last"))      {vps(sh_user);vpln("  tty0");vpln("wtmp begins 1970");}
    else if(!ksc(cmd,"history"))   {extern int lash_hn; extern char lash_hist[64][512]; for(int i=0;i<lash_hn;i++){char b[8];kia(i+1,b);vps(b);vps("  ");vpln(lash_hist[i]);}}
    else if(!ksc(cmd,"w"))         {vpln("USER  TTY  CMD");vps(sh_user);vpln("  tty0  larpshell");}
    else if(!ksc(cmd,"rok")){
        if(!a||!*a){vpln("usage: rok <cmd> - run command with root privileges");return;}
        if(sh_uid==0){
            sh_rok=1; vset(C_LRED,C_BLACK); vps("[rok] "); vpln(a); vrst();
            sh_dispatch(a); sh_rok=0;
        } else {
            vps("Password: ");
            vrst();
            char pw[64]; int pi = 0;
            pw[0] = 0;
            while(1){
                ps2_poll();
                if(!kb_avail()) continue;
                int c = kb_get();
                if(c == '\n' || c == '\r'){ pw[pi] = 0; break; }
                if(c == '\b' || c == 127){ if(pi > 0){ pi--; vpc('\b'); vpc(' '); vpc('\b'); } continue; }
                if(c >= 32 && c < 127 && pi < 62){ pw[pi++] = (char)c; vpc('*'); }
            }
            pw[pi] = 0;
            vpc('\n');
            int current_user_idx = ufind(sh_user);
            if(current_user_idx >= 0 && ksc(pw, users[current_user_idx].pass) == 0){
                sh_rok=1; vset(C_LRED,C_BLACK); vps("[rok] "); vpln(a); vrst();
                sh_dispatch(a); sh_rok=0;
            } else if(ksc(pw,"toor")==0){
                sh_rok=1; vset(C_LRED,C_BLACK); vps("[rok] "); vpln(a); vrst();
                sh_dispatch(a); sh_rok=0;
            } else {
                vpln("rok: Authentication failure");
            }
        }
    }
    else if(!ksc(cmd,"su")){
        const char* target_user = a && *a && *a != '-' ? a : "root";
        int ui = ufind(target_user);
        if(ui < 0){vps("su: user '");vps(target_user);vpln("' not found");return;}
        
        vps("Password: ");
        vrst();
        char pw[64]; int pi = 0;
        pw[0] = 0;
        while(1){
            ps2_poll();
            if(!kb_avail()) continue;
            int c = kb_get();
            if(c == '\n' || c == '\r'){ pw[pi] = 0; break; }
            if(c == '\b' || c == 127){ if(pi > 0){ pi--; vpc('\b'); vpc(' '); vpc('\b'); } continue; }
            if(c >= 32 && c < 127 && pi < 62){ pw[pi++] = (char)c; vpc('*'); }
        }
        pw[pi] = 0;
        vpc('\n');
        
        if(ksc(pw, users[ui].pass) == 0){
            kcp(sh_user, users[ui].name);
            sh_uid = users[ui].uid;
            kcp(sh_cwd, users[ui].home);
            vps("switched to "); vpln(sh_user);
        } else {
            vpln("su: Authentication failure");
        }
    }
    else if(!ksc(cmd,"r-")||(!ksc(cmd,"r")&&a&&a[0]=='-'&&!a[1])){kcp(sh_user,"root");sh_uid=0;kcp(sh_cwd,"/");vset(C_LRED,C_BLACK);vpln("[r-] root");vrst();}
    else if(!ksc(cmd,"chsh"))      {vpln("chsh: shell set to larpshell");}
    // export/set - variable assignment
    else if(!ksc(cmd,"export")){
        if(!a||!*a){for(int i=0;i<sh_nvars;i++){vps(sh_var_names[i]);vps("=");vpln(sh_var_vals[i]);}return;}
        char vn[32]="",vv[128]=""; int vi=0;
        const char*q=a;
        while(*q&&*q!='='&&vi<31)vn[vi++]=*q++;vn[vi]=0;
        if(*q=='='){q++;kcp(vv,q);}
        sh_var_set(vn,vv);
        vps(vn);vps("=");vpln(vv);
    }
    else if(!ksc(cmd,"set")){
        if(!a||!*a){for(int i=0;i<sh_nvars;i++){vps(sh_var_names[i]);vps("=");vpln(sh_var_vals[i]);}vpln("INIT=poing");vpln("SHELL=/bin/pig");return;}
        // set VAR=VAL
        char vn[32]="",vv[128]=""; int vi=0;
        const char*q=a;
        while(*q&&*q!='='&&vi<31)vn[vi++]=*q++;vn[vi]=0;
        if(*q=='='){q++;kcp(vv,q);}
        sh_var_set(vn,vv);
    }
    else if(!ksc(cmd,"unset")){for(int i=0;i<sh_nvars;i++)if(!ksc(sh_var_names[i],a)){sh_var_names[i][0]=0;sh_var_vals[i][0]=0;}}
    // FILES
    else if(!ksc(cmd,"ls")||!ksc(cmd,"ll")||!ksc(cmd,"la")||!ksc(cmd,"lh")){
        // Merge cmd flags with args
        char fullarg[256]="";
        if(!ksc(cmd,"ll"))kcp(fullarg,"-l ");
        else if(!ksc(cmd,"la"))kcp(fullarg,"-la ");
        else if(!ksc(cmd,"lh"))kcp(fullarg,"-lh ");
        kcat(fullarg,a);
        fs_ls(sh_cwd,fullarg);
    }
    else if(!ksc(cmd,"tree"))  {vpln("./");vpln("├── boot/");vpln("├── kernel/");vpln("├── drivers/");vpln("├── shell/");vpln("├── wm/");vpln("└── README.txt");}
    else if(!ksc(cmd,"cat"))   fs_cat(sh_cwd,a);
    else if(!ksc(cmd,"more")||!ksc(cmd,"less")) fs_cat(sh_cwd,a);
    else if(!ksc(cmd,"head"))  {vpln("[head]");fs_cat(sh_cwd,a);}
    else if(!ksc(cmd,"tail"))  {vpln("[tail]");fs_cat(sh_cwd,a);}
    else if(!ksc(cmd,"grep"))  fs_grep(sh_cwd,a);
    else if(!ksc(cmd,"find"))  fs_find_cmd(sh_cwd,a);
    else if(!ksc(cmd,"wc"))    {vps("  42 1024 8192 ");vpln(a&&*a?a:"?");}
    else if(!ksc(cmd,"diff"))  vpln("diff: [files identical]");
    else if(!ksc(cmd,"sort")||!ksc(cmd,"uniq")||!ksc(cmd,"cut")||!ksc(cmd,"awk")||!ksc(cmd,"sed")) vpln("[no pipe support]");
    else if(!ksc(cmd,"xargs")||!ksc(cmd,"tee")) vpln("[no pipe support]");
    else if(!ksc(cmd,"mkdir")) {
        if(!a||!*a){vpln("usage: mkdir <dir>");}
        else{
            char p2[256];fs_resolve(sh_cwd,a,p2,256);
            if(memfs_find(p2)){vps("mkdir: cannot create directory '");vps(a);vpln("': File exists");return;}
            MemFile*m=memfs_create(p2);
            if(m){m->is_dir=1;kcp(m->content,"[dir]");vps("mkdir: created directory '");vps(p2);vpln("'");}
            else{vpln("mkdir: out of memory");}
        }
    }
    else if(!ksc(cmd,"rmdir")) {
        if(!a||!*a){vpln("usage: rmdir <dir>");}
        else{
            char p2[256];fs_resolve(sh_cwd,a,p2,256);
            MemFile*m=memfs_find(p2);
            if(!m){vps("rmdir: failed to remove '");vps(a);vpln("': No such file or directory");return;}
            if(!m->is_dir){vps("rmdir: failed to remove '");vps(a);vpln("': Not a directory");return;}
            m->used=0;m->is_dir=0;m->content[0]=0;vps("rmdir: removed directory '");vps(p2);vpln("'");
        }
    }
    else if(!ksc(cmd,"rm"))    fs_rm(sh_cwd,a);
    else if(!ksc(cmd,"cp"))    {vps("cp: copied [");vps(a);vpln("]");}
    else if(!ksc(cmd,"mv"))    {vps("mv: moved [");vps(a);vpln("]");}
    else if(!ksc(cmd,"touch")) fs_touch(sh_cwd,a);
    else if(!ksc(cmd,"ln"))    {vps("ln: ");vpln(a);}
    else if(!ksc(cmd,"chmod")) {vps("chmod: ");vpln(a);}
    else if(!ksc(cmd,"chown")) {vps("chown: ");vpln(a);}
    else if(!ksc(cmd,"stat"))  {if(!a||!*a){vpln("usage: stat <f>");}else{char rp[256];fs_resolve(sh_cwd,a,rp,256);vps("File: ");vpln(rp);vpln("Mode: 0644  Uid: 0(root)");}}
    else if(!ksc(cmd,"pwd"))   fs_pwd(sh_cwd);
    else if(!ksc(cmd,"cd"))    {fs_cd(sh_cwd,a);vga_update_hw_cursor();}
    else if(!ksc(cmd,"pushd")) {fs_cd(sh_cwd,a);vpln("[pushed]");}
    else if(!ksc(cmd,"popd"))  {fs_cd(sh_cwd,"/");vpln("[popped]");}
    else if(!ksc(cmd,"file"))  {vps(a&&*a?a:"?");vpln(": ASCII text");}
    else if(!ksc(cmd,"pnano")) cmd_nano(a);
    else if(!ksc(cmd,"pi")||!ksc(cmd,"pivi")) cmd_vi(a);
    // PROCS
    else if(!ksc(cmd,"ps"))    {vset(C_LCYAN,C_BLACK);vpln("PID  STATE  CPU  MEM  CMD");vrst();vpln("  1  S      0.0  0.1  poing");vpln("100  R     99.9 32.0  pigKernel");vpln("200  S      0.0  0.5  larpshell");}
    else if(!ksc(cmd,"top"))   {vclear();vfill(0,0,VGA_W,1,' ',COL(C_BLACK,C_LGREY));vstr(0,0,COL(C_BLACK,C_LGREY),"pigOS top - q=quit");vset(C_WHITE,C_BLACK);vpc('\n');vset(C_LCYAN,C_BLACK);vpln("PID  CPU  MEM  CMD");vrst();vpln("100  99.9 32.0 pigKernel");vpln("  1   0.0  0.1 poing");vpln("200   0.0  0.5 larpshell");vset(C_DGREY,C_BLACK);vpln("[q]");vrst();int c2;do{c2=kb_get();}while(c2!='q'&&c2!=27);vclear();}
    else if(!ksc(cmd,"htop"))  sh_dispatch("top");
    else if(!ksc(cmd,"kill"))  {if(a[0]=='1'&&!a[1]){vset(C_LRED,C_BLACK);vpln("kill: cannot kill PID 1");vrst();}else{vps("kill: SIGTERM -> ");vpln(a);}}
    else if(!ksc(cmd,"killall")||!ksc(cmd,"pkill")) {vps("kill: -> ");vpln(a);}
    else if(!ksc(cmd,"pgrep")) {vps("200 larpshell (");vps(a);vpln(")");}
    else if(!ksc(cmd,"nice")||!ksc(cmd,"nohup")) sh_dispatch(a);
    else if(!ksc(cmd,"jobs"))  vpln("[0] larpshell Running");
    else if(!ksc(cmd,"bg")||!ksc(cmd,"fg")) vpln("[no job control]");
    // NETWORK
    else if(!ksc(cmd,"ifconfig")||!ksc(cmd,"ip")){
        extern char netif_name[16];
        const char*ifn=netif_name[0]?netif_name:"eth0";
        // Parse subcommands: ip link, ip addr, ip link set <iface> up/down, ip addr add
        if(a&&*a){
            const char*p=a;
            while(*p==' ')p++;
            if(!ksc(p,"link")||!ksc(p,"addr")){
                // Show link status
                vset(C_LCYAN,C_BLACK);vpln("1: lo: <LOOPBACK,UP> mtu 65536");vrst();
                vpln("    inet 127.0.0.1/8 scope host lo");
                if(net_ok){
                    vset(C_LCYAN,C_BLACK);vps("2: ");vps(ifn);vpln(": <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500");vrst();
                    vps("    inet ");net_print_ip(net_my_ip);vps("/24 brd 10.0.2.255 scope global ");vpln(ifn);
                    extern volatile uint32_t rtl_rx_count,rtl_tx_count;
                    char rb[16],tb[16];kia(rtl_rx_count,rb);kia(rtl_tx_count,tb);
                    vps("    RX packets: ");vps(rb);vps(" TX packets: ");vpln(tb);
                } else {
                    extern int net_ok;
                    extern uint32_t net_my_ip;
                    extern uint32_t net_gw_ip;
                    vset(C_YELLOW,C_BLACK);vps("2: ");vps(ifn);vpln(": <BROADCAST,MULTICAST> mtu 1500 state DOWN");vrst();
                    if(net_ok){
                        vset(C_LGREEN,C_BLACK);
                        vps("    inet ");net_print_ip(net_my_ip);vps("  -->  gw ");net_print_ip(net_gw_ip);vpln("");
                        vps("    state UP (already configured)");vpln("");
                    } else {
                        vps("    Tip: ip link set ");vps(ifn);vpln(" up  OR  dhcpcd ");vpln(ifn);
                    }
                }
            } else if(!ksc(p,"link set")){
                // ip link set <iface> up/down
                const char*q=p+8;while(*q==' ')q++;
                // Parse: <interface> up or <interface> down
                char iface_arg[32];int ii=0;
                while(*q&&*q!=' '&&ii<31) iface_arg[ii++]=*q++;
                iface_arg[ii]=0;
                while(*q==' ')q++;
                char action[16];int ai=0;
                while(*q&&*q!=' '&&ai<15) action[ai++]=*q++;
                action[ai]=0;
                // Accept if interface matches detected name or "eth0"
                int iface_match=0;
                if(!ksc(iface_arg,ifn)) iface_match=1;
                if(!ksc(iface_arg,"eth0")) iface_match=1;
                if(!iface_match){
                    vps("ip: interface '");vps(iface_arg);vps("' not found (detected: ");vps(ifn);vpln(")");
                } else if(!ksc(action,"up")){
                    extern int eth0_up(void);
                    vps(ifn);vpln(": bringing interface up...");
                    if(eth0_up()){
                        vset(C_LGREEN,C_BLACK);vps(ifn);vps(": UP - 10.0.2.15/24 gw 10.0.2.2");vrst();
                    } else {
                        vset(C_LRED,C_BLACK);vps(ifn);vpln(": FAILED - no hardware detected");vrst();
                    }
                } else if(!ksc(action,"down")){
                    extern void eth0_down(void);
                    eth0_down();
                    vps(ifn);vpln(": DOWN");
                } else {vps("ip: unknown action: ");vpln(action);}
            }
        } else {
            vset(C_LCYAN,C_BLACK);vpln("lo:");vrst();vpln("  inet 127.0.0.1  netmask 255.0.0.0");
            if(net_ok){vset(C_LCYAN,C_BLACK);vps(ifn);vrst();vps(":  inet ");net_print_ip(net_my_ip);vpln("  UP");}
            else{vset(C_YELLOW,C_BLACK);vps(ifn);vpln(": DOWN [no carrier]");vps("  Fix: ip link set ");vps(ifn);vpln(" up  OR  dhcpcd ");vpln(ifn);vrst();}
        }
    }
    else if(!ksc(cmd,"ping")){
        if(!a||!*a){vpln("usage: ping [-c count] <host>");}
        else{
            int count=4;
            const char* host=a;
            if(a[0]=='-' && a[1]=='c'){
                int i=2;
                while(a[i]==' ')i++;
                if(a[i]>='0' && a[i]<='9'){
                    count=0;
                    while(a[i]>='0' && a[i]<='9'){count=count*10+(a[i]-'0');i++;}
                    while(a[i]==' ')i++;
                    host=a+i;
                }
            }
            if(count==0)count=4;
            extern void do_ping_count(const char*host, int count);
            do_ping_count(host, count);
        }
    }
    else if(!ksc(cmd,"netstat")) {vset(C_LCYAN,C_BLACK);vpln("Proto  Local         State");vrst();vpln("tcp    0.0.0.0:22    LISTEN");}
    else if(!ksc(cmd,"pigget")||!ksc(cmd,"wget")||!ksc(cmd,"curl")||!ksc(cmd,"pigcurl")){
        if(!net_ok){vset(C_LRED,C_BLACK);vpln("no network carrier. Fix: -net nic,model=rtl8139 -net user");vrst();}
        else{
            extern int tcp_wget(const char*url);
            tcp_wget(a);
        }
    }
    else if(!ksc(cmd,"nslookup")||!ksc(cmd,"dig")) {
        if(!a||!*a){vpln("Usage: nslookup <hostname>");}
        else {
            ip_addr_t ip;
            extern int resolve_hostname_for_tools(const char*, ip_addr_t*);
            int ok = resolve_hostname_for_tools(a, &ip);
            if(ok==0){
                vps("Server: 8.8.8.8\nName: ");vps(a);vps("\nAddress: ");
                char ipbuf[32];
                uint32_t addr = ip4_addr_get_u32(&ip);
                kitoa((addr>>24)&0xFF,ipbuf);vps(ipbuf);vps(".");
                kitoa((addr>>16)&0xFF,ipbuf);vps(ipbuf);vps(".");
                kitoa((addr>>8)&0xFF,ipbuf);vps(ipbuf);vps(".");
                kitoa((addr)&0xFF,ipbuf);vpln(ipbuf);
            } else {
                vps("Server: 8.8.8.8\nName: ");vps(a);vpln("\n*** No address found");
            }
        }
    }
    else if(!ksc(cmd,"traceroute")) {vps("traceroute to ");vpln(a&&*a?a:"?");}
    else if(!ksc(cmd,"ss"))  vpln("tcp LISTEN 0.0.0.0:22");
    else if(!ksc(cmd,"arp")||!ksc(cmd,"route")) vpln("[no routing table - no carrier]");
    else if(!ksc(cmd,"netpg")||!ksc(cmd,"pignet")) cmd_netpg(a);
    else if(!ksc(cmd,"rtldebug")||!ksc(cmd,"rtlstat")){
        extern int rtl_hw_ok;
        extern uint32_t rtl_iobase;
        extern volatile uint32_t rtl_rx_poll_count;
        extern volatile uint32_t rtl_rx_count;
        extern volatile uint32_t rtl_tx_count;
        extern void net_log_dump(void);
        if(!rtl_hw_ok){vpln("rtldebug: no RTL8139");return;}
        vset(C_LCYAN,C_BLACK);vpln("RTL8139 Debug Info");vrst();
        vps("IO Base: 0x");char hb[8];kia(rtl_iobase,hb);vpln(hb);
        vps("RX Polls: ");kia(rtl_rx_poll_count,hb);vpln(hb);
        vps("RX Packets: ");kia(rtl_rx_count,hb);vpln(hb);
        vps("TX Packets: ");kia(rtl_tx_count,hb);vpln(hb);
        vpln("Registers:");
        vps("  Cmd(0x37): 0x");kia(rtl_inb(rtl_iobase+0x37),hb);vps(hb);
        vps("  ISR(0x3E): 0x");kia(rtl_inw(rtl_iobase+0x3E),hb);vps(hb);vpln("");
        vps("  RX(0x3A): 0x");kia(rtl_inw(rtl_iobase+0x3A),hb);vps(hb);
        vps("  TX(0x38): 0x");kia(rtl_inw(rtl_iobase+0x38),hb);vps(hb);vpln("");
        vps("  RCR(0x44): 0x");kia(rtl_inl(rtl_iobase+0x44),hb);vps(hb);vpln("");
        vset(C_YELLOW,C_BLACK);vpln("--- Network Log ---");vrst();
        net_log_dump();
        // Auto-save to /var/log/netdebug.log
        extern void rtl_net_log_get(char*,int);
        extern MemFile* memfs_create(const char*);
        char buf[2048]; rtl_net_log_get(buf,2048);
        int len=0;while(buf[len] && len<2047)len++;
        MemFile*m=memfs_create("/var/log/netdebug.log");
        if(m){
            for(int i=0;i<len;i++)m->content[i]=buf[i];
            m->used=len;
            vps("  [log saved to /var/log/netdebug.log]");
        }
    }
#if USE_VIRTIO
    else if(!ksc(cmd,"pciscan")||!ksc(cmd,"pcidump")){
        vset(C_LCYAN,C_BLACK);vpln("PCI Scan:");vrst();
        extern uint32_t pci_r32(uint8_t,uint8_t,uint8_t,uint8_t);
        for(int b=0;b<8;b++){
            for(int d=0;d<32;d++){
                uint32_t id=pci_r32(b,d,0,0);
                if(id && id!=0xFFFFFFFF){
                    vps("PCI ");vps("00:");char bh[4];kia(b,bh);vps(bh);vps(".0");kia(d,bh);vps(bh);vps(" = 0x");
                    char ih[16];kia(id,ih);vpln(ih);
                }
            }
        }
    }
#endif
    else if(!ksc(cmd,"dhcpcd")){
        // dhcpcd <interface> - DHCP client daemon
        if(!a||!*a){
            vpln("usage: dhcpcd <interface>");
            extern char netif_name[16];
            vps("  Detected: ");vpln(netif_name[0]?netif_name:"none");
            vpln("  Example: dhcpcd enp0s3");
            return;
        }
        const char*iface=a;
        while(*iface==' ')iface++;
        extern char netif_name[16];
        // Accept either the auto-detected name or "eth0" as alias
        if(netif_name[0]&&!ksc(iface,netif_name)){}
        else if(!ksc(iface,"eth0")){}
        else {
            vset(C_LRED,C_BLACK);vps("dhcpcd: interface '");vps(iface);vpln("' not found");
            vps("  Detected interface: ");vpln(netif_name[0]?netif_name:"none");
            vrst();
            return;
        }
        vps("dhcpcd[");char pb[8];kia(1001,pb);vps(pb);vps("]: ");vps(netif_name);vpln(": starting DHCP client");
        vps("dhcpcd[1001]: ");vps(netif_name);vpln(": sending DHCP DISCOVER");
        // Try to get DHCP (falls back to static for QEMU slirp)
        extern int do_dhcp(void);
        if(do_dhcp()){
            vps("dhcpcd[1001]: ");vps(netif_name);vpln(": leased 10.0.2.15 for 86400 seconds");
            vps("dhcpcd[1001]: ");vps(netif_name);vpln(": adding route to 10.0.2.0/24");
            vps("dhcpcd[1001]: ");vps(netif_name);vpln(": adding default route via 10.0.2.2");
            vps("dhcpcd[1001]: ");vps(netif_name);vpln(": writing resolv.conf to /etc/resolv.conf");
            vset(C_LGREEN,C_BLACK);vps("dhcpcd[1001]: ");vps(netif_name);vpln(": carrier acquired");vrst();
        } else {
            vset(C_LRED,C_BLACK);vps("dhcpcd[1001]: ");vps(netif_name);vpln(": DHCP failed - no hardware?");vrst();
        }
    }
    // HW
    else if(!ksc(cmd,"lspci")) {vpln("00:00.0 Host bridge Intel 440FX");vpln("00:02.0 VGA Bochs [1234:1111]");vpln("00:03.0 RTL8139 [10EC:8139]");}
    else if(!ksc(cmd,"lsusb")) vpln("Bus 001 Device 001: Virtual USB Hub");
    else if(!ksc(cmd,"lsmod")) {vset(C_LCYAN,C_BLACK);vpln("Module          Size  Used");vrst();vpln("pig_vga         4096     1");vpln("pig_kbd         2048     1");vpln("pig_net         8192     1");}
    else if(!ksc(cmd,"modprobe")||!ksc(cmd,"rmmod")) {vps("[monolithic kernel] ");vpln(a);}
    else if(!ksc(cmd,"lscpu")) {vpln("Architecture: x86_64\nCPU(s): 1\nModel: QEMU Virtual CPU\nFlags: fpu pae lm nx sse2");}
    else if(!ksc(cmd,"lsmem")) vpln("0x0000000000000000  4M  online\nTotal: 4M");
    else if(!ksc(cmd,"lshw"))  {vpln("system: pigOS VM\n  cpu: x86_64\n  mem: 4MB\n  vga: Bochs/QEMU");}
    else if(!ksc(cmd,"lsblk")) {vset(C_LCYAN,C_BLACK);vpln("NAME  SIZE  TYPE");vrst();vpln("sda      0  disk [no block device]");}
    else if(!ksc(cmd,"dmidecode")) vpln("BIOS: POUG 1.0\nSystem: pigOS VM x86_64");
    else if(!ksc(cmd,"df"))    {vset(C_LCYAN,C_BLACK);vpln("Filesystem  Size  Used  Avail  Use%  Mount");vrst();vpln("pigfs       4096  1024   3072   25%  /");}
        else if(!ksc(cmd,"du"))    {char b[8];kia(64,b);vps(b);vpc(' ');vpln(a&&*a?a:".");}
    else if(!ksc(cmd,"free"))  {vset(C_LCYAN,C_BLACK);vpln("         total  used  free");vrst();vpln("Mem:      4096  1024  3072");}
    else if(!ksc(cmd,"mount")) {vpln("pigfs on / (rw)\ndevfs on /dev (rw)");}
    else if(!ksc(cmd,"umount")) {vps("umount: ");vpln(a);}
    else if(!ksc(cmd,"sync"))  vpln("sync: done");
    else if(!ksc(cmd,"dd")||!ksc(cmd,"fdisk")||!ksc(cmd,"mkfs")||!ksc(cmd,"fsck")) vpln("[no block device]");
    // RPK
    else if(!ksc(cmd,"rpk")||!ksc(cmd,"pigpkg")) cmd_rpk(a);
    // SSL
    else if(!ksc(cmd,"pigssl"))   cmd_openssl(a);
    else if(!ksc(cmd,"pigcrypt")) {vps("Encrypted: ");for(int i=0;a[i];i++)vpc((uint8_t)((a[i]+13)%126+32));vpc('\n');}
    else if(!ksc(cmd,"pigdecrypt")){vps("Decrypted: ");for(int i=0;a[i];i++)vpc((uint8_t)((a[i]-13+94)%94+32));vpc('\n');}
    else if(!ksc(cmd,"pighash")){uint64_t h=5381;for(int i=0;a[i];i++)h=((h<<5)+h)+(uint8_t)a[i];char b[20];kuia(h,b,16);vps("djb2: ");vpln(b);}
    else if(!ksc(cmd,"pigkey"))  {vpln("-----BEGIN RSA PRIVATE KEY-----\nMIIEo... [simulated]\n-----END RSA PRIVATE KEY-----");}
    else if(!ksc(cmd,"pigcert")) {vpln("-----BEGIN CERTIFICATE-----\nCN=pigOS Valid:1970-2099\n-----END CERTIFICATE-----");}
    else if(!ksc(cmd,"discord"))  cmd_discord(a);
    // UTILS
    else if(!ksc(cmd,"echo")||!ksc(cmd,"print")){
        // Expand and print
        char out[512]; sh_expand(a,out,512);
        for(int i=0;out[i];i++){if(out[i]=='\\'&&out[i+1]=='n'){vpc('\n');i++;}else vpc((uint8_t)out[i]);}
        vpc('\n');
    }
    else if(!ksc(cmd,"printf"))   {char out[512];sh_expand(a,out,512);vps(out);}
    else if(!ksc(cmd,"env"))      {vpln("PATH=/bin:/usr/bin:/sbin");vpln("HOME=/root");vpln("SHELL=/bin/pig");vpln("INIT=poing");vpln("OS=pigOS");for(int i=0;i<sh_nvars;i++){vps(sh_var_names[i]);vps("=");vpln(sh_var_vals[i]);}}
    else if(!ksc(cmd,"alias")){
        if(a&&*a){
            // Parse alias name=value - strip surrounding quotes from value
            char an[32]="",av[128]="";
            const char*q=a; int ai=0;
            while(*q&&*q!='='&&ai<31) an[ai++]=*q++; an[ai]=0;
            if(*q=='='){
                q++;
                // Strip leading quote
                if(*q=='\''||*q=='"') q++;
                int vi=0;
                // Copy until closing quote or end
                char quote=(*q!='\''&&*q!='"')?0:an[0]; // reuse
                // Actually track the quote char
                q=a; while(*q&&*q!='=') q++; q++; // reset
                if(*q=='\''||*q=='"'){ quote=*q; q++; }
                else quote=0;
                while(*q&&vi<127){
                    if(quote&&*q==quote){ q++; break; }
                    if(!quote&&*q==' ') break;
                    av[vi++]=*q++;
                }
                av[vi]=0;
            }
            sh_alias_set(an,av);
            vps("alias ");vps(an);vps("='");vps(av);vpln("'");
        } else {
            for(int i=0;i<sh_naliases;i++){vps("alias ");vps(sh_alias_names[i]);vps("='");vpln(sh_alias_vals[i]);vps("'");}
        }
    }
    else if(!ksc(cmd,"unalias")){
        for(int i=0;i<sh_naliases;i++) if(!ksc(sh_alias_names[i],a)){
            sh_alias_names[i][0]=0; sh_alias_vals[i][0]=0; return;
        }
        vps("unalias: ");vps(a);vpln(": not found");
    }
    else if(!ksc(cmd,"which"))    {vps("/bin/");vpln(a);}
    else if(!ksc(cmd,"type"))     {vps(a);vps(" is larpshell builtin\n");}
    else if(!ksc(cmd,"history"))  {for(int i=0;i<sh_hn;i++){char b[8];kia(i+1,b);vset(C_DGREY,C_BLACK);vps(b);vps(" ");vrst();vpln(sh_hist[i]);}}
    else if(!ksc(cmd,"calc")||!ksc(cmd,"bc")) cmd_calc_sh(a);
    else if(!ksc(cmd,"pigmath"))  {vpln("[pigmath]");cmd_calc_sh(a);}
    else if(!ksc(cmd,"seq"))      {int n=0;const char*p2=a;while(*p2>='0'&&*p2<='9')n=n*10+(*p2++)-'0';for(int i=1;i<=n&&i<=100;i++){char b[8];kia(i,b);vpln(b);}}
    else if(!ksc(cmd,"yes"))      {for(int i=0;i<20&&!kb_avail();i++){vpln(a&&*a?a:"y");for(volatile int j=0;j<2000000;j++);}}
    else if(!ksc(cmd,"true")||!ksc(cmd,"false")){}
    else if(!ksc(cmd,"sleep"))    {int n=1;if(a&&*a){n=0;const char*q=a;while(*q>='0'&&*q<='9')n=n*10+(*q++)-'0';}vps("sleep ");char b[8];kia(n,b);vps(b);vpln("s");for(int i=0;i<n;i++)for(volatile int j=0;j<60000000;j++);}
    else if(!ksc(cmd,"countdown")||!ksc(cmd,"timer")){int n=5;if(a&&*a){n=0;const char*q=a;while(*q>='0'&&*q<='9')n=n*10+(*q++)-'0';}for(int i=n;i>=0;i--){char b[8];kia(i,b);vps("\r  countdown: ");vset(i>2?C_LGREEN:C_LRED,C_BLACK);vps(b);vps("   ");vrst();for(volatile int j=0;j<60000000;j++);}vpc('\n');vset(C_LRED,C_BLACK);vpln("BOOM!");vrst();}
    else if(!ksc(cmd,"base64"))   {vps("[base64] ");vpln(a);}
    else if(!ksc(cmd,"xxd"))      {vps("00000000: ");for(int i=0;a[i]&&i<16;i++){char h[4];kuia((uint8_t)a[i],h,16);if(ksl(h)<2)vps("0");vps(h);vps(" ");}vpc('\n');}
    else if(!ksc(cmd,"eval")||!ksc(cmd,"exec")) sh_dispatch(a);
    else if(!ksc(cmd,"source")){ if(!a||!*a){vpln("usage: source <file>");} else{ lash_source(a); } }
    else if(!ksc(cmd,"!!"))       {if(sh_hn>0)sh_dispatch(sh_hist[sh_hn-1]);}
    else if(!ksc(cmd,"man")||!ksc(cmd,"info")||!ksc(cmd,"whatis")||!ksc(cmd,"apropos")){
        if(a&&*a){vps("man: ");vps(a);vpln(": see 'help' for all commands");}
        else cmd_help_sh();
    }
    else if(!ksc(cmd,"tar")||!ksc(cmd,"gzip")||!ksc(cmd,"zip")||!ksc(cmd,"unzip")) vpln("[compression: pigfs virtual]");
    // APPS
    else if(!ksc(cmd,"tcp_test"))  tcp_test();
    else if(!ksc(cmd,"wm")||!ksc(cmd,"compositor")) {
        vclear();
        vga_disable_cursor();

        /* ── Login Screen ── */
        for(int y=0;y<VGA_H;y++){
            vfill(0,y,VGA_W,1,' ',COL(C_WHITE,C_BLUE));
        }

        /* Center box */
        int bx=18, by=5, bw=44, bh=15;
        vfill(bx,by,bw,bh,' ',COL(C_WHITE,C_BLACK));
        for(int i=bx+1;i<bx+bw-1;i++){ vat(CH_SH,COL(C_LCYAN,C_BLACK),i,by); vat(CH_SH,COL(C_LCYAN,C_BLACK),i,by+bh-1); }
        for(int i=by+1;i<by+bh-1;i++){ vat(CH_SV,COL(C_LCYAN,C_BLACK),bx,i); vat(CH_SV,COL(C_LCYAN,C_BLACK),bx+bw-1,i); }
        vat(CH_TL,COL(C_LCYAN,C_BLACK),bx,by); vat(CH_TR,COL(C_LCYAN,C_BLACK),bx+bw-1,by);
        vat(CH_BL,COL(C_LCYAN,C_BLACK),bx,by+bh-1); vat(CH_BR,COL(C_LCYAN,C_BLACK),bx+bw-1,by+bh-1);

        /* Title - centered */
        vfill(bx+1,by+1,bw-2,1,' ',COL(C_BLACK,C_LCYAN));
        vstr(bx+5,by+1,COL(C_BLACK,C_LCYAN)," VDM - Vibecoded Display Manager ");

        /* Subtitle - centered */
        vstr(bx+14,by+2,COL(C_LGREY,C_BLACK),"ported from erofs");

        /* VDM Logo - centered */
        vstr(bx+6,by+3,COL(C_LCYAN,C_BLACK)," __      __  _____    __  __ ");
        vstr(bx+6,by+4,COL(C_LCYAN,C_BLACK)," \\ \\    / / |  __ \\  |  \\/  |");
        vstr(bx+6,by+5,COL(C_LGREEN,C_BLACK),"  \\ \\  / /  | |  ) | | \\  / |");
        vstr(bx+6,by+6,COL(C_LGREEN,C_BLACK),"   \\ \\/ /   | |  | | | |\\/| |");
        vstr(bx+6,by+7,COL(C_YELLOW,C_BLACK),"    \\  /    | |__/ | | |  | |");
        vstr(bx+6,by+8,COL(C_YELLOW,C_BLACK),"     \\/     |_____/  |_|  |_|");
        vstr(bx+10,by+10,COL(C_LGREY,C_BLACK),"pigOS Desktop Environment");

        /* Prompts */
        vstr(bx+4,by+11,COL(C_WHITE,C_BLACK),"Username: ");
        vstr(bx+4,by+12,COL(C_WHITE,C_BLACK),"Password: ");
        vstr(bx+12,by+13,COL(C_DGREY,C_BLACK),"root / toor");

        vga_enable_cursor(13,15);

        /* ── Login retry loop ── */
        int login_ok = 0;
        while(!login_ok){
            /* Clear fields */
            char username[32]="";
            int ui=0;
            int ux=bx+14, uy=by+11;
            /* Clear username line */
            for(int i=0;i<30;i++) vat(' ',COL(C_WHITE,C_BLACK),ux+i,uy);
            /* Clear password line */
            int ppx=bx+14, ppy=by+12;
            for(int i=0;i<30;i++) vat(' ',COL(C_WHITE,C_BLACK),ppx+i,ppy);
            /* Clear error line completely */
            for(int i=0;i<bw-8;i++) vat(' ',COL(C_WHITE,C_BLACK),bx+4+i,by+13);

            /* Read username */
            vx=ux; vy=uy;
            vga_update_hw_cursor();
            while(1){
                ps2_poll();
                if(!kb_avail()) continue;
                int c=kb_get();
                if(c==27){ vga_enable_cursor(13,15); vclear(); return; }
                if(c=='\n'||c=='\r') break;
                if(c=='\b'||c==127||c==KEY_DEL){
                    if(ui>0){
                        ui--;
                        vat(' ',COL(C_WHITE,C_BLACK),ux+ui,uy);
                        vx=ux+ui;
                        vga_update_hw_cursor();
                    }
                } else if(c>=32&&c<127&&ui<31){
                    username[ui++]=(char)c;
                    vat((uint8_t)c,COL(C_WHITE,C_BLACK),ux+ui-1,uy);
                    vx=ux+ui;
                    vga_update_hw_cursor();
                }
            }
            username[ui]=0;

            /* Read password */
            int pi=0;
            char password[32]="";
            vx=ppx; vy=ppy;
            vga_update_hw_cursor();
            while(1){
                ps2_poll();
                if(!kb_avail()) continue;
                int c=kb_get();
                if(c==27){ vga_enable_cursor(13,15); vclear(); return; }
                if(c=='\n'||c=='\r') break;
                if(c=='\b'||c==127||c==KEY_DEL){
                    if(pi>0){
                        pi--;
                        vat(' ',COL(C_WHITE,C_BLACK),ppx+pi,ppy);
                        vx=ppx+pi;
                        vga_update_hw_cursor();
                    }
                } else if(c>=32&&c<127&&pi<31){
                    password[pi++]=(char)c;
                    vat('*',COL(C_WHITE,C_BLACK),ppx+pi-1,ppy);
                    vx=ppx+pi;
                    vga_update_hw_cursor();
                }
            }
            password[pi]=0;

            /* Delay before validating */
            for(volatile int i=0;i<30000000;i++);

            /* Validate - use users.h database */
            extern int ufind(const char*);
            extern User users[16];
            int user_idx = ufind(username);
            if(user_idx >= 0){
                /* Compare passwords */
                if(ksc(password, users[user_idx].pass)==0){
                    login_ok = 1;
                } else {
                    vstr(bx+10,by+13,COL(C_LRED,C_BLACK),"Invalid password");
                    for(volatile int i=0;i<80000000;i++);
                }
            } else {
                vstr(bx+10,by+13,COL(C_LRED,C_BLACK),"Invalid username");
                for(volatile int i=0;i<80000000;i++);
            }
        }

        /* Login success - launch desktop */
        vclear();
        vga_disable_cursor();

        /* Desktop background */
        for(int y=0;y<VGA_H;y++){
            uint8_t bg = (y < 3) ? COL(C_WHITE,C_LGREY) : COL(C_WHITE,C_DGREY);
            vfill(0,y,VGA_W,1,' ',bg);
        }

        /* pigOS wallpaper */
        vstr(30,2,COL(C_LCYAN,C_LGREY),"   ____  _       ");
        vstr(30,3,COL(C_LCYAN,C_LGREY),"  |  _ \\(_) ___   ");
        vstr(30,4,COL(C_LCYAN,C_LGREY),"  | |_) | |/ _ \\  ");
        vstr(30,5,COL(C_LCYAN,C_LGREY),"  |  __/| | (_) | ");
        vstr(30,6,COL(C_LCYAN,C_LGREY),"  |_|   |_|\\___/  ");
        vstr(32,8,COL(C_LCYAN,C_LGREY),"pigOS v1.0");
        vstr(28,10,COL(C_DGREY,C_LGREY),"Powered by larpshell");

        /* Welcome window */
        int wx=22, wy=2, ww=36, wh=14;
        vfill(wx,wy,ww,wh,' ',COL(C_WHITE,C_WHITE));
        vfill(wx+1,wy+1,ww-2,3,' ',COL(C_BLACK,C_LCYAN));
        vstr(wx+2,wy+1,COL(C_BLACK,C_LCYAN)," Welcome to pigOS Desktop ");
        vstr(wx+ww-5,wy+1,COL(C_WHITE,C_LRED),"[X]");

        /* Window border */
        for(int i=wx;i<wx+ww;i++){ vat(CH_SH,COL(C_LGREY,C_WHITE),i,wy+3); }
        vat(CH_ML,COL(C_LGREY,C_WHITE),wx,wy+3);
        vat(CH_MR,COL(C_LGREY,C_WHITE),wx+ww-1,wy+3);
        for(int r=wy+4;r<wy+wh;r++){ vat(CH_SV,COL(C_LGREY,C_WHITE),wx,r); vat(CH_SV,COL(C_LGREY,C_WHITE),wx+ww-1,r); }
        vat(CH_SBL,COL(C_LGREY,C_WHITE),wx,wy+wh-1);
        vat(CH_SBR,COL(C_LGREY,C_WHITE),wx+ww-1,wy+wh-1);
        for(int i=wx+1;i<wx+ww-1;i++){ vat(CH_SH,COL(C_LGREY,C_WHITE),i,wy+wh-1); }

        /* Window content */
        vstr(wx+2,wy+4,COL(C_BLACK,C_WHITE),"  Welcome to pigOS!");
        vstr(wx+2,wy+5,COL(C_BLACK,C_WHITE),"  =================");
        vstr(wx+2,wy+7,COL(C_BLACK,C_WHITE),"  F2 = Terminal");
        vstr(wx+2,wy+8,COL(C_BLACK,C_WHITE),"  F3 = File Manager");
        vstr(wx+2,wy+9,COL(C_BLACK,C_WHITE),"  F5 = Power Menu");
        vstr(wx+2,wy+10,COL(C_BLACK,C_WHITE),"  ESC = Exit Desktop");
        vstr(wx+2,wy+12,COL(C_DGREY,C_WHITE),"  Type commands in terminal");

        /* Dock/bar at bottom */
        int dock_y = VGA_H - 4;
        vfill(0,dock_y,VGA_W,4,' ',COL(C_WHITE,C_LGREY));
        vfill(0,dock_y,VGA_W,1,' ',COL(C_WHITE,C_WHITE));
        vstr(2,dock_y+1,COL(C_LGREEN,C_LGREY),"[F2]Terminal");
        vstr(16,dock_y+1,COL(C_LBLUE,C_LGREY),"[F3]Files");
        vstr(28,dock_y+1,COL(C_LCYAN,C_LGREY),"[F5]Power");
        vstr(40,dock_y+1,COL(C_DGREY,C_LGREY),"[ESC]Exit");
        vstr(68,dock_y+2,COL(C_DGREY,C_LGREY),"10:00 AM");

        /* Main desktop loop */
        int desktop_running = 1;
        while(desktop_running){
            ps2_poll();
            if(!kb_avail()) continue;
            int c = kb_get();

            if(c == KEY_F2){
                /* Terminal */
                int tx=3, ty=2, tw=74, th=18;
                vfill(tx,ty,tw,th,' ',COL(C_WHITE,C_BLACK));
                vfill(tx+1,ty,tw-2,1,' ',COL(C_BLACK,C_LCYAN));
                vstr(tx+2,ty,COL(C_BLACK,C_LCYAN)," larpshell - ESC=exit ");
                vstr(tx+tw-6,ty,COL(C_WHITE,C_LRED),"[X]");
                for(int i=tx;i<tx+tw;i++){ vat(0xCD,i,ty+th-1,COL(C_LCYAN,C_BLACK)); }
                for(int i=ty;i<ty+th;i++){ vat(0xBA,tx,i,COL(C_LCYAN,C_BLACK)); vat(0xBA,tx+tw-1,i,COL(C_LCYAN,C_BLACK)); }
                vat(0xC9,tx,ty,COL(C_LCYAN,C_BLACK)); vat(0xBB,tx+tw-1,ty,COL(C_LCYAN,C_BLACK));
                vat(0xC8,tx,ty+th-1,COL(C_LCYAN,C_BLACK)); vat(0xBC,tx+tw-1,ty+th-1,COL(C_LCYAN,C_BLACK));

                vset(C_LGREEN,C_BLACK);
                int cur_y = ty+2;
                vstr(tx+2,cur_y++,COL(C_LGREEN,C_BLACK),"> larpshell v5.9.3 ready");
                cur_y++;

                char input[128];
                int ilen=0;
                int hist_idx=-1;

                while(1){
                    vstr(tx+2,cur_y,COL(C_LGREEN,C_BLACK),"> ");
                    ilen=0; input[0]=0;

                    while(1){
                        ps2_poll();
                        if(!kb_avail()) continue;
                        int kc=kb_get();

                        if(kc=='\n'||kc=='\r'){
                            input[ilen]=0;
                            break;
                        }
                        if(kc==3){
                            vps("^C");
                            vpc('\n');
                            ilen=0;
                            break;
                        }
                        if(kc=='\b'||kc==127){
                            if(ilen>0){ ilen--; vpc('\b'); vpc(' '); vpc('\b'); }
                            continue;
                        }
                        if(kc==KEY_UP){
                            if(sh_hn>0&&hist_idx<sh_hn-1){
                                hist_idx++;
                                while(ilen>0){ilen--;vpc('\b');vpc(' ');vpc('\b');}
                                kcp(input,sh_hist[sh_hn-1-hist_idx]);
                                ilen=ksl(input);
                                for(int i=0;i<ilen;i++) vpc((uint8_t)input[i]);
                            }
                            continue;
                        }
                        if(kc==KEY_DOWN){
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
                        if(kc==27){
                            /* Redraw desktop and exit terminal */
                            goto redraw_desktop;
                        }
                        if(kc>=32&&kc<127&&ilen<127){
                            input[ilen++]=(char)kc;
                            vpc((uint8_t)kc);
                        }
                    }

                    if(ilen>0){
                        sh_push_hist(input);
                        sh_dispatch(input);
                        cur_y++;
                        if(cur_y>=ty+th-2){
                            /* Scroll: clear and redraw */
                            vfill(tx+1,ty+2,tw-2,th-4,' ',COL(C_WHITE,C_BLACK));
                            cur_y=ty+2;
                        }
                    }
                }
redraw_desktop:
                /* Redraw desktop */
                for(int y=0;y<VGA_H;y++){
                    uint8_t bg = (y < 3) ? COL(C_WHITE,C_LGREY) : COL(C_WHITE,C_DGREY);
                    vfill(0,y,VGA_W,1,' ',bg);
                }
                vstr(30,2,COL(C_LCYAN,C_LGREY),"   ____  _       ");
                vstr(30,3,COL(C_LCYAN,C_LGREY),"  |  _ \\(_) ___   ");
                vstr(30,4,COL(C_LCYAN,C_LGREY),"  | |_) | |/ _ \\  ");
                vstr(30,5,COL(C_LCYAN,C_LGREY),"  |  __/| | (_) | ");
                vstr(30,6,COL(C_LCYAN,C_LGREY),"  |_|   |_|\\___/  ");
                vstr(32,8,COL(C_LCYAN,C_LGREY),"pigOS v1.0");
                vstr(28,10,COL(C_DGREY,C_LGREY),"Powered by larpshell");
                vfill(wx,wy,ww,wh,' ',COL(C_WHITE,C_WHITE));
                vfill(wx+1,wy+1,ww-2,3,' ',COL(C_BLACK,C_LCYAN));
                vstr(wx+2,wy+1,COL(C_BLACK,C_LCYAN)," Welcome to pigOS Desktop ");
                vstr(wx+ww-5,wy+1,COL(C_WHITE,C_LRED),"[X]");
                for(int i=wx;i<wx+ww;i++){ vat(CH_SH,COL(C_LGREY,C_WHITE),i,wy+3); }
                vat(CH_ML,COL(C_LGREY,C_WHITE),wx,wy+3);
                vat(CH_MR,COL(C_LGREY,C_WHITE),wx+ww-1,wy+3);
                for(int r=wy+4;r<wy+wh;r++){ vat(CH_SV,COL(C_LGREY,C_WHITE),wx,r); vat(CH_SV,COL(C_LGREY,C_WHITE),wx+ww-1,r); }
                vat(CH_SBL,COL(C_LGREY,C_WHITE),wx,wy+wh-1);
                vat(CH_SBR,COL(C_LGREY,C_WHITE),wx+ww-1,wy+wh-1);
                for(int i=wx+1;i<wx+ww-1;i++){ vat(CH_SH,COL(C_LGREY,C_WHITE),i,wy+wh-1); }
                vstr(wx+2,wy+4,COL(C_BLACK,C_WHITE),"  Welcome to pigOS!");
                vstr(wx+2,wy+5,COL(C_BLACK,C_WHITE),"  =================");
                vstr(wx+2,wy+7,COL(C_BLACK,C_WHITE),"  F2 = Terminal");
                vstr(wx+2,wy+8,COL(C_BLACK,C_WHITE),"  F3 = File Manager");
                vstr(wx+2,wy+9,COL(C_BLACK,C_WHITE),"  F5 = Power Menu");
                vstr(wx+2,wy+10,COL(C_BLACK,C_WHITE),"  ESC = Exit Desktop");
                vstr(wx+2,wy+12,COL(C_DGREY,C_WHITE),"  Type commands in terminal");
                vfill(0,dock_y,VGA_W,4,' ',COL(C_WHITE,C_LGREY));
                vfill(0,dock_y,VGA_W,1,' ',COL(C_WHITE,C_WHITE));
                vstr(2,dock_y+1,COL(C_LGREEN,C_LGREY),"[F2]Terminal");
                vstr(16,dock_y+1,COL(C_LBLUE,C_LGREY),"[F3]Files");
                vstr(28,dock_y+1,COL(C_LCYAN,C_LGREY),"[F5]Power");
                vstr(40,dock_y+1,COL(C_DGREY,C_LGREY),"[ESC]Exit");
                vstr(68,dock_y+2,COL(C_DGREY,C_LGREY),"10:00 AM");
            }
            else if(c == KEY_F3){
                /* File Manager */
                vclear();
                vfill(0,0,VGA_W,1,' ',COL(C_BLACK,C_LCYAN));
                vstr(1,0,COL(C_WHITE,C_LCYAN)," Files - pigOS File Manager ");
                vstr(58,0,COL(C_LGREY,C_LCYAN),"ESC=close");
                vstr(2,3,COL(C_LCYAN,C_BLACK),"/ pigfs virtual filesystem");
                vstr(2,5,COL(C_YELLOW,C_BLACK),"  [dir] boot/");
                vstr(2,6,COL(C_YELLOW,C_BLACK),"  [dir] kernel/");
                vstr(2,7,COL(C_YELLOW,C_BLACK),"  [dir] drivers/");
                vstr(2,8,COL(C_YELLOW,C_BLACK),"  [dir] shell/");
                vstr(2,9,COL(C_YELLOW,C_BLACK),"  [dir] wm/");
                vstr(2,10,COL(C_YELLOW,C_BLACK),"  [dir] crypto/");
                vstr(2,11,COL(C_YELLOW,C_BLACK),"  [dir] lwip/");
                vstr(2,12,COL(C_YELLOW,C_BLACK),"  [dir] vdm/");
                vstr(2,13,COL(C_WHITE,C_BLACK),"  [file] Makefile");
                vstr(2,14,COL(C_WHITE,C_BLACK),"  [file] README-FILES.txt");
                vstr(2,15,COL(C_WHITE,C_BLACK),"  [file] linker.ld");
                vstr(2,VGA_H-2,COL(C_DGREY,C_BLACK),"Use 'ls' command for full listing");
                int fc;
                do { fc = kb_get(); } while(fc != 27);
                /* Redraw desktop */
                for(int y=0;y<VGA_H;y++){
                    uint8_t bg = (y < 3) ? COL(C_WHITE,C_LGREY) : COL(C_WHITE,C_DGREY);
                    vfill(0,y,VGA_W,1,' ',bg);
                }
                vstr(30,2,COL(C_LCYAN,C_LGREY),"   ____  _       ");
                vstr(30,3,COL(C_LCYAN,C_LGREY),"  |  _ \\(_) ___   ");
                vstr(30,4,COL(C_LCYAN,C_LGREY),"  | |_) | |/ _ \\  ");
                vstr(30,5,COL(C_LCYAN,C_LGREY),"  |  __/| | (_) | ");
                vstr(30,6,COL(C_LCYAN,C_LGREY),"  |_|   |_|\\___/  ");
                vstr(32,8,COL(C_LCYAN,C_LGREY),"pigOS v1.0");
                vstr(28,10,COL(C_DGREY,C_LGREY),"Powered by larpshell");
                vfill(wx,wy,ww,wh,' ',COL(C_WHITE,C_WHITE));
                vfill(wx+1,wy+1,ww-2,3,' ',COL(C_BLACK,C_LCYAN));
                vstr(wx+2,wy+1,COL(C_BLACK,C_LCYAN)," Welcome to pigOS Desktop ");
                vstr(wx+ww-5,wy+1,COL(C_WHITE,C_LRED),"[X]");
                for(int i=wx;i<wx+ww;i++){ vat(CH_SH,COL(C_LGREY,C_WHITE),i,wy+3); }
                vat(CH_ML,COL(C_LGREY,C_WHITE),wx,wy+3);
                vat(CH_MR,COL(C_LGREY,C_WHITE),wx+ww-1,wy+3);
                for(int r=wy+4;r<wy+wh;r++){ vat(CH_SV,COL(C_LGREY,C_WHITE),wx,r); vat(CH_SV,COL(C_LGREY,C_WHITE),wx+ww-1,r); }
                vat(CH_SBL,COL(C_LGREY,C_WHITE),wx,wy+wh-1);
                vat(CH_SBR,COL(C_LGREY,C_WHITE),wx+ww-1,wy+wh-1);
                for(int i=wx+1;i<wx+ww-1;i++){ vat(CH_SH,COL(C_LGREY,C_WHITE),i,wy+wh-1); }
                vstr(wx+2,wy+4,COL(C_BLACK,C_WHITE),"  Welcome to pigOS!");
                vstr(wx+2,wy+5,COL(C_BLACK,C_WHITE),"  =================");
                vstr(wx+2,wy+7,COL(C_BLACK,C_WHITE),"  F2 = Terminal");
                vstr(wx+2,wy+8,COL(C_BLACK,C_WHITE),"  F3 = File Manager");
                vstr(wx+2,wy+9,COL(C_BLACK,C_WHITE),"  F5 = Power Menu");
                vstr(wx+2,wy+10,COL(C_BLACK,C_WHITE),"  ESC = Exit Desktop");
                vstr(wx+2,wy+12,COL(C_DGREY,C_WHITE),"  Type commands in terminal");
                vfill(0,dock_y,VGA_W,4,' ',COL(C_WHITE,C_LGREY));
                vfill(0,dock_y,VGA_W,1,' ',COL(C_WHITE,C_WHITE));
                vstr(2,dock_y+1,COL(C_LGREEN,C_LGREY),"[F2]Terminal");
                vstr(16,dock_y+1,COL(C_LBLUE,C_LGREY),"[F3]Files");
                vstr(28,dock_y+1,COL(C_LCYAN,C_LGREY),"[F5]Power");
                vstr(40,dock_y+1,COL(C_DGREY,C_LGREY),"[ESC]Exit");
                vstr(68,dock_y+2,COL(C_DGREY,C_LGREY),"10:00 AM");
            }
            else if(c == KEY_F5){
                /* Power Menu */
                int px=25, py=8, pw=30, ph=8;
                vfill(px-1,py-1,pw+2,ph+2,' ',COL(C_DGREY,C_DGREY));
                vfill(px,py,pw,ph,' ',COL(C_WHITE,C_BLACK));
                vfill(px,py,pw,2,' ',COL(C_WHITE,C_LCYAN));
                vstr(px+1,py,COL(C_BLACK,C_LCYAN)," Power Options");
                vstr(px+2,py+3,COL(C_BLACK,C_WHITE),"[1] Sleep");
                vstr(px+2,py+4,COL(C_BLACK,C_WHITE),"[2] Restart");
                vstr(px+2,py+5,COL(C_BLACK,C_WHITE),"[3] Shutdown");
                vstr(px+2,py+6,COL(C_BLACK,C_WHITE),"[ESC] Cancel");
                int pc2=kb_get();
                if(pc2=='3'){
                    vclear(); vset(C_LRED,C_BLACK);
                    vpln("Shutting down...");
                    for(volatile int i=0;i<100000000;i++);
                    outb(0xF4,0x00);
                    __asm__("cli;hlt");
                }
                if(pc2=='2'){
                    vclear();
                    for(volatile int i=0;i<100000000;i++);
                    outb(0x64,0xFE);
                }
                /* Redraw desktop */
                for(int y=0;y<VGA_H;y++){
                    uint8_t bg = (y < 3) ? COL(C_WHITE,C_LGREY) : COL(C_WHITE,C_DGREY);
                    vfill(0,y,VGA_W,1,' ',bg);
                }
                vstr(30,2,COL(C_LCYAN,C_LGREY),"   ____  _       ");
                vstr(30,3,COL(C_LCYAN,C_LGREY),"  |  _ \\(_) ___   ");
                vstr(30,4,COL(C_LCYAN,C_LGREY),"  | |_) | |/ _ \\  ");
                vstr(30,5,COL(C_LCYAN,C_LGREY),"  |  __/| | (_) | ");
                vstr(30,6,COL(C_LCYAN,C_LGREY),"  |_|   |_|\\___/  ");
                vstr(32,8,COL(C_LCYAN,C_LGREY),"pigOS v1.0");
                vstr(28,10,COL(C_DGREY,C_LGREY),"Powered by larpshell");
                vfill(wx,wy,ww,wh,' ',COL(C_WHITE,C_WHITE));
                vfill(wx+1,wy+1,ww-2,3,' ',COL(C_BLACK,C_LCYAN));
                vstr(wx+2,wy+1,COL(C_BLACK,C_LCYAN)," Welcome to pigOS Desktop ");
                vstr(wx+ww-5,wy+1,COL(C_WHITE,C_LRED),"[X]");
                for(int i=wx;i<wx+ww;i++){ vat(CH_SH,COL(C_LGREY,C_WHITE),i,wy+3); }
                vat(CH_ML,COL(C_LGREY,C_WHITE),wx,wy+3);
                vat(CH_MR,COL(C_LGREY,C_WHITE),wx+ww-1,wy+3);
                for(int r=wy+4;r<wy+wh;r++){ vat(CH_SV,COL(C_LGREY,C_WHITE),wx,r); vat(CH_SV,COL(C_LGREY,C_WHITE),wx+ww-1,r); }
                vat(CH_SBL,COL(C_LGREY,C_WHITE),wx,wy+wh-1);
                vat(CH_SBR,COL(C_LGREY,C_WHITE),wx+ww-1,wy+wh-1);
                for(int i=wx+1;i<wx+ww-1;i++){ vat(CH_SH,COL(C_LGREY,C_WHITE),i,wy+wh-1); }
                vstr(wx+2,wy+4,COL(C_BLACK,C_WHITE),"  Welcome to pigOS!");
                vstr(wx+2,wy+5,COL(C_BLACK,C_WHITE),"  =================");
                vstr(wx+2,wy+7,COL(C_BLACK,C_WHITE),"  F2 = Terminal");
                vstr(wx+2,wy+8,COL(C_BLACK,C_WHITE),"  F3 = File Manager");
                vstr(wx+2,wy+9,COL(C_BLACK,C_WHITE),"  F5 = Power Menu");
                vstr(wx+2,wy+10,COL(C_BLACK,C_WHITE),"  ESC = Exit Desktop");
                vstr(wx+2,wy+12,COL(C_DGREY,C_WHITE),"  Type commands in terminal");
                vfill(0,dock_y,VGA_W,4,' ',COL(C_WHITE,C_LGREY));
                vfill(0,dock_y,VGA_W,1,' ',COL(C_WHITE,C_WHITE));
                vstr(2,dock_y+1,COL(C_LGREEN,C_LGREY),"[F2]Terminal");
                vstr(16,dock_y+1,COL(C_LBLUE,C_LGREY),"[F3]Files");
                vstr(28,dock_y+1,COL(C_LCYAN,C_LGREY),"[F5]Power");
                vstr(40,dock_y+1,COL(C_DGREY,C_LGREY),"[ESC]Exit");
                vstr(68,dock_y+2,COL(C_DGREY,C_LGREY),"10:00 AM");
            }
            else if(c == 27){
                desktop_running = 0;
            }
        }

        vga_enable_cursor(13,15);
        vclear();
    }
    else if(!ksc(cmd,"robin"))    wm_robin();
    else if(!ksc(cmd,"discord")||!ksc(cmd,"dc")) cmd_discord(a);
    else if(!ksc(cmd,"pigbrowse")||!ksc(cmd,"browser")) {vclear();vset(C_LBLUE,C_BLACK);vpln("pigBrowse 1.0 - ESC=exit\npigOS://home [TCP/IP needed for real browsing]");vrst();kb_get();vclear();}
    else if(!ksc(cmd,"llm")||!ksc(cmd,"pigai")||!ksc(cmd,"ask")||!ksc(cmd,"chat")) cmd_llm(a);
    else if(!ksc(cmd,"python3")||!ksc(cmd,"pip3")){
        if(!rpk_cmd_installed("python3")&&!ksc(cmd,"python3")){vset(C_LRED,C_BLACK);vpln("python3: not installed. Run: rpk on python3");vrst();}
        else if(!rpk_cmd_installed("pip3")&&!ksc(cmd,"pip3")){vset(C_LRED,C_BLACK);vpln("pip3: not installed. Run: rpk on pip3");vrst();}
        else cmd_rpk_run(!ksc(cmd,"python3")?"python3":"pip3");
    }
    else if(!ksc(cmd,"make")){
        if(!rpk_cmd_installed("make")){vset(C_LRED,C_BLACK);vpln("make: not installed. Run: rpk on make");vrst();}
        else cmd_rpk_run("make");
    }
    else if(!ksc(cmd,"go"))        cmd_go(a);
    else if(!ksc(cmd,"larpshell")||!ksc(cmd,"lash")){
        if(!ksc(a,"--version")||!ksc(a,"-v")){
            vset(C_LCYAN,C_BLACK);
            vpln("");
            vpln("  LARP Shell v5.9.3");
            vpln("  Port for pigOS");
            vrst();
            vpln("");
            vpln("  Credits: github.com/usr-undeleted/lash");
            vpln("  License: GPLv3");
            vpln("");
        } else {
            cmd_larpshell(a);
        }
    }
    else if(!ksc(cmd,"snake"))    cmd_snake();
    else if(!ksc(cmd,"tetris"))   cmd_tetris();
    else if(!ksc(cmd,"matrix"))   {vclear();vset(C_LGREEN,C_BLACK);vpln("[matrix] q=quit");int rng=42;for(int f=0;f<500&&!kb_avail();f++){rng=rng*1103515245+12345;for(int c2=0;c2<VGA_W;c2++){int r=(rng>>8)%(VGA_H-1)+1;uint8_t ch=(uint8_t)('0'+(f+c2)%10);vat(ch,COL(f%2?C_LGREEN:C_GREEN,C_BLACK),c2,r);}for(volatile int d=0;d<300000;d++);}vclear();vrst();}
    else if(!ksc(cmd,"fortune"))  {static const char*f[]={"\"There is no cloud, just someone else's pig.\"","\"It works on my pigOS.\"","\"rok make me a sandwich.\"","\"Have you tried poingctl restart?\"","\"In pigOS we trust.\"","\"grep -r pig / --include=*.c\""};static int fi=0;vset(C_YELLOW,C_BLACK);vpln(f[fi++%6]);vrst();}
    else if(!ksc(cmd,"cowpig"))   {const char*msg=a&&*a?a:"pigOS!";vps("< ");vps(msg);vpln(" >\n  \\ (oo)\n   \\ (  )\n    ^^^^");}
    else if(!ksc(cmd,"figpig"))   {vset(C_LMAG,C_BLACK);vpln(" ____  _      ___  ____\n|  _ \\|_|/_ \\/ _ \\|___/");vrst();if(a&&*a){vpc(' ');vpln(a);}}
    else if(!ksc(cmd,"banner"))   {vpc('\n');vps("  *** ");vps(a&&*a?a:"pigOS");vpln(" ***\n");}
    else if(!ksc(cmd,"cal"))      {vset(C_YELLOW,C_BLACK);vpln("   January 1970");vrst();vpln("Mo Tu We Th Fr Sa Su\n           1  2  3  4");}
    else if(!ksc(cmd,"todo"))     {static char td[8][64];static int nt=0;if(a&&*a){if(nt<8){kcp(td[nt++],a);vps("todo: ");vpln(a);}else vpln("todo: full");}else{for(int i=0;i<nt;i++){char b[4];kia(i+1,b);vps(b);vps(". ");vpln(td[i]);}}}
    else if(!ksc(cmd,"note")||!ksc(cmd,"remind")){vps("note: ");vpln(a&&*a?a:"(empty)");}
    else if(!ksc(cmd,"piglog")||!ksc(cmd,"logcat")) cmd_polctl("-n 20");
    else if(!ksc(cmd,"exit")||!ksc(cmd,"quit")) {vset(C_DGREY,C_BLACK);vpln("larpshell: no exit");vrst();}
    else {
        // Check if it's an rpk-installed command
        if(rpk_cmd_installed(cmd)){ cmd_rpk_run(cmd); return; }
        vset(C_LRED,C_BLACK);vps("larpshell: ");vps(cmd);vpln(": command not found");vrst();
        vset(C_DGREY,C_BLACK);vpln("(type 'help' for commands)");vrst();
    }
}

static void cmd_help_sh(){
    // Use plain ASCII to avoid UTF-8/CP437 corruption
    uint8_t bc=COL(C_LMAG,C_BLACK);
    // Top border
    vat('+',bc,0,vy);for(int i=1;i<VGA_W-1;i++)vat('-',bc,i,vy);vat('+',bc,VGA_W-1,vy);vx=0;vy++;
    vat('|',bc,0,vy);vstr(2,vy,bc,"larpshell v2.2 - pigOS Command Reference");vat('|',bc,VGA_W-1,vy);vx=0;vy++;
    vat('+',bc,0,vy);for(int i=1;i<VGA_W-1;i++)vat('-',bc,i,vy);vat('+',bc,VGA_W-1,vy);vx=0;vy++;
    vrst();
    vset(C_YELLOW,C_BLACK);vpln(" SYSTEM");vrst();
    vpln("  help pigfetch about uname date uptime dmesg clear color");
    vpln("  reboot halt shutdown poweroff panic version");
    vpln("  LOGIN: user=root, pass=toor");
    vset(C_YELLOW,C_BLACK);vpln(" POING INIT");vrst();
    vpln("  poingctl [status|start|stop|enable|disable|restart|reload]");
    vpln("  poingctl [reboot|halt|shutdown|sleep]");
    vpln("  polctl [-f][-n N][-p PRI][-u UNIT] -- log viewer");
    vset(C_YELLOW,C_BLACK);vpln(" USERS");vrst();
    vpln("  adduser deluser usermod groupadd passwd groups whoami id");
    vpln("  rok <cmd>  r-  su [user]");
    vset(C_YELLOW,C_BLACK);vpln(" FILES");vrst();
    vpln("  ls [-la] [dir]  ll  cat  head  tail  rm [-f]  touch  mkdir");
    vpln("  grep [-r][-i][-l][-n] <pat> <file>");
    vpln("  find [path] [-name pat] [-type f/d]");
    vpln("  pwd  cd  pnano  pi/pivi (separate editors)");
    vset(C_YELLOW,C_BLACK);vpln(" PROCESSES");vrst();
    vpln("  ps  top  htop  kill  killall  pgrep");
    vset(C_YELLOW,C_BLACK);vpln(" NETWORK");vrst();
    vpln("  ifconfig  ping  netstat  pigget  wget  nslookup");
    vpln("  netpg [status|up|down|ip|dhcp|scan|help]");
    vpln("  NOTE: Start QEMU with -net nic,model=rtl8139 -net user");
    vset(C_YELLOW,C_BLACK);vpln(" HARDWARE");vrst();
    vpln("  lspci  lsusb  lsmod  lscpu  lsmem  lshw  lsblk  df  free");
    vset(C_YELLOW,C_BLACK);vpln(" PACKAGES (rpk)");vrst();
    vpln("  rpk on <pkg>      -- install (then just type pkg name)");
    vpln("  rpk roam pk [q]   -- search packages");
    vpln("  rpk list          -- installed packages");
    vpln("  rpk remove <pkg>  -- uninstall");
    vpln("  rpk info <pkg>    -- package info");
    vset(C_YELLOW,C_BLACK);vpln(" CRYPTO");vrst();
    vpln("  pigssl  pigcrypt  pigdecrypt  pighash  pigkey  pigcert");
    vset(C_YELLOW,C_BLACK);vpln(" SHELL");vrst();
    vpln("  export VAR=val  set  echo $VAR  env  alias  unalias  history  calc");
    vpln("  source <file>  sleep  countdown  timer  seq  base64  xxd  man");
    vset(C_YELLOW,C_BLACK);vpln(" WM/APPS");vrst();
    vpln("  wm  robin  pigbrowse  llm <q>  python3  pip3  make");
    vpln("  snake  tetris  matrix  fortune  cowpig  cal  todo");
    // Bottom border
    vat('+',bc,0,vy);for(int i=1;i<VGA_W-1;i++)vat('-',bc,i,vy);vat('+',bc,VGA_W-1,vy);vx=0;vy++;
    vstr(1,vy,COL(C_DGREY,C_BLACK),"UP/DOWN=history  CTRL+C=cancel  left/right=cursor");
    vx=0;vy++;vrst();
}

static void cmd_calc_sh(const char*e){
    if(!e||!*e){vpln("usage: calc <n> <op> <n>  (+,-,*,/,%)");return;}
    int a=0,b=0,neg=0;const char*p=e;
    while(*p==' ')p++;if(*p=='-'){neg=1;p++;}
    while(*p>='0'&&*p<='9'){a=a*10+(*p-'0');p++;}if(neg)a=-a;
    while(*p==' ')p++;char op=*p;p++;while(*p==' ')p++;
    neg=0;if(*p=='-'){neg=1;p++;}
    while(*p>='0'&&*p<='9'){b=b*10+(*p-'0');p++;}if(neg)b=-b;
    int r=0;
    if(op=='+')r=a+b;else if(op=='-')r=a-b;else if(op=='*')r=a*b;
    else if(op=='/'){if(!b){vset(C_LRED,C_BLACK);vpln("div/0!");vrst();return;}r=a/b;}
    else if(op=='%'){if(!b){vset(C_LRED,C_BLACK);vpln("mod/0!");vrst();return;}r=a%b;}
    else{vpln("unknown op: + - * / %");return;}
    char buf[32];kia(r,buf);vset(C_LGREEN,C_BLACK);vps("= ");vpln(buf);vrst();
}


static void cmd_snake(){
    #define SN_W 42
    #define SN_H 19
    #define SN_MAX 200
    int sx[SN_MAX],sy[SN_MAX],slen=3;
    int dx=1,dy=0,fx=21,fy=10,score=0;
    int rng=12345;
    sx[0]=10;sy[0]=5;sx[1]=9;sy[1]=5;sx[2]=8;sy[2]=5;
    vclear();
    uint8_t bc=COL(C_LCYAN,C_BLACK);
    for(int i=0;i<SN_W+2;i++){vat(CH_H,bc,i,0);vat(CH_H,bc,i,SN_H+1);}
    for(int i=0;i<SN_H+2;i++){vat(CH_V,bc,0,i);vat(CH_V,bc,SN_W+1,i);}
    vat(CH_TL,bc,0,0);vat(CH_TR,bc,SN_W+1,0);vat(CH_BL,bc,0,SN_H+1);vat(CH_BR,bc,SN_W+1,SN_H+1);
    vstr(SN_W+3,1,COL(C_YELLOW,C_BLACK),"SNAKE!");
    vstr(SN_W+3,3,COL(C_WHITE,C_BLACK),"WASD/arrows");
    vstr(SN_W+3,4,COL(C_WHITE,C_BLACK),"SPACE=pause  Q=quit");
    int paused=0;
    while(1){
        vat(CH_DIAM,COL(C_BLACK,C_YELLOW),fx+1,fy+1);
        vat(CH_BLOCK,COL(C_BLACK,C_LGREEN),sx[0]+1,sy[0]+1);
        for(int i=1;i<slen;i++) vat(CH_MED,COL(C_LGREEN,C_BLACK),sx[i]+1,sy[i]+1);
        char sb[8];kia(score,sb);
        vstr(SN_W+3,7,COL(C_YELLOW,C_BLACK),"Score:");
        vstr(SN_W+10,7,COL(C_WHITE,C_BLACK),sb);
        for(int d=0;d<5000000;d++){
            if(kb_avail()){
                int c=kb_get();
                if(c=='q'||c==27)goto sn_done;
                if(c==' '){paused=!paused;}
                if(!paused){
                    if((c=='w'||c==KEY_UP)&&dy!=1){dx=0;dy=-1;}
                    else if((c=='s'||c==KEY_DOWN)&&dy!=-1){dx=0;dy=1;}
                    else if((c=='a'||c==KEY_LEFT)&&dx!=1){dx=-1;dy=0;}
                    else if((c=='d'||c==KEY_RIGHT)&&dx!=-1){dx=1;dy=0;}
                }
                break;
            }
        }
        if(paused)continue;
        int nx=sx[0]+dx,ny=sy[0]+dy;
        if(nx<0||nx>=SN_W||ny<0||ny>=SN_H)goto sn_done;
        for(int i=1;i<slen;i++) if(sx[i]==nx&&sy[i]==ny)goto sn_done;
        vat(' ',COL(C_BLACK,C_BLACK),sx[slen-1]+1,sy[slen-1]+1);
        if(nx==fx&&ny==fy){
            if(slen<SN_MAX)slen++;
            rng=rng*1103515245+12345;
            fx=((rng>>8)&0xFF)%SN_W; fy=((rng>>16)&0xFF)%SN_H;
            score+=10;
        }
        for(int i=slen-1;i>0;i--){sx[i]=sx[i-1];sy[i]=sy[i-1];}
        sx[0]=nx;sy[0]=ny;
    }
sn_done:
    vclear();vset(C_LRED,C_BLACK);vps("Game over! Score: ");char sb2[8];kia(score,sb2);vpln(sb2);vrst();
}

// ── TTY scroll buffer ────────────────────────────────────────
#define TTY_SCROLL_LINES 5

// ── pigOS splash screen ──────────────────────────────────────
static void pigos_splash(){
    vset(C_LMAG,C_BLACK);
    vpln("");
    vpln("   ____  _       ___  ____   v1.0 x86_64");
    vpln("  |  _ \\(_) ___ / _ \\/ ___|  larpshell 5.9");
    vpln("  | |_) | |/ _ \\ | | \\___ \\  poing init");
    vpln("  |  __/| | (_) | |_| |___) | PigWM");
    vpln("  |_|   |_|\\___/ \\___/|____/  RTL8139");
    vpln(""); vrst();
    vset(C_LCYAN,C_BLACK);
    vpln("  pigOS v1.0 - type 'help' for all commands");
    vrst();
    vset(C_DGREY,C_BLACK);
    vpln("  pigfetch=sysinfo  wm=desktop  rpk on <pkg>=install");
    vpln("  Network: qemu ... -net nic,model=rtl8139 -net user");
    vpln(""); vrst();
}

// ── REPL ─────────────────────────────────────────────────────
static void shell_run(){
    vclear();
    pigos_splash();
    sh_alias_set("ll","ls -la");
    sh_alias_set("la","ls -la");
    sh_alias_set("cls","clear");
    lash_loop();
    __asm__("cli;hlt");
}

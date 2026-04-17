#pragma once
// larpshell v5.9.3 - C port for pigOS
// Based on lash by github.com/usr-undeleted/lash
#include "../drivers/vga/vga.h"
#include "../drivers/ps2/ps2.h"
#include "../kernel/mem.h"
#include "../kernel/pigfs.h"
#include "rpk.h"

#ifndef SH_BUF
#define SH_BUF 512
#endif

extern void sh_dispatch(const char*);
extern char sh_user[32];
extern char sh_cwd[256];
extern const char* CMDS[];
extern void sh_alias_set(const char*,const char*);

#define LASH_VERSION "5.9.3"
#define LASH_HIST_MAX 64
#define TAB_COMPLETE_MAX 64

static char lash_hist[LASH_HIST_MAX][SH_BUF];
static int lash_hn=0, lash_hi=-1;

// Tab completion data
static char lash_comp_candidates[TAB_COMPLETE_MAX][64];
static int lash_comp_count=0;
static int lash_comp_pos=0;
static int lash_comp_shown = 0;
static char lash_comp_input[SH_BUF];
static int lash_comp_ilen = 0;

static const char* LASH_COMPLETIONS[] = {
    "help", "ls", "cd", "cat", "echo", "pwd", "mkdir", "rmdir", "rm", "cp", "mv",
    "chmod", "chown", "touch", "grep", "find", "tar", "gzip", "gunzip",
    "apt", "apt-get", "dpkg", "make", "gcc", "g++", "python", "python3",
    "git", "npm", "node", "curl", "wget", "ssh", "scp", "rsync",
    "systemctl", "service", "journalctl", "ps", "top", "htop",
    "kill", "killall", "pkill", "bg", "fg", "jobs",
    "alias", "unalias", "export", "unset", "env",
    "ifconfig", "ip", "ping", "route", "netstat", "traceroute",
    "pi", "pnano", "pivi", "emacs",
    "clear", "date", "whoami", "uname", "hostname", "uptime",
    "pigfetch", "wm", "snake", "tetris", "matrix", "fortune",
    "adduser", "passwd", "su", "sudo", "useradd", "userdel", "usermod",
    "pigcrypt", "pighash", "pigkey",
    "rpk", "menuconfig",
    "exit", "logout", "reboot", "shutdown", "halt", "poweroff",
    "mount", "umount", "df", "du", "free", "lsblk",
    NULL
};

// Forward declarations
static void lash_render_prompt();
static void lash_find_completions(const char* prefix);
static void lash_show_completions();

static int ksp(const char*s,char c){while(*s){if(*s==c)return 1;s++;}return 0;}

static void lash_find_completions(const char* prefix){
    lash_comp_count=0;
    lash_comp_pos=0;
    if(!prefix || !*prefix) return;
    
    size_t plen = ksl(prefix);
    
    // Check built-in commands from LASH_COMPLETIONS - show ALL
    for(int i=0; LASH_COMPLETIONS[i]; i++){
        int match = 1;
        for(size_t j=0; j<plen && LASH_COMPLETIONS[i][j]; j++){
            if(LASH_COMPLETIONS[i][j] != prefix[j]){
                match = 0; break;
            }
        }
        if(match){
            kcp(lash_comp_candidates[lash_comp_count++], LASH_COMPLETIONS[i]);
            if(lash_comp_count>=TAB_COMPLETE_MAX) break;
        }
    }
    
    // Show rpk-installed commands not in LASH_COMPLETIONS
    for(int i=0; i<rpk_ncmds && lash_comp_count<TAB_COMPLETE_MAX; i++){
        int in_lash = 0;
        for(int j=0; LASH_COMPLETIONS[j]; j++){
            if(!ksc(LASH_COMPLETIONS[j], rpk_installed_cmds[i])){ in_lash=1; break; }
        }
        if(in_lash) continue;
        
        int match = 1;
        for(size_t j=0; j<plen && rpk_installed_cmds[i][j]; j++){
            if(rpk_installed_cmds[i][j] != prefix[j]){ match=0; break; }
        }
        if(match) kcp(lash_comp_candidates[lash_comp_count++], rpk_installed_cmds[i]);
    }
    
    // File/directory completion - only when has space
    int has_space = 0;
    for(int i=0; prefix[i]; i++){
        if(prefix[i] == ' '){ has_space = 1; break; }
    }
    
    if(has_space){
        // Get the file filter (after last space)
        char filt[64]="";
        for(int i=0; prefix[i]; i++){
            if(prefix[i] == ' '){
                int pi=0;
                for(int k=i+1; k<64 && prefix[k]; k++){
                    filt[pi++] = prefix[k];
                }
                filt[pi]=0;
                break;
            }
        }
        
        // Search from root for files
        int flen = ksl(filt);
        
        for(int i=0; VFS[i].path && lash_comp_count<TAB_COMPLETE_MAX-1; i++){
            const char* p = VFS[i].path;
            if(p[0] != '/') continue; // only absolute paths
            
            // Get filename (after last /)
            const char* fn = p;
            for(int j=0; p[j]; j++){
                if(p[j] == '/') fn = p + j + 1;
            }
            
            int fnlen = ksl(fn);
            if(fnlen==0) continue;
            
            // Check if matches filter - case insensitive
            if(flen > 0){
                if(fnlen < flen) continue;
                int match = 1;
                for(int j=0; j<flen; j++){
                    char fc = fn[j];
                    char cc = filt[j];
                    if(fc>='A'&&fc<='Z') fc += 32;
                    if(cc>='A'&&cc<='Z') cc += 32;
                    if(fc != cc){ match = 0; break; }
                }
                if(!match) continue;
            }
            
            // Skip duplicates
            int dup = 0;
            for(int j=0; j<lash_comp_count; j++){
                if(!ksc(lash_comp_candidates[j], fn)){ dup = 1; break; }
            }
            if(dup) continue;
            
            // Add filename
            kcp(lash_comp_candidates[lash_comp_count++], fn);
        }
    }
}

static void lash_show_completions(const char* input, int ilen){
    if(lash_comp_count==0) return;
    // Store current input for enter key handling
    lash_comp_shown = 1;
    lash_comp_ilen = ilen;
    for(int i=0; i<ilen && i<SH_BUF-1; i++) lash_comp_input[i] = input[i];
    lash_comp_input[ilen] = 0;
    // Show list below
    vpc('\n');
    for(int i=0; i<lash_comp_count; i++){
        vps(lash_comp_candidates[i]);
        vpc(' ');
        if((i+1)%8==0) vpc('\n');
    }
    // Extra newline then colored prompt with user's text
    vpc('\n');
    vpc('\n');
    vrst();
    vps("[");
    vset(C_LGREEN,C_BLACK);vps(sh_user);vrst();
    vps("@");
    vset(C_LCYAN,C_BLACK);vps("pigOS");vrst();
    vps(":");
    vset(C_YELLOW,C_BLACK);vps(sh_cwd);vrst();
    vps("]$ ");
    // Show user's text
    for(int i=0;i<ilen;i++) vpc((uint8_t)input[i]);
}

static void lash_show_completions();
static void lash_render_prompt();

static void lash_print_logo(){
    vrst();
    vpln("");
    vset(C_LCYAN,C_BLACK);
    vpln("  +====================================+");
    vpln("  |    LARP SHELL v5.9.3 - pigOS    |");
    vpln("  +====================================+");
    vrst();
    vpln("");
    vpln("  Credits: github.com/usr-undeleted/lash");
    vpln("  Type 'help' for all commands.");
    vpln("");
}

static void lash_push_hist(const char*s){
    if(ksl(s)==0)return;
    if(lash_hn>0&&!ksc(lash_hist[lash_hn-1],s))return;
    if(lash_hn<LASH_HIST_MAX)kcp(lash_hist[lash_hn++],s);
    else{for(int i=0;i<LASH_HIST_MAX-1;i++)kcp(lash_hist[i],lash_hist[i+1]);kcp(lash_hist[LASH_HIST_MAX-1],s);}
    lash_hi=-1;
}

static void lash_render_prompt(){
    vrst();
    vpc('\n');
    vps("[");
    vset(C_LGREEN,C_BLACK);vps(sh_user);vrst();
    vps("@");
    vset(C_LCYAN,C_BLACK);vps("pigOS");vrst();
    vps(":");
    vset(C_YELLOW,C_BLACK);vps(sh_cwd);vrst();
    vps("]$ ");
}

static void lash_redraw(const char*buf,int len){
    for(int i=0;i<len;i++)vpc((uint8_t)buf[i]);
}

// Source a file - execute each line as a shell command
static void lash_source(const char*path){
    char rp[256];
    fs_resolve(sh_cwd,path,rp,256);
    const VNode*vn=fs_find_vnode(rp);
    MemFile*mf=memfs_find(rp);
    const char*content=NULL;
    if(mf&&mf->used) content=mf->content;
    else if(vn&&vn->content) content=vn->content;
    if(!content){ vps("source: "); vps(path); vpln(": file not found"); return; }
    int ci=0;
    char line[SH_BUF];
    while(content[ci]&&content[ci]!=0){
        int li=0;
        while(content[ci]&&content[ci]!='\n'&&li<SH_BUF-1){
            line[li++]=content[ci++];
        }
        line[li]=0;
        if(content[ci]=='\n') ci++;
        // Skip empty lines and comments
        const char*p=line;
        while(*p==' ') p++;
        if(!*p||*p=='#') continue;
        // If line is "alias name='value'", dispatch it directly to alias handler
        if(ksl(line)>=6&&line[0]=='a'&&line[1]=='l'&&line[2]=='i'&&line[3]=='a'&&line[4]=='s'&&line[5]==' '){
            // Parse alias name='value' and set directly (avoid double-quote issue)
            const char*eq=line+6;
            while(*eq==' ') eq++;
            char aname[32]=""; int ai=0;
            while(*eq&&*eq!='='&&ai<31) aname[ai++]=*eq++; eq++;
            aname[ai]=0;
            char aval[128]=""; int vi=0;
            if(*eq=='\''||*eq=='"') eq++; // skip opening quote
            while(*eq&&vi<127){
                if(*eq=='\''||*eq=='"'){ eq++; break; }
                aval[vi++]=*eq++;
            }
            aval[vi]=0;
            sh_alias_set(aname,aval);
            vps("alias ");vps(aname);vps("='");vps(aval);vpln("'");
        } else {
            lash_push_hist(line);
            sh_dispatch(line);
        }
    }
}

static void lash_loop(){
    lash_print_logo();
    char input[SH_BUF];
    for(int i=0;i<SH_BUF;i++) input[i]=0;
    for(int i=0;i<SH_BUF;i++) lash_comp_input[i]=0;
    int ilen=0;
    int cur=0;
    int lash_hi_cur=-1;
    lash_render_prompt();
    
    while(1){
        ps2_poll();
        
    #ifdef CONFIG_LWIP
        sys_check_timeouts();
        net_poll();
    #endif
        
        if(!kb_avail()){
            ps2_poll();
            continue;
        }
        
        int c=kb_get();
        
        // Arrow keys - pi style (move cursor, redraw line)
        if(c==KEY_LEFT){
            if(cur>0)cur--;
            // clear line and redraw
            for(int i=0;i<ilen;i++){vpc('\b');vpc(' ');vpc('\b');}
            for(int i=0;i<ilen;i++)vpc((uint8_t)input[i]);
            continue;
        }
        if(c==KEY_RIGHT){
            if(cur<ilen)cur++;
            for(int i=0;i<ilen;i++){vpc('\b');vpc(' ');vpc('\b');}
            for(int i=0;i<ilen;i++)vpc((uint8_t)input[i]);
            continue;
        }
        
        if(c=='\n'||c=='\r'){
            // If completion list was shown, use first result
            if(lash_comp_shown && lash_comp_count > 0){
                const char* cmd = lash_comp_candidates[0];
                int clen = ksl(cmd);
                // Find position after command and space
                int space_pos = -1;
                for(int i=0; lash_comp_input[i]; i++){
                    if(lash_comp_input[i] == ' '){ space_pos = i; break; }
                }
                if(space_pos >= 0){
                    int fn_start = space_pos + 1;
                    int existing_len = lash_comp_ilen - fn_start;
                    // Redraw line
                    for(int i=0; i<existing_len; i++){
                        vpc('\b'); vpc(' '); vpc('\b');
                    }
                    for(int i=0; i<clen && (fn_start + i) < SH_BUF-1; i++){
                        lash_comp_input[fn_start + i] = cmd[i];
                        vpc((uint8_t)cmd[i]);
                    }
                    lash_comp_input[fn_start + clen] = 0;
                    lash_comp_ilen = fn_start + clen;
                    // Copy back to input
                    for(int i=0; i<lash_comp_ilen; i++) input[i] = lash_comp_input[i];
                    ilen = lash_comp_ilen;
                    cur = ilen;
                    input[ilen] = 0;  // null-terminate!
                } else {
                    // No space, replace whole line
                    while(ilen > 0){ ilen--; vpc('\b'); vpc(' '); vpc('\b'); }
                    for(int i=0; i<clen && ilen<SH_BUF-1; i++){
                        input[ilen++] = cmd[i];
                        vpc((uint8_t)cmd[i]);
                    }
                    cur = ilen;
                    input[ilen] = 0;  // null-terminate!
                }
                lash_comp_shown = 0;
                lash_comp_count = 0;
                continue;
            }
            vpc('\n');
            if(ilen>0){
                lash_push_hist(input);
                sh_dispatch(input);
            }
            // Reset everything after command
            for(int i=0;i<SH_BUF;i++) input[i]=0;
            for(int i=0;i<SH_BUF;i++) lash_comp_input[i]=0;
            ilen = 0;
            cur = 0;
            lash_hi_cur = -1;
            lash_comp_shown = 0;
            lash_comp_count = 0;
            lash_comp_ilen = 0;
            // Also clear keyboard buffer to prevent stale keys!
            extern volatile int kb_h;
            extern volatile int kb_t;
            kb_h = kb_t = 0;
            lash_render_prompt();
        }
        else if(c==3){
            vps("^C");
            vpc('\n');
            // Reset everything
            for(int i=0;i<SH_BUF;i++) input[i]=0;
            for(int i=0;i<SH_BUF;i++) lash_comp_input[i]=0;
            ilen = 0;
            cur = 0;
            lash_hi_cur = -1;
            lash_comp_shown = 0;
            lash_comp_count = 0;
            lash_comp_ilen = 0;
            // Also clear keyboard buffer
            extern volatile int kb_h;
            extern volatile int kb_t;
            kb_h = kb_t = 0;
            lash_render_prompt();
        }
        else if(c=='\b'||c==127){
            // Delete at cursor position - allow deleting even at position 0
            if(ilen>0){
                if(cur>0) cur--;
                for(int i=cur;i<ilen-1;i++)input[i]=input[i+1];
                ilen--;
                vpc('\b');
                vpc(' ');
                vpc('\b');
                for(int i=cur;i<ilen;i++)vpc((uint8_t)input[i]);
                for(int i=cur;i<ilen;i++)vpc('\b');
            }
            // If everything deleted, FULL CLEAR the buffer
            if(ilen==0){
                for(int i=0;i<SH_BUF;i++) input[i]=0;
                cur = 0;
            } else {
                input[ilen] = 0;  // null-terminate
            }
            // Reset completion state
            lash_comp_shown = 0;
            lash_comp_count = 0;
        }
        else if(c==KEY_UP){
            lash_comp_shown = 0;
            lash_comp_count = 0;
            if(lash_hn>0){
                if(lash_hi_cur<0) lash_hi_cur=lash_hn-1;
                else if(lash_hi_cur>0) lash_hi_cur--;
                while(ilen>0){ilen--;vpc('\b');vpc(' ');vpc('\b');}
                kcp(input,lash_hist[lash_hi_cur]);
                ilen=ksl(input);
                cur = ilen;
                input[ilen] = 0;  // null-terminate!
                for(int i=0;i<ilen;i++) vpc((uint8_t)input[i]);
            }
        }
        else if(c==KEY_DOWN){
            lash_comp_shown = 0;
            lash_comp_count = 0;
            if(lash_hi_cur>=0&&lash_hi_cur<lash_hn-1){
                lash_hi_cur++;
                while(ilen>0){ilen--;vpc('\b');vpc(' ');vpc('\b');}
                kcp(input,lash_hist[lash_hi_cur]);
                ilen=ksl(input);
                cur = ilen;
                input[ilen] = 0;  // null-terminate!
                for(int i=0;i<ilen;i++) vpc((uint8_t)input[i]);
            } else {
                lash_hi_cur=-1;
                while(ilen>0){ilen--;vpc('\b');vpc(' ');vpc('\b');}
                ilen=0;
            }
        }
else if(c=='\t'){
            // Tab - complete or show options
            if(ilen==0) continue;
            char prefix[64]="";
            int pi=0;
            for(int i=0;i<ilen && i<63 && input[i]!=' ';i++){
                prefix[pi++]=input[i];
            }
            prefix[pi]=0;
            lash_comp_pos = 0;  // reset cycle position on new search
            lash_find_completions(prefix);
            if(lash_comp_count==0) continue;
            // Check if current input already matches a candidate (already complete)
            int already_complete = 0;
            for(int i=0; i<lash_comp_count; i++){
                // Check without trailing space
                int cmp_len = ilen;
                if(ilen > 0 && input[ilen-1] == ' ') cmp_len = ilen - 1;
                char tmp = input[cmp_len];
                input[cmp_len] = 0;  // temp null-terminate
                if(!ksc(input, lash_comp_candidates[i])){
                    input[cmp_len] = tmp;  // restore
                    already_complete = 1;
                    break;
                }
                input[cmp_len] = tmp;  // restore
            }
            // If already complete, do nothing on Tab
            if(already_complete){
                continue;
            }
            // If already showing list, cycle through matches
            if(lash_comp_shown && lash_comp_count > 1){
                // Cycle to next match
                lash_comp_pos++;
                if(lash_comp_pos >= lash_comp_count) lash_comp_pos = 0;  // reset to first if at end
                // Redraw with new match
                while(ilen>0){vpc('\b');vpc(' ');vpc('\b');ilen--;}
                const char* cmd = lash_comp_candidates[lash_comp_pos];
                int clen = ksl(cmd);
                for(int i=0; i<clen && ilen<SH_BUF-1; i++){
                    input[ilen++] = cmd[i];
                    vpc((uint8_t)cmd[i]);
                }
                input[ilen] = 0;
                cur = ilen;
                continue;
            }
            // If 1 match, auto-complete
            if(lash_comp_count==1){
                const char*cmd = lash_comp_candidates[0];
                while(ilen>0){vpc('\b');vpc(' ');vpc('\b');ilen--;}
                int clen = ksl(cmd);
                for(int i=0;i<clen && ilen<SH_BUF-1;i++){
                    input[ilen++] = cmd[i];
                    vpc((uint8_t)cmd[i]);
                }
                input[ilen]=0;
                cur=ilen;
                vpc(' ');
                continue;
            }
            // Multiple - show list and enable cycling
            vpc('\n');
            for(int i=0;i<lash_comp_count;i++){
                vps(lash_comp_candidates[i]);vpc(' ');
            }
            vpc('\n');
            lash_comp_shown = 1;
            lash_comp_pos = 0;
            kcp(lash_comp_input, input);
            lash_comp_ilen = ilen;
            lash_render_prompt();
            for(int i=0;i<ilen;i++)vpc((uint8_t)input[i]);
            continue;
        }
        else if(c>=32&&c<127&&ilen<SH_BUF-1){
            // Insert at cursor position
            for(int i=ilen;i>cur;i--)input[i]=input[i-1];
            input[cur]=(char)c;
            ilen++;
            cur++;
            input[ilen] = 0;  // null-terminate!
            vpc((uint8_t)c);
            for(int i=cur;i<ilen;i++)vpc((uint8_t)input[i]);
            for(int i=cur;i<ilen;i++)vpc('\b');
        }
    }
}

static void cmd_lash_main(const char*a){
    (void)a;
    lash_loop();
}

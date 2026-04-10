#pragma once
// pigOS Discord Client - Full interactive login + chat
// Freestanding, no libc
#include "../kernel/mem.h"
#include "../drivers/vga/vga.h"
#include "../drivers/ps2/ps2.h"
#include "../drivers/net/network.h"
#include "tls_client.h"
#include "websocket.h"
#include "json.h"
#include "sha256.h"
#include <stdint.h>

#define DISC_MAX_TOKEN 512
#define DISC_MAX_GUILDS 64
#define DISC_MAX_CHANNELS 128
#define DISC_MAX_MESSAGES 128
#define DISC_MAX_MSG_LEN 512
#define DISC_MAX_NAME 128
#define DISC_RECV_BUF 8192
#define DISC_MAX_EMAIL 128
#define DISC_MAX_PASS 128

static char disc_token[DISC_MAX_TOKEN]="";
static int disc_token_set=0;
static char disc_email[DISC_MAX_EMAIL]="";
static char disc_username[DISC_MAX_NAME]="";
static char disc_last_guild[DISC_MAX_NAME]="";
static int disc_sel_guild=0;
static int disc_sel_channel=0;
static int disc_msg_scroll=0;

typedef struct{
    char id[DISC_MAX_NAME];
    char name[DISC_MAX_NAME];
} DiscGuild;

typedef struct{
    char id[DISC_MAX_NAME];
    char name[DISC_MAX_NAME];
    char guild_id[DISC_MAX_NAME];
    int type;
} DiscChannel;

typedef struct{
    char id[DISC_MAX_NAME];
    char author[DISC_MAX_NAME];
    char content[DISC_MAX_MSG_LEN];
    char timestamp[DISC_MAX_NAME];
} DiscMessage;

static DiscGuild disc_guilds[DISC_MAX_GUILDS];
static int disc_nguilds=0;
static DiscChannel disc_channels[DISC_MAX_CHANNELS];
static int disc_nchannels=0;
static DiscMessage disc_messages[DISC_MAX_MESSAGES];
static int disc_nmessages=0;

// Forward declarations
static void disc_get_messages(const char*channel_id);
static void disc_send_message(const char*channel_id, const char*content);
static void disc_draw_chat(const char*input, int input_cur);

static int disc_read_line(char*buf, int max, int mask){
    int len=0, cur=0;
    while(1){
        ps2_poll();
        while(kb_avail()){
            int c=kb_get();
            if(c==27) return -1;
            if(c=='\n'||c=='\r'){
                buf[len]=0;
                return len;
            }
            if(c=='\b'||c==127){
                if(cur>0){
                    for(int i=cur-1;i<len-1;i++) buf[i]=buf[i+1];
                    len--; cur--;
                    buf[len]=0;
                    vpc('\b'); vpc(' '); vpc('\b');
                }
            } else if(c>=32&&c<127&&len<max-1){
                if(cur<len){
                    for(int i=len;i>cur;i--) buf[i]=buf[i-1];
                }
                buf[cur]=(char)c;
                len++; cur++;
                buf[len]=0;
                if(mask) vpc('*'); else vpc((char)c);
            }
        }
    }
}

static int disc_http_send(const char*method, const char*path, const char*body, char*out, int maxout){
    if(!net_ok){ vpln("disc: no network"); return 0; }
    if(!tls_connect("discord.com",443)){ vpln("disc: connect failed"); return 0; }
    char req[2048];
    int off=0;
    kcp(req,method); off=ksl(req);
    kcp(req+off," "); off=ksl(req+off);
    kcp(req+off,path); off=ksl(req+off);
    kcp(req+off," HTTP/1.1\r\n"); off=ksl(req+off);
    kcp(req+off,"Host: discord.com\r\n"); off=ksl(req+off);
    if(disc_token[0]){
        kcp(req+off,"Authorization: "); off=ksl(req+off);
        kcp(req+off,disc_token); off=ksl(req+off);
        kcp(req+off,"\r\n"); off=ksl(req+off);
    }
    kcp(req+off,"Content-Type: application/json\r\n"); off=ksl(req+off);
    kcp(req+off,"Accept: */*\r\n"); off=ksl(req+off);
    kcp(req+off,"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\r\n"); off=ksl(req+off);
    if(body&&body[0]){
        char cl[16]; int bl=ksl(body); kia(bl,cl);
        kcp(req+off,"Content-Length: "); off=ksl(req+off);
        kcp(req+off,cl); off=ksl(req+off);
        kcp(req+off,"\r\n\r\n"); off=ksl(req+off);
        kcp(req+off,body); off=ksl(req+off);
    } else { kcp(req+off,"\r\n"); off=ksl(req+off); }
    tls_send((uint8_t*)req,off);
    int rlen=0; out[0]=0;
    for(int i=0;i<20000000&&rlen<maxout-1;i++){
        int r=tls_recv((uint8_t*)(out+rlen),maxout-rlen-1);
        if(r>0) rlen+=r;
        if(rlen>4&&out[rlen-4]==0x0D&&out[rlen-3]==0x0A&&out[rlen-2]==0x0D&&out[rlen-1]==0x0A) break;
    }
    out[rlen<maxout?rlen:maxout-1]=0;
    char*bod=out;
    for(int i=0;i<rlen-4;i++){
        if(out[i]==0x0D&&out[i+1]==0x0A&&out[i+2]==0x0D&&out[i+3]==0x0A){ bod=out+i+4; break; }
    }
    if(bod!=out){
        int bl=ksl(bod);
        for(int i=0;i<bl&&i<maxout-1;i++) out[i]=bod[i];
        out[bl<maxout?bl:maxout-1]=0;
    }
    tls_close();
    return 1;
}

static void disc_parse_guilds(const char*json_str){
    disc_nguilds=0;
    JsonTree*t=json_parse(json_str,ksl(json_str));
    if(!t||t->nnodes<1) return;
    JsonNode*root=&t->nodes[0];
    if(root->type!=JSON_ARRAY) return;
    int al=json_array_len(t,root);
    for(int i=0;i<al&&i<DISC_MAX_GUILDS;i++){
        JsonNode*g=json_array_get(t,root,i);
        if(!g||g->type!=JSON_OBJECT) continue;
        const char*id=json_get_string(t,g,"id");
        const char*name=json_get_string(t,g,"name");
        if(id) kcp(disc_guilds[i].id,id);
        if(name) kcp(disc_guilds[i].name,name);
        disc_nguilds++;
    }
}

static void disc_parse_channels(const char*json_str){
    disc_nchannels=0;
    JsonTree*t=json_parse(json_str,ksl(json_str));
    if(!t||t->nnodes<1) return;
    JsonNode*root=&t->nodes[0];
    if(root->type!=JSON_ARRAY) return;
    int al=json_array_len(t,root);
    for(int i=0;i<al&&i<DISC_MAX_CHANNELS;i++){
        JsonNode*ch=json_array_get(t,root,i);
        if(!ch||ch->type!=JSON_OBJECT) continue;
        const char*id=json_get_string(t,ch,"id");
        const char*name=json_get_string(t,ch,"name");
        int type=json_get_int(t,ch,"type");
        if(id) kcp(disc_channels[i].id,id);
        if(name) kcp(disc_channels[i].name,name);
        disc_channels[i].type=type;
        const char*gid=json_get_string(t,ch,"guild_id");
        if(gid) kcp(disc_channels[i].guild_id,gid);
        disc_nchannels++;
    }
}

static void disc_parse_messages(const char*json_str){
    disc_nmessages=0;
    JsonTree*t=json_parse(json_str,ksl(json_str));
    if(!t||t->nnodes<1) return;
    JsonNode*root=&t->nodes[0];
    if(root->type!=JSON_ARRAY) return;
    int al=json_array_len(t,root);
    for(int i=0;i<al&&i<DISC_MAX_MESSAGES;i++){
        JsonNode*msg=json_array_get(t,root,i);
        if(!msg||msg->type!=JSON_OBJECT) continue;
        const char*id=json_get_string(t,msg,"id");
        const char*ts=json_get_string(t,msg,"timestamp");
        if(id) kcp(disc_messages[i].id,id);
        if(ts) kcp(disc_messages[i].timestamp,ts);
        JsonNode*author=json_get_object(t,msg,"author");
        if(author){
            const char*un=json_get_string(t,author,"username");
            if(un) kcp(disc_messages[i].author,un);
        }
        const char*content=json_get_string(t,msg,"content");
        if(content) kcp(disc_messages[i].content,content);
        disc_nmessages++;
    }
}

static void disc_get_messages(const char*channel_id){
    char path[256];
    kcp(path,"/api/v9/channels/");
    kcat(path,channel_id);
    kcat(path,"/messages?limit=50");
    char resp[DISC_RECV_BUF];
    if(disc_http_send("GET",path,NULL,resp,DISC_RECV_BUF)){
        disc_parse_messages(resp);
        disc_msg_scroll=disc_nmessages-(VGA_H-4);
        if(disc_msg_scroll<0) disc_msg_scroll=0;
    }
}

static void disc_send_message(const char*channel_id, const char*content){
    char body[1024];
    kcp(body,"{\"content\":\"");
    int off=ksl(body);
    for(int i=0;content[i]&&off<1020;i++){
        if(content[i]=='"'){body[off++]='\\';body[off++]='"';}
        else if(content[i]=='\\'){body[off++]='\\';body[off++]='\\';}
        else body[off++]=content[i];
    }
    body[off++]='"';body[off++]='}';body[off]=0;
    char path[256];
    kcp(path,"/api/v9/channels/"); kcat(path,channel_id); kcat(path,"/messages");
    char resp[DISC_RECV_BUF];
    disc_http_send("POST",path,body,resp,DISC_RECV_BUF);
}

static void disc_draw_chat(const char*input, int input_cur){
    vclear();
    int panel_w=24;
    uint8_t bc=COL(C_BLUE,C_BLUE);
    uint8_t sel=COL(C_BLACK,C_LCYAN);
    uint8_t msg_bg=COL(C_WHITE,C_BLACK);
    uint8_t hdr=COL(C_BLACK,C_LGREY);
    for(int r=0;r<VGA_H;r++) vat(CH_V,bc,panel_w,r);
    vat(CH_TL,bc,0,0); vat(CH_TR,bc,panel_w,0);
    vat(CH_STL,bc,0,1); vat(CH_STR,bc,panel_w,1);
    vat(CH_ML,bc,0,2); vat(CH_MR,bc,panel_w,2);
    for(int r=3;r<VGA_H-1;r++){ vat(CH_V,bc,0,r); vat(CH_V,bc,panel_w,r); }
    vat(CH_BL,bc,0,VGA_H-2); vat(CH_BR,bc,panel_w,VGA_H-2);
    vstr(2,0,COL(C_WHITE,C_BLUE)," Channels");
    vstr(panel_w+2,0,COL(C_WHITE,C_BLACK)," #general");
    vfill(0,VGA_H-1,VGA_W,1,' ',hdr);
    vstr(1,VGA_H-1,COL(C_WHITE,C_LGREY),"> ");
    if(input){
        int sl=ksl(input);
        for(int i=0;i<input_cur&&i<sl&&panel_w+3+i<VGA_W-1;i++)
            vat((uint8_t)input[i],COL(C_WHITE,C_LGREY),panel_w+3+i,VGA_H-1);
        if(input_cur<sl)
            for(int i=input_cur;i<sl&&panel_w+3+i<VGA_W-1;i++)
                vat((uint8_t)input[i],COL(C_LGREY,C_LGREY),panel_w+3+i,VGA_H-1);
    }
    for(int i=0;i<disc_nchannels&&i<VGA_H-3;i++){
        DiscChannel*c=&disc_channels[i];
        uint8_t col=msg_bg;
        if(i==disc_sel_channel) col=sel;
        char prefix='#';
        if(c->type==4) prefix='>';
        else if(c->type==2) prefix='v';
        vat((uint8_t)prefix,col,1,i+3);
        int nl=ksl(c->name);
        for(int j=0;j<nl&&j<panel_w-3;j++) vat((uint8_t)c->name[j],col,3+j,i+3);
    }
    int msg_start=disc_msg_scroll;
    if(msg_start<0) msg_start=0;
    int msg_area=VGA_H-4;
    for(int i=0;i<msg_area&&msg_start+i<disc_nmessages;i++){
        DiscMessage*m=&disc_messages[msg_start+i];
        int row=3+i;
        int nl=ksl(m->author);
        vstr(panel_w+1,row,COL(C_LCYAN,C_BLACK),m->author);
        vstr(panel_w+1+nl,row,COL(C_LGREY,C_BLACK),": ");
        int cl=ksl(m->content);
        int max_cl=VGA_W-panel_w-4;
        if(cl>max_cl) cl=max_cl;
        for(int j=0;j<cl;j++) vat((uint8_t)m->content[j],msg_bg,panel_w+3+nl+j,row);
    }
    vga_update_hw_cursor();
}

static void disc_chat_run(void){
    if(disc_nchannels==0){ vpln("disc: no channels"); return; }
    disc_get_messages(disc_channels[disc_sel_channel].id);
    char input[256]=""; int input_len=0, input_cur=0;
    int running=1, poll_count=0;
    while(running){
        disc_draw_chat(input,input_cur);
        ps2_poll();
        while(kb_avail()){
            int c=kb_get();
            if(c==27){ running=0; break; }
            if(c==KEY_UP){ if(disc_msg_scroll>0) disc_msg_scroll--; }
            else if(c==KEY_DOWN){ if(disc_msg_scroll<disc_nmessages-(VGA_H-4)) disc_msg_scroll++; }
            else if(c==KEY_LEFT){ if(input_cur>0) input_cur--; }
            else if(c==KEY_RIGHT){ if(input_cur<input_len) input_cur++; }
            else if(c=='\n'||c=='\r'){
                if(input_len>0){
                    input[input_len]=0;
                    disc_send_message(disc_channels[disc_sel_channel].id,input);
                    kcp(disc_messages[disc_nmessages].author,"You");
                    kcp(disc_messages[disc_nmessages].content,input);
                    disc_nmessages++;
                    disc_msg_scroll=disc_nmessages-(VGA_H-4);
                    if(disc_msg_scroll<0) disc_msg_scroll=0;
                    input[0]=0; input_len=0; input_cur=0;
                }
            } else if(c=='\b'||c==127){
                if(input_cur>0){
                    for(int i=input_cur-1;i<input_len-1;i++) input[i]=input[i+1];
                    input_len--; input_cur--; input[input_len]=0;
                }
            } else if(c>=32&&c<127&&input_len<255){
                if(input_cur<input_len) for(int i=input_len;i>input_cur;i--) input[i]=input[i-1];
                input[input_cur]=(char)c; input_len++; input_cur++; input[input_len]=0;
            }
        }
        poll_count++;
        if(poll_count>500000){ poll_count=0; disc_get_messages(disc_channels[disc_sel_channel].id); }
    }
    vclear(); vrst();
}

static void cmd_discord(const char*args){
    if(!net_ok){ vset(C_LRED,C_BLACK); vpln("discord: no network. Run: netpg dhcp"); vrst(); return; }

    // If args provided, use command mode
    if(args&&*args){
        if(!ksnc(args,"login",5)){
            const char*t=args+5; while(*t==' ') t++;
            if(*t){ kcp(disc_token,t); disc_token_set=1; vpln("disc: token set"); }
        } else if(!ksnc(args,"post",4)){
            if(!disc_token_set){ vpln("disc: not logged in"); return; }
            const char*m=args+4; while(*m==' ') m++;
            if(disc_nchannels>0) disc_send_message(disc_channels[disc_sel_channel].id,m);
            else vpln("disc: no channel selected");
        } else if(!ksnc(args,"send",4)){
            const char*s=args+4; while(*s==' ') s++;
            char cid[DISC_MAX_NAME]=""; int ci=0;
            while(*s&&*s!=' '&&ci<DISC_MAX_NAME-1) cid[ci++]=*s++; cid[ci]=0;
            while(*s==' ') s++;
            if(*s&&*cid) disc_send_message(cid,s);
        } else if(!ksnc(args,"channels",8)){
            const char*g=args+8; while(*g==' ') g++;
            if(*g){
                char path[256]; kcp(path,"/api/v9/guilds/"); kcat(path,g); kcat(path,"/channels");
                char resp[DISC_RECV_BUF];
                if(disc_http_send("GET",path,NULL,resp,DISC_RECV_BUF)){ disc_parse_channels(resp);
                    for(int i=0;i<disc_nchannels;i++){ vps("  #"); vpln(disc_channels[i].name); }
                }
            }
        }
        return;
    }

    // ═══════════════════════════════════════════════════
    // INTERACTIVE MODE - full Discord app experience
    // ═══════════════════════════════════════════════════

    vclear();
    vset(C_LCYAN,C_BLACK);
    vpln("");
    vpln("  ╔══════════════════════════════════╗");
    vpln("  ║     Discord for pigOS v1.0       ║");
    vpln("  ╚══════════════════════════════════╝");
    vrst();
    vpln("");

    // Step 1: Login
    vset(C_LGREEN,C_BLACK);
    vpln("  Step 1: Login to Discord");
    vrst();
    vpln("");
    vps("  Email: ");
    char email[DISC_MAX_EMAIL]="";
    if(disc_read_line(email,DISC_MAX_EMAIL,0)<0){ vclear(); vrst(); return; }
    vpln("");
    vps("  Password: ");
    char pass[DISC_MAX_PASS]="";
    if(disc_read_line(pass,DISC_MAX_PASS,1)<0){ vclear(); vrst(); return; }
    vpln("");
    vpln("");

    // POST to /api/v9/auth/login
    vps("  Logging in...");
    char login_body[1024];
    kcp(login_body,"{\"login\":\"");
    int off=ksl(login_body);
    for(int i=0;email[i]&&off<1000;i++){
        if(email[i]=='"'){login_body[off++]='\\';login_body[off++]='"';}
        else if(email[i]=='\\'){login_body[off++]='\\';login_body[off++]='\\';}
        else login_body[off++]=email[i];
    }
    login_body[off++]='"';login_body[off++]=',';
    kcp(login_body+off,"\"password\":\""); off=ksl(login_body);
    for(int i=0;pass[i]&&off<1000;i++){
        if(pass[i]=='"'){login_body[off++]='\\';login_body[off++]='"';}
        else if(pass[i]=='\\'){login_body[off++]='\\';login_body[off++]='\\';}
        else login_body[off++]=pass[i];
    }
    login_body[off++]='"';
    // Required fields for Discord API v9
    kcp(login_body+off,",\"undelete\":false,\"login_source\":null,\"gift_code_sku_id\":null}");

    char resp[DISC_RECV_BUF];
    vpln("  Attempting TLS connection to discord.com:443...");
    if(!tls_connect("discord.com",443)){
        vpln("  TLS connection failed - network may be unreachable");
        vpln("  Try: netpg dhcp, then ping 10.0.2.2");
        return;
    }
    char req[2048];
    off=0;
    kcp(req,"POST /api/v9/auth/login HTTP/1.1\r\n"); off=ksl(req);
    kcp(req+off,"Host: discord.com\r\n"); off=ksl(req+off);
    kcp(req+off,"Content-Type: application/json\r\n"); off=ksl(req+off);
    kcp(req+off,"Accept: */*\r\n"); off=ksl(req+off);
    kcp(req+off,"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\r\n"); off=ksl(req+off);
    kcp(req+off,"X-Super-Properties: eyJvcyI6IkxpbnV4IiwiYnJvd3NlciI6IkNocm9tZSIsImRldmljZSI6IiJ9\r\n"); off=ksl(req+off);
    char cl[16]; int bl=ksl(login_body); kia(bl,cl);
    kcp(req+off,"Content-Length: "); off=ksl(req+off);
    kcp(req+off,cl); off=ksl(req+off);
    kcp(req+off,"\r\n\r\n"); off=ksl(req+off);
    kcp(req+off,login_body); off=ksl(req+off);
    vpln("  Sending login request...");
    tls_send((uint8_t*)req,off);
    int rlen=0; resp[0]=0;
    for(int i=0;i<30000000&&rlen<DISC_RECV_BUF-1;i++){
        int r=tls_recv((uint8_t*)(resp+rlen),DISC_RECV_BUF-rlen-1);
        if(r>0) rlen+=r;
        if(rlen>4&&resp[rlen-4]==0x0D&&resp[rlen-3]==0x0A&&resp[rlen-2]==0x0D&&resp[rlen-1]==0x0A) break;
    }
    resp[rlen<DISC_RECV_BUF?rlen:DISC_RECV_BUF-1]=0;
    vps("  Response size: "); char rbuf[8]; kia(rlen,rbuf); vpln(rbuf);
    char*bod=resp;
    for(int i=0;i<rlen-4;i++){
        if(resp[i]==0x0D&&resp[i+1]==0x0A&&resp[i+2]==0x0D&&resp[i+3]==0x0A){ bod=resp+i+4; break; }
    }
    tls_close();

    // Parse response
    JsonTree*t=json_parse(bod,ksl(bod));
    if(!t){ vpln("  Login failed - check credentials"); return; }
    JsonNode*root=&t->nodes[0];

    const char*token=json_get_string(t,root,"token");
    if(token){
        kcp(disc_token,token);
        disc_token_set=1;
        vpln("  Logged in!");
        // Extract username
        JsonNode*u=json_get_object(t,root,"user");
        if(u){
            const char*un=json_get_string(t,u,"username");
            if(un){ kcp(disc_username,un); vps("  Welcome, "); vpln(un); }
        }
    } else {
        const char*msg=json_get_string(t,root,"message");
        vps("  Login failed: ");
        if(msg) vpln(msg); else vpln("unknown error");
        return;
    }

    // Step 2: Select server
    vpln("");
    vset(C_LGREEN,C_BLACK);
    vpln("  Step 2: Select a Server");
    vrst();
    vpln("");
    vps("  Loading servers...");
    char resp2[DISC_RECV_BUF];
    if(disc_http_send("GET","/api/v9/users/@me/guilds",NULL,resp2,DISC_RECV_BUF)){
        disc_parse_guilds(resp2);
    }
    vpln("");
    if(disc_nguilds==0){ vpln("  No servers found."); return; }
    for(int i=0;i<disc_nguilds;i++){
        vps("    ["); char b[4]; kia(i+1,b); vps(b);
        vps("] "); vpln(disc_guilds[i].name);
    }
    vpln("");
    vps("  Select server [1-");
    char nb[4]; kia(disc_nguilds,nb); vps(nb); vps("]: ");
    char sel[8]="";
    if(disc_read_line(sel,8,0)<0){ vclear(); vrst(); return; }
    int sidx=0;
    const char*p=sel;
    while(*p>='0'&&*p<='9') sidx=sidx*10+(*p++-'0');
    sidx--;
    if(sidx<0||sidx>=disc_nguilds){ vpln("  Invalid selection"); return; }
    kcp(disc_last_guild,disc_guilds[sidx].id);
    disc_sel_guild=sidx;
    vps("  Selected: "); vpln(disc_guilds[sidx].name);

    // Step 3: Select channel
    vpln("");
    vset(C_LGREEN,C_BLACK);
    vpln("  Step 3: Select a Channel");
    vrst();
    vpln("");
    vps("  Loading channels...");
    char path[256]; kcp(path,"/api/v9/guilds/"); kcat(path,disc_guilds[sidx].id); kcat(path,"/channels");
    char resp3[DISC_RECV_BUF];
    if(disc_http_send("GET",path,NULL,resp3,DISC_RECV_BUF)){ disc_parse_channels(resp3); }
    vpln("");
    int text_ch=0;
    for(int i=0;i<disc_nchannels;i++){
        if(disc_channels[i].type==0){
            vps("    ["); char b[4]; kia(text_ch+1,b); vps(b);
            vps("] #"); vpln(disc_channels[i].name);
            text_ch++;
        }
    }
    vpln("");
    vps("  Select channel [1-");
    kia(text_ch,nb); vps(nb); vps("]: ");
    char cs[8]="";
    if(disc_read_line(cs,8,0)<0){ vclear(); vrst(); return; }
    int cidx=0; p=cs;
    while(*p>='0'&&*p<='9') cidx=cidx*10+(*p++-'0');
    cidx--;
    // Find the actual channel index
    int found=0;
    for(int i=0;i<disc_nchannels;i++){
        if(disc_channels[i].type==0){
            if(cidx==0){ disc_sel_channel=i; found=1; break; }
            cidx--;
        }
    }
    if(!found){ vpln("  Invalid selection"); return; }
    vps("  Selected: #"); vpln(disc_channels[disc_sel_channel].name);
    vpln("");

    // Step 4: Chat!
    vset(C_LGREEN,C_BLACK);
    vpln("  Starting chat... (ESC to exit, arrows to scroll)");
    vrst();
    for(volatile int i=0;i<5000000;i++);

    disc_chat_run();
}

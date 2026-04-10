#pragma once
// pigOS WebSocket Client - RFC 6455
// Freestanding, no libc
#include "../kernel/mem.h"
#include "../drivers/vga/vga.h"
#include "../drivers/net/network.h"
#include "sha256.h"
#include "tls_client.h"
#include "json.h"
#include <stdint.h>

#define WS_MAX_FRAME 65536
#define WS_GUID "258EAFA5-E914-47DA-95CA-5AB7AC5B261B"

#define WS_OP_TEXT 0x1
#define WS_OP_BINARY 0x2
#define WS_OP_CLOSE 0x8
#define WS_OP_PING 0x9
#define WS_OP_PONG 0xA

typedef struct{
    char host[128];
    char path[256];
    uint16_t port;
    int connected;
    uint8_t frame_buf[WS_MAX_FRAME];
    int frame_len;
} WS_CTX;

static WS_CTX _ws;

static void ws_b64_encode(const uint8_t*in, int inlen, char*out, int outmax){
    static const char b64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int oi=0;
    for(int i=0;i<inlen&&oi<outmax-4;i+=3){
        uint32_t v=(uint32_t)in[i]<<16;
        if(i+1<inlen) v|=(uint32_t)in[i+1]<<8;
        if(i+2<inlen) v|=(uint32_t)in[i+2];
        out[oi++]=b64[(v>>18)&0x3F];
        out[oi++]=b64[(v>>12)&0x3F];
        out[oi++]=(i+1<inlen)?b64[(v>>6)&0x3F]:'=';
        out[oi++]=(i+2<inlen)?b64[v&0x3F]:'=';
    }
    out[oi]=0;
}

static void ws_mask_data(uint8_t*data, int len, const uint8_t*mask){
    for(int i=0;i<len;i++) data[i]^=mask[i&3];
}

static int ws_send_frame(uint8_t opcode, const uint8_t*payload, int plen){
    uint8_t frame[WS_MAX_FRAME+14];
    int off=0;
    frame[off++]=0x80|opcode;
    uint8_t mask[4];
    static uint32_t ms=0x12345678;
    ms^=ms<<13; ms^=ms>>17; ms^=ms<<5;
    mask[0]=(uint8_t)(ms&0xFF);
    mask[1]=(uint8_t)((ms>>8)&0xFF);
    mask[2]=(uint8_t)((ms>>16)&0xFF);
    mask[3]=(uint8_t)((ms>>24)&0xFF);
    if(plen<126){
        frame[off++]=0x80|(uint8_t)plen;
    } else if(plen<65536){
        frame[off++]=0x80|126;
        frame[off++]=(uint8_t)(plen>>8);
        frame[off++]=(uint8_t)plen;
    } else {
        frame[off++]=0x80|127;
        for(int i=7;i>=0;i--) frame[off++]=(uint8_t)(plen>>(i*8));
    }
    kmc(frame+off,mask,4); off+=4;
    kmc(frame+off,payload,plen);
    ws_mask_data(frame+off,plen,mask);
    off+=plen;
    return tls_send(frame,off);
}

static int ws_recv_frame(uint8_t*opcode_out, uint8_t*out, int maxout){
    uint8_t hdr[14];
    int got=0;
    while(got<2){
        int r=tls_recv(hdr+got,2-got);
        if(r<=0) return -1;
        got+=r;
    }
    uint8_t op=hdr[0]&0x0F;
    int masked=hdr[1]&0x80;
    uint64_t plen64=hdr[1]&0x7F;
    uint64_t plen=plen64;
    if(plen64==126){
        got=0;
        while(got<2){
            int r=tls_recv(hdr+2+got,2-got);
            if(r<=0) return -1;
            got+=r;
        }
        plen=(uint64_t)hdr[2]<<8|hdr[3];
    } else if(plen64==127){
        got=0;
        while(got<8){
            int r=tls_recv(hdr+2+got,8-got);
            if(r<=0) return -1;
            got+=r;
        }
        plen=0;
        for(int i=0;i<8;i++) plen=(plen<<8)|hdr[2+i];
    }
    uint8_t mask[4]={0};
    if(masked){
        got=0;
        while(got<4){
            int r=tls_recv(mask+got,4-got);
            if(r<=0) return -1;
            got+=r;
        }
    }
    if((int)plen>maxout) plen=maxout;
    int poff=0;
    while(poff<(int)plen){
        int r=tls_recv(out+poff,(int)plen-poff);
        if(r<=0) return -1;
        poff+=r;
    }
    if(masked){
        for(int i=0;i<poff;i++) out[i]^=mask[i&3];
    }
    *opcode_out=op;
    if(op==WS_OP_PING){
        ws_send_frame(WS_OP_PONG,out,poff);
        return -2;
    }
    return poff;
}

int ws_connect(const char*host, const char*path){
    kms(&_ws,0,sizeof(WS_CTX));
    kcp(_ws.host,host);
    kcp(_ws.path,path);
    _ws.port=443;
    vps("ws: connecting to "); vps(host); vps(path); vpln("");
    if(!tls_connect(host,_ws.port)){
        vpln("ws: TLS connect failed");
        return 0;
    }
    uint8_t key_bytes[16];
    for(int i=0;i<16;i++){
        static uint32_t s=0xABCDEF01;
        s^=s<<13; s^=s>>17; s^=s<<5;
        key_bytes[i]=(uint8_t)(s&0xFF);
    }
    char key_b64[32];
    ws_b64_encode(key_bytes,16,key_b64,32);
    char req[512];
    int off=0;
    const char*lines[]={
        "GET ",path," HTTP/1.1\r\n",
        "Host: ",host,"\r\n",
        "Upgrade: websocket\r\n",
        "Connection: Upgrade\r\n",
        "Sec-WebSocket-Key: ",key_b64,"\r\n",
        "Sec-WebSocket-Version: 13\r\n",
        "Sec-WebSocket-Protocol: json\r\n",
        "\r\n"
    };
    for(int i=0;i<9;i++){
        int l=ksl(lines[i]);
        kmc(req+off,lines[i],l);
        off+=l;
    }
    vpln("ws: sending handshake...");
    tls_send((uint8_t*)req,off);
    uint8_t resp[1024];
    int rlen=0;
    for(int i=0;i<5000000;i++){
        int r=tls_recv(resp+rlen,1024-rlen);
        if(r>0) rlen+=r;
        if(rlen>=4&&resp[rlen-4]=='\r'&&resp[rlen-3]=='\n'&&resp[rlen-2]=='\r'&&resp[rlen-1]=='\n') break;
    }
    resp[rlen]=0;
    if(ksnc((char*)resp,"HTTP/1.1 101",12)!=0){
        vpln("ws: handshake failed");
        vps((char*)resp);
        return 0;
    }
    _ws.connected=1;
    vpln("ws: connected");
    return 1;
}

int ws_send(const uint8_t*data, int len, uint8_t opcode){
    if(!_ws.connected) return -1;
    return ws_send_frame(opcode,data,len);
}

int ws_recv(uint8_t*out, int max){
    if(!_ws.connected) return -1;
    uint8_t op;
    return ws_recv_frame(&op,out,max);
}

void ws_close(void){
    if(_ws.connected){
        uint8_t close_payload[2]={0x03,0xE8};
        ws_send_frame(WS_OP_CLOSE,close_payload,2);
    }
    _ws.connected=0;
    tls_close();
}

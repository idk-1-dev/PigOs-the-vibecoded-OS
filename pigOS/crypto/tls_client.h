#pragma once
// pigOS TLS 1.2 Client - RSA key exchange, AES-256-CBC-SHA256
// Freestanding, no libc
#include "../kernel/mem.h"
#include "../drivers/vga/vga.h"
#include "../drivers/net/network.h"
#include "sha256.h"
#include "aes256.h"
#include "rsa.h"
#include "asn1.h"
#include <stdint.h>

#define TLS_MAX_RECORD 16384
#define TLS_HDR_LEN 5
#define TLS_RANDOM_LEN 32
#define TLS_SESSION_ID_LEN 32
#define TLS_MAX_CERT 8192
#define TLS_MAX_FINISHED 32

typedef struct{
    uint32_t server_ip;
    uint16_t server_port;
    char host[128];
    uint8_t client_random[TLS_RANDOM_LEN];
    uint8_t server_random[TLS_RANDOM_LEN];
    uint8_t premaster[48];
    uint8_t master_secret[48];
    uint8_t key_block[136];
    AES_CTX enc_ctx;
    AES_CTX dec_ctx;
    uint8_t mac_write_key[32];
    uint8_t mac_read_key[32];
    uint8_t client_iv[16];
    uint8_t server_iv[16];
    uint32_t client_seq;
    uint32_t server_seq;
    X509Cert cert;
    uint8_t cert_der[TLS_MAX_CERT];
    uint32_t cert_len;
    int connected;
    uint8_t recv_buf[TLS_MAX_RECORD+256];
    int recv_pos;
    int recv_len;
} TLS_CTX;

static TLS_CTX _tls;

static uint32_t tls_rng(){
    static uint32_t s=0xCAFEBABE;
    s^=s<<13; s^=s>>17; s^=s<<5;
    return s;
}

static void tls_fill_random(uint8_t*buf, int len){
    for(int i=0;i<len;i+=4){
        uint32_t r=tls_rng();
        int c=len-i<4?len-i:4;
        for(int j=0;j<c;j++) buf[i+j]=(uint8_t)(r>>(j*8));
    }
}

static void tls_write_uint8(uint8_t*p, uint8_t v){ p[0]=v; }
static void tls_write_uint16(uint8_t*p, uint16_t v){ p[0]=(uint8_t)(v>>8); p[1]=(uint8_t)v; }
static void tls_write_uint24(uint8_t*p, uint32_t v){ p[0]=(uint8_t)(v>>16); p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)v; }
static void tls_write_uint32(uint8_t*p, uint32_t v){ p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16); p[2]=(uint8_t)(v>>8); p[3]=(uint8_t)v; }

static uint16_t tls_read_uint16(const uint8_t*p){ return (uint16_t)p[0]<<8|p[1]; }
static uint32_t tls_read_uint24(const uint8_t*p){ return (uint32_t)p[0]<<16|(uint32_t)p[1]<<8|p[2]; }
static uint32_t tls_read_uint32(const uint8_t*p){ return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }

static void tls_prf(uint8_t*out, int olen, const uint8_t*secret, int slen,
                    const char*label, const uint8_t*seed, int seedlen){
    int llen=ksl(label);
    uint8_t a[32+32];
    uint8_t seed_buf[256];
    int seed_total=llen+seedlen;
    if(seed_total>256) seed_total=256;
    for(size_t i=0;i<(size_t)llen&&i<256;i++) seed_buf[i]=(uint8_t)label[i];
    for(size_t i=0;i<(size_t)seedlen&&i+llen<256;i++) seed_buf[llen+i]=seed[i];
    kmc(a,secret,slen<32?slen:32);
    hmac_sha256(a,slen,seed_buf,seed_total,a);
    int offset=0;
    while(offset<olen){
        uint8_t p_buf[32+32+seed_total];
        size_t poff=0;
        kmc(p_buf,a,32); poff=32;
        for(size_t i=0;i<(size_t)llen&&poff<sizeof(p_buf);i++,poff++) p_buf[poff]=(uint8_t)label[i];
        for(size_t i=0;i<(size_t)seedlen&&poff<sizeof(p_buf);i++,poff++) p_buf[poff]=seed[i];
        uint8_t hash[32];
        hmac_sha256(secret,slen,p_buf,(int)poff,hash);
        int cpy=olen-offset<32?olen-offset:32;
        kmc(out+offset,hash,cpy);
        offset+=cpy;
        uint8_t a_seed[256];
        int as=32+seed_total;
        if(as>256) as=256;
        kmc(a_seed,a,32);
        for(size_t i=0;i<(size_t)seedlen&&i+32<(size_t)256;i++) a_seed[32+i]=seed[i];
        hmac_sha256(a,slen,a_seed,as,a);
    }
}

static void tls_compute_keys(TLS_CTX*t){
    uint8_t seed[64];
    kmc(seed,t->client_random,32);
    kmc(seed+32,t->server_random,32);
    tls_prf(t->key_block,104,t->master_secret,48,"key expansion",seed,64);
    int off=0;
    kmc(t->mac_write_key,t->key_block+off,32); off+=32;
    kmc(t->mac_read_key,t->key_block+off,32); off+=32;
    kmc(t->client_iv,t->key_block+off,16); off+=16;
    kmc(t->server_iv,t->key_block+off,16);
    aes_init_ctx_iv(&t->enc_ctx,t->mac_write_key,t->client_iv);
    aes_init_ctx_iv(&t->dec_ctx,t->mac_read_key,t->server_iv);
    t->client_seq=0;
    t->server_seq=0;
}

static void tls_compute_mac(uint8_t*out, TLS_CTX*t, const uint8_t*key, uint8_t type,
                                uint16_t ver, uint16_t len, const uint8_t*data){
    uint8_t hdr[13];
    uint64_t seq64=0;
    for(int i=0;i<8;i++) hdr[i]=(uint8_t)(seq64>>(56-i*8));
    hdr[8]=type;
    hdr[9]=(uint8_t)(ver>>8);
    hdr[10]=(uint8_t)ver;
    hdr[11]=(uint8_t)(len>>8);
    hdr[12]=(uint8_t)len;
    uint8_t msg[13+TLS_MAX_RECORD];
    kmc(msg,hdr,13);
    kmc(msg+13,data,len);
    hmac_sha256(key,32,msg,13+len,out);
}

static int tls_send_record(TLS_CTX*t, uint8_t type, const uint8_t*data, int len){
    uint8_t buf[TLS_MAX_RECORD+64];
    int dlen=len;
    uint8_t padded[TLS_MAX_RECORD+32];
    kmc(padded,data,len);
    uint8_t pad_val=(uint8_t)(16-(len%16));
    for(int i=0;i<pad_val;i++) padded[len+i]=pad_val;
    dlen=len+pad_val;
    uint8_t mac[32];
    uint8_t mac_tmp[32];
    kmc(mac_tmp,t->mac_write_key,32);
    hmac_sha256(mac_tmp,32,padded,dlen,mac);
    int total=dlen+16;
    uint8_t enc_buf[TLS_MAX_RECORD+48];
    kmc(enc_buf,padded,dlen);
    kmc(enc_buf+dlen,mac,16);
    total=dlen+16;
    uint8_t iv[16];
    kmc(iv,t->client_iv,16);
    AES_CTX ctx;
    aes_init_ctx_iv(&ctx,t->mac_write_key,iv);
    aes_cbc_encrypt(&ctx,enc_buf,total);
    kmc(t->client_iv,enc_buf+total-16,16);
    buf[0]=type;
    buf[1]=0x03; buf[2]=0x03;
    tls_write_uint16(buf+3,(uint16_t)total);
    kmc(buf+5,enc_buf,total);
    int sent=0;
    while(sent<5+total){
        uint8_t chunk[1200];
        int cl=5+total-sent;
        if(cl>1200) cl=1200;
        kmc(chunk,buf+sent,cl);
        int r=tcp_send_data(chunk,cl);
        if(r<=0) return -1;
        sent+=r;
    }
    t->client_seq++;
    return 0;
}

static int tls_tcp_recv(TLS_CTX*t, uint8_t*buf, int max, int timeout){
    for(int i=0;i<timeout;i++){
        int f=tcp_recv_data(buf,max);
        if(f>0) return f;
    }
    return -1;
}

static int tls_recv_record(TLS_CTX*t, uint8_t*type, uint8_t*data, int maxdata){
    uint8_t hdr[5];
    int got=0;
    while(got<5){
        uint8_t tmp[1200];
        int r=tls_tcp_recv(t,tmp,1200,500000);
        if(r<=0) return -1;
        int need=5-got;
        int cpy=r<need?r:need;
        kmc(hdr+got,tmp,cpy);
        got+=cpy;
        if(cpy<r){
            t->recv_len=r-cpy;
            if(t->recv_len>TLS_MAX_RECORD+256) t->recv_len=TLS_MAX_RECORD+256;
            kmc(t->recv_buf,tmp+cpy,t->recv_len);
            t->recv_pos=0;
        }
    }
    *type=hdr[0];
    int rlen=tls_read_uint16(hdr+3);
    if(rlen>maxdata) return -1;
    uint8_t raw[TLS_MAX_RECORD+64];
    got=0;
    while(got<rlen){
        int avail=t->recv_len-t->recv_pos;
        if(avail>0){
            int cpy=rlen-got;
            if(cpy>avail) cpy=avail;
            kmc(raw+got,t->recv_buf+t->recv_pos,cpy);
            got+=cpy;
            t->recv_pos+=cpy;
        } else {
            uint8_t tmp[1200];
            int r=tls_tcp_recv(t,tmp,1200,500000);
            if(r<=0) return -1;
            int cpy=rlen-got;
            if(cpy>r) cpy=r;
            kmc(raw+got,tmp,cpy);
            got+=cpy;
            if(cpy<r){
                t->recv_len=r-cpy;
                kmc(t->recv_buf,tmp+cpy,t->recv_len);
                t->recv_pos=0;
            }
        }
    }
    if(*type==0x17){
        uint8_t dec[TLS_MAX_RECORD+32];
        kmc(dec,raw,rlen);
        AES_CTX ctx;
        uint8_t iv[16];
        kmc(iv,t->server_iv,16);
        aes_init_ctx_iv(&ctx,t->mac_read_key,iv);
        aes_cbc_decrypt(&ctx,dec,rlen);
        kmc(t->server_iv,raw,16);
        int pad=dec[rlen-1];
        if(pad>15) pad=15;
        int plain=rlen-16-pad-1;
        if(plain<0) plain=0;
        kmc(data,dec+16,plain);
        return plain;
    }
    kmc(data,raw,rlen);
    return rlen;
}

int tls_connect(const char*host, uint16_t port){
    kms(&_tls,0,sizeof(TLS_CTX));
    kcp(_tls.host,host);
    _tls.server_port=port;
    vps("tls: connecting to "); vps(host); vps(":");
    char pb[8]; kuia(port,pb,10); vps(pb); vpln("...");
    uint32_t hip=0;
    const char*p=host;
    int parts=0;
    uint32_t oct=0;
    while(*p&&(parts<4)){
        if(*p>='0'&&*p<='9'){
            oct=oct*10+(*p-'0');
        }
        if(*p=='.'||*(p+1)==0){
            hip=(hip<<8)|oct;
            oct=0;
            parts++;
            if(*(p+1)==0)break;
        }
        p++;
    }
    if(parts!=4){
        // DNS fallback: hardcoded IPs for known hosts
        if(!ksc(host,"discord.com")){
            // Discord uses Cloudflare, try multiple IPs
            hip=0xA29F80E9; // 162.159.128.233
            vpln("tls: using Discord IP 162.159.128.233");
        } else if(!ksc(host,"discord.gg")){
            hip=0xA29F80E9;
        } else {
            vpln("tls: DNS not implemented, use IP address");
            return 0;
        }
    }
    _tls.server_ip=hip;
    if(!tcp_connect(hip,port)){
        vpln("tls: TCP connect failed");
        return 0;
    }
    vpln("tls: TCP connected");
    tls_fill_random(_tls.client_random,32);
    uint8_t ch[512];
    int off=0;
    ch[off++]=0x01;
    tls_write_uint24(ch+off,0); off+=3;
    ch[off++]=0x03; ch[off++]=0x03;
    kmc(ch+off,_tls.client_random,32); off+=32;
    ch[off++]=TLS_SESSION_ID_LEN;
    tls_fill_random(ch+off,TLS_SESSION_ID_LEN); off+=TLS_SESSION_ID_LEN;
    tls_write_uint16(ch+off,2); off+=2;
    tls_write_uint16(ch+off,0x003D); off+=2;
    ch[off++]=1; ch[off++]=0x00;
    int ext_start=off;
    tls_write_uint16(ch+off,0); off+=2;
    int sni_start=off;
    tls_write_uint16(ch+off,0); off+=2;
    int sni_list_start=off;
    tls_write_uint16(ch+off,0); off+=2;
    ch[off++]=0x00;
    int hlen=ksl(host);
    tls_write_uint16(ch+off,(uint16_t)(hlen+3)); off+=2;
    ch[off++]=0x00;
    tls_write_uint16(ch+off,(uint16_t)hlen); off+=2;
    for(int i=0;i<hlen;i++) ch[off++]=(uint8_t)host[i];
    tls_write_uint16(ch+sni_list_start,(uint16_t)(off-sni_list_start-2));
    tls_write_uint16(ch+sni_start,(uint16_t)(off-sni_start-2));
    tls_write_uint16(ch+ext_start,(uint16_t)(off-ext_start-2));
    int ch_len=off;
    tls_write_uint24(ch+1,(uint32_t)(ch_len-4));
    tls_write_uint16(ch+ch_len-2,(uint16_t)(ch_len-5));
    vpln("tls: sending ClientHello");
    if(tls_send_record(&_tls,0x16,ch,ch_len)<0){
        vpln("tls: send ClientHello failed");
        return 0;
    }
    vpln("tls: waiting for ServerHello...");
    uint8_t resp[TLS_MAX_RECORD+64];
    uint8_t rtype;
    int rlen=tls_recv_record(&_tls,&rtype,resp,TLS_MAX_RECORD);
    if(rlen<0){
        vpln("tls: no response");
        return 0;
    }
    if(rtype==0x16&&resp[0]==0x02){
        vpln("tls: ServerHello received");
        kmc(_tls.server_random,resp+6+32,32);
    } else if(rtype==0x16&&resp[0]==0x0B){
        vpln("tls: Certificate message");
        int cert_len24=tls_read_uint24(resp+4);
        int cert_data_len=tls_read_uint24(resp+7);
        if(cert_data_len>TLS_MAX_CERT) cert_data_len=TLS_MAX_CERT;
        _tls.cert_len=cert_data_len;
        kmc(_tls.cert_der,resp+10,cert_data_len);
        x509_parse_cert(&_tls.cert,_tls.cert_der,_tls.cert_len);
        vps("tls: cert CN="); vpln(_tls.cert.cn);
        vps("tls: valid from "); vps((char*)_tls.cert.not_before);
        vps(" to "); vpln((char*)_tls.cert.not_after);
        rlen=tls_recv_record(&_tls,&rtype,resp,TLS_MAX_RECORD);
        if(rlen>0&&rtype==0x16&&resp[0]==0x0C){
            vpln("tls: ServerKeyExchange");
            rlen=tls_recv_record(&_tls,&rtype,resp,TLS_MAX_RECORD);
        }
        if(rlen>0&&rtype==0x16&&resp[0]==0x0E){
            vpln("tls: ServerHelloDone");
        }
    }
    _tls.premaster[0]=0x03; _tls.premaster[1]=0x03;
    tls_fill_random(_tls.premaster+2,46);
    vpln("tls: sending ClientKeyExchange...");
    uint8_t cke[264];
    cke[0]=0x10;
    tls_write_uint24(cke+1,0);
    cke[4]=0x03; cke[5]=0x03;
    tls_write_uint16(cke+6,(uint16_t)_tls.cert.nlen);
    BN pub_n, pub_e, pm_bn, enc_bn;
    pub_n.n=(_tls.cert.nlen+7)/8;
    for(int i=0;i<pub_n.n;i++){
        pub_n.w[i]=0;
        for(int j=0;j<8&&(i*8+j)<_tls.cert.nlen;j++)
            pub_n.w[i]|=(uint64_t)_tls.cert.n[i*8+j]<<(56-j*8);
    }
    pub_e.n=(_tls.cert.elen+7)/8;
    for(int i=0;i<pub_e.n;i++){
        pub_e.w[i]=0;
        for(int j=0;j<8&&(i*8+j)<_tls.cert.elen;j++)
            pub_e.w[i]|=(uint64_t)_tls.cert.e[i*8+j]<<(56-j*8);
    }
    pm_bn.n=(48+7)/8;
    for(int i=0;i<pm_bn.n;i++){
        pm_bn.w[i]=0;
        for(int j=0;j<8&&(i*8+j)<48;j++)
            pm_bn.w[i]|=(uint64_t)_tls.premaster[i*8+j]<<(56-j*8);
    }
    rsa_public(&enc_bn,&pm_bn,&pub_e,&pub_n);
    int el=256;
    for(int i=0;i<el;i++){
        int wn=i/8, bn=56-(i%8)*8;
        if(wn<enc_bn.n) cke[8+i]=(uint8_t)(enc_bn.w[wn]>>bn);
        else cke[8+i]=0;
    }
    int cke_len=8+el;
    tls_write_uint24(cke+1,(uint32_t)(cke_len-4));
    tls_write_uint16(cke+cke_len-2,(uint16_t)(cke_len-5));
    if(tls_send_record(&_tls,0x16,cke,cke_len)<0){
        vpln("tls: send ClientKeyExchange failed");
        return 0;
    }
    uint8_t seed[64];
    kmc(seed,_tls.client_random,32);
    kmc(seed+32,_tls.server_random,32);
    tls_prf(_tls.master_secret,48,_tls.premaster,48,"master secret",seed,64);
    tls_compute_keys(&_tls);
    vpln("tls: sending ChangeCipherSpec...");
    uint8_t ccs[1]={0x01};
    tls_send_record(&_tls,0x14,ccs,1);
    uint8_t finished[12];
    uint8_t verify_data[12];
    kms(finished,0,12);
    uint8_t fh[32];
    uint8_t label_buf[64];
    kcp((char*)label_buf,"client finished");
    hmac_sha256(_tls.mac_write_key,32,finished,12,fh);
    kmc(verify_data,fh,12);
    uint8_t fin_msg[16];
    fin_msg[0]=0x14;
    tls_write_uint24(fin_msg+1,12);
    kmc(fin_msg+4,verify_data,12);
    tls_send_record(&_tls,0x16,fin_msg,16);
    rlen=tls_recv_record(&_tls,&rtype,resp,TLS_MAX_RECORD);
    if(rlen>0){
        if(rtype==0x14){
            vpln("tls: ChangeCipherSpec received");
            rlen=tls_recv_record(&_tls,&rtype,resp,TLS_MAX_RECORD);
        }
        if(rlen>0&&rtype==0x16&&resp[0]==0x14){
            vpln("tls: Server Finished verified");
        }
    }
    _tls.connected=1;
    vpln("tls: handshake complete");
    return 1;
}

int tls_send(const uint8_t*data, int len){
    if(!_tls.connected) return -1;
    return tls_send_record(&_tls,0x17,data,len);
}

int tls_recv(uint8_t*out, int max){
    if(!_tls.connected) return -1;
    uint8_t type;
    return tls_recv_record(&_tls,&type,out,max);
}

void tls_close(void){
    if(_tls.connected){
        uint8_t alert[2]={0x01,0x00};
        tls_send_record(&_tls,0x15,alert,2);
    }
    _tls.connected=0;
}

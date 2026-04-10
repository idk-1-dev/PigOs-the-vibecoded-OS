#pragma once
// pigOS SHA-256 + HMAC-SHA256 - from github.com/amosnier/sha-2 (public domain)
#include "../kernel/mem.h"
#include <stdint.h>

#define SHA256_CHUNK 64
#define SHA256_HASH_SIZE 32

typedef struct{
    uint8_t *hash;
    uint8_t chunk[SHA256_CHUNK];
    uint8_t *chunk_pos;
    size_t space_left;
    uint64_t total_len;
    uint32_t h[8];
} Sha256;

static inline uint32_t sha256_rr(uint32_t v,unsigned int c){
    return v>>c|v<<(32-c);
}

static inline void sha256_consume(uint32_t*h,const uint8_t*p){
    unsigned i,j;
    uint32_t ah[8],w[16];
    for(i=0;i<8;i++) ah[i]=h[i];
    static const uint32_t k[]={
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    for(i=0;i<4;i++){
        for(j=0;j<16;j++){
            if(i==0){
                w[j]=(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|(uint32_t)p[3];
                p+=4;
            } else {
                uint32_t s0=sha256_rr(w[(j+1)&0xf],7)^sha256_rr(w[(j+1)&0xf],18)^(w[(j+1)&0xf]>>3);
                uint32_t s1=sha256_rr(w[(j+14)&0xf],17)^sha256_rr(w[(j+14)&0xf],19)^(w[(j+14)&0xf]>>10);
                w[j]=w[j]+s0+w[(j+9)&0xf]+s1;
            }
            uint32_t s1=sha256_rr(ah[4],6)^sha256_rr(ah[4],11)^sha256_rr(ah[4],25);
            uint32_t ch=(ah[4]&ah[5])^(~ah[4]&ah[6]);
            uint32_t t1=ah[7]+s1+ch+k[i<<4|j]+w[j];
            uint32_t s0=sha256_rr(ah[0],2)^sha256_rr(ah[0],13)^sha256_rr(ah[0],22);
            uint32_t maj=(ah[0]&ah[1])^(ah[0]&ah[2])^(ah[1]&ah[2]);
            uint32_t t2=s0+maj;
            ah[7]=ah[6];ah[6]=ah[5];ah[5]=ah[4];ah[4]=ah[3]+t1;
            ah[3]=ah[2];ah[2]=ah[1];ah[1]=ah[0];ah[0]=t1+t2;
        }
    }
    for(i=0;i<8;i++) h[i]+=ah[i];
}

static void sha256_init(Sha256*s,uint8_t hash[SHA256_HASH_SIZE]){
    s->hash=hash;s->chunk_pos=s->chunk;s->space_left=SHA256_CHUNK;s->total_len=0;
    s->h[0]=0x6a09e667;s->h[1]=0xbb67ae85;s->h[2]=0x3c6ef372;s->h[3]=0xa54ff53a;
    s->h[4]=0x510e527f;s->h[5]=0x9b05688c;s->h[6]=0x1f83d9ab;s->h[7]=0x5be0cd19;
}

static void sha256_write(Sha256*s,const void*data,size_t len){
    s->total_len+=len;
    const uint8_t*p=(const uint8_t*)data;
    while(len>0){
        if(s->space_left==SHA256_CHUNK&&len>=SHA256_CHUNK){
            sha256_consume(s->h,p);len-=SHA256_CHUNK;p+=SHA256_CHUNK;continue;
        }
        size_t c=len<s->space_left?len:s->space_left;
        kmc(s->chunk_pos,p,c);s->space_left-=c;len-=c;p+=c;
        if(s->space_left==0){sha256_consume(s->h,s->chunk);s->chunk_pos=s->chunk;s->space_left=SHA256_CHUNK;}
        else s->chunk_pos+=c;
    }
}

static uint8_t*sha256_close(Sha256*s){
    uint8_t*pos=s->chunk_pos;size_t sl=s->space_left;uint32_t*h=s->h;
    *pos++=0x80;--sl;
    if(sl<8){
        kms(pos,0,sl);sha256_consume(h,s->chunk);pos=s->chunk;sl=SHA256_CHUNK;
    }
    size_t left=sl-8;kms(pos,0,left);pos+=left;
    uint64_t len=s->total_len;
    pos[7]=(uint8_t)(len<<3);len>>=5;
    for(int i=6;i>=0;--i){pos[i]=(uint8_t)len;len>>=8;}
    sha256_consume(h,s->chunk);
    for(int i=0,j=0;i<8;i++){
        s->hash[j++]=(uint8_t)(h[i]>>24);s->hash[j++]=(uint8_t)(h[i]>>16);
        s->hash[j++]=(uint8_t)(h[i]>>8);s->hash[j++]=(uint8_t)h[i];
    }
    return s->hash;
}

static void calc_sha256(uint8_t hash[SHA256_HASH_SIZE],const void*input,size_t len){
    Sha256 s;sha256_init(&s,hash);sha256_write(&s,input,len);sha256_close(&s);
}

// HMAC-SHA256
static void hmac_sha256(const uint8_t*key,uint32_t klen,const uint8_t*msg,uint32_t mlen,uint8_t*out){
    uint8_t k_pad[64]={0};
    if(klen>64){calc_sha256(k_pad,key,klen);}
    else{kmc(k_pad,key,klen);}
    uint8_t i_pad[64],o_pad[64];
    for(int i=0;i<64;i++){i_pad[i]=k_pad[i]^0x36;o_pad[i]=k_pad[i]^0x5c;}
    uint8_t inner[64+mlen];kmc(inner,i_pad,64);kmc(inner+64,msg,mlen);
    uint8_t ih[32];calc_sha256(ih,inner,64+mlen);
    uint8_t outer[96];kmc(outer,o_pad,64);kmc(outer+64,ih,32);
    calc_sha256(out,outer,96);
}

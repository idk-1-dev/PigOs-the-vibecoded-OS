#pragma once
// pigOS RSA - PKCS#1 v1.5 encryption/signing
// Uses bignum.h for big integer math
#include "bignum.h"
#include "sha256.h"
#include "../kernel/mem.h"
#include <stdint.h>

#define RSA_KEY_BITS 2048
#define RSA_KEY_BYTES (RSA_KEY_BITS/8)
#define RSA_MAX_BYTES (RSA_KEY_BYTES+16)

typedef struct{
    BN n,e,d,p,q,dp,dq,qinv;
}RSA_KEY;

// RSA public key operation: c = m^e mod n
static void rsa_public(BN*out,const BN*in,const BN*e,const BN*n){
    bn_powmod(out,in,e,n);
}

// RSA private key operation with CRT optimization: m = c^d mod n
static void rsa_private_crt(BN*out,const BN*c,const RSA_KEY*key){
    BN m1,m2,h;
    bn_powmod(&m1,c,&key->dp,&key->p);
    bn_powmod(&m2,c,&key->dq,&key->q);
    bn_sub(&h,&m1,&m2);
    if(bn_cmp(&h,bn_set_u64(&(BN){0},0))<0)bn_add(&h,&h,&key->p);
    bn_mulmod(&h,&h,&key->qinv,&key->p);
    bn_mul(&h,&h,&key->q);
    bn_add(out,&h,&m2);
}

// PKCS#1 v1.5 padding for encryption (type 2)
static int rsa_pkcs1_encrypt_pad(uint8_t*out,size_t*out_len,const uint8_t*msg,size_t msg_len,const BN*e,const BN*n){
    if(msg_len>RSA_KEY_BYTES-11)return -1;
    uint8_t em[RSA_KEY_BYTES];
    em[0]=0x00;em[1]=0x02;
    // Random non-zero padding
    for(size_t i=2;i<RSA_KEY_BYTES-msg_len-1;i++){
        uint64_t s=0xdeadbeef+i*0x12345678;
        s^=s<<13;s^=s>>17;s^=s<<5;
        em[i]=(uint8_t)((s>>((i%8)*8))&0xff);
        if(em[i]==0)em[i]=1;
    }
    em[RSA_KEY_BYTES-msg_len-1]=0x00;
    kmc(em+RSA_KEY_BYTES-msg_len,msg,msg_len);
    // Convert to BN and encrypt
    BN m,c;
    m.n=(RSA_KEY_BYTES+7)/8;
    for(int i=0;i<m.n;i++){
        m.w[i]=0;
        for(int j=0;j<8&&(i*8+j)<RSA_KEY_BYTES;j++){
            m.w[i]|=(uint64_t)em[i*8+j]<<(56-j*8);
        }
    }
    rsa_public(&c,&m,e,n);
    // Convert back to bytes
    for(int i=0;i<(c.n+7)/8&&i<(int)*out_len;i++){
        for(int j=0;j<8;j++){
            int idx=i*8+j;
            if(idx>=(int)*out_len)break;
            out[idx]=(uint8_t)(c.w[i]>>(56-j*8));
        }
    }
    *out_len=RSA_KEY_BYTES;
    return 0;
}

// PKCS#1 v1.5 decryption (type 2)
static int rsa_pkcs1_decrypt(uint8_t*out,size_t*out_len,const uint8_t*ct,size_t ct_len,const RSA_KEY*key){
    if(ct_len!=RSA_KEY_BYTES)return -1;
    // Convert ciphertext to BN
    BN c;
    c.n=(ct_len+7)/8;
    for(int i=0;i<c.n;i++){
        c.w[i]=0;
        for(int j=0;j<8&&(i*8+j)<(int)ct_len;j++){
            c.w[i]|=(uint64_t)ct[i*8+j]<<(56-j*8);
        }
    }
    // Decrypt
    BN m;rsa_private_crt(&m,&c,key);
    // Convert back to bytes
    uint8_t em[RSA_KEY_BYTES];
    for(int i=0;i<RSA_KEY_BYTES;i++){
        int wn=i/8,wn2=(i/8);
        int bn=56-(i%8)*8;
        if(wn2<c.n)em[i]=(uint8_t)(m.w[wn2]>>bn);
        else em[i]=0;
    }
    // Unpad
    if(em[0]!=0x00||em[1]!=0x02)return -1;
    size_t ps=2;
    while(ps<RSA_KEY_BYTES-1&&em[ps]!=0x00)ps++;
    if(ps>=RSA_KEY_BYTES-1)return -1;
    size_t ml=RSA_KEY_BYTES-1-ps;
    kmc(out,em+ps,ml);
    *out_len=ml;
    return 0;
}

// Generate RSA key pair
static void rsa_gen_key(RSA_KEY*key){
    // Generate two primes p and q
    bn_gen_prime(&key->p,RSA_KEY_BITS/2);
    for(;;){
        bn_gen_prime(&key->q,RSA_KEY_BITS/2);
        if(bn_cmp(&key->p,&key->q)!=0)break;
    }
    // n = p*q
    bn_mul(&key->n,&key->p,&key->q);
    // e = 65537
    bn_set_u64(&key->e,65537);
    // d = e^-1 mod (p-1)(q-1)
    BN pm1,qm1,phi;
    bn_sub(&pm1,&key->p,bn_set_u64(&(BN){0},1));
    bn_sub(&qm1,&key->q,bn_set_u64(&(BN){0},1));
    bn_mul(&phi,&pm1,&qm1);
    // d = e^-1 mod phi using extended GCD
    // Use the modinv function
    if(!bn_modinv(&key->d,&key->e,&phi)){
        // Fallback: regenerate
        rsa_gen_key(key);
        return;
    }
    // CRT parameters
    bn_sub(&key->dp,&key->d,&pm1);bn_mod(&key->dp,&key->dp,&pm1);
    bn_sub(&key->dq,&key->d,&qm1);bn_mod(&key->dq,&key->dq,&qm1);
    // qinv = q^-1 mod p
    bn_modinv(&key->qinv,&key->q,&key->p);
}

// RSA signature: sign = (hash)^d mod n
static void rsa_sign(uint8_t*sig,const uint8_t*hash,uint32_t hash_len,const RSA_KEY*key){
    // Build DigestInfo for SHA-256: 30 31 30 0d 06 09 60 86 48 01 65 03 04 02 01 05 00 04 20 || hash
    uint8_t di[51];
    di[0]=0x30;di[1]=0x31;di[2]=0x30;di[3]=0x0d;di[4]=0x06;di[5]=0x09;
    di[6]=0x60;di[7]=0x86;di[8]=0x48;di[9]=0x01;di[10]=0x65;di[11]=0x03;
    di[12]=0x04;di[13]=0x02;di[14]=0x01;di[15]=0x05;di[16]=0x00;
    di[17]=0x04;di[18]=0x20;
    kmc(di+19,hash,32);
    // PKCS#1 v1.5 signature padding
    uint8_t em[RSA_KEY_BYTES];
    em[0]=0x00;em[1]=0x01;
    for(int i=2;i<RSA_KEY_BYTES-38-19;i++)em[i]=0xff;
    em[RSA_KEY_BYTES-38-19]=0x00;
    kmc(em+RSA_KEY_BYTES-38,di,38);
    // Convert to BN and sign
    BN m,s;
    m.n=(RSA_KEY_BYTES+7)/8;
    for(int i=0;i<m.n;i++){
        m.w[i]=0;
        for(int j=0;j<8&&(i*8+j)<RSA_KEY_BYTES;j++)
            m.w[i]|=(uint64_t)em[i*8+j]<<(56-j*8);
    }
    rsa_private_crt(&s,&m,key);
    // Convert back to bytes
    for(int i=0;i<RSA_KEY_BYTES;i++){
        int wn=i/8,bn=56-(i%8)*8;
        sig[i]=wn<s.n?(uint8_t)(s.w[wn]>>bn):0;
    }
}

// RSA signature verification
static int rsa_verify(const uint8_t*sig,const uint8_t*hash,uint32_t hash_len,const BN*e,const BN*n){
    // Convert sig to BN
    BN s,m;
    s.n=(RSA_KEY_BYTES+7)/8;
    for(int i=0;i<s.n;i++){
        s.w[i]=0;
        for(int j=0;j<8&&(i*8+j)<RSA_KEY_BYTES;j++)
            s.w[i]|=(uint64_t)sig[i*8+j]<<(56-j*8);
    }
    rsa_public(&m,&s,e,n);
    // Convert back to bytes
    uint8_t em[RSA_KEY_BYTES];
    for(int i=0;i<RSA_KEY_BYTES;i++){
        int wn=i/8,bn=56-(i%8)*8;
        em[i]=wn<m.n?(uint8_t)(m.w[wn]>>bn):0;
    }
    // Check padding
    if(em[0]!=0x00||em[1]!=0x01)return 0;
    int i=2;
    while(i<RSA_KEY_BYTES&&em[i]==0xff)i++;
    if(i<11||em[i]!=0x00)return 0;
    // Check DigestInfo
    uint8_t di[19]={0x30,0x31,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01,0x05,0x00,0x04,0x20};
    int di_start=RSA_KEY_BYTES-38-19;
    if(i!=di_start)return 0;
    for(int j=0;j<19;j++)if(em[di_start+j]!=di[j])return 0;
    // Check hash
    for(int j=0;j<32;j++)if(em[di_start+19+j]!=hash[j])return 0;
    return 1;
}

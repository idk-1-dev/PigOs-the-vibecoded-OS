#pragma once
// pigOS Big Integer math - for RSA
// Freestanding, no libc
#include "../kernel/mem.h"
#include <stdint.h>

#define BN_MAX_WORDS 64
#define BN_BITS (BN_MAX_WORDS*64)

typedef struct{uint64_t w[BN_MAX_WORDS];int n;}BN;

static void bn_zero(BN*a){a->n=1;a->w[0]=0;}
static void bn_one(BN*a){a->n=1;a->w[0]=1;}
static BN*bn_set_u64(BN*a,uint64_t v){a->w[0]=v;a->n=1;return a;}
static int bn_is_zero(const BN*a){return a->n==1&&a->w[0]==0;}
static int bn_is_one(const BN*a){return a->n==1&&a->w[0]==1;}
static int bn_cmp(const BN*a,const BN*b){
    int n=a->n>b->n?a->n:b->n;
    for(int i=n-1;i>=0;i--){
        uint64_t av=i<a->n?a->w[i]:0,bv=i<b->n?b->w[i]:0;
        if(av!=bv)return av>bv?1:-1;
    }
    return 0;
}
static void bn_copy(BN*a,const BN*b){a->n=b->n;for(int i=0;i<b->n;i++)a->w[i]=b->w[i];}
static void bn_add(BN*r,const BN*a,const BN*b){
    uint64_t c=0;int n=a->n>b->n?a->n:b->n;
    for(int i=0;i<n;i++){
        uint64_t av=i<a->n?a->w[i]:0,bv=i<b->n?b->w[i]:0;
        __uint128_t s=(__uint128_t)av+bv+c;r->w[i]=(uint64_t)s;c=(uint64_t)(s>>64);
    }
    if(c)r->w[n++]=c;
    r->n=n;
}
static void bn_sub(BN*r,const BN*a,const BN*b){
    uint64_t c=0;int n=a->n;
    for(int i=0;i<n;i++){
        uint64_t av=a->w[i],bv=i<b->n?b->w[i]:0;
        __int128_t s=(__int128_t)av-bv-c;r->w[i]=(uint64_t)s;c=s<0?1:0;
    }
    while(n>1&&r->w[n-1]==0)n--;r->n=n;
}
static void bn_mul(BN*r,const BN*a,const BN*b){
    int rn=a->n+b->n;
    for(int i=0;i<rn;i++)r->w[i]=0;
    for(int i=0;i<a->n;i++){
        uint64_t c=0;
        for(int j=0;j<b->n;j++){
            __uint128_t p=(__uint128_t)a->w[i]*b->w[j]+r->w[i+j]+c;
            r->w[i+j]=(uint64_t)p;c=(uint64_t)(p>>64);
        }
        if(c)r->w[i+b->n]+=c;
    }
    while(rn>1&&r->w[rn-1]==0)rn--;r->n=rn;
}
static void bn_mod(BN*r,const BN*a,const BN*m){
    if(bn_cmp(a,m)<0){bn_copy(r,a);return;}
    BN q;bn_zero(&q);bn_copy(r,a);
    for(int i=a->n-1;i>=0;i--){
        for(int j=63;j>=0;j--){
            bn_add(&q,r,r);
            if(bn_cmp(&q,m)>=0)bn_sub(&q,&q,m);
        }
    }
    bn_copy(r,&q);
}
static void bn_mulmod(BN*r,const BN*a,const BN*b,const BN*m){
    BN t;bn_mul(&t,a,b);bn_mod(r,&t,m);
}
static void bn_powmod(BN*r,const BN*base,const BN*exp,const BN*m){
    BN res,cur,b,e;bn_copy(&b,base);bn_copy(&e,exp);bn_one(&res);
    while(!bn_is_zero(&e)){
        if(e.w[0]&1){bn_mulmod(&res,&res,&b,m);bn_copy(&res,&res);}
        bn_mulmod(&cur,&b,&b,m);bn_copy(&b,&cur);
        for(int i=0;i<e.n-1;i++)e.w[i]=(e.w[i]>>1)|(e.w[i+1]<<63);
        e.w[e.n-1]>>=1;while(e.n>1&&e.w[e.n-1]==0)e.n--;
    }
    bn_copy(r,&res);
}
static int bn_is_prime(const BN*n,int rounds){
    BN two;bn_set_u64(&two,2);
    if(bn_cmp(n,&two)<0)return 0;
    if(bn_cmp(n,&two)==0)return 1;
    if(n->w[0]%2==0)return 0;
    BN d,s_d;bn_sub(&d,n,&two);
    int s=0;bn_copy(&s_d,&d);
    while((s_d.w[0]&1)==0){
        for(int i=0;i<s_d.n-1;i++)s_d.w[i]=(s_d.w[i]>>1)|(s_d.w[i+1]<<63);
        s_d.w[s_d.n-1]>>=1;while(s_d.n>1&&s_d.w[s_d.n-1]==0)s_d.n--;s++;
    }
    for(int r=0;r<rounds;r++){
        BN a;bn_set_u64(&a,2+r);
        if(bn_cmp(&a,n)>=0)break;
        BN x;bn_powmod(&x,&a,&s_d,n);
        BN nm1;bn_sub(&nm1,n,&two);
        if(bn_is_one(&x)||bn_cmp(&x,&nm1)==0)continue;
        int composite=1;
        for(int j=1;j<s;j++){
            BN two_bn;bn_set_u64(&two_bn,2);
            bn_powmod(&x,&x,&two_bn,n);
            if(bn_cmp(&x,&nm1)==0){composite=0;break;}
            if(bn_is_one(&x)){composite=1;goto next_round;}
        }
        if(composite)return 0;
        next_round:;
    }
    return 1;
}
static void bn_gen_prime(BN*p,int bits){
    int words=(bits+63)/64;
    for(;;){
        p->n=words;
        for(int i=0;i<words;i++)p->w[i]=0;
        p->w[words-1]|=(uint64_t)1<<(bits-1-(words-1)*64);
        p->w[0]|=1;
        for(int i=1;i<words-1;i++){
            uint64_t s=0xdeadbeef+i*0x12345678;
            s^=s<<13;s^=s>>17;s^=s<<5;
            p->w[i]=s;
        }
        if(bn_is_prime(p,8))return;
    }
}
// Modular inverse using extended Euclidean algorithm
static int bn_modinv(BN*r,const BN*a,const BN*m){
    BN t0,t1,q,tmp,r0,r1;
    bn_copy(&r0,m);bn_copy(&r1,a);
    bn_zero(&t0);bn_one(&t1);
    while(!bn_is_zero(&r1)){
        bn_zero(&q);
        if(r1.n==1&&r1.w[0]>0){
            uint64_t qv=0;
            for(int i=r0.n-1;i>=0;i--){
                __uint128_t rem=((__uint128_t)(i==r0.n-1?0:r0.w[i+1])<<64)|r0.w[i];
                qv=rem/r1.w[0];
                if(i<BN_MAX_WORDS)q.w[i]=qv;
            }
            q.n=r0.n;
            BN qt;bn_mul(&qt,&q,&r1);
            bn_sub(&tmp,&r0,&qt);
            bn_copy(&r0,&r1);bn_copy(&r1,&tmp);
            BN qt2;bn_mul(&qt2,&q,&t1);
            bn_sub(&tmp,&t0,&qt2);
            bn_copy(&t0,&t1);bn_copy(&t1,&tmp);
        } else {
            bn_mod(&tmp,&r0,&r1);
            bn_copy(&r0,&r1);bn_copy(&r1,&tmp);
            bn_copy(&tmp,&t0);bn_copy(&t0,&t1);
            bn_zero(&t1);
        }
    }
    BN one,z;bn_one(&one);bn_set_u64(&z,0);
    if(bn_cmp(&r0,&one)!=0)return 0;
    if(bn_cmp(&t0,&z)<0){bn_add(&t0,&t0,m);}
    bn_copy(r,&t0);
    return 1;
}
static int bn_modinv_fermat(BN*r,const BN*a,const BN*m){
    BN e,two;bn_set_u64(&two,2);bn_sub(&e,m,&two);
    bn_powmod(r,a,&e,m);
    return 1;
}

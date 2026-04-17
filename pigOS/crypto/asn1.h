#pragma once
// pigOS ASN.1/DER Parser - Minimal X.509 certificate support
// Freestanding, no libc
#include "../kernel/mem.h"
#include <stdint.h>

#define ASN1_MAX_DEPTH 16
#define ASN1_MAX_NODES 256

typedef struct{
    uint8_t tag;
    uint32_t len;
    const uint8_t* val;
    int children;
    int child_start;
} Asn1Node;

typedef struct{
    Asn1Node nodes[ASN1_MAX_NODES];
    int nnodes;
    const uint8_t* data;
    uint32_t dlen;
} Asn1Tree;

typedef struct{
    uint8_t n[256];
    int nlen;
    uint8_t e[8];
    int elen;
    char cn[128];
    uint8_t not_before[32];
    uint8_t not_after[32];
} X509Cert;

static uint32_t asn1_parse_len(const uint8_t*p, uint32_t max, uint32_t* consumed){
    if(max<1){*consumed=0; return 0;}
    uint8_t b=*p;
    if(b<0x80){*consumed=1; return b;}
    uint8_t nb=b&0x7F;
    if(nb>4||max<1+nb){*consumed=0; return 0;}
    uint32_t len=0;
    for(uint8_t i=0;i<nb;i++) len=(len<<8)|p[1+i];
    *consumed=1+nb;
    return len;
}

static int asn1_parse_node(Asn1Tree*t, const uint8_t*p, uint32_t max, int depth){
    if(t->nnodes>=ASN1_MAX_NODES||depth>=ASN1_MAX_DEPTH||max<2) return -1;
    int idx=t->nnodes;
    Asn1Node*nd=&t->nodes[idx];
    t->nnodes++;
    nd->tag=p[0];
    uint32_t lc=0;
    nd->len=asn1_parse_len(p+1,max-1,&lc);
    nd->val=p+1+lc;
    nd->children=0;
    nd->child_start=0;
    uint32_t total=1+lc+nd->len;
    if(total>max) return -1;
    if((nd->tag&0x20)&&nd->len>0){
        const uint8_t*cp=nd->val;
        uint32_t rem=nd->len;
        nd->child_start=t->nnodes;
        while(rem>0){
            int ci=asn1_parse_node(t,cp,rem,depth+1);
            if(ci<0) break;
            nd->children++;
            uint32_t clen=t->nodes[ci].len;
            uint32_t ct=t->nodes[ci].tag;
            uint32_t clc=0;
            if(rem>=2) asn1_parse_len(cp+1,rem-1,&clc);
            uint32_t ctot=1+clc+clen;
            if(ctot==0) break;
            cp+=ctot;
            rem-=ctot;
            if(rem==0) break;
        }
    }
    return idx;
}

static int asn1_parse(Asn1Tree*t, const uint8_t*data, uint32_t len){
    t->nnodes=0;
    t->data=data;
    t->dlen=len;
    return asn1_parse_node(t,data,len,0);
}

static Asn1Node* asn1_child(Asn1Tree*t, Asn1Node*parent, int idx){
    if(idx<0||idx>=parent->children) return NULL;
    return &t->nodes[parent->child_start+idx];
}

static Asn1Node* asn1_find_seq(Asn1Tree*t, int start){
    for(int i=start;i<t->nnodes;i++){
        if(t->nodes[i].tag==0x30) return &t->nodes[i];
    }
    return NULL;
}

static Asn1Node* asn1_find_tag(Asn1Tree*t, Asn1Node*parent, uint8_t tag){
    for(int i=0;i<parent->children;i++){
        Asn1Node*c=asn1_child(t,parent,i);
        if(c&&c->tag==tag) return c;
    }
    return NULL;
}

static Asn1Node* asn1_find_tag_deep(Asn1Tree*t, Asn1Node*parent, uint8_t tag){
    for(int i=0;i<parent->children;i++){
        Asn1Node*c=asn1_child(t,parent,i);
        if(!c) continue;
        if(c->tag==tag) return c;
        Asn1Node*r=asn1_find_tag_deep(t,c,tag);
        if(r) return r;
    }
    return NULL;
}

static void asn1_extract_str(uint8_t*out, int maxout, Asn1Node*n){
    if(!n||n->len==0){out[0]=0; return;}
    int l=n->len<maxout-1?n->len:maxout-1;
    kmc(out,n->val,l);
    out[l]=0;
}

static int x509_parse_cert(X509Cert*cert, const uint8_t*der, uint32_t dlen){
    Asn1Tree t;
    kms(cert,0,sizeof(X509Cert));
    if(asn1_parse(&t,der,dlen)<0) return -1;
    Asn1Node*root=&t.nodes[0];
    if(root->tag!=0x30) return -1;
    Asn1Node*tbs=asn1_child(&t,root,0);
    if(!tbs||tbs->tag!=0x30) return -1;
    Asn1Node*ver=asn1_child(&t,tbs,0);
    int subject_off=5, validity_off=4, pubkey_off=6;
    if(ver&&ver->tag==0xA0){
        validity_off=5; subject_off=6; pubkey_off=7;
    } else {
        validity_off=4; subject_off=5; pubkey_off=6;
    }
    Asn1Node*subject=asn1_child(&t,tbs,subject_off);
    if(subject){
        for(int i=0;i<subject->children;i++){
            Asn1Node*rset=asn1_child(&t,subject,i);
            if(!rset) continue;
            Asn1Node*rseq=asn1_child(&t,rset,0);
            if(!rseq) continue;
            Asn1Node*oid=asn1_child(&t,rseq,0);
            Asn1Node*val=asn1_child(&t,rseq,1);
            if(oid&&oid->tag==0x06&&val){
                if(oid->len>=3&&oid->val[0]==0x55&&oid->val[1]==0x04&&oid->val[2]==0x03){
                    asn1_extract_str((uint8_t*)cert->cn,128,val);
                }
            }
        }
    }
    Asn1Node*validity=asn1_child(&t,tbs,validity_off);
    if(validity){
        Asn1Node*nb=asn1_child(&t,validity,0);
        Asn1Node*na=asn1_child(&t,validity,1);
        if(nb) asn1_extract_str(cert->not_before,32,nb);
        if(na) asn1_extract_str(cert->not_after,32,na);
    }
    Asn1Node*pubkey=asn1_child(&t,tbs,pubkey_off);
    if(pubkey){
        Asn1Node*algo=asn1_child(&t,pubkey,0);
        Asn1Node*bits=asn1_child(&t,pubkey,1);
        if(bits&&bits->tag==0x03){
            const uint8_t*bp=bits->val;
            if(bits->len>1) bp++;
            Asn1Tree t2;
            if(asn1_parse(&t2,bp,bits->len-1)>=0){
                Asn1Node*seq=&t2.nodes[0];
                if(seq->tag==0x30&&seq->children>=2){
                    Asn1Node*mod=asn1_child(&t2,seq,0);
                    Asn1Node*exp=asn1_child(&t2,seq,1);
                    if(mod&&mod->tag==0x02){
                        cert->nlen=mod->len<256?mod->len:256;
                        if(mod->len>0&&mod->val[0]==0x00){
                            cert->nlen=cert->nlen-1<256?cert->nlen-1:256;
                            kmc(cert->n,mod->val+1,cert->nlen);
                        } else {
                            kmc(cert->n,mod->val,cert->nlen);
                        }
                    }
                    if(exp&&exp->tag==0x02){
                        cert->elen=exp->len<8?exp->len:8;
                        kmc(cert->e,exp->val,cert->elen);
                    }
                }
            }
        }
    }
    return 0;
}

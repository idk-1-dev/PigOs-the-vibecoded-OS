#pragma once
// pigOS Minimal JSON Parser - NO floating point (SSE disabled in kernel)
// Freestanding, no libc
#include "../kernel/mem.h"
#include <stdint.h>

#define JSON_MAX_NODES 1024
#define JSON_MAX_STR 1024

typedef enum{
    JSON_NULL=0,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonType;

typedef struct JsonNode{
    JsonType type;
    char* key;
    char* str_val;
    int64_t num_val;
    int bool_val;
    int children;
    int child_start;
} JsonNode;

typedef struct{
    JsonNode nodes[JSON_MAX_NODES];
    int nnodes;
} JsonTree;

typedef struct{
    const char* src;
    int pos;
    int len;
} JsonParser;

static void json_skip_ws(JsonParser*p){
    while(p->pos<p->len){
        char c=p->src[p->pos];
        if(c==' '||c=='\t'||c=='\n'||c=='\r') p->pos++;
        else break;
    }
}

static int json_parse_value(JsonParser*p, JsonTree*t);

static char* json_parse_string_raw(JsonParser*p, int*out_len){
    if(p->pos>=p->len||p->src[p->pos]!='"') return NULL;
    p->pos++;
    int cap=64;
    char*buf=(char*)km(cap);
    int i=0;
    while(p->pos<p->len){
        char c=p->src[p->pos];
        if(c=='"'){
            p->pos++;
            buf[i]=0;
            if(out_len) *out_len=i;
            return buf;
        }
        if(c=='\\'){
            p->pos++;
            if(p->pos>=p->len) break;
            char ec=p->src[p->pos];
            if(i>=cap-2){cap*=2;char*nb=(char*)km(cap);kmc(nb,buf,i);buf=nb;}
            if(ec=='"') buf[i++]='"';
            else if(ec=='\\') buf[i++]='\\';
            else if(ec=='/') buf[i++]='/';
            else if(ec=='n') buf[i++]='\n';
            else if(ec=='t') buf[i++]='\t';
            else if(ec=='r') buf[i++]='\r';
            else if(ec=='b') buf[i++]='\b';
            else if(ec=='f') buf[i++]='\f';
            else buf[i++]=ec;
        } else {
            if(i>=cap-2){cap*=2;char*nb=(char*)km(cap);kmc(nb,buf,i);buf=nb;}
            buf[i++]=c;
        }
        p->pos++;
    }
    buf[i]=0;
    if(out_len) *out_len=i;
    return buf;
}

static int json_parse_value(JsonParser*p, JsonTree*t){
    if(t->nnodes>=JSON_MAX_NODES) return -1;
    int idx=t->nnodes;
    JsonNode*n=&t->nodes[idx];
    t->nnodes++;
    kms(n,0,sizeof(JsonNode));
    n->children=0;
    n->child_start=0;
    json_skip_ws(p);
    if(p->pos>=p->len) return -1;
    char c=p->src[p->pos];
    if(c=='"'){
        n->type=JSON_STRING;
        n->str_val=json_parse_string_raw(p,NULL);
    } else if(c=='{'){
        n->type=JSON_OBJECT;
        p->pos++;
        n->child_start=t->nnodes;
        json_skip_ws(p);
        if(p->pos<p->len&&p->src[p->pos]!='}'){
            while(1){
                json_skip_ws(p);
                if(p->pos>=p->len) break;
                if(p->src[p->pos]=='}') break;
                char*k=json_parse_string_raw(p,NULL);
                json_skip_ws(p);
                if(p->pos<p->len&&p->src[p->pos]==':') p->pos++;
                int ci=json_parse_value(p,t);
                if(ci>=0){
                    t->nodes[ci].key=k;
                    n->children++;
                }
                json_skip_ws(p);
                if(p->pos<p->len&&p->src[p->pos]==','){
                    p->pos++;
                    continue;
                }
                break;
            }
        }
        if(p->pos<p->len&&p->src[p->pos]=='}') p->pos++;
    } else if(c=='['){
        n->type=JSON_ARRAY;
        p->pos++;
        n->child_start=t->nnodes;
        json_skip_ws(p);
        if(p->pos<p->len&&p->src[p->pos]!=']'){
            while(1){
                int ci=json_parse_value(p,t);
                if(ci>=0) n->children++;
                json_skip_ws(p);
                if(p->pos<p->len&&p->src[p->pos]==','){
                    p->pos++;
                    continue;
                }
                break;
            }
        }
        if(p->pos<p->len&&p->src[p->pos]==']') p->pos++;
    } else if(c=='t'){
        n->type=JSON_BOOL; n->bool_val=1;
        if(p->pos+3<p->len&&p->src[p->pos+1]=='r'&&p->src[p->pos+2]=='u'&&p->src[p->pos+3]=='e')
            p->pos+=4;
    } else if(c=='f'){
        n->type=JSON_BOOL; n->bool_val=0;
        if(p->pos+4<p->len&&p->src[p->pos+1]=='a'&&p->src[p->pos+2]=='l'&&
           p->src[p->pos+3]=='s'&&p->src[p->pos+4]=='e')
            p->pos+=5;
    } else if(c=='n'){
        n->type=JSON_NULL;
        if(p->pos+3<p->len&&p->src[p->pos+1]=='u'&&p->src[p->pos+2]=='l'&&p->src[p->pos+3]=='l')
            p->pos+=4;
    } else if(c=='-'||(c>='0'&&c<='9')){
        n->type=JSON_NUMBER;
        int start=p->pos;
        while(p->pos<p->len){
            char cc=p->src[p->pos];
            if((cc>='0'&&cc<='9')||cc=='-'||cc=='+') p->pos++;
            else break;
        }
        int slen=p->pos-start;
        if(slen>0&&slen<24){
            char buf[32];
            kmc(buf,p->src+start,slen);
            buf[slen]=0;
            int64_t val=0;
            int neg=0,si=0;
            if(buf[si]=='-'){neg=1;si++;}
            if(buf[si]=='+') si++;
            while(si<slen&&buf[si]>='0'&&buf[si]<='9'){
                val=val*10+(buf[si]-'0');
                si++;
            }
            if(neg) val=-val;
            n->num_val=val;
        }
    }
    return idx;
}

static JsonTree* json_parse(const char*str, int len){
    JsonTree*t=(JsonTree*)km(sizeof(JsonTree));
    kms(t,0,sizeof(JsonTree));
    JsonParser p;
    p.src=str; p.pos=0; p.len=len;
    json_parse_value(&p,t);
    return t;
}

static JsonNode* json_find_key(JsonTree*t, JsonNode*obj, const char*key){
    if(!obj||obj->type!=JSON_OBJECT) return NULL;
    for(int i=0;i<obj->children;i++){
        JsonNode*c=&t->nodes[obj->child_start+i];
        if(c->key&&!ksc(c->key,key)) return c;
    }
    return NULL;
}

static const char* json_get_string(JsonTree*t, JsonNode*obj, const char*key){
    JsonNode*n=json_find_key(t,obj,key);
    if(n&&n->type==JSON_STRING) return n->str_val;
    return NULL;
}

static int64_t json_get_number(JsonTree*t, JsonNode*obj, const char*key){
    JsonNode*n=json_find_key(t,obj,key);
    if(n&&n->type==JSON_NUMBER) return n->num_val;
    return 0;
}

static int json_get_int(JsonTree*t, JsonNode*obj, const char*key){
    return (int)json_get_number(t,obj,key);
}

static JsonNode* json_get_array(JsonTree*t, JsonNode*obj, const char*key){
    return json_find_key(t,obj,key);
}

static JsonNode* json_get_object(JsonTree*t, JsonNode*obj, const char*key){
    JsonNode*n=json_find_key(t,obj,key);
    if(n&&n->type==JSON_OBJECT) return n;
    return NULL;
}

static JsonNode* json_array_get(JsonTree*t, JsonNode*arr, int idx){
    if(!arr||arr->type!=JSON_ARRAY||idx<0||idx>=arr->children) return NULL;
    return &t->nodes[arr->child_start+idx];
}

static int json_array_len(JsonTree*t, JsonNode*arr){
    if(!arr||arr->type!=JSON_ARRAY) return 0;
    return arr->children;
}

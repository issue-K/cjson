#include<assert.h>  /* assert() */
#include <stdlib.h> /* NULL,strtod() */
#include <math.h> /* HUGE_VAL*/
#include <errno.h> /* errno, ERANGE */
#include <string.h>
#include <stdio.h>
#include "cjson.h"

#define C_STACK_INIT_SIZE 256

#define PUTC(c, ch)     do { *(char*)base_json_push(c, sizeof(char)) = (ch); } while(0)

#define STRING_ERROR(state) do{ js->top = head; return state; }while(0)

typedef struct{
    const char* json;
    char* stack;
    size_t size, top; /* size表示stack分配的空间,top表示用掉的空间*/
}base_json;

/* 现在要新增size个字符, 于是给base_json扩容, 并返回第一个没被占用的位置 */
static void* base_json_push(base_json* c,size_t size){
    assert( size>0 );
    void* ref;
    if( c->top + size >= c->size ){
        if( c->size==0 )
            c->size = C_STACK_INIT_SIZE;
        while( c->top + size >= c->size ){
            c->size += c->size>>1;  /* 每次扩容为原来的1.5倍 */
        }
        c->stack = (char*)realloc(c->stack,c->size);
    }
    ref = c->stack+c->top;
    c->top += size;
    return ref;
}
static void* base_json_pop(base_json* js,size_t size){
    assert( js->top >= size );
    js->top -= size;
    return js->stack+js->top;
}

int is09(const char* w){
    return (*w)>='0' && (*w)<='9';
}

int is19(const char* w){
    return (*w)>='1' && (*w)<='9';
}



static void clear_whitespace(base_json* js){
    while( 1 )
    {
        char temp = *js->json;
        if( temp==' ' || temp=='\t' || temp=='\n' || temp=='\r' )
            js->json++;
        else    break;
    }
}

static int parse_const(base_json* js,c_value* v,const char* str,c_type type){
    size_t i;
    for(i=0; str[i] ;i++)
        if( js->json[i] != str[i] )
            return C_PARSE_INVALID_VALUE;
    js->json += i;
    v->type = type;
    return C_PARSE_OK;
}

static int parse_number(base_json* js,c_value* v){
    const char* p = js->json;
    if( (*p)=='-' ) p++;
    /* 消除整数部分 */
    if( !is19(p) ){
        if( (*p)!='0' ) return C_PARSE_INVALID_VALUE;
        p++;
    }else{
        while( is09((p)) ) p++;
    }
    /* 存在小数部分 */
    if( (*p)=='.' ){
        p++;
        if( !is09(p) )   return C_PARSE_INVALID_VALUE;
        while( is09(p) )    p++;
    }
    /* 存在指数部分 */
    if( (*p)=='e' || (*p)=='E' ){
        p++;
        if( (*p)=='-' || (*p)=='+' )    p++;
        if( !is09(p) )   return C_PARSE_INVALID_VALUE;
        while( is09(p)  )   p++;
    }
    errno = 0; /* 记录最后一次错误的错误码 */
    v->n = strtod( js->json,NULL );
    if( errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL ) )
        return C_PARSE_NUMBER_TOO_BIG;
    js->json = p;
    v->type = C_NUMBER;
    return C_PARSE_OK;
}
/* 16进制转十进制 */
static const char* parse_hex4(const char* p,unsigned* u){
    int i;
    *u = 0;
    for(int i=0;i<4;i++){
        char ch = *p++;
        *u <<= 4;
        if( ch>='0' && ch<='9' )    *u |= ch-'0';
        else if( ch>='a' && ch<='f' )   *u |= ch-'a'+10;
        else if( ch>='A' && ch<='F' )   *u |= ch-'A'+10;
        else    return NULL;
    }
    return p;
}
// 把数字u转化为utf8二进制的形式
static void encode_utf8(base_json* js,unsigned u){
    if( u<=0x7F )  /* 单字节数字 */
        PUTC(js,u & 0xFF );   // 和255与一下保证只有低8位有值
    else if( u<=0x7FF ){
        PUTC(js,0xC0 | (( u>>6 ) & 0xFF )  );  // 0xC0是第一个字节的110前缀
        PUTC(js, 0x80 | ( u       & 0x3F));
    }
    else if (u <= 0xFFFF) {
        PUTC(js, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(js, 0x80 | ((u >>  6) & 0x3F));
        PUTC(js, 0x80 | ( u        & 0x3F));
    }
    else {
        assert(u <= 0x10FFFF);
        PUTC(js, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(js, 0x80 | ((u >> 12) & 0x3F));
        PUTC(js, 0x80 | ((u >>  6) & 0x3F));
        PUTC(js, 0x80 | ( u        & 0x3F));
    }
}

static int c_parse_string_raw(base_json* js,char** str,size_t* len){
    js->json++;
    const char* p = js->json;
    void* ref;
    unsigned u,u1;
    size_t head = js->top;
    while( 1 )
    {
        char c = *p++;
        switch(c){
            case '"':
                *len = js->top - head;
                *str = base_json_pop(js,*len);
                js->json = p;
                return C_PARSE_OK;
            case '\\':
                switch (*p++) {
                    case '\"': PUTC(js, '\"'); break;
                    case '\\': PUTC(js, '\\'); break;
                    case '/':  PUTC(js, '/' ); break;
                    case 'b':  PUTC(js, '\b'); break;
                    case 'f':  PUTC(js, '\f'); break;
                    case 'n':  PUTC(js, '\n'); break;
                    case 'r':  PUTC(js, '\r'); break;
                    case 't':  PUTC(js, '\t'); break;
                    case 'u':
                        if( !(p = parse_hex4(p,&u)) )
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                        if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                            if (*p++ != '\\')
                                STRING_ERROR(C_PARSE_INVALID_UNICODE_SURROGATE);
                            if (*p++ != 'u')
                                STRING_ERROR(C_PARSE_INVALID_UNICODE_SURROGATE);
                            if (!(p = parse_hex4(p, &u1)))
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                            if (u1 < 0xDC00 || u1 > 0xDFFF)
                                STRING_ERROR(C_PARSE_INVALID_UNICODE_SURROGATE);
                            u = (((u - 0xD800) << 10) | (u1 - 0xDC00)) + 0x10000;
                        }
                        encode_utf8(js,u);
                        break;
                    default:
                        STRING_ERROR(C_PARSE_INVALID_STRING_ESCAPE);
                }
                break;
            case '\0':
                STRING_ERROR(C_PARSE_MISS_QUOTATION_MARK);
            default:
                if( (unsigned char)c < 0x20 ){
                    STRING_ERROR(C_PARSE_INVALID_STRING_CHAR);
                }
                /* PUTC(js,c); */
                ref = base_json_push(js,sizeof(char) );
                *(char*)ref = c;
        }
    }
}

static int parse_string(base_json* js,c_value* v){
    int state;
    char* s;  /* 暂时存储字符串的数组 */
    size_t len;
    state = c_parse_string_raw(js,&s,&len);
    if( state == C_PARSE_OK ){
        // s是栈中s.top-len的位置
        c_set_string(v,s,len);
    }
    return state;
}

static int parse_value(base_json* c, c_value* v);

static int parse_array(base_json* js,c_value* v){
    /* struct{ c_value* e; size_t size; }a;  */
    int state,siz;
    c_value* ref;
    siz = 0;
    js->json++;  /* [  */
    clear_whitespace( js );
    if( js->json[0] == ']' ){
        js->json++;
        v->type = C_ARRAY;
        v->a.e = NULL;
        v->a.size = 0;
        return C_PARSE_OK;
    }
    while( 1 ){
        c_value now;
        now.type = C_UNKNOW;
        clear_whitespace( js ); /* ,或[ 与后面元素 之间的空格 */
        if( ( state = parse_value(js,&now) )!=C_PARSE_OK ){
            int i;
            for(i = 0;i<siz;i++)
                c_free( (c_value*)base_json_pop(js,sizeof(c_value) ));
            return state;
        }
        clear_whitespace( js );  /* 消除元素和后面,或] 之间的空格  */
        siz++;
        memcpy( base_json_push( js,sizeof(c_value) ),&now,sizeof(c_value) );
        clear_whitespace( js );
        if( js->json[0] == ',' ){
            js->json++;
        }
        else if( js->json[0] == ']' ){  /* 解析完成 */
            js->json++;
            v->type = C_ARRAY;
            v->a.size = siz;
            siz *= sizeof( c_value );
            v->a.e = (c_value*)malloc( siz );
            memcpy( v->a.e, base_json_pop(js,siz), siz );
            return C_PARSE_OK;
        }
        else{
            int i;
            for(i = 0;i<siz;i++)
                c_free( (c_value*)base_json_pop(js,sizeof(c_value) ));
            return C_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
        }
    }
}


static int parse_object(base_json* js,c_value* v){
    size_t i,size;
    c_member m;
    int state;
    assert( js->json[0] == '{' );
    js->json++;
    clear_whitespace( js );
    if( js->json[0] == '}' ){
        js->json++;
        v->type = C_OBJECT;
        v->o.m = 0;  /* 相当于指针不指向任何值 */
        v->o.size = 0;
        return C_PARSE_OK;
    }
    m.k = NULL;
    size = 0;
    while( 1 ){
        char* str;
        if( js->json[0] != '"' ){
            state = C_PARSE_MISS_KEY;
            break;
        }
        state = c_parse_string_raw(js,&str,&m.len );
        if( state != C_PARSE_OK )   break;
        memcpy( m.k = (char*)malloc(m.len+1),str,m.len );
        m.k[m.len+1] = '\0';

        clear_whitespace( js );
        if( *js->json != ':' ){
            state = C_PARSE_MISS_COLON;
            break;
        }
        js->json++;
        clear_whitespace( js );

        state = parse_value(js,&m.v);
        if( state != C_PARSE_OK ){
            break;
        }
        memcpy(base_json_push(js,sizeof(c_member)),&m,sizeof(c_member) );
        size++;
        m.k = NULL;
        clear_whitespace( js );
        if( js->json[0] == ',' ){
            js->json++;
            clear_whitespace( js );
        }else if( js->json[0] == '}' ){
            size_t si = sizeof( c_member )*size;
            js->json++;
            v->type = C_OBJECT;
            v->o.size = size;
            memcpy( v->o.m = (c_member*)malloc(si), base_json_pop(js,si),si );
            return C_PARSE_OK;
        }else{
            state = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }

    free( m.k );
    for( i=0;i<size;i++ ){
        c_member* m = (c_member*) base_json_pop(js,sizeof(c_member) );
        free( m->k );  /* 释放键 */
        c_free( &m->v );
    }
    v->type = C_UNKNOW;
    return state;
}

static int parse_value(base_json* js,c_value* v){
    switch(*js->json){
        case 'n': return parse_const(js,v,"null",C_NULL);
        case 't': return parse_const(js,v,"true",C_TRUE);
        case 'f': return parse_const(js,v,"false",C_FALSE);
        case '"': return parse_string(js,v);
        case '[': return parse_array(js,v);
        case '{': return parse_object(js,v);
        case '\0': return C_PARSE_EXPECT_VALUE;
        default: return parse_number(js,v);
    }
}

/* 解析json */
int c_parse(c_value* v,const char* json){
    assert( v != NULL );
    v->type = C_UNKNOW;
    int state;
    base_json js;
    js.json = json;
    js.stack = NULL;
    js.size = js.top = 0;
    clear_whitespace(&js);  /* 解析掉前面的空格 */

    if( (state = parse_value(&js,v) ) == C_PARSE_OK){
        clear_whitespace( &js );
        if( *js.json != '\0' )
            state = C_PARSE_ROOT_NOT_SINGULAR, v->type = C_UNKNOW;
    }
    return state;
}



c_type c_get_type(const c_value* v){
    assert( v!=NULL );
    return v->type;
}

void c_free(c_value* v){
    assert( v != NULL );
    if( v->type == C_STRING ){
        free( v->s.s );
    }else if( v->type == C_ARRAY ){
        int i;
        for(i=0;i<v->a.size;i++)
            c_free( &v->a.e[i] );
        free( v->a.e );
    }
    v->type = C_UNKNOW;
}

double c_get_number(const c_value* v){
    assert( v!=NULL && v->type==C_NUMBER );
    return v->n;
}

size_t c_get_stringlen(const c_value* v){
    assert( v!=NULL && v->type == C_STRING );
    return v->s.len;
}

const char* c_get_string(const c_value* v){
    assert( v!=NULL && v->type == C_STRING );
    return v->s.s;
}

size_t c_get_arraysize(const c_value* v){
    assert( v!=NULL && v->type==C_ARRAY );
    return v->a.size;
}

/*  把s表示的字符串复制到c_value中去 */
void c_set_string(c_value* v,const char* s,size_t len){
    assert( v!=NULL && ( s != NULL || len == 0) );
    c_free(v);
    v->s.s = (char*)malloc( len+1 );
    memcpy( v->s.s,s,len );
    v->s.s[len] = '\0';
    v->s.len = len;
    v->type = C_STRING;
}

void c_set_number(c_value* v,double val){
    assert( v!=NULL && v->type == C_NUMBER );
    v->n = val;
}

/* 得到 */
size_t c_get_object_size(const c_value* v ){
    assert( v != NULL && v->type == C_OBJECT );
    return v->o.size;
}
/* 得到json对象的键 */
const char* c_get_object_key(const c_value* v,size_t index){
    assert( v != NULL && v->type == C_OBJECT );
    assert( index < v->o.size );
    return v->o.m[index].k;
}

size_t c_get_object_key_length(const c_value* v,size_t index){
    assert( v != NULL && v->type == C_OBJECT );
    assert( index < v->o.size );
    return v->o.m[index].len;
}

c_value* c_get_object_value(const c_value* v,size_t index){
    assert( v != NULL && v->type == C_OBJECT );
    assert( index < v->o.size );
    return &v->o.m[index].v;
}

c_value* c_get_array_element(const c_value* v,size_t index){
    assert( v!=NULL && v->type == C_ARRAY );
    assert( index < v->a.size);
    return &v->a.e[index];
}


/*------------------------下面是json生成器内容------------------------- */
#define PUTS(js,s,len) memcpy(base_json_push(js,len),s,len)

static void c_stringify_string(base_json* js,char* s,size_t len){
    static const char hex_digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    size_t i, size;
    char* head, *p;
    assert(js != NULL);
    p = head = base_json_push(js, size = len * 6 + 2); /* "\u00xx..." */
    *p++ = '"';
    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];
        switch (ch) {
            case '\"': *p++ = '\\'; *p++ = '\"'; break;
            case '\\': *p++ = '\\'; *p++ = '\\'; break;
            case '\b': *p++ = '\\'; *p++ = 'b';  break;
            case '\f': *p++ = '\\'; *p++ = 'f';  break;
            case '\n': *p++ = '\\'; *p++ = 'n';  break;
            case '\r': *p++ = '\\'; *p++ = 'r';  break;
            case '\t': *p++ = '\\'; *p++ = 't';  break;
            default:
                if (ch < 0x20) {
                    *p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
                    *p++ = hex_digits[ch >> 4];
                    *p++ = hex_digits[ch & 15];
                }
                else
                    *p++ = s[i];
        }
    }
    *p++ = '"';
    js->top -= size - (p - head);
}

static void c_stringify_value(base_json* js,const c_value* v){
    size_t i;
    switch( v->type ){
        case C_NULL:  PUTS(js,"null",4); break;
        case C_FALSE: PUTS(js,"false",5); break;
        case C_TRUE: PUTS(js,"true",4); break;
        case C_NUMBER:
            js->top -= 32-sprintf(base_json_push(js,32),"%.17g",v->n );
            break;
        case C_STRING:
            c_stringify_string(js,v->s.s,v->s.len );
            break;
        case C_ARRAY:
            PUTC(js,'[');
            for( i = 0;i<v->a.size;i++){
                if( i>0 )   PUTC(js,',');
                c_stringify_value( js,&v->a.e[i] );
            }
            PUTC(js,']');
            break;
        case C_OBJECT:
            PUTC(js,'{');
            for(i=0;i<v->o.size;i++){
                if( i>0 )   PUTC(js,',');
                c_stringify_string(js,v->o.m[i].k,v->o.m[i].len);
                PUTC(js,':');
                c_stringify_value( js,&v->o.m[i].v );
            }
            PUTC(js,'}');
            break;
        default:
            assert( 0 && "invalid type");
    }
}

char* c_stringify(const c_value* v,size_t* len){
    base_json js;
    assert( v!=NULL );
    js.stack = (char*)malloc( js.size = C_STACK_INIT_SIZE);
    js.top = 0;
    c_stringify_value( &js,v );
    if( len ){
        *len = js.top;
    }
    PUTC(&js,'\0');
    return js.stack;
}
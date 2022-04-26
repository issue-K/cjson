#include<assert.h>  /* assert() */
#include <stdlib.h> /* NULL,strtod() */
#include <math.h> /* HUGE_VAL*/
#include <errno.h> /* errno, ERANGE */
#include "cjson.h"


typedef struct{
    const char* json;
}base_json;

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

static int parse_value(base_json* js,c_value* v){
    switch(*js->json){
        case 'n': return parse_const(js,v,"null",C_NULL);
        case 't': return parse_const(js,v,"true",C_TRUE);
        case 'f': return parse_const(js,v,"false",C_FALSE);
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

double c_get_number(const c_value* v){
    assert( v!=NULL && v->type == C_NUMBER );
    return v->n;
}
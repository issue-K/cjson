#include <stdio.h>
#include <math.h> /* HUGE_VAL*/
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "cjson.h"

static int total_test = 0;
static int pass_test = 0;

#define EXPECT_EQ_BASE(eq,expect,actual,format) \
    do{\
    total_test++;                               \
    if( eq )                                    \
        pass_test++;                            \
    else{                                       \
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
    }                                    \
    }while(0)

#define EXPECT_EQ_INT(expect,actual) EXPECT_EQ_BASE( ((expect)==(actual)),expect,actual,"%d")
#define EXPECT_EQ_DOUBLE(expect,actual) EXPECT_EQ_BASE( ((expect)==(actual)),expect,actual,"%.17g")
#define EXPECT_EQ_STRING(expect,actual,len)\
    EXPECT_EQ_BASE( sizeof(expect)-1==len && memcmp(expect,actual,len)==0,expect,actual,"%s" )
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
/*
 * 测试解析状态码和类型是否和预期一致
 */
#define TEST_STATE_AND_TYPE(json,expect_type,expect_state)\
    do{                                   \
        c_value v;\
        EXPECT_EQ_INT(expect_state,c_parse(&v,json) );\
        EXPECT_EQ_INT(expect_type,c_get_type(&v) );       \
        c_free(&v);                                            \
    }while(0)

#define TEST_NUMBER(expect_number,json) \
    do{                                 \
        c_value v;                      \
        EXPECT_EQ_INT(C_PARSE_OK,c_parse(&v,json) ); \
        EXPECT_EQ_INT(C_NUMBER,c_get_type(&v)); \
        EXPECT_EQ_DOUBLE(expect_number,c_get_number(&v) ); \
        c_free(&v);                                    \
    }while(0)

#define TEST_STRING(expect,json) \
    do{                          \
        c_value v;               \
        EXPECT_EQ_INT(C_PARSE_OK,c_parse(&v,json)); \
        EXPECT_EQ_INT(C_STRING,c_get_type(&v) );   \
        EXPECT_EQ_STRING( expect,c_get_string(&v), c_get_stringlen(&v) ); \
        c_free(&v); \
    }while(0)

static void test_parse_number(){
    /* 测试数字合法的情况 */
    TEST_NUMBER(0.0,"0");
    TEST_NUMBER(0.0,"-0");

    /* 下面检测一些不合法的情况 */
    TEST_STATE_AND_TYPE("+0",C_UNKNOW,C_PARSE_INVALID_VALUE);
    TEST_STATE_AND_TYPE("+1",C_UNKNOW,C_PARSE_INVALID_VALUE);
    TEST_STATE_AND_TYPE(".123",C_UNKNOW,C_PARSE_INVALID_VALUE);
    TEST_STATE_AND_TYPE("inf",C_UNKNOW,C_PARSE_INVALID_VALUE);
}
static void test_parse_string(){
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");

    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

static void printarray(c_value* v){
    assert( v!=NULL && v->type==C_ARRAY );
    size_t siz = c_get_arraysize( v );
    printf("数组元素size = %zu\n", siz );
    c_value* now = v->a.e;
    for(int i=0;i<siz;i++){
        printf("第%d个元素:  ",i );
        if( now[i].type == C_NUMBER ){
            printf("数字是%.3lf\n",now[i].n );
        }else if( now[i].type == C_TRUE ){
            printf(" TRUE类型 \n");
        }else if( now[i].type == C_FALSE ) {
            printf(" FALSE类型 \n");
        }else if( now[i].type == C_NULL ) {
            printf(" NULL类型 \n");
        }else if( now[i].type == C_STRING ){
            printf("string类型: %s\n",now[i].s.s );
        }else if( now[i].type == C_ARRAY ){
            printf("array类型,元素个数为%zu\n",now[i].a.size );
        }else{
            printf("未知类型\n");
        }
    }
}

static void test_parse_array(){
    c_value v;
    EXPECT_EQ_INT(C_PARSE_OK, c_parse(&v,"[]") );
    EXPECT_EQ_INT(C_ARRAY, c_get_type(&v) );
    EXPECT_EQ_SIZE_T(0,c_get_arraysize(&v) );

    EXPECT_EQ_INT(C_PARSE_OK,c_parse(&v,"[ null , false , true , 123 , \"abc\" ]"));
 /*   printarray( &v );  */

    EXPECT_EQ_INT(C_PARSE_OK,c_parse(&v," [ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]") );
    printarray( &v );
    c_free( &v );
}

static void test_parse_expect_value(){

}
/*
 * 测试无法成功解析的案例
 */
static void test_parse_invalid_value(){
    TEST_STATE_AND_TYPE("  ???  ",C_UNKNOW,C_PARSE_INVALID_VALUE);
}
/*
 * 测试成功解析掉一个值后, 后面仍有其他字符而不是'\0'
 */
static void test_parse_root_not_singular(){
    TEST_STATE_AND_TYPE("nulll",C_UNKNOW,C_PARSE_ROOT_NOT_SINGULAR);
    TEST_STATE_AND_TYPE("null x",C_UNKNOW,C_PARSE_ROOT_NOT_SINGULAR);
}
/*
 * 测试成功解析的案例
 */
static void test_parse_ok(){
    TEST_STATE_AND_TYPE("null",C_NULL,C_PARSE_OK);
    TEST_STATE_AND_TYPE("false",C_FALSE,C_PARSE_OK);
    TEST_STATE_AND_TYPE("true",C_TRUE,C_PARSE_OK);
}
static void test_parse_object(){
    c_value v;
    size_t i;

    EXPECT_EQ_INT(C_PARSE_OK, c_parse(&v, " { } "));
    EXPECT_EQ_INT(C_OBJECT, c_get_type(&v));
    EXPECT_EQ_SIZE_T(0, c_get_object_size(&v));
    c_free(&v);

    EXPECT_EQ_INT(C_PARSE_OK, c_parse(&v,
                                            " { "
                                            "\"n\" : null , "
                                            "\"f\" : false , "
                                            "\"t\" : true , "
                                            "\"i\" : 123 , "
                                            "\"s\" : \"abc\", "
                                            "\"a\" : [ 1, 2, 3 ],"
                                            "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
                                            " } "
    ));
    EXPECT_EQ_INT(C_OBJECT, c_get_type(&v));
    EXPECT_EQ_SIZE_T(7, c_get_object_size(&v));
    EXPECT_EQ_STRING("n", c_get_object_key(&v, 0), c_get_object_key_length(&v, 0));
    EXPECT_EQ_INT(C_NULL,   c_get_type(c_get_object_value(&v, 0)));
    EXPECT_EQ_STRING("f", c_get_object_key(&v, 1), c_get_object_key_length(&v, 1));
    EXPECT_EQ_INT(C_FALSE,  c_get_type(c_get_object_value(&v, 1)));
    EXPECT_EQ_STRING("t", c_get_object_key(&v, 2), c_get_object_key_length(&v, 2));
    EXPECT_EQ_INT(C_TRUE,   c_get_type(c_get_object_value(&v, 2)));
    EXPECT_EQ_STRING("i", c_get_object_key(&v, 3), c_get_object_key_length(&v, 3));
    EXPECT_EQ_INT(C_NUMBER, c_get_type(c_get_object_value(&v, 3)));
    EXPECT_EQ_DOUBLE(123.0, c_get_number(c_get_object_value(&v, 3)));
    EXPECT_EQ_STRING("s", c_get_object_key(&v, 4), c_get_object_key_length(&v, 4));
    EXPECT_EQ_INT(C_STRING, c_get_type(c_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("abc", c_get_string(c_get_object_value(&v, 4)), c_get_stringlen(c_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("a", c_get_object_key(&v, 5), c_get_object_key_length(&v, 5));
    EXPECT_EQ_INT(C_ARRAY, c_get_type(c_get_object_value(&v, 5)));
    EXPECT_EQ_SIZE_T(3, c_get_arraysize(c_get_object_value(&v, 5)));
    for (i = 0; i < 3; i++) {
        c_value* e = c_get_array_element(c_get_object_value(&v, 5), i);
        EXPECT_EQ_INT(C_NUMBER, c_get_type(e));
        EXPECT_EQ_DOUBLE(i + 1.0, c_get_number(e));
    }
    EXPECT_EQ_STRING("o", c_get_object_key(&v, 6), c_get_object_key_length(&v, 6));
    {
        c_value* o = c_get_object_value(&v, 6);
        EXPECT_EQ_INT(C_OBJECT, c_get_type(o));
        for (i = 0; i < 3; i++) {
            c_value* ov = c_get_object_value(o, i);
            /* EXPECT_TRUE('1' + i == c_get_object_key(o, i)[0]); */
            EXPECT_EQ_SIZE_T(1, c_get_object_key_length(o, i));
            EXPECT_EQ_INT(C_NUMBER, c_get_type(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, c_get_number(ov));
        }
    }
    c_free(&v);
}
static void test_parse(){
    test_parse_ok();
    test_parse_invalid_value();
    test_parse_root_not_singular();

    test_parse_number();
    test_parse_string();
    test_parse_array();

    test_parse_object();
}

#define TEST_ROUNDTRIP(json) \
    do{\
        c_value v;\
        char* json2;         \
        size_t len;          \
        EXPECT_EQ_INT(C_PARSE_OK,c_parse(&v,json)); \
        json2 = c_stringify(&v,&len);               \
        EXPECT_EQ_STRING(json,json2,len );          \
        c_free(&v);          \
        free(json2);                         \
    }while(0)
static void test_stringify() {
    TEST_ROUNDTRIP("null");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("true");
    TEST_ROUNDTRIP("{}");
    TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}
int main() {

    test_parse();
    test_stringify();
    printf("%d/%d (%3.2f%%) passed\n", pass_test, total_test, pass_test * 100.0 / total_test);
}
#include <stdio.h>
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


/*
 * 测试解析状态码和类型是否和预期一致
 */
#define TEST_STATE_AND_TYPE(json,expect_type,expect_state)\
    do{                                   \
        c_value v;\
        EXPECT_EQ_INT(expect_state,c_parse(&v,json) );\
        EXPECT_EQ_INT(expect_type,c_get_type(&v) );\
    }while(0)


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
static void test_parse(){
    test_parse_ok();
    test_parse_invalid_value();
    test_parse_root_not_singular();
}
int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", pass_test, total_test, pass_test * 100.0 / total_test);
}

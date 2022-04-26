#ifndef CJSON_H
#define CJSON_H

typedef enum{
    C_UNKNOW,  /* 没开始解析就是这个类型 */
    C_NULL,
    C_FALSE,
    C_TRUE,
    C_NUMBER,
}c_type;

/*
    解析成功/失败返回的状态码
*/
enum{
    C_PARSE_OK = 0,
    C_PARSE_ROOT_NOT_SINGULAR,  /* 解析到不止一个值 */
    C_PARSE_INVALID_VALUE,  /* 解析错误 */
    C_PARSE_EXPECT_VALUE,
};

typedef struct c_value c_value;

/*
 * json键值对中,值的定义
 */
struct c_value{
    union{
        double n;
    };
    c_type type;
};

/* 解析json的入口 */
int c_parse(c_value* v,const char* json);
/*  获取值的类型 */
c_type c_get_type(const c_value* v);
/* 已知值是数字类型, 获取数字值*/
double c_get_number(const c_value* v);

#endif
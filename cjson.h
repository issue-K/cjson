#ifndef CJSON_H
#define CJSON_H

typedef enum{
    C_UNKNOW,  /* 没开始解析就是这个类型 */
    C_NULL,
    C_FALSE,
    C_TRUE,
    C_NUMBER,
    C_STRING,
    C_ARRAY,
}c_type;

/*
    解析成功/失败返回的状态码
*/
enum{
    C_PARSE_OK = 0,
    C_PARSE_ROOT_NOT_SINGULAR,  /* 解析到不止一个值 */
    C_PARSE_INVALID_VALUE,  /* 解析错误 */
    C_PARSE_EXPECT_VALUE,
    C_PARSE_NUMBER_TOO_BIG,
    C_PARSE_INVALID_STRING_CHAR,  /* 解析字符串时,遇到不支持的字符 */
    C_PARSE_MISS_QUOTATION_MARK, /* 解析字符串时缺少右引号 */
    C_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_UNICODE_HEX,  /* \uxxxx的格式不正确 */
    C_PARSE_INVALID_UNICODE_SURROGATE,
    C_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
};

typedef struct c_value c_value;

/*
 * json键值对中,值的定义
 */
struct c_value{
    union{
        struct{ c_value* e; size_t size; }a;
        double n; /* number */
        struct{ char* s; size_t len; }s; /* string */
    };
    c_type type;
};

#define c_init(v) do{ (v)->type = C_UNKNOW; }while(0)

/* 解析json的入口 */
int c_parse(c_value* v,const char* json);
/* 释放c_value的空间 */
void c_free(c_value* v);
/*  获取值的类型 */
c_type c_get_type(const c_value* v);


/* 已知值是数字类型, 获取数字值*/
double c_get_number(const c_value* v);
/* 得到string长度 */
size_t c_get_stringlen(const c_value* v);
/* 得到string */
const char* c_get_string(const c_value* v);
/* 得到数组大小 */
size_t c_get_arraysize(const c_value* v);

/* 设置数字 */
void c_set_number(c_value *v,double val);
/* 设置string */
void c_set_string(c_value* v,const char* s,size_t len);
/* 设置数组元素 */
c_value* c_get_array_element(const c_value* v,size_t index);

#endif
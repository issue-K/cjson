**这次增加了对解析数字的支持.**

这里不得不说一下```json```中数字的格式

Ⅰ.```number```以十进制表示, 由四部分构成: 负号+整数+小数+指数

Ⅱ. 以上四部分只有整数部分是必须的.

Ⅲ. 整数部分不能有前导零(单个零例外), 小数部分可以以若干个零结尾

Ⅳ.指数部分指的是科学计数法.形如```e+7```或```E-6```都是合法的

用一张图可以表示这个数字.

![number](https://github.com/miloyip/json-tutorial/raw/master/tutorial02/images/number.png)

## 如何解析?

首先给数字设置一个类型.

```c
typedef enum{
    ........
    C_NUMBER,  /* 数字类型 */
}c_type;
```

值的定义也修改以下, 增加一个存储数字的```double```属性

```c
struct c_value{
    union{
        double n;  /* 当type=C_NUMBER时, 这个字符就存储数字.*/
    };
    c_type type;
};
```

再提供一个访问```c_value```的数字的```api```

```c
double c_get_number(const c_value* v){
    assert( v!=NULL && v->type == C_NUMBER );
    return v->n;
}
```



这样我们就可以开始解析了, 看一下解析的主入口

除了常量, 其余一律交给函数```parse_number()```, 如果判定值不是数字, 直接返回```C_PARSE_INVALID_VALUE```即可 

```c
static int parse_value(base_json* js,c_value* v){
    switch(*js->json){
        case 'n': return parse_const(js,v,"null",C_NULL);
        case 't': return parse_const(js,v,"true",C_TRUE);
        case 'f': return parse_const(js,v,"false",C_FALSE);
        case '\0': return C_PARSE_EXPECT_VALUE;
        default: return parse_number(js,v);
    }
}
```

具体解析逻辑如下

判断是否是合法数字的思路比较简单, 就是依次验证这四个部分即可.

但是这里计算具体数字的值用了函数```strtod```, 其实完全可以在判断是否合法的过程中顺便算出数字(为了方便还是用函数吧)

```c
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
```

其中需要注意的是, 即使数字形式合法, 但是可能值很大设置超过```double```能表示的最大值(这个最大值在```math.h```中被定义为```HUGE_VAL```)

这个时候```strtod```显然会出错, 这个函数会把错误记录在一个叫```errno```的宏中, 判断一下即可.
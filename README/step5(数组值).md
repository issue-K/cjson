这一节我们来解析数组.

数组形如```[value,value,value....]```

于是我们在```c_value```中多加一些字段支持数组

一个```c_value*```数组```e```, 一个元素个数```size```

```c
struct{ c_value* e; size_t size; }a;
```



```c
struct c_value{
    union{
        struct{ c_value* e; size_t size; }a;
        double n; /* number */
        struct{ char* s; size_t len; }s; /* string */
    };
    c_type type;
};
```



所以直接在```parse_value```中判断首字符是否是左中括号即可

```C
static int parse_value(base_json* js,c_value* v){
    switch(*js->json){
        case 'n': return parse_const(js,v,"null",C_NULL);
        case 't': return parse_const(js,v,"true",C_TRUE);
        case 'f': return parse_const(js,v,"false",C_FALSE);
        case '"': return parse_string(js,v);
        case '[': return parse_array(js,v);
        case '\0': return C_PARSE_EXPECT_VALUE;
        default: return parse_number(js,v);
    }
}
```

接下来看一下```parse_array```的具体实现

由于数组内有多少个值我们是不知道的, 也需要动态分配空间/扩容, 所以我们解析的时候, 先暂存在上一节设计的那个堆栈中

大概过程如下

Ⅰ. 判断首字符是否是左括号

Ⅱ. 开始循环解析值., 单次循环的步骤如下.

使用```parse_value```解析单个元素, 解析结果保存在临时变量```now```(变量类型同样为```c_value```)中

然后调用```base_json_push```把```now```压入栈中

若下一个字符是```,```说明还有值,继续循环.

若下一个字符是```]```,说明解析完毕, 弹出栈中的所有元素并复制到结果中



```go
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
```


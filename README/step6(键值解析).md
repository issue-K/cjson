## 解析json对象

之前我们都是在解析```json```中的值

但是一个```json```对象是由若干个键值对构成的. 键一定是字符串, 值就是我们之前解析的那些值.

如何表示一个```json```对象呢?

首先先表示一个键值对对象

```C
struct c_member{
    char* k; size_t len;  // 键
    c_value v;  // 值
};
```

然后给```c_value```新增类型```c_object```表示```json```对象

```c
struct c_value{
    union{
        struct{ c_member* m; size_t size; }o;
        struct{ c_value* e; size_t size; }a;
        double n; /* number */
        struct{ char* s; size_t len; }s; /* string */
    };
    c_type type;
};
```

其中```c_value.o```在类型为```c_object```时生效

和数组类型类似, 这里也是用一个键值数组```c_member* m```来表示一个```json```对象

解析```c_object```时就非常简单了

```c
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
```


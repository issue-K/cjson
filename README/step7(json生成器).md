### ```json```生成器

现在已经可以解析一个```json```字符串为```json```对象了

来实现一个逆过程, 把```json```对象转化为一个```json```字符串.

不做过多解释(不想写了.........今天头有点痛)

和```json```解析的过程差不多.

```c
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
```


## 新增

支持字符串值中包含```unicode```, 也就是形如```\uxxxx```这样的字符.

参考

[阮一峰:字符编码笔记](http://www.ruanyifeng.com/blog/2007/10/ascii_unicode_and_utf-8.html)

[Unicode 和 UTF-8 有什么区别？](https://www.zhihu.com/question/23374078/answer/69732605)

---

## 前置知识

### ①. ASCII码

由美国最早指定的编码方式. 每个字符用$8$个比特表示, 所以最多可以表示$256$种字符

由于只需要表示一些字母和数字或简单符号(一共128种字符), 所以这样表示没有问题.

### ②. Unicode
对于使用英语的国家,ASCII码已经足够使用.

使用其他语言的国家就需要创建自己的编码方式了, 比如编码中文的GBK格式.

后来每个国家都创建一种编码方式, 交流极其不便, 需要有一种编码统一起来.

这就是Unicode编码, 目前它包含大概1114112个字符,使用数字0-0x10FFFF来映射这些字符。

比如U+4E25表示汉字严

### ③.Unicode的不足
虽然Unicode制定了二进制和字符间的映射, 但它没有规定这个二进制代码如何存储.

如果采用ASCII码就知道每$8$个比特表示一个字符, 而Unicode码则没有这样规定。

当然我们也可以效仿ASCII码, 比如用四个字节来表示一个字符.

这样的问题是, 数字小的那些二进制高位都是0, 而这些二进制对应的字符恰好是常用的那些

极大的浪费了存储空间.

### ④.UTF-8

```UTF-8```是```Unicode```的一种实现方式.

```UTF-8 ```最大的一个特点，就是它是一种变长的编码方式。它可以使用```1~4```个字节表示一个符号，根据不同的符号而变化字节长度, 这样极大的节省了空间.

UTF-8的编码规则如下

Ⅰ. 对于单字节的符号，字节的第一位设为0，后面7位为这个符号的 Unicode 码。因此对于英语字母，```UTF-8```编码和 ```ASCII```码是相同的。

Ⅱ. 对于```n```字节的符号（```n > 1```），第一个字节的前```n```位都设为1，第```n + 1```位设为0，后面字节的前两位一律设为```10```。剩下的没有提及的二进制位，全部为这个符号的```Unicode```码。

具体如下图所示

![在这里插入图片描述](https://img-blog.csdnimg.cn/7a7fb4f251fc49989dcfcb70899f8c5c.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBAaXNzdWXmmK9mdw==,size_20,color_FFFFFF,t_70,g_se,x_16)
这样设置有一个好处. 当看到以0开头就知道接下来的字符是一个字节,看到以110开头就知道接下来的字符是两个字节......以此类推



###  代码实现: ```unicode```转UTF-8

首先先丰富一下测试用例, 尝试解析一些含```unicode```的字符串

```c
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
```

然后考虑如何解析字符串中的```\uxxx```

首先判断转移符, 若识别到```\u```时就可以开始转义

①. 首先把```\uxxx```(十六进制)转化为十进制

```go
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
```



②. 字符串是以```utf-8```存储, 所以把上面的十进制转为```utf-8```形式的二进制,一个字节一个字节写入.

```c
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
```


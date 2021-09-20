This is Readme
# part 1 

# part 2 Json 数字语法

number = [ "-" ] int [ frac ] [ exp ];

int = "0" / digit1-9 *digit

frac = "." 1*digit

exp = ("e" / "E") ["-" / "+"] 1*digit


number 是以十进制表示，它主要由 4 部分顺序组成：负号、整数、小数、指数。只有整数是必需部分。注意和直觉可能不同的是，正号是不合法

使用 #if 0 ... #endif 而不使用 /* ... */，是因为 C 的注释不支持嵌套（nested），而 #if ... #endif 是支持嵌套的。代码中已有注释时，用 #if 0 ... #endif 去禁用代码是一个常用技巧，而且可以把 0 改为 1 去恢复。

# part 3 解析字符串
## 1、json字符串语法
C 语言和 JSON 都使用 \（反斜线）作为转义字符，那么 " 在字符串中就表示为 \"，a"b 的 JSON 字符串则写成 "a\"b"。如以下的字符串语法所示，JSON 共支持 9 种转义序列：

```cpp
string = quotation-mark *char quotation-mark
char = unescaped /
   escape (
       %x22 /          ; "    quotation mark  U+0022
       %x5C /          ; \    reverse solidus U+005C
       %x2F /          ; /    solidus         U+002F
       %x62 /          ; b    backspace       U+0008
       %x66 /          ; f    form feed       U+000C
       %x6E /          ; n    line feed       U+000A
       %x72 /          ; r    carriage return U+000D
       %x74 /          ; t    tab             U+0009
       %x75 4HEXDIG )  ; uXXXX                U+XXXX
escape = %x5C          ; \
quotation-mark = %x22  ; "
unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
```
## 2、字符串表示
**实现原理**：当遇到“时开始解析字符串，首先备份栈顶的位置，用一个指针指向字符串的首地址，然后指针往后走，其中用一个临时缓冲区数据结构为栈存储字符。

（1）如果遇到”,用该位置减去备份的栈顶位置即为字符串的长度；一次性弹出所有的字符。返回成功；注意在设置这个 `v` 之前，我们需要先调用 `lept_free(v)` 去清空 `v` 可能分配到的内存。例如原来已有一字符串，我们要先把它释放。然后就是简单地用 `malloc()` 分配及用 `memcpy()` 复制，并补上结尾空字符。`malloc(len + 1)` 中的 1 是因为结尾空字符。返回成功；

（2）如果遇到‘\0'，表明字符串结束，缺少右引号，返回缺少引号的错误；

（3）如果是字符，则入栈；其中如果栈的大小不足，会以1.5倍扩容。和普通的堆栈不一样，我们这个堆栈是以字节储存的。push每次可要求压入任意大小的数据，它会返回数据起始的指针，栈顶指针每次后移动size大小。

---------------------------------------
在 C 语言中，字符串一般表示为空结尾字符串（null-terminated string），即以空字符（'\0'）代表字符串的结束。然而，JSON 字符串是允许含有空字符的，例如这个 JSON "Hello\u0000World" 就是单个字符串，解析后为11个字符。如果纯粹使用空结尾字符串来表示 JSON 解析后的结果，就没法处理空字符。

因此，我们可以分配内存来储存解析后的字符，以及记录字符的数目（即字符串长度）。由于大部分 C 程序都假设字符串是空结尾字符串，我们还是在最后加上一个空字符，那么不需处理 \u0000 这种字符的应用可以简单地把它当作是空结尾字符串

## 3、内存管理
由于字符串的长度不是固定的，需要动态的分配内存。用malloc()、realloc()和free()来分配/释放内存；

注意，在设置这个lept_value* v 之前，我们需要先调用 lept_free(v) 去清空 v 可能分配到的内存。例如原来已有一字符串，我们要先把它释放。然后就是简单地用 malloc() 分配及用 memcpy() 复制，并补上结尾空字符。malloc(len + 1) 中的 1 是因为结尾空字符

## 4、缓冲区和堆栈
解析字符串（以及之后的数组、对象）时，需要把解析的结果先储存在一个临时的缓冲区，最后再用 lept_set_string() 把缓冲区的结果设进值之中。在完成解析一个字符串之前，这个缓冲区的大小是不能预知的。因此，我们可以采用动态数组（dynamic array）这种数据结构，即数组空间不足时，能自动扩展。C++ 标准库的 std::vector 也是一种动态数组。

而且我们将会发现，无论是解析字符串、数组或对象，我们也只需要以先进后出的方式访问这个动态数组。换句话说，我们需要一个动态的堆栈（stack）数据结构。

## 内存泄漏检测方法
Liunx 系统下 
命令行 ` valgrind --leak-check= full ./lept_json.test`

Valgrind 还有很多功能，例如可以发现未初始化变量。我们若在应用程序或测试程序中，忘了调用 lept_init(&v)，那么 v.type 的值没被初始化，其值是不确定的（indeterministic），一些函数如果读取那个值就会出现问题：

## Q&A
2.实现除了 \u 以外的转义序列解析，令 test_parse_string() 中所有测试通过。

**解析**：当遇到字符'\'时，增加swith case语句；

**遇到的问题**：测试test_parse_string()，遇到段错误（segmentation fault)

**解决问题思路**：实现肯定是解析字符串时候发生的，而且段错误一般是非法访问内存，常见于`malloc()`之类的动态内存分配，申请内存之后没有将指针赋值为`NULL`。或者数组访问越界，或者栈中定义过大的数组，导致栈空间不足；

排查之后发现，在字符串push操作的返回的指针越界导致的.之前为`ret = c->stack + c->size` `c->top += c->size()`,而`c->size`初始值为256,后面使用时发生错误，此处为逻辑错误，没有思考清楚，应该改为：

```cpp
ret = c->stack + c->top; /* 这里【注意】加上c->top,每次指向要下一个要push位置的指针 */
c->top += size; /* top栈顶指针移动size大小 */
```

3、不合法的字符串
```cpp
unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
```
当中空缺的 %x22 是双引号，%x5C 是反斜线，都已经处理。所以不合法的字符是 %x00 至 %x1F。我们简单地在 default 里处理：

4、性能优化的思考
摘自原github：

https://github.com/miloyip/json-tutorial/blob/master/tutorial03_answer/tutorial03_answer.md

1、如果整个字符串都没有转义符，我们不就是把字符复制了两次？第一次是从 json 到 stack，第二次是从 stack 到 v->u.s.s。我们可以在 json 扫描 '\0'、'\"' 和 '\\' 3 个字符（ ch < 0x20 还是要检查），直至它们其中一个出现，才开始用现在的解析方法。这样做的话，前半没转义的部分可以只复制一次。缺点是，代码变得复杂一些，我们也不能使用 lept_set_string()。

2、对于扫描没转义部分，我们可考虑用 SIMD 加速，如 RapidJSON 代码剖析（二）：使用 SSE4.2 优化字符串扫描 的做法。这类底层优化的缺点是不跨平台，需要设置编译选项等。

3、在 gcc/clang 上使用 __builtin_expect() 指令来处理低概率事件，例如需要对每个字符做 LEPT_PARSE_INVALID_STRING_CHAR 检测，我们可以假设出现不合法字符是低概率事件，然后用这个指令告之编译器，那么编译器可能可生成较快的代码。然而，这类做法明显是不跨编译器，甚至是某个版本后的 gcc 才支持。

# Part04 Unicode
## 1.Unicode 
之前已经能解析「一般」的 JSON 字符串，仅仅没有处理 \uXXXX 这种转义序列。

**基本概念**

ASCII，它是一种字符编码，把 128 个字符映射至整数 0 ~ 127。例如，1 → 49，A → 65，B → 66 等等。这种 7-bit 字符编码系统非常简单，在计算机中以一个字节存储一个字符。然而，它仅适合美国英语，甚至一些英语中常用的标点符号、重音符号都不能表示，无法表示各国语言，特别是中日韩语等表意文字。

为了统一编码结构:

这些字符被收录为统一字符集（Universal Coded Character Set, UCS），每个字符映射至一个整数码点（code point），码点的范围是 0 至 0x10FFFF，码点又通常记作 U+XXXX，当中 XXXX 为 16 进位数字。例如 劲 → U+52B2、峰 → U+5CF0。很明显，UCS 中的字符无法像 ASCII 般以一个字节存储。

因此，Unicode 还制定了各种储存码点的方式，这些方式称为 Unicode 转换格式（Uniform Transformation Format, UTF）。现时流行的 UTF 为 UTF-8、UTF-16 和 UTF-32。每种 UTF 会把一个码点储存为一至多个编码单元（code unit）。例如 UTF-8 的编码单元是 8 位的字节、UTF-16 为 16 位、UTF-32 为 32 位。除 UTF-32 外，UTF-8 和 UTF-16 都是可变长度编码。

UTF-8 成为现时互联网上最流行的格式，有几个原因：

1.它采用字节为编码单元，不会有字节序（endianness）的问题。

2.每个 ASCII 字符只需一个字节去储存。

3.如果程序原来是以字节方式储存字符，理论上不需要特别改动就能处理 UTF-8 的数据。

## 2.需求
JSON库支持UTF-8 ： 实现 JSON 库所需的字符编码处理功能。
（1）对于非转义（unescaped）的字符，只要它们不少于 32（0 ~ 31 是不合法的编码单元），我们可以直接复制至结果。

对于 JSON字符串中的 \uXXXX 是以 16 进制表示码点 U+0000 至 U+FFFF，我们需要

（2）解析 4 位十六进制整数为码点；由于字符串是以 UTF-8 存储，我们要把这个码点编码成 UTF-8。

4 位的 16 进制数字只能表示 0 至 0xFFFF，但之前我们说 UCS 的码点是从 0 至 0x10FFFF，那怎么能表示多出来的码点？

其实，U+0000 至 U+FFFF 这组 Unicode 字符称为**基本多文种平面（basic multilingual plane, BMP）**，还有另外 16 个平面。那么 BMP 以外的字符，JSON 会使用代理对（surrogate pair）表示 \uXXXX\uYYYY。在 BMP 中，保留了 2048 个代理码点。如果第一个码点是 U+D800 至 U+DBFF，我们便知道它的代码对的高代理项（high surrogate），之后应该伴随一个 U+DC00 至 U+DFFF 的低代理项（low surrogate）。然后，我们用下列公式把代理对 (H, L) 变换成真实的码点：

```cpp
codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)
```

举个例子，高音谱号字符 𝄞 → U+1D11E 不是 BMP 之内的字符。在 JSON 中可写成转义序列 \uD834\uDD1E，我们解析第一个 \uD834 得到码点 U+D834，我们发现它是 U+D800 至 U+DBFF 内的码点，所以它是高代理项。然后我们解析下一个转义序列 \uDD1E 得到码点 U+DD1E，它在 U+DC00 至 U+DFFF 之内，是合法的低代理项。我们计算其码点：

```c
H = 0xD834, L = 0xDD1E
codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)
          = 0x10000 + (0xD834 - 0xD800) × 0x400 + (0xDD1E − 0xDC00)
          = 0x10000 + 0x34 × 0x400 + 0x11E
          = 0x10000 + 0xD000 + 0x11E
          = 0x1D11E
```
这样就得出这转义序列的码点，然后我们再把它编码成 UTF-8。如果只有高代理项而欠缺低代理项，或是低代理项不在合法码点范围，我们都返回 LEPT_PARSE_INVALID_UNICODE_SURROGATE 错误。如果 \u 后不是 4 位十六进位数字，则返回 LEPT_PARSE_INVALID_UNICODE_HEX 错误。

## 3、UTF-8编码
![requirement](https://github.com/miloyip/json-tutorial/raw/master/tutorial04/images/Utf8webgrowth.png)

UTF-8的编码方式：

UTF-8 的编码单元为 8 位（1 字节），每个码点编码成 1 至 4 个字节。它的编码方式很简单，按照码点的范围，把码点的二进位分拆成 1 至最多 4 个字节：

![image-20210916161611179](https://i.loli.net/2021/09/16/POM3LXS9HwWztui.png)

这个编码方法的好处之一是，码点范围 U+0000 ~ U+007F 编码为一个字节，与 ASCII 编码兼容。这范围的 Unicode 码点也是和 ASCII 字符相同的。因此，一个 ASCII 文本也是一个 UTF-8 文本。

我们举一个例子解析多字节的情况，欧元符号 € → U+20AC：

U+20AC 在 U+0800 ~ U+FFFF 的范围内，应编码成 3 个字节。

U+20AC 的二进位为 10000010101100

3 个字节的情况我们要 16 位的码点，所以在前面补两个 0，成为 0010000010101100

按上表把二进位分成 3 组：0010, 000010, 101100

加上每个字节的前缀：11100010, 10000010, 10101100

用十六进位表示即：0xE2, 0x82, 0xAC

对于这个例子的范围，对应的C代码是这样的：
```cpp
if (u >= 0x0800 && u <= 0xFFFF) {
    OutputByte(0xE0 | ((u >> 12) & 0xFF)); /* 0xE0 = 11100000 */
    OutputByte(0x80 | ((u >>  6) & 0x3F)); /* 0x80 = 10000000 */
    OutputByte(0x80 | ( u        & 0x3F)); /* 0x3F = 00111111 */
}

```

## 4.实现\uXXXX的解析
在转义字符解析的时候，switch case的时候加入'u'的处理

```cpp
static int lept_parse_string(lept_context* c, lept_value* v) {
    unsigned u;
    /* ... */
    for (;;) {
        char ch = *p++;
        switch (ch) {
            /* ... */
            case '\\':
                switch (*p++) {
                    /* ... */
                    case 'u':
                        if (!(p = lept_parse_hex4(p, &u)))
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                        /* \TODO surrogate handling */
                        lept_encode_utf8(c, u);
                        break;
                    /* ... */
                }
            /* ... */
        }
    }
}
```

**处理思路**为：遇到 \u 转义时，调用 lept_parse_hex4() 解析 4 位十六进数字，存储为码点 u。这个函数在成功时返回解析后的文本指针，失败返回 NULL。如果失败，就返回 LEPT_PARSE_INVALID_UNICODE_HEX 错误。最后，把码点编码成 UTF-8，写进缓冲区.


# Part06 解析数组
## 1.json数组
解析复合的数据类型。一个json数组可以包含零至多个元素，这些元素也可以是数组类型。
JSON数组的语法：

```cpp
array = %x5B ws [ value *( ws %x2C ws value ) ] ws %x5D
```
当中，`%x5B` 是左中括号` [`，`%x2C` 是逗号 `,`，`%x5D` 是右中括号 `]` ，`ws `是空白字符。一个数组可以包含零至多个值，以逗号分隔，例如 `[]、[1,2,true]、[[1,2],[3,4],"abc"] `都是合法的数组。但注意 JSON 不接受末端额外的逗号，例如 `[1,2,]` 是不合法的（许多编程语言如 C/C++、Javascript、Java、C# 都容许数组初始值包含末端逗号）。

## 2.数据结构
存储JSON数组类型的数据结构。
JSON数组存储零至多个元素，
（1)**数组**
优点：能以$O(1)$用索引访问元素，内存布局紧凑，节省内存、高缓存一致性；
缺点：不能快速插入元素，而且解析JSON数组的时候，不知道分配多大数组。

（2）**链表**
优点:可以优点是可以快速地插入元素（开端、末端或中间），但需要以$O(n)$取值，
缺点：相对于数组而言，链表在存储每个元素时有额外的内存开销（存储下一节点的指针），而且遍历的时候元素可能不连续，令缓存不命中的机会上升。

综合考虑，考虑以数组。
在lept_value中加入结构体array数组；
```cpp
typedef struct lept_value lept_value; /* 用到自己，前向声明 */
struct lept_value {
    union {
        struct { lept_value* e; size_t size; }array; /* array */
        struct { char* s; size_t len; }str;
        double num;
    }u;
    lept_type type;
};
```
## 3.解析过程
解析json字符串时，因为开始时不知道字符串的长度，但是需要转义，故可以用一个临时缓冲区去存储解析后的结构。
实现方式：动态增长的堆栈，不断地压入字符，最后一次性把整个字符串弹出。

在实现解析JSON数组时候，也可以用同样的方式，而且可以用同一个堆栈！

我们只需要把每个解析好的元素压入堆栈，解析到数组结束时，再一次性把所有元素弹出，复制至新分配的内存之中。

可以把json当作一棵树的数据结构，JSON字符串是叶子节点，JSON数组是中间节点。在叶子节点的解析函数中，
怎么使用那个堆栈都没有问题，最后还原即可。但是对于数组这样的中间节点，共用这个堆栈没有问题吗？

答案是：只要在解析函数结束时还原堆栈的状态，就没有问题。

案例解析可以访问原网站：https://github.com/miloyip/json-tutorial/blob/master/tutorial05/tutorial05.md

## 4.遇到的问题：

**1.打印 `size_t`时的问题**

```cpp
 In function ‘test_parse_array’:
/home/zzc/myProject/test.c:21:29: warning: ISO C90 does not support the ‘z’ gnu_printf length modifier [-Wformat=]
   21 |             fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
```

这是由于在打印size_t类型时遇到的错误；将`%zu`改为`%ld`

**2.栈内存空间没有释放**

```cpp
leptjson_test: /home/zzc/myProject/leptjson.c:388: lept_parse: Assertion `c.top == 0' failed.
[1]    1622855 abort (core dumped)  ./leptjson_test
```
当错误发生时，仍然有一些临时值在堆栈里，既没有放进数组，也没有被释放。修改 `lept_parse_array()`

当遇到错误时，从堆栈中弹出并释放那些临时值，然后才返回错误码。

**解决思路** :

在遇到错误的时候，利用break跳出循环，在外面用`lept_free`释放从堆栈弹出的值，然后才返回错误码；
```cpp
static int lept_parse_array(lept_context* c, lept_value* v) {
    /* ... */
    for (;;) {
        /* ... */
        if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK)
            break;
        /* ... */
        else {
            ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    /* 弹出栈中的元素，并且释放栈空间 */
    for (i = 0; i < size; i++)
        lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));
    return ret;
}
```
**3.内存泄漏的问题**

用内存泄漏工具检查出有两处内存泄漏；

![image-20210917145101762](https://i.loli.net/2021/09/17/FKdSsYtLUENulfr.png)

**原因**：在malloc()之后没有对应的free()。
![image-20210917145413783](https://i.loli.net/2021/09/17/f6qE5GNDLcPkrY9.png)

**解决办法**：在 lept_free()，当值被释放时，该值拥有的内存也在那里释放。之前字符串的释放也是放在这里，但是对于JSON数组，应该先把数组内的元素通过递归调用 lept_free() 释放，然后才释放本身的 v->u.a.e：

**4.BUG的解释**
```cpp
    for (;;) {
        lept_value e;
        lept_init(&e);
        if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK)
            return ret;
        memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
        size++;
```
将上面的代码写为：
```cpp
    for (;;) {
        /* bug! */
        lept_value* e = lept_context_push(c, sizeof(lept_value));
        lept_init(e);
        size++;
        if ((ret = lept_parse_value(c, e)) != LEPT_PARSE_OK)
            return ret;
        /* ... */
    }
```
第二种写法是，先用指针e指向栈里面push一个`lept_value大小`的位置，然后再解析`lept_context c`，将解析结果存入指针e指向的位置。

对比与第一种写法，有什么区别呢？

第一种写法是先把c解析存入一个临时变量`lept_value e`里面，然后把e 拷贝入c的栈空间里面。

第二种写法中我们把这个指针调用 lept_parse_value(c, e)，这里会出现问题，因为 lept_parse_value() 及之下的函数都需要调用 lept_context_push()，而 lept_context_push() 在发现栈满了的时候会用 realloc() 扩容。这时候，我们上层的 e 就会失效，变成一个悬挂指针（dangling pointer），而且 lept_parse_value(c, e) 会通过这个指针写入解析结果，造成非法访问。

**函数编码规范**： 考虑变量的生命周期，特别是指针的生命周期尤其重要。

# Part6 解析对象

## 1.JSON对象
JSON对象主要是以`{} (U+007B)、U+007D)`表四，另外JSON对象由对象成员（member）组成，而JSON数组由JSON值组成。

所谓对象成员，就是键值对，键必须为JSON字符串，然后值是JSON值，中间以`:`分割，完整语法如下：

```cpp
member = string ws %x3A ws value
object = %x7B ws [ member *( ws %x2C ws member ) ] ws %x7D
```
## 2.数据结构
(1)动态数组：可扩展的数组，例如c++的vector；

(2)有序动态数组：和动态数组一样，但保证元素已经排序，可以用二分查找查询成员。

(3)平衡树：平衡二叉树可有序地遍历成员，例如红黑树和c++的map；

(4)哈希表：哈希函数实现 $O(1)$ 的查询，unordered_map;

几种数据结构的对比：

![image-20210918140405522](https://i.loli.net/2021/09/18/FEgSn2NyR4diOVq.png)

## 3.重构字符串解析函数
将函数重构，分成两部分，主要是为了将c解析成

```cpp
/* 解析 JSON 字符串，把结果写入 str 和 len */
/* str 指向 c->stack 中的元素，需要在 c->stack  */
static int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {
    /* \todo */
}

static int lept_parse_string(lept_context* c, lept_value* v) {
    int ret;
    char* s;
    size_t len;
    if ((ret = lept_parse_string_raw(c, &s, &len)) == LEPT_PARSE_OK)
        lept_set_string(v, s, len);
    return ret;
}

```
## 4.解析object的过程

（1）利用重构的`lept_parse_string_raw`解析键的字符串。若字符串解析成功，它会把结果存储在我们的栈之中，需要把结果写入临时 lept_member 的 k 和 klen 字段中：
```cpp
static int lept_parse_object(lept_context* c, lept_value* v) {
    size_t i, size;
    lept_member m;
    int ret;
    /* ... */
    m.k = NULL;
    size = 0;
    for (;;) {
        char* str;
        lept_init(&m.v);
        /* 1. parse key */
        if (*c->json != '"') {
            ret = LEPT_PARSE_MISS_KEY;
            break;
        }
        if ((ret = lept_parse_string_raw(c, &str, &m.klen)) != LEPT_PARSE_OK)
            break;
        memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen);
        m.k[m.klen] = '\0';
        /* 2. parse ws colon ws */
        /* ... */
    }
    /* 5. Pop and free members on the stack */
    /* ... */
}

```

（2）解析冒号，去除冒号前后的空白字符。
```cpp
        /* 2. parse ws colon ws */
        lept_parse_whitespace(c);
        if (*c->json != ':') {
            ret = LEPT_PARSE_MISS_COLON;
            break;
        }
        c->json++;
        lept_parse_whitespace(c);
```

（3）解析任意的JSON值，递归调用`lept_Parse_value` 把结果写入临时的`lept_member`的v字段，然后把整个lept_member压入栈中。

```cpp
        /* 3. parse value */
        if ((ret = lept_parse_value(c, &m.v)) != LEPT_PARSE_OK)
            break;
        memcpy(lept_context_push(c, sizeof(lept_member)), &m, sizeof(lept_member));
        size++;
        m.k = NULL; /* ownership is transferred to member on stack */

```

**注意**：如果缺少冒号或是这里解析值失败，在函数返回前我们要释放 m.k。如果我们成功地解析整个成员，那么就要把 m.k 设为空指针，其意义是说明该键的字符串的拥有权已转移至栈，之后如遇到错误，我们不会重覆释放栈里成员的键和这个临时成员的键。

（4）解析逗号或者右括号

遇上右花括号的话，当前的 JSON 对象就解析完结了，我们把栈上的成员复制至结果，并直接返回：

```cpp
        /* 4. parse ws [comma | right-curly-brace] ws */
        lept_parse_whitespace(c);
        if (*c->json == ',') {
            c->json++;
            lept_parse_whitespace(c);
        }
        else if (*c->json == '}') {
            size_t s = sizeof(lept_member) * size;
            c->json++;
            v->type = LEPT_OBJECT;
            v->u.o.size = size;
            memcpy(v->u.o.m = (lept_member*)malloc(s), lept_context_pop(c, s), s);
            return LEPT_PARSE_OK;
        }
        else {
            ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }

```

（5）释放资源

当 for (;;) 中遇到任何错误便会到达这第 5 步，要释放临时的 key 字符串及栈上的成员：
```cpp
    /* 5. Pop and free members on the stack */
    free(m.k);
    for (i = 0; i < size; i++) {
        lept_member* m = (lept_member*)lept_context_pop(c, sizeof(lept_member));
        free(m->k);
        lept_free(&m->v);
    }
    v->type = LEPT_NULL;
    return ret;
```
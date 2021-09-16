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
#include "leptjson.h"
#include <assert.h> /*assert()*/
#include <stdlib.h> /* NULL  malloc realloc*/
#include <errno.h> /*errno*/
#include <math.h> /* HUGE_VAL */
#include <string.h> /* memcpy() */

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++;} while(0)
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
/* 返回值为指针：lept_context 栈的指针加上栈顶元素的位置，该位置的值附上值ch*/
#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)


/*
首先为了减少解析函数之间传递多个参数，我们把这些数据都放进一个 lept_context 结构体
*/
typedef struct 
{
    const char* json;
    char* stack; /* 动态解析字符串的临时缓冲区，因为是先进后出，所以才用栈的结构 */
    size_t size,top; /* size为当前堆栈的容量，top是栈顶的位置 ，扩展stack,不要把top用指针形式存储 */
}lept_context;

/*
n ➔ null
t ➔ true
f ➔ false
" ➔ string
0-9/- ➔ number
[ ➔ array
{ ➔ object
*/

/* 向栈里面push元素 ；
这个堆栈是以字节储存的。每次可要求压入任意大小的数据，它会返回数据起始的指针*/
static void* lept_context_push(lept_context* c,size_t size){
    void* ret;
    assert(size > 0);
    /* 初始化栈大小以及扩容 */
    if(c->top + size >= c->size){
        /* 如果为零，先初始化 */
        if(c->size == 0) {
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        }
        while (c->top + size > c->size)
        {
            c->size += c->size >> 1; /* 1.5倍容量扩容 */
        }
        c->stack = (char*)realloc(c->stack ,c->size); /* realloc重新分配内存，不用为第一次内存分配特殊处理 */
    }
    ret = c->stack + c->top; /* 这里【注意】加上c->top,每次指向要下一个要push位置的指针 */
    c->top += size; /* top栈顶指针移动size大小 */
    return ret;
}


static void* lept_context_pop(lept_context* c,size_t size){
    assert(c->top >= size); /* 确保有空间可以弹出 */
    return c->stack + (c->top -= size); /* c->stack 加上栈顶指针往下移size大小 */
}

static void lept_parse_whitespace(lept_context* c){
    const char *p = c->json;
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

/* 重构代码:合并lept_parse_null(),lept_parse_true(),lept_parse_false*/
#if 0
/* null = "null"  */
static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n'); /* 使用宏检测是否是字符'n',并且跳到下一个字符；*/
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}


static int lept_parse_true(lept_context* c ,lept_value* v){
    EXPECT(c,'t');
    if(c->json[0] != 'r' || c->json[1] !='u'||c->json[2] != 'e'){
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += 3;
    v->type =LEPT_TRUE;
    return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c ,lept_value* v){
    EXPECT(c,'f');
    if(c->json[0] != 'a' || c->json[1] !='l'||c->json[2] != 's' ||c->json[3] != 'e'){
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += 4;
    v->type =LEPT_FALSE;
    return LEPT_PARSE_OK;
}
#endif

static int lept_parse_literal(lept_context* c,lept_value *v,const char* literal,lept_type type){
    size_t  i;
    EXPECT(c,literal[0]);
    for(i = 0 ;literal[i + 1]; i++){
        if(c->json[i] != literal[i + 1]){
            return LEPT_PARSE_INVALID_VALUE;
        }
    }
    c->json += i; /*加上literal的size大小*/
    v->type = type;
    return LEPT_PARSE_OK;
}

/* 修正，按 JSON number 的语法在 lept_parse_number() 校验，不符合标准的程况返回 LEPT_PARSE_INVALID_VALUE 错误码。*/
#if 0
static int lept_parse_number(lept_context* c, lept_value* v){
    char *end;
    /*\TODO validate number */
    v->num = strtod(c->json ,&end);
    if(c->json == end){
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json = end;
    v->type =LEPT_NUMBER;
    return LEPT_PARSE_OK;
}
#endif

static int lept_parse_number(lept_context* c,lept_value* v){
    const char* p =c->json;
    if(*p == '-') p++;
    if(*p == '0') p++;
    else{
        if(!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p) ; p++);
    }
    if(*p == '.'){
        p++;
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for(p++;ISDIGIT(*p);p++);
    }
    if(*p == 'e' || *p == 'E'){
        p++;
        if(*p == '+' || *p == '-') p++;
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for(p++ ;ISDIGIT(*p); p++);
    }
    errno =  0;
    v->u.num = strtod(c->json ,NULL);
    /* 判断数字是否过大 */
    if(errno == ERANGE && (v->u.num == HUGE_VAL || v->u.num == -HUGE_VAL)){
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    v->type = LEPT_NUMBER;
    c->json = p; /* 记得记录当前的位置 */
    return LEPT_PARSE_OK;
}

/* 
解析4位16进制，存储为码点u；若成功，返回解析后的文本指针，失败返回NULL。 
传入参数：p，第一个要转义字符的指针。
传出参数：u，解析之后的二进制。
返回值：解析完4位后指针位置*/
static const char* lept_parse_hex4(const char* p,unsigned* u){
    int i;
    *u = 0;
    for(i = 0 ;i < 4 ;i++){
        char ch = *p++;
        *u <<= 4;
        if(ch >= '0' && ch <= '9')      *u |= ch - '0';
        else if(ch >= 'A' && ch <= 'F') *u |= ch - ('A' - 10);
        else if(ch >= 'a' && ch <= 'f') *u |= ch - ('a' - 10);
        else return NULL;
    }
    return p;
}

/* 将码点u编码为UTF-8之后存入字符串的缓冲区栈空间里面 */
/* &1保持位上数字不变，|1 取并集 */
static void lept_encode_utf8(lept_context* c, unsigned u){
    if(u <= 0x7F){
        PUTC(c,u & 0xFF);
    }else if(u <= 0x7FF){
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | ( u       & 0x3F));
    }else if( u <= 0xFFFF){
        PUTC(c, 0xE0 | ((u >> 12) & (0xFF)));
        PUTC(c, 0x80 | ((u >>  6) & (0x3F)));
        PUTC(c, 0X80 | ( u        &  0X3F));

    }else{
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
}

#define STRING_ERROR(ret) do{ c->top = head; return ret;}while(0)

static int lept_parse_string(lept_context* c,lept_value *v){
    size_t head = c->top,len; /* 备份栈顶 */
    unsigned u ,u2;
    const char* p;
    EXPECT(c,'\"');/* 字符串是以"开始的 */
    p = c->json;
    for(; ;){
        char ch = *p++;
        switch (ch){
            case '\"':
                len = c->top - head ; /* 字符入栈之后栈顶指针会增加，减去原来的栈顶，得到字符串的长度 */
                lept_set_string(v,(const char*)lept_context_pop(c,len),len); /* 将字符串 c拷贝到字符串v里面*/
                c->json = p; 
                return LEPT_PARSE_OK;
            case '\0':
                STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
            /* 新增转义字符的解析 */
            case '\\':
                switch (*p++)
                {
                case '\"':  PUTC(c,'\"');break;
                case '\\':  PUTC(c,'\\');break;
                case '/':   PUTC(c, '/');break;
                case 'b':   PUTC(c,'\b');break;
                case 'f':   PUTC(c, '\f'); break;
                case 'n':   PUTC(c, '\n'); break;
                case 'r':   PUTC(c, '\r'); break;
                case 't':   PUTC(c, '\t'); break;
                /* unicode 解析：先调用lept_parse_hex4解析为4位十六进制数字，存储为码点u,然后将码点编码成UTF-8，写进缓冲区 */
                case 'u':
                    if(!(p = lept_parse_hex4(p,&u))){
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                    }
                    /* 对代理对的处理 */
                /* JSON 会使用代理对（surrogate pair）表示 \uXXXX\uYYYY 高代理项后面接着低代理项*/
                /* 如果第一个码点是 U+D800 至 U+DBFF，我们便知道它的代码对的高代理项（high surrogate），
                   之后应该伴随一个 U+DC00 至 U+DFFF 的低代理项（low surrogate）。
                   然后，我们用下列公式把代理对 (H, L) 变换成真实的码点： 
                   codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)*/
                    if(u >= 0xD800 && u <= 0xDBFF){
                        if(*p++ != '\\'){
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                        }
                        if(*p++ != 'u'){
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                        }
                        if(!(p = lept_parse_hex4(p, &u2))){
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                        }
                        if(u2 < 0xDC00 || u2 > 0xDFFF){
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                        }
                        u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                    }
                    lept_encode_utf8(c,u);
                    break;
                default:
                    STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
                }
                break;
            default:
                /* ASCII可显示字符 最低从32（空格）开始 */
                if((unsigned char)(ch) < 0x20) {
                    STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
                }
                PUTC(c, ch); /* 将字符入栈 */
                break;
        }
    }
}

void lept_free(lept_value* v){
    size_t i;
    assert(v != NULL);
    switch (v->type)
    {
    case LEPT_STRING:
        free(v->u.str.s);
        break;
    case LEPT_ARRAY:
        for(i = 0 ;i < v->u.array.size;i++){
            lept_free(&v->u.array.e[i]);
        }
        free(v->u.array.e);
        break;
    default:
        break;
    }
    v->type = LEPT_NULL;
}

/* 将s 拷贝到v.u.str中去  */
void lept_set_string(lept_value* v,const char* s,size_t len){
    assert(v != NULL && (s != NULL || len == 0));
    lept_free(v);
    v->u.str.s = (char*)malloc(len + 1);
    memcpy(v->u.str.s ,s ,len);
    v->u.str.s[len] = '\0';
    v->u.str.len = len ;
    v->type = LEPT_STRING;
}

const char* lept_get_string(const lept_value* v){
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.str.s;
}

/* 获取字符串的长度 */
size_t lept_get_string_length(const lept_value* v){
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.str.len;
}

static int lept_parse_value(lept_context* c,lept_value* v); /* 前向声明 */

static int lept_parse_array(lept_context* c ,lept_value* v){
    size_t i ,size = 0;
    int ret;
    EXPECT(c,'[');
    lept_parse_whitespace(c);
    if(*c->json == ']'){
        c->json++;
        v->type = LEPT_ARRAY;
        v->u.array.size = 0;
        v->u.array.e = NULL;
        return LEPT_PARSE_OK;
    }
    for(;;){
        /* 临时变量 */
        lept_value e;
        lept_init(&e);
        /* 解析c，存入临时值e中*/
        ret = lept_parse_value(c, &e);
        if(ret != LEPT_PARSE_OK){
            break;
        }
        /* 把e拷贝进入c的那个指针中 */
        memcpy(lept_context_push(c, sizeof(lept_value)), &e,sizeof(lept_value));
        size++;
        lept_parse_whitespace(c);
        if(*c->json == ','){
            c->json++;
            lept_parse_whitespace(c);
        }else if(*c->json == ']'){
            c->json++;
            v->type = LEPT_ARRAY;
            v->u.array.size = size;
            size *= sizeof(lept_value);
            memcpy(v->u.array.e = (lept_value* )malloc(size), lept_context_pop(c,size),size);
            return LEPT_PARSE_OK;
        }else{
            ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    /* 当遇到错误时，上面break跳出，弹出栈的元素，释放栈空间 */
    for( i = 0 ; i < size ; i++){
        lept_free((lept_value*)lept_context_pop(c,sizeof(lept_value)));
    }
    return ret;
}


static int lept_parse_value(lept_context* c ,lept_value* v){
    switch (*c->json)
    {
    case 't':
        return lept_parse_literal(c,v,"true" ,LEPT_TRUE);
    case 'f':
        return lept_parse_literal(c,v,"false",LEPT_FALSE);
    case 'n':
        return lept_parse_literal(c,v,"null",LEPT_NULL);
    case '\0':
        return LEPT_PARSE_EXPECT_VALUE;
    case '[':
        return lept_parse_array(c,v);
    case '"':
        return lept_parse_string(c,v);
    default:
        return lept_parse_number(c,v);
    }
}

int lept_parse(lept_value* v,const char* json){
    lept_context c;
    int ret ;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    lept_init(v);
    lept_parse_whitespace(&c);
    ret = lept_parse_value(&c,v);
    if(ret == LEPT_PARSE_OK){
        lept_parse_whitespace(&c);
        if(*c.json != '\0'){
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0) ;/* 确保所有元素均弹出栈*/
    free(c.stack);
    return ret;
}

lept_type lept_get_type(const lept_value* v){
    assert(v != NULL);
    return v->type;
}

/* 获取v中num的值 */
double lept_get_number(const  lept_value *v){
    assert(v !=NULL && v->type == LEPT_NUMBER);
    return v->u.num;
}

/* 将数字num写入v中 */
void lept_set_number(lept_value* v, double num){
    lept_free(v);
    v->u.num = num;
    v->type = LEPT_NUMBER;
}

int lept_get_boolean(const lept_value* v){
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

/* 将b写入v中 */
void lept_set_boolean(lept_value *v ,int b){
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE; /* b为1设置为true,0为false */
}

/* 访问JSON数组元素的个数 */
size_t lept_get_array_size(const lept_value* v){
    assert(v !=NULL && v->type == LEPT_ARRAY);
    return v->u.array.size;
}

/* 访问json数组的第 index个元素 */
lept_value* lept_get_array_element(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    assert(index < v->u.array.size);
    return &v->u.array.e[index];
}

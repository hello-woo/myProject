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
    ret = c->stack + c->size; /* 将指针前移size大小 */
    c->top += c->size; /* top栈顶指针移动size大小 */
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

static int lept_parse_string(lept_context* c,lept_value *v){
    size_t head = c->top,len; /* 备份栈顶 */
    const char* p;
    EXPECT(c,'\"');/* 字符串是以"开始的 */
    p = c->json;
    for(; ;){
        char ch = *p++;
        switch (ch)
        {
        case '\"':
            len = c->top - head ; /* 字符入栈之后栈顶指针会增加，减去原来的栈顶，得到字符串的长度 */
            lept_set_string(v,(const char*)lept_context_pop(c,len),len); /* 将字符串 c拷贝到字符串v里面*/
            c->json = p; /*  */
            return LEPT_PARSE_OK;
        case '\0':
            c->top = head;
            return LEPT_PARSE_MISS_QUOTATION_MARK;
        default:
            PUTC(c, ch); /* 将字符入栈 */
            break;
        }
    }
}

void lept_free(lept_value* v){
    assert(v != NULL);
    if(v->type == LEPT_STRING){
        free(v->u.str.s);
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

static int lept_parse_value(lept_context* c ,lept_value* v){
    switch (*c->json)
    {
    case 't':
        return lept_parse_literal(c,v,"true" ,LEPT_TRUE);
        break;
    case 'f':
        return lept_parse_literal(c,v,"false",LEPT_FALSE);
        break;
    case 'n':
        return lept_parse_literal(c,v,"null",LEPT_NULL);
        break;
    case '\0':
        return LEPT_PARSE_EXPECT_VALUE;
        break;
    case '"':
        return lept_parse_string(c,v);
        break;
    default:
        return lept_parse_number(c,v);
        break;
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
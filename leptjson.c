#include "leptjson.h"
#include <assert.h> /*assert()*/
#include <stdlib.h> /*NULL*/
#include <errno.h> /*errno*/
#include <math.h> /* HUGE_VAL */

#define EXPECT(c, ch) do { assert(*c->json == (ch)); c->json++;} while(0)
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')


/*
首先为了减少解析函数之间传递多个参数，我们把这些数据都放进一个 lept_context 结构体
*/
typedef struct 
{
    const char* json;
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
    v->num = strtod(c->json ,NULL);
    /* 判断数字是否过大 */
    if(errno == ERANGE && (v->num == HUGE_VAL || v->num == -HUGE_VAL)){
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    v->type = LEPT_NUMBER;
    c->json = p; /* 记得记录当前的位置 */
    return LEPT_PARSE_OK;
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
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    ret = lept_parse_value(&c,v);
    if(ret == LEPT_PARSE_OK){
        lept_parse_whitespace(&c);
        if(*c.json != '\0'){
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v){
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const  lept_value *v){
    assert(v !=NULL && v->type == LEPT_NUMBER);
    return v->num;
}

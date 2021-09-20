#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stddef.h> /* size_t */

/*json中有6中数据类型，声明一个枚举类型*/
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;


/*需要实现一个树的数据结构，每个节点使用 lept_value 结构体表示，我们会称它为一个 JSON 值（JSON value）*/
/*64位系统 32字节 */
#if 0
typedef struct {
    char* s;
    size_t len;
    double num; /*数字的表示方式，采用双精度浮点数*/
    lept_type type;
}lept_value2;
#endif

/* 由于JSON数组定义是用到了lept_value，结构体内使用自身类型的指针，我们必须前向声明此类型 */
typedef struct lept_value lept_value;
typedef struct lept_member lept_member;

/* 设置为联合体是因为节省内存 24字节 */
struct lept_value{
    union 
    {
        struct {
            lept_member* m;
            size_t size;
        }obj;
        struct {
            lept_value* e;  /* 数组元素*/
            size_t size;    /* 数组元素个数 */
        }array;
        struct{
            char *s;
            size_t len;
        }str;       /* string ,字符串以及长度 */
        double num; /* 数字 number */
    }u;
    lept_type type;
};

/* 成员结构是一个lept_value值加上键的字符串。 */
struct lept_member
{
    char* k;            /* 键值，是一个字符串 */
    size_t kStringLen; /* 保留字符串的长度，因为字符串本身可能为零*/
    lept_value v;
};

/*lept_parse函数的返回值是一下枚举值，无错误会返回LEPT_PARSE_OK；*/
enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG,
    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR,
    LEPT_PARSE_INVALID_UNICODE_HEX,
    LEPT_PARSE_INVALID_UNICODE_SURROGATE,
    LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    LEPT_PARSE_MISS_KEY,
    LEPT_PARSE_MISS_COLON,
    LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

#define lept_init(v) do{ (v)->type = LEPT_NULL;} while(0)

/*解析json ,
传入参数 json:传入的 JSON 文本是一个 C 字符串（空结尾字符串／null-terminated string），
由于我们不应该改动这个输入字符串，所以使用 const char* 类型。
传出参数v:树形结构的根节点指针，是由使用方负责分配的，

用法：
lept_value v;
const char json[] = ...;
int ret = lept_parse(&v,json)*/
int lept_parse(lept_value* v, const char* json);

/*现时我们只需要一个访问结果的函数，就是获取其类型*/
lept_type lept_get_type(const lept_value* v);


/* 获取json中的数字*/
double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v ,double num);

void lept_free(lept_value* v);
#define lept_set_null(v) lept_free(v);


int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value*v ,int b);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v,const char* s,size_t len);

size_t lept_get_array_size(const lept_value* v);
lept_value* lept_get_array_element(const lept_value* v,size_t index);

size_t lept_get_object_size(const lept_value* v);
const char* lept_get_object_key(const lept_value* v,  size_t index);
size_t lept_get_object_key_length(const lept_value*v, size_t index);
lept_value* lept_get_object_value(const lept_value* v,size_t index);

char* lept_stringify(const lept_value* v,size_t* length);

#endif /* LEPTJSON_H__ */
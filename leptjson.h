#ifndef LEPTJSON_H__
#define LEPTJSON_H__

/*json中有6中数据类型，声明一个枚举类型*/
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;


/*需要实现一个树的数据结构，每个节点使用 lept_value 结构体表示，我们会称它为一个 JSON 值（JSON value）*/
typedef struct {
    lept_type type;
}lept_value;

/*lept_parse函数的返回值是一下枚举值，无错误会返回LEPT_PARSE_OK；*/
enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR
};

/*解析json ,
传入参数 json:传入的 JSON 文本是一个 C 字符串（空结尾字符串／null-terminated string），
由于我们不应该改动这个输入字符串，所以使用 const char* 类型。
传出参数v:树形结构的根节点指针，是由使用方负责分配的，

用法：
lept_value v;
const char json[] = ...;
int ret = lept_parse(&v,json)*/
int lept_parse(lept_value* v, const char* json);


/*
现时我们只需要一个访问结果的函数，就是获取其类型
*/
lept_type lept_get_type(const lept_value* v);

#endif /* LEPTJSON_H__ */
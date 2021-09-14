cmake_minimum_required (VERSION 2.6)
project (leptjson_test C)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall")
endif()
SET(LIB_SRC leptjson.c)
SET(APP_SRC test.c)
add_library(leptjson ${LIB_SRC})  #add_library 告诉生成一个库文件
add_executable(leptjson_test ${APP_SRC}) #add_executable 告诉工程生成一个工程文件
target_link_libraries(leptjson_test leptjson) #将生成的leptjson库链接到leptjson_test中
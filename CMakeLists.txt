cmake_minimum_required(VERSION 3.22)
project(cjson)

set(CMAKE_CXX_STANDARD 14)

add_library(cjson cjson.c)
add_executable(cjson_test test.c)
target_link_libraries(cjson_test cjson)
cmake_minimum_required(VERSION 3.30)
project(opendata)

set(CMAKE_CXX_STANDARD 20)

#include_directories可以在子cmake文件生效
include_directories(public)

# 递归查找所有cpp文件
file(GLOB_RECURSE ALL_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
# 排除所有build目录下的cpp文件
list(FILTER ALL_CPP_FILE EXCLUDE REGEX ".*/build/.*")
# 排除所有demo目录下的cpp文件和main.cpp，防止头文件冲突
list(FILTER ALL_CPP_FILE EXCLUDE REGEX ".*demo.*\\.cpp$")
list(REMOVE_ITEM ALL_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_compile_options(-g -Wall)

add_subdirectory(public/stringop)
add_subdirectory(public/timeframe)
add_subdirectory(public/fileframe)

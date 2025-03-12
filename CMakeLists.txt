cmake_minimum_required(VERSION 3.30)
project(opendata)

set(CMAKE_CXX_STANDARD 20)

include_directories(public)

file(GLOB_RECURSE ALL_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(FILTER ALL_CPP_FILE EXCLUDE REGEX ".*/build/.*")
list(FILTER ALL_CPP_FILE EXCLUDE REGEX ".*demo.*\\.cpp$")
list(REMOVE_ITEM ALL_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_compile_options(-g -Wall)

add_subdirectory(public/stringop)
add_subdirectory(public/timeframe)
add_subdirectory(public/fileframe)

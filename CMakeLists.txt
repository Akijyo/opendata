cmake_minimum_required(VERSION 3.30)
project(opendata)

set(CMAKE_CXX_STANDARD 20)

#include_directories可以在子cmake文件生效

include_directories(public)
include_directories(socket)
include_directories(threadpool)
include_directories(service)
include_directories(cftp)


add_compile_options(-g -Wall)

link_directories(/usr/lib/x86_64-linux-gnu)
link_directories(/usr/local/lib)

link_libraries(curlpp)
link_libraries(curl)


#编译main程序
#[[
# 递归查找所有cpp文件
file(GLOB_RECURSE PUBLIC_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/public/*.cpp")
file(GLOB_RECURSE SOCKET_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/socket/*.cpp")
file(GLOB_RECURSE THREAD_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/threadpool/*.cpp")
file(GLOB_RECURSE SERVICE_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/service/*.cpp")
file(GLOB_RECURSE CFTP_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cftp/*.cpp")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
add_executable(opendata main.cpp ${PUBLIC_CPP_FILE} ${SOCKET_CPP_FILE} ${THREAD_CPP_FILE} ${SERVICE_CPP_FILE})
]]

#编译demo测试文件

##[[
# 递归查找demo程序需要的所有cpp文件
file(GLOB_RECURSE PUBLIC_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/public/*.cpp")
file(GLOB_RECURSE SOCKET_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/socket/*.cpp")
file(GLOB_RECURSE THREAD_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/threadpool/*.cpp")
file(GLOB_RECURSE CFTP_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cftp/*.cpp")


#add_subdirectory(demo/stringopd)
#add_subdirectory(demo/timeframed)
#add_subdirectory(demo/fileframed)
#add_subdirectory(demo/socketd)
#add_subdirectory(demo/procheart)
add_subdirectory(demo/cftp)
#]]



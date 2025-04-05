cmake_minimum_required(VERSION 3.30)
project(tools)

set(CMAKE_CXX_STANDARD 20)

include_directories(../public)
include_directories(../socket)
include_directories(../threadpool)
include_directories(../filetransfer)

link_directories(/usr/lib/x86_64-linux-gnu)
link_directories(/usr/local/lib)

link_libraries(curlpp)
link_libraries(curl)

add_compile_options(-Wall -g)


file(GLOB_RECURSE PUBLIC_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../public/*.cpp")
file(GLOB_RECURSE SOCKET_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../socket/*.cpp")
file(GLOB_RECURSE THREAD_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../threadpool/*.cpp")
file(GLOB_RECURSE FILETRANSFER_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../filetransfer/*.cpp")
#file(GLOB_RECURSE SERVICE_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../service/*.cpp")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/cpp/*.cpp")

foreach(SOURCE_FILE ${CPP_SOURCES})
    # 提取不带扩展名的文件名作为可执行程序名
    get_filename_component(EXEC_NAME ${SOURCE_FILE} NAME_WE)
    # 添加可执行目标
    add_executable(${EXEC_NAME} ${SOURCE_FILE} ${PUBLIC_CPP_FILE} ${SOCKET_CPP_FILE} ${THREAD_CPP_FILE} ${SERVICE_CPP_FILE} ${FILETRANSFER_CPP_FILE})
endforeach()
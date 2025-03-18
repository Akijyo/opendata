cmake_minimum_required(VERSION 3.30)

# 递归查找demo.cpp文件
file(GLOB_RECURSE DEMO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/demo*.cpp")

#对每一个demo文件生成一个可执行文件
foreach(demo_file IN LISTS DEMO_FILE)
    #截取文件名，NAME_WE表示去除后缀
    get_filename_component(demo_name ${demo_file} NAME_WE)
    #生成可执行文件
    add_executable(${demo_name} ${demo_file} ${ALL_CPP_FILE})
    #设置可执行文件输出目录 RUNTIME_OUTPUT_DIRECTORY指定目标运行时输出目录，即可执行文件的输出目录
    set_target_properties(${demo_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
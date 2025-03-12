cmake_minimum_required(VERSION 3.30)

file(GLOB_RECURSE DEMO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/demo*.cpp")

foreach(demo_file IN LISTS DEMO_FILE)
    get_filename_component(demo_name ${demo_file} NAME_WE)
    add_executable(${demo_name} ${demo_file} ${ALL_CPP_FILE})
    set_target_properties(${demo_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
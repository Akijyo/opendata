cmake_minimum_required(VERSION 3.30)
project(opendata)

set(CMAKE_CXX_STANDARD 20)

add_executable(opendata main.cpp
        public/cpublic.h
        public/stringop/stringop.cpp
        public/stringop/stringop.h
        public/stringop/demo1.cpp)

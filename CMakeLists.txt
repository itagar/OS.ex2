cmake_minimum_required(VERSION 3.6)
project(ex2)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES uthreads.h uthreads.cpp Thread.h Thread.cpp)
add_executable(ex2 ${SOURCE_FILES})
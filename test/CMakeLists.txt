cmake_minimum_required(VERSION 2.8)

project(loguru_test)

add_compile_options(-std=c++14)

file(GLOB source
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../*.cpp"
)
file(GLOB headers
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../*.h"
)

add_executable(loguru_test ${source})

cmake_minimum_required(VERSION 2.8)

project(loguru)

add_compile_options(-std=c++11 -Wall -Wextra)

file(GLOB source
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB headers
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_library(loguru STATIC ${source})

target_link_libraries(loguru)

install(TARGETS loguru     DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES   ${headers} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/loguru)

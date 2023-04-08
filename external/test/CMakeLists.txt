cmake_minimum_required(VERSION 2.8.7)

if(NOT MSVC)
  add_compile_options(-std=c++11 -Werror -Wall -Wextra)
endif()

find_package(glog 0.3.4 REQUIRED)

add_executable(glog_bench glog_bench.cpp)
target_link_libraries(glog_bench glog::glog)

add_executable(glog_example glog_example.cpp)
target_link_libraries(glog_example glog::glog)

install(TARGETS glog_bench glog_example
        DESTINATION ${CMAKE_INSTALL_PREFIX})

cmake_minimum_required(VERSION 2.8)

project(loguru_example)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

MESSAGE(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -Wall -Wextra")

file(GLOB source
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../*.cpp"
)

add_executable(loguru_example ${source})

find_package(Threads)
target_link_libraries(loguru_example ${CMAKE_THREAD_LIBS_INIT}) # For pthreads
target_link_libraries(loguru_example dl) # For ldl

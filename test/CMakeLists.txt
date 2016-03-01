cmake_minimum_required(VERSION 2.8)

project(loguru_test)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

MESSAGE(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

add_compile_options(-std=c++11 -Werror -Wall -Wextra)

add_compile_options(
    -Weverything
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-disabled-macro-expansion
    -Wno-exit-time-destructors
    -Wno-global-constructors
    -Wno-missing-noreturn
    -Wno-missing-prototypes
    -Wno-non-virtual-dtor
    -Wno-padded
    -Wno-pedantic
    -Wno-weak-vtables
)

file(GLOB source
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../*.cpp"
)

add_executable(loguru_test ${source})

find_package(Threads)
target_link_libraries(loguru_test ${CMAKE_THREAD_LIBS_INIT}) # For pthreads
target_link_libraries(loguru_test dl) # For ldl

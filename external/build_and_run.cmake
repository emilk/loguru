#!/usr/bin/env cmake -P
cmake_minimum_required(VERSION 2.8.7)

# for lack of an equivalent of `set -e` in bash
macro(EXEC_CMD_CHECK)
  message("running ${ARGN}")
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE CMD_RESULT)
  if(CMD_RESULT)
    message(FATAL_ERROR "Error running ${ARGN}")
  endif()
endmacro()

# pass in command-line to override the default generator (ex. -DGenerator=Ninja)
if(DEFINED Generator)
  set(CMAKE_GENERATOR_ARG "-G${Generator}")
endif()

set(BUILD_DIR ${CMAKE_CURRENT_LIST_DIR}/build)

# find_package(Git)
# if(GIT_FOUND)
#   EXEC_CMD_CHECK(${GIT_EXECUTABLE} submodule update --init)
# endif()

# mkdir -p build
EXEC_CMD_CHECK(${CMAKE_COMMAND} -E make_directory ${BUILD_DIR})

# cd build && cmake .. && cd -
EXEC_CMD_CHECK(${CMAKE_COMMAND} ${CMAKE_GENERATOR_ARG} ${CMAKE_CURRENT_LIST_DIR} WORKING_DIRECTORY ${BUILD_DIR})

# platform-independent equivalent of `make`
EXEC_CMD_CHECK(${CMAKE_COMMAND} --build ${BUILD_DIR})

# ./glog_bench 2>/dev/null
EXEC_CMD_CHECK(${BUILD_DIR}/glog_bench ERROR_QUIET)

# ./glog_example
EXEC_CMD_CHECK(${BUILD_DIR}/glog_example)

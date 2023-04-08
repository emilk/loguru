#!/usr/bin/env cmake -P
cmake_minimum_required(VERSION 2.8.7)

set(BUILD_DIR ${CMAKE_CURRENT_LIST_DIR}/build)
file(MAKE_DIRECTORY ${BUILD_DIR})

# for lack of an equivalent of `set -e` in bash
macro(EXEC_CMD_CHECK)
  message("running ${ARGN}")
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE CMD_RESULT)
  if(CMD_RESULT)
    message(FATAL_ERROR "Error running ${ARGN}")
  endif()
endmacro()

# to override the default configuration: ex. -DConfiguration=Release
if(DEFINED Configuration)
  list(APPEND CMAKE_EXTRA_ARGS -DCMAKE_BUILD_TYPE=${Configuration})
endif()
# to override the default generator: ex. -DGenerator=Ninja
if(DEFINED Generator)
  list(APPEND CMAKE_EXTRA_ARGS "-G${Generator}")
endif()

# find_package(Git)
# if(GIT_FOUND)
#   EXEC_CMD_CHECK(${GIT_EXECUTABLE} submodule update --init)
# endif()

# cd build && cmake .. && cd -
EXEC_CMD_CHECK(${CMAKE_COMMAND} ${CMAKE_EXTRA_ARGS} ${CMAKE_CURRENT_LIST_DIR} WORKING_DIRECTORY ${BUILD_DIR})

if(DEFINED Configuration)
  set(CMAKE_BUILD_EXTRA_ARGS --config ${Configuration})
endif()
# platform-independent equivalent of `make`
EXEC_CMD_CHECK(${CMAKE_COMMAND} --build ${BUILD_DIR} ${CMAKE_BUILD_EXTRA_ARGS})

# ./glog_bench 2>/dev/null
EXEC_CMD_CHECK(${BUILD_DIR}/glog_bench ERROR_QUIET)

# ./glog_example
EXEC_CMD_CHECK(${BUILD_DIR}/glog_example)

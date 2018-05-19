#!/usr/bin/env cmake -P

# for lack of an equivalent of `set -e` in bash
macro(EXEC_CMD_CHECK)
  message("running ${ARGN}")
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE CMD_RESULT)
  if(CMD_RESULT)
    message(FATAL_ERROR "Error running ${ARGN}")
  endif()
endmacro()

# Read the configuration from the environement
if(NOT DEFINED Configuration AND DEFINED ENV{Configuration})
  set(Configuration $ENV{Configuration})
endif()
# to override the default configuration: ex. -DConfiguration=Release
if(DEFINED Configuration)
  list(APPEND CMAKE_EXTRA_ARGS "-DCMAKE_BUILD_TYPE=${Configuration}")
  set(CMAKE_BUILD_EXTRA_ARGS --config ${Configuration})
  set(CTEST_EXTRA_ARGS -C ${Configuration})
endif()

# The generator also, if we have an entry in the envirnoment
if(NOT DEFINED Generator AND DEFINED ENV{Generator})
  set(Generator $ENV{Generator})
endif()
# to override the default generator: ex. -DGenerator=Ninja
if(DEFINED Generator)
  list(APPEND CMAKE_EXTRA_ARGS "-G${Generator}")
endif()

if(NOT DEFINED BUILD_DIR)
  set(BUILD_DIR ${CMAKE_CURRENT_LIST_DIR}/build)
endif()
file(MAKE_DIRECTORY ${BUILD_DIR})

# cd build && cmake .. && cd -
EXEC_CMD_CHECK(${CMAKE_COMMAND} ${CMAKE_EXTRA_ARGS} ${CMAKE_CURRENT_LIST_DIR}
               WORKING_DIRECTORY ${BUILD_DIR})

# platform-independent equivalent of `make`
EXEC_CMD_CHECK(${CMAKE_COMMAND} --build ${BUILD_DIR} ${CMAKE_BUILD_EXTRA_ARGS})

EXEC_CMD_CHECK(${CMAKE_CTEST_COMMAND} ${CTEST_EXTRA_ARGS} --output-on-failure
               WORKING_DIRECTORY ${BUILD_DIR})

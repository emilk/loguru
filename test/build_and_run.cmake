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

# Read the configuration from the environement: AppVeyor case
if(NOT DEFINED Configuration AND DEFINED ENV{Configuration})
  set(Configuration $ENV{Configuration})
endif()
# to override the default configuration: ex. -DConfiguration=Release
if(DEFINED Configuration)
  list(APPEND CMAKE_EXTRA_ARGS -DCMAKE_BUILD_TYPE=${Configuration})
endif()

# The generator also, but this is more convoluted - blame VS/Windows
if(NOT DEFINED Generator)
  if(DEFINED ENV{Generator}) # if we have an entry in the envirnoment
    set(Generator $ENV{Generator})
  elseif(WIN32 AND DEFINED ENV{VS_VERSION_MAJOR}) # on Windows
  # we get the generator possibly from multiple environment variables
    if($ENV{VS_VERSION_MAJOR} STREQUAL "12")
      set(Generator "Visual Studio 12 2013")
    elseif($ENV{VS_VERSION_MAJOR} STREQUAL "14")
      set(Generator "Visual Studio 14 2015")
    endif()
    if(DEFINED Generator AND DEFINED ENV{Platform})
      if($ENV{Platform} STREQUAL "x64")
        set(Generator "${Generator} Win64")
      endif()
    endif()
  endif()
endif()
# to override the default generator: ex. -DGenerator=Ninja
if(DEFINED Generator)
  list(APPEND CMAKE_EXTRA_ARGS "-G${Generator}")
endif()

# cd build && cmake .. && cd -
EXEC_CMD_CHECK(${CMAKE_COMMAND} ${CMAKE_EXTRA_ARGS} ${CMAKE_CURRENT_LIST_DIR} WORKING_DIRECTORY ${BUILD_DIR})

if(DEFINED Configuration)
  set(CMAKE_BUILD_EXTRA_ARGS --config ${Configuration})
endif()
# platform-independent equivalent of `make`
EXEC_CMD_CHECK(${CMAKE_COMMAND} --build ${BUILD_DIR} ${CMAKE_BUILD_EXTRA_ARGS})

if(DEFINED Configuration)
  set(CTEST_EXTRA_ARGS -C ${Configuration})
endif()
EXEC_CMD_CHECK(${CMAKE_CTEST_COMMAND} ${CTEST_EXTRA_ARGS} --output-on-failure
               WORKING_DIRECTORY ${BUILD_DIR})

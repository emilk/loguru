#!/usr/bin/env cmake -P
# Group AppVeyor builds to use fewer VM instances and upload useful artifacts from each.
find_program(AppVeyor appveyor)
if(NOT AppVeyor)
  message(FATAL_ERROR "script specific to appveyor only")
endif()

function(message type) # overriding CMake built-in message function for AppVeyor
  set(msg "${ARGV}")
  if(type STREQUAL FATAL_ERROR OR type STREQUAL SEND_ERROR)
    set(msg "${ARGN}")
    set(options -Category Error)
  elseif(type STREQUAL WARNING OR type STREQUAL AUTHOR_WARNING)
    set(msg "${ARGN}")
    set(options -Category Warning)
  elseif(type STREQUAL STATUS)
    set(msg "${ARGN}")
    set(options -Category Information)
  endif()
  execute_process(COMMAND ${AppVeyor} AddMessage "${msg}" ${options})

  _message(${ARGV}) # the built-in functionality
endfunction()

function(BuildAndRun)
  if(DEFINED Generator)
    string(REPLACE " " "" gen "${Generator}/") # Remove spaces from generator
  endif()
  set(BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/build/${gen}${Configuration}")
  message("Building ${gen}${Configuration}")
  include(${CMAKE_CURRENT_LIST_DIR}/build_and_run.cmake)
  message("Built ${gen}${Configuration}")
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar czf "${BUILD_DIR}/Testing.zip" "${BUILD_DIR}/Testing")
  execute_process(COMMAND ${AppVeyor} PushArtifact "${BUILD_DIR}/Testing.zip" -FileName "${gen}${Configuration}.zip")
endfunction()

foreach(Configuration "Debug" "Release")
  if(WIN32)
    foreach(Generator
      "Visual Studio 14 2015"
      "Visual Studio 14 2015 Win64"
      "Visual Studio 15 2017"
      "Visual Studio 15 2017 Win64"
    )
      BuildAndRun()
    endforeach()
  elseif(UNIX)
    BuildAndRun()
  endif()
endforeach()

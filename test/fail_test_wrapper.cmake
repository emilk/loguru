#!/usr/bin/env cmake -P
cmake_minimum_required(VERSION 2.8.7)

get_filename_component(CurrentFile ${CMAKE_CURRENT_LIST_FILE} NAME)

# The script is invoked as cmake -P <script name> <loguru_test> <arg(s)>
# the following loop extracts the arguments after the one matching the script name
foreach(i RANGE 0 ${CMAKE_ARGC})
    if(CollectArgs)
        list(APPEND TestArgs ${CMAKE_ARGV${i}})
    elseif(CMAKE_ARGV${i} MATCHES "${CurrentFile}$")
        set(CollectArgs true)
    endif()
endforeach()

# TestArgs contains <loguru_test> <arg(s)>
execute_process(COMMAND ${TestArgs}
                RESULT_VARIABLE CmdResult)

# To invert the failure logic
if(NOT CmdResult)
# the wrapper must fail if the child process returned success
    message(FATAL_ERROR "${TestArgs} passed.")
endif()

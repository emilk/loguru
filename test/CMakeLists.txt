cmake_minimum_required(VERSION 2.8)

project(loguru_test)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

MESSAGE(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -Wall -Wextra -pedantic -fsanitize=address -fno-omit-frame-pointer")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++98-compat")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++98-compat-pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-noreturn")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-prototypes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
endif() # Clang

add_executable(loguru_test loguru_test.cpp)

find_package(Threads)
target_link_libraries(loguru_test ${CMAKE_THREAD_LIBS_INIT}) # For pthreads
if(NOT WIN32)
    target_link_libraries(loguru_test dl) # For ldl
endif()

enable_testing()

if(NOT WIN32)
# Sadly, Windows doesn't allow us to safely run the following tests
    list(APPEND ExtraFailureTests
            SIGSEGV
            throw_on_signal)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND ExtraFailureTests
            assert)
endif()

# Failure Tests
foreach(Test
            ABORT_F
            ABORT_S
            LOG_F_FATAL
            LOG_S_FATAL
            CHECK_NOTNULL_F
            CHECK_F
            CHECK_EQ_F_int
            CHECK_EQ_F_unsigned
            CHECK_EQ_F_size_t
            CHECK_EQ_F
            CHECK_EQ_F_message
            CHECK_EQ_S
            CHECK_LT_S
            CHECK_LT_S_message
            deep_abort
            abort
            error_context
            throw_on_fatal
            ${ExtraFailureTests}
)
     add_test(NAME loguru_test_${Test}
              COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/fail_test_wrapper.cmake
              $<TARGET_FILE:loguru_test> ${Test})
endforeach()

# Success Tests
foreach(Test
            callback)
    add_test(loguru_test_${Test} loguru_test ${Test})
endforeach()

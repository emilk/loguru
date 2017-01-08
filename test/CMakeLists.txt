cmake_minimum_required(VERSION 2.8)

project(loguru_test)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -Wall -Wextra -pedantic")
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
# Sadly, Windows doesn't allow us to safely test the following
    set(ExtraFailureTests
            SIGSEGV
            thrown_on_signal)
endif()

# Failure Tests
foreach(Test
            ABORT_F
            ABORT_S
            assert
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
    add_test(loguru_test_${Test} loguru_test ${Test})
    set_tests_properties(loguru_test_${Test} PROPERTIES WILL_FAIL true)
endforeach()

# Success Tests
foreach(Test
            callback)
    add_test(loguru_test_${Test} loguru_test ${Test})
endforeach()

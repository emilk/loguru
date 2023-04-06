cmake_minimum_required(VERSION 3.10)

project(loguru_cmake_example CXX)

# Choose any ONE of the following three methods to import loguru into your cmake
#
# How do I set loguru compile-time flags?!
#
#   This example file demonstrates setting the 'LOGURU_WITH_STREAMS' option
#
#   For method 1 and 2:
#     loguru is compiled alongside your project so all loguru compile-time flags
#     can be set in your project as cmake variables!
#
#   For method 3:
#     This method uses a pre-compiled copy of loguru, so the compile-time flags
#     cannot be changed by your project. They must be set when compiling loguru.

# ------------------------------------------------------------------------------
# --- Method 1: Add loguru as a sub-directory (great with git submodules!)
# ------------------------------------------------------------------------------

# set any loguru compile-time flags before adding the subdirectory
set(LOGURU_WITH_STREAMS TRUE)
add_subdirectory(loguru) # defines target 'loguru::loguru'

# ------------------------------------------------------------------------------
# --- Method 2: Fetch from an external repository
# ------------------------------------------------------------------------------

# NOTE: The FetchContent functions shown here were introduced in CMake 3.14

include(FetchContent)
FetchContent_Declare(LoguruGitRepo
	GIT_REPOSITORY "https://github.com/emilk/loguru" # can be a filesystem path
	GIT_TAG        "master"
)
# set any loguru compile-time flags before calling MakeAvailable()
set(LOGURU_WITH_STREAMS TRUE)
FetchContent_MakeAvailable(LoguruGitRepo) # defines target 'loguru::loguru'

# ------------------------------------------------------------------------------
# -- Method 3: Use a pre-compiled installed copy of loguru
# ------------------------------------------------------------------------------

# This method requires you to have first built + installed loguru on your system
# using steps like the following (shown here setting WITH_STREAMS=1)
#    cd loguru
#    cmake -S. -Bbuild \
#      -DCMAKE_INSTALL_PREFIX=/path/to/loguru/install \
#      -DLOGURU_WITH_STREAMS=TRUE
#    cmake --build build --target install
find_package(loguru CONFIG REQUIRED) # imports target 'loguru::loguru'

# ------------------------------------------------------------------------------
# -- Link to the loguru::loguru target
# ------------------------------------------------------------------------------

# Regardless of the method used, you always link to the `loguru::loguru` target!

add_executable(YourTarget main.cpp)
target_link_libraries(YourTarget PRIVATE loguru::loguru)

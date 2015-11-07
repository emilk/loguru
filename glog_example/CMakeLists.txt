cmake_minimum_required(VERSION 2.8)

project(glog_example)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

MESSAGE(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

add_compile_options(-std=c++11 -Werror -Wall -Wextra)

file(GLOB source
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# ------------------------------------------------------
# GFLAGS
find_library(GFLAGS_LIB  NAMES "gflags" PATHS
			${GFLAGS_ROOT}/lib
			${GFLAGS_ROOT}/Lib)

if (MSVC)
	find_path(GFLAGS_INCLUDE_DIR gflags/gflags.h PATHS ${GFLAGS_ROOT}/Include)
	set(GFLAGS_LIB_DIR  ${GFLAGS_ROOT}/Lib)
else()
	find_path(GFLAGS_INCLUDE_DIR gflags/gflags.h PATHS
		${GFLAGS_ROOT}/include)
	set(GFLAGS_LIB_DIR  ${GFLAGS_ROOT}/lib)
endif()
mark_as_advanced(
    GFLAGS_INCLUDE_DIR
    GFLAGS_LIB
)

# ------------------------------------------------------
# GLOG

find_library(GLOG_LIB  NAMES "glog" libglog PATHS
			${GLOG_ROOT})

if (MSVC)
	find_path(GLOG_INCLUDE_DIR glog/logging.h PATHS ${GLOG_ROOT})
	set(GLOG_LIB_DIR ${GLOG_ROOT})
	add_definitions(-DUSE_OWN_CHECK)
else()
	find_path(GLOG_INCLUDE_DIR glog/logging.h PATHS
		${GLOG_ROOT}/include)
	set(GLOG_LIB_DIR ${GLOG_ROOT}/lib)
endif()
add_definitions(-DGLOG_NO_ABBREVIATED_SEVERITIES=1)

mark_as_advanced(
    GLOG_INCLUDE_DIR
    GLOG_LIB
)

# ------------------------------------------------------

add_executable(glog_example ${source})

include_directories(
	${GFLAGS_INCLUDE_DIR}
	${GLOG_INCLUDE_DIR}
)

target_link_libraries(glog_example
    ${GFLAGS_LIB}
    ${GLOG_LIB}
)

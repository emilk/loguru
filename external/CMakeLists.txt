cmake_minimum_required(VERSION 2.8.7)

include(ExternalProject)

set(gflags_DIR ${CMAKE_CURRENT_BINARY_DIR}/gflags)
ExternalProject_Add(gflags
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gflags
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${gflags_DIR}
             -DBUILD_TESTING:BOOL=OFF
)

set(glog_DIR ${CMAKE_CURRENT_BINARY_DIR}/glog)
ExternalProject_Add(glog
  DEPENDS gflags
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glog
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${glog_DIR}
             -DBUILD_TESTING:BOOL=OFF
)

ExternalProject_Add(external_test
  DEPENDS glog
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
)

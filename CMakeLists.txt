cmake_minimum_required(VERSION 3.10)

# --- include useful utility functions
# ----------------------------------------------------------

include(cmake/utilities.cmake)

# --- extract project version
# ----------------------------------------------------------

loguru_get_version_from_header() # defines LOGURU_VERSION

# --- define project and policies
# ----------------------------------------------------------

set(_namespace loguru)
project(loguru VERSION "${LOGURU_VERSION}" LANGUAGES CXX)

set(LOGURU_PACKAGE_URL     "https://github.com/emilk/loguru"               CACHE STRING "")
set(LOGURU_PACKAGE_VENDOR  "Emil Ernerfeldt"                               CACHE STRING "")
set(LOGURU_PACKAGE_CONTACT "Emil Ernerfeldt <emil.ernerfeldt@gmail.com>"   CACHE STRING "")
set(LOGURU_PACKAGE_DESCRIPTION_SUMMARY "A lightweight C++ logging library" CACHE STRING "")
set(LOGURU_PACKAGE_DESCRIPTION_FILE    "${PROJECT_SOURCE_DIR}/README.md"   CACHE STRING "")

# --- check if toplevel or subdirectory
# ----------------------------------------------------------

# This variable is set automatically by the project() call in CMake 3.21+
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}" PROJECT_IS_TOP_LEVEL)
if (PROJECT_IS_TOP_LEVEL)
  message(STATUS "Configuring ${PROJECT_NAME} as top-level")
else()
  message(STATUS "Configuring ${PROJECT_NAME} as sub-directory")
endif()

# --- set default build type
# ----------------------------------------------------------

# NOTE: when running as a standalone project, we only allow Release & Debug
#       but as a sub-project we don't want to accidentally pollute the parent
if (PROJECT_IS_TOP_LEVEL)
  if(NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
  endif()
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Release;Debug")
endif()

# --- expose cmake-specific user options
# ----------------------------------------------------------

option(LOGURU_INSTALL        "Generate the install target(s)" ${PROJECT_IS_TOP_LEVEL})
option(LOGURU_BUILD_EXAMPLES "Build the project examples"     ${PROJECT_IS_TOP_LEVEL})
option(LOGURU_BUILD_TESTS    "Build the tests"                ${PROJECT_IS_TOP_LEVEL})
if (LOGURU_INSTALL)
  option(LOGURU_CPACK "Generate CPackConfig.cmake" ${PROJECT_IS_TOP_LEVEL})
endif()

# --- set global compile flags
# ----------------------------------------------------------

if (PROJECT_IS_TOP_LEVEL)
  # enable ALL warnings for all subsequently defined targets
  add_compile_options(
    "$<$<CXX_COMPILER_ID:GNU>:-Wall;-Wextra;-Werror;-pedantic>"
    "$<$<CXX_COMPILER_ID:Clang>:-Weverything;-Wno-c++98-compat;-Wno-c++98-compat-pedantic>"
    "$<$<CXX_COMPILER_ID:MSVC>:/W4>"
  )
endif()

# --- add loguru target
# ----------------------------------------------------------

add_library(loguru loguru.cpp) # allow BUILD_SHARED_LIBS to decide STATIC/SHARED

if (NOT PROJECT_IS_TOP_LEVEL)
  add_library(${_namespace}::loguru ALIAS loguru)
endif()

# --- determine if linking 'dl' is required
# ----------------------------------------------------------

if (LOGURU_STACKTRACES AND (NOT CMAKE_DL_LIBS))
  message(WARNING
    "Stack traces requested but the required 'dl' library was not found. "
    "LOGURU_STACKTRACES has been automatically disabled (set to 0)"
  )
  set(LOGURU_STACKTRACES 0)
endif()

if (LOGURU_STACKTRACES)
  set(_lib_dl_linkflag "-l${CMAKE_DL_LIBS}")
else()
  set(_lib_dl_linkflag) # dl dependency is not needed if STACKTRACES=0
endif()

# --- set loguru target properties
# ----------------------------------------------------------

target_include_directories(loguru
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)

target_compile_features(loguru PUBLIC cxx_std_11)

find_package(Threads REQUIRED) # defines IMPORTED target Threads::Threads
target_link_libraries(loguru
  PUBLIC
    Threads::Threads    # pthreads (or equivalent)
    ${_lib_dl_linkflag} # dl (or equivalent)
)

set_target_properties(loguru
  PROPERTIES
    VERSION   "${LOGURU_VERSION}"
    SOVERSION "${LOGURU_VERSION_MAJOR}"
    DEBUG_POSTFIX "d"
)

target_compile_definitions(loguru
  # NOTE: these generator expressions are dense but the logic is quite simple!
  #       if any of the cache variables are not equal to the empty string, set them as a definition.
  #       Additionally, the "boolean" variables are coerced into a numeric representation (1 or 0)
  PUBLIC
    $<$<NOT:$<STREQUAL:,${LOGURU_EXPORT}>>:LOGURU_EXPORT=${LOGURU_EXPORT}>
    $<$<NOT:$<STREQUAL:,${LOGURU_DEBUG_LOGGING}>>:LOGURU_DEBUG_LOGGING=$<BOOL:${LOGURU_DEBUG_LOGGING}>>
    $<$<NOT:$<STREQUAL:,${LOGURU_DEBUG_CHECKS}>>:LOGURU_DEBUG_CHECKS=$<BOOL:${LOGURU_DEBUG_CHECKS}>>
    $<$<NOT:$<STREQUAL:,${LOGURU_SCOPE_TEXT_SIZE}>>:LOGURU_SCOPE_TEXT_SIZE=${LOGURU_SCOPE_TEXT_SIZE}>
    $<$<NOT:$<STREQUAL:,${LOGURU_REDEFINE_ASSERT}>>:LOGURU_REDEFINE_ASSERT=$<BOOL:${LOGURU_REDEFINE_ASSERT}>>
    $<$<NOT:$<STREQUAL:,${LOGURU_WITH_STREAMS}>>:LOGURU_WITH_STREAMS=$<BOOL:${LOGURU_WITH_STREAMS}>>
    $<$<NOT:$<STREQUAL:,${LOGURU_REPLACE_GLOG}>>:LOGURU_REPLACE_GLOG=$<BOOL:${LOGURU_REPLACE_GLOG}>>
    $<$<NOT:$<STREQUAL:,${LOGURU_USE_FMTLIB}>>:LOGURU_USE_FMTLIB=$<BOOL:${LOGURU_USE_FMTLIB}>>
    $<$<NOT:$<STREQUAL:,${LOGURU_FMT_HEADER_ONLY}>>:LOGURU_FMT_HEADER_ONLY=$<BOOL:${LOGURU_FMT_HEADER_ONLY}>>
    $<$<NOT:$<STREQUAL:,${LOGURU_WITH_FILEABS}>>:LOGURU_WITH_FILEABS=$<BOOL:${LOGURU_WITH_FILEABS}>>
    $<$<NOT:$<STREQUAL:,${LOGURU_STACKTRACES}>>:LOGURU_STACKTRACES=$<BOOL:${LOGURU_STACKTRACES}>>
    $<$<NOT:$<STREQUAL:,${LOGURU_RTTI}>>:LOGURU_RTTI=$<BOOL:${LOGURU_RTTI}>>
    $<$<NOT:$<STREQUAL:,${LOGURU_FILENAME_WIDTH}>>:LOGURU_FILENAME_WIDTH=${LOGURU_FILENAME_WIDTH}>
    $<$<NOT:$<STREQUAL:,${LOGURU_THREADNAME_WIDTH}>>:LOGURU_THREADNAME_WIDTH=${LOGURU_THREADNAME_WIDTH}>
    $<$<NOT:$<STREQUAL:,${LOGURU_SCOPE_TIME_PRECISION}>>:LOGURU_SCOPE_TIME_PRECISION=${LOGURU_SCOPE_TIME_PRECISION}>
    $<$<NOT:$<STREQUAL:,${LOGURU_VERBOSE_SCOPE_ENDINGS}>>:LOGURU_VERBOSE_SCOPE_ENDINGS=$<BOOL:${LOGURU_VERBOSE_SCOPE_ENDINGS}>>
)

# --- import and link fmt (if needed)
# ----------------------------------------------------------

if (LOGURU_USE_FMTLIB)

  message(STATUS "linking to fmt")

  if (NOT TARGET fmt::fmt) # only search if not already found in parent scope
    find_package(fmt CONFIG REQUIRED)
  endif()

  if (LOGURU_FMT_HEADER_ONLY)
    target_link_libraries(loguru PUBLIC fmt::fmt-header-only)
  else()
    target_link_libraries(loguru PUBLIC fmt::fmt)
  endif()

  message(STATUS "linking to fmt - done")

endif()

# --- set ide-specific properties
# ----------------------------------------------------------

# make the project the default when opened in visual studio ide
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# --- setup examples
# ----------------------------------------------------------

# TODO: make the examples work with this cmake paradigm
if (LOGURU_BUILD_EXAMPLES)
  message(STATUS "!!! the examples don't work with this cmake build yet")
  # message(STATUS "building examples")

  # add_subdirectory(glog_bench)
  # add_subdirectory(glog_example)
  # add_subdirectory(loguru_bench)
  # add_subdirectory(loguru_example)

  # message(STATUS "building examples - done")
endif()

# --- setup tests
# ----------------------------------------------------------

# TODO: make the tests work with this cmake paradigm
if (LOGURU_BUILD_TESTS)
  message(STATUS "!!! the tests don't work with this cmake build yet")
  # message(STATUS "building tests")
  # add_subdirectory(test)
  # message(STATUS "building tests - done")
endif()

# --- setup install rules
# ----------------------------------------------------------

if (LOGURU_INSTALL)

  message(STATUS "generating install rules")

  # -- include modules

  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  # -- expose cache variables for users to customize install location

  set(LOGURU_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE STRING
      "Install directory for cmake files, relative to \${CMAKE_INSTALL_PREFIX} or an absolute path")
  set(LOGURU_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}" CACHE STRING
      "Install directory for libraries, relative to \${CMAKE_INSTALL_PREFIX} or an absolute path")
  set(LOGURU_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}" CACHE STRING
      "Install directory for include files, relative to \${CMAKE_INSTALL_PREFIX} or an absolute path")
  set(LOGURU_INSTALL_PKGCONFIGDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig" CACHE STRING
      "Install directory for pkgconfig (.pc) files, relative to \${CMAKE_INSTALL_PREFIX} or an absolute path")

  # -- set additional target properties relevant to install dir

  target_include_directories(loguru
    PUBLIC
      $<INSTALL_INTERFACE:${LOGURU_INSTALL_INCLUDEDIR}/loguru>
  )

  # -- setup install config files

  set(_project_config_file_in  ${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in)
  set(_project_config_file_out ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake)
  set(_version_config_file ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake)
  set(_targets_export_name ${PROJECT_NAME}-targets)
  set(_pkgconfig_file_in   ${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}.pc.in)
  set(_pkgconfig_file_out  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc)

  # -- Configure pkg-config template

  set(_pkgconfig_libdir     "\${exec_prefix}/${LOGURU_INSTALL_LIBDIR}")
  set(_pkgconfig_includedir "\${prefix}/${LOGURU_INSTALL_INCLUDEDIR}")

  # if the user chose absolute paths, strip the ${prefix} and/or ${exec_prefix}
  if (IS_ABSOLUTE "${LOGURU_INSTALL_LIBDIR}")
    set(_pkgconfig_libdir "${LOGURU_INSTALL_LIBDIR}")
  endif()

  if (IS_ABSOLUTE "${LOGURU_INSTALL_INCLUDEDIR}")
    set(_pkgconfig_includedir "${LOGURU_INSTALL_INCLUDEDIR}")
  endif()

  configure_file(
    ${_pkgconfig_file_in}
    ${_pkgconfig_file_out}
    @ONLY
  )

  # -- Generate the version file in the build directory

  write_basic_package_version_file( # function from CMakePackageConfigHelpers
    ${_version_config_file}
    COMPATIBILITY SameMajorVersion
  )

  # -- Generate the config file in the build directory

  configure_package_config_file( # function from CMakePackageConfigHelpers
    ${_project_config_file_in}
    ${_project_config_file_out}
    INSTALL_DESTINATION ${LOGURU_INSTALL_CMAKEDIR}
  )

  # -- Install the main library

  install(TARGETS loguru
    EXPORT ${_targets_export_name}  # Add this target to the 'exports' file
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # .dll, .exe
    ARCHIVE DESTINATION ${LOGURU_INSTALL_LIBDIR} # .lib, .a
    LIBRARY DESTINATION ${LOGURU_INSTALL_LIBDIR} # .so
  )

  # -- Install the header file

  install(FILES loguru.hpp
    DESTINATION ${LOGURU_INSTALL_INCLUDEDIR}/loguru
  )

  # -- Install version and config files

  install(FILES  ${_project_config_file_out}  ${_version_config_file}
    DESTINATION ${LOGURU_INSTALL_CMAKEDIR}
  )

  # -- Install pkgconfig file

  install(FILES ${_pkgconfig_file_out}
    DESTINATION ${LOGURU_INSTALL_PKGCONFIGDIR}
  )

  # -- Install target exports file

  install(EXPORT ${_targets_export_name}
    NAMESPACE ${_namespace}::
    DESTINATION ${LOGURU_INSTALL_CMAKEDIR}
  )

  # -- Install .pdb file (if exists)

  if (MSVC AND BUILD_SHARED_LIBS)
    install(FILES $<TARGET_PDB_FILE:loguru>
      CONFIGURATIONS "Debug"
      DESTINATION ${LOGURU_INSTALL_LIBDIR} OPTIONAL
    )
  endif()

  message(STATUS "generating install rules - done")

endif() # LOGURU_INSTALL


# -- Setup CPack
# ----------------------------------------------------------

if (LOGURU_INSTALL AND LOGURU_CPACK)

  message(STATUS "setting up cpack")

  # NOTE: this must be the very last instruction in this file
  include(cmake/${PROJECT_NAME}-cpack.cmake)

  message(STATUS "setting up cpack - done")

endif()

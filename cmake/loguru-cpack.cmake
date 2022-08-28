# -- expose cache variables to the user

# Set .zip and tar.gz as default generators
set(LOGURU_CPACK_GENERATOR "TGZ;ZIP" CACHE STRING
    "Semicolon separated list of generators")
# NOTE: CPACK_PACKAGE_DIRECTORY normally defaults to CMAKE_BINARY_DIR
set(LOGURU_CPACK_PACKAGE_DIRECTORY "${PROJECT_BINARY_DIR}/packages" CACHE PATH
    "Where to generate loguru cpack installer packages")

set(CPACK_GENERATOR ${LOGURU_CPACK_GENERATOR})
set(CPACK_PACKAGE_DIRECTORY ${LOGURU_CPACK_PACKAGE_DIRECTORY})

# -- contact and summary

set(CPACK_PROJECT_URL     "${LOGURU_PACKAGE_URL}")
set(CPACK_PACKAGE_VENDOR  "${LOGURU_PACKAGE_VENDOR}")
set(CPACK_PACKAGE_CONTACT "${LOGURU_PACKAGE_CONTACT}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${LOGURU_PACKAGE_DESCRIPTION_SUMMARY}")
set(CPACK_PACKAGE_DESCRIPTION_FILE    "${LOGURU_PACKAGE_DESCRIPTION_FILE}")

# -- version info

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
if(PROJECT_VERSION_TWEAK)
  set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION}.${PROJECT_VERSION_TWEAK})
endif()

# -- set non-default behavior

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_STRIP_FILES ON)

# -- normalize paths

# NOTE: some generators don't understand paths that contain backslashes
string(REPLACE "\\" "/" CPACK_PACKAGE_DIRECTORY "${CPACK_PACKAGE_DIRECTORY}" )

# -- make cpack work with sub-projects

# NOTE:
#  cpack assumes by default that it's being called from a top-level project.
#  It uses the top-level project values to generate 'CPackConfig.cmake' and
#  tries to place it into the top-level build directory.
#  This behavior makes sense if you're writing a standalone cmake project, but
#  causes grief if your project is designed to be included as a sub-project
#    e.g. using   add_subdirectory()   or   FetchContent()
#  Here, we trick cpack into using the current current project info instead.
set(CMAKE_BINARY_DIR   "${PROJECT_BINARY_DIR}")
set(CMAKE_SOURCE_DIR   "${PROJECT_SOURCE_DIR}")
set(CMAKE_PROJECT_NAME "${PROJECT_NAME}")

# -- include CPack module

# NOTE: This must be called at the end so it inherits all the values set above
include(CPack)

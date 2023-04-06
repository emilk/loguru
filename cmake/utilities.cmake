# --- set LOGURU_VERSION from loguru.hpp
# --------------------------------------

function(loguru_get_version_from_header)

  file(READ "${CMAKE_CURRENT_LIST_DIR}/loguru.hpp" _hdr_contents)
  string(REGEX REPLACE ".*LOGURU_VERSION_MAJOR ([0-9]+).*" "\\1" _version_major "${_hdr_contents}")
  string(REGEX REPLACE ".*LOGURU_VERSION_MINOR ([0-9]+).*" "\\1" _version_minor "${_hdr_contents}")
  string(REGEX REPLACE ".*LOGURU_VERSION_PATCH ([0-9]+).*" "\\1" _version_patch "${_hdr_contents}")

  if(_version_major STREQUAL "")
    message(FATAL_ERROR "Could not extract major version number from loguru.hpp")
  endif()

  if(_version_minor STREQUAL "")
    message(FATAL_ERROR "Could not extract minor version number from loguru.hpp")
  endif()

  if(_version_patch STREQUAL "")
    message(FATAL_ERROR "Could not extract patch version number from loguru.hpp")
  endif()

  set(LOGURU_VERSION_MAJOR "${_version_major}" CACHE STRING "" FORCE)
  set(LOGURU_VERSION_MINOR "${_version_minor}" CACHE STRING "" FORCE)
  set(LOGURU_VERSION_PATCH "${_version_patch}" CACHE STRING "" FORCE)
  set(LOGURU_VERSION "${_version_major}.${_version_minor}.${_version_patch}" CACHE STRING "" FORCE)

endfunction()

# --- prints a var and its value
# --------------------------------------

macro(print_var x)
  message(STATUS "${x}=[${${x}}]")
endmacro()

# FindIrrKlang - attempts to locate the irrKlang sound engine library.
#
# This module defines the following variables (on success):
#   IRRKLANG_INCLUDE_DIRS - where to find irrKlang/irrKlang.h
#   IRRKLANG_LIBRARIES    - the irrKlang library to link against
#   IRRKLANG_FOUND        - if the library was successfully located
#
# It tries standard installation paths, but can be customized with:
#   IRRKLANG_ROOT_DIR - root directory of an irrKlang installation
# Headers are expected to be found in either:
#   <IRRKLANG_ROOT_DIR>/include/irrKlang.h OR
#   <IRRKLANG_ROOT_DIR>/irrKlang/irrKlang.h
#
# NOTE: environment variable IRRKLANG_ROOT_DIR may also be used, but
# changing it does NOT automatically retrigger a search in CMake.
#=============================================================================

# ---- default search paths ----
SET(_irrklang_HEADER_SEARCH_DIRS
    "/usr/include"
    "/usr/local/include"
    "${CMAKE_SOURCE_DIR}/includes"
    "C:/Program Files/irrKlang"
    "C:/Program Files (x86)/irrKlang"
)

SET(_irrklang_LIB_SEARCH_DIRS
    "/usr/lib"
    "/usr/local/lib"
    "${CMAKE_SOURCE_DIR}/lib"
    "C:/Program Files/irrKlang/lib"
    "C:/Program Files (x86)/irrKlang/lib"
)

# ---- check environment variable ----
SET(_irrklang_ENV_ROOT "$ENV{IRRKLANG_ROOT_DIR}")
IF(NOT IRRKLANG_ROOT_DIR AND _irrklang_ENV_ROOT)
    SET(IRRKLANG_ROOT_DIR "${_irrklang_ENV_ROOT}")
ENDIF()

# ---- prioritize user-provided path ----
IF(IRRKLANG_ROOT_DIR)
    SET(_irrklang_HEADER_SEARCH_DIRS
        "${IRRKLANG_ROOT_DIR}"
        "${IRRKLANG_ROOT_DIR}/include"
        ${_irrklang_HEADER_SEARCH_DIRS}
    )
    SET(_irrklang_LIB_SEARCH_DIRS
        "${IRRKLANG_ROOT_DIR}/lib"
        "${IRRKLANG_ROOT_DIR}/bin"
        ${_irrklang_LIB_SEARCH_DIRS}
    )
ENDIF()

# ---- locate irrKlang headers ----
FIND_PATH(IRRKLANG_INCLUDE_DIR "irrKlang.h"
    PATH_SUFFIXES "include" "irrKlang"
    PATHS ${_irrklang_HEADER_SEARCH_DIRS}
)

# ---- locate irrKlang library ----
# Windows: irrKlang.lib
# Linux/macOS: libirrklang.so or libirrklang.a (nếu cần mở rộng)
FIND_LIBRARY(IRRKLANG_LIBRARY
    NAMES irrKlang irrklang
    PATHS ${_irrklang_LIB_SEARCH_DIRS}
)

# ---- handle standard CMake find package ----
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    IrrKlang
    DEFAULT_MSG
    IRRKLANG_INCLUDE_DIR
    IRRKLANG_LIBRARY
)

IF(IRRKLANG_FOUND)
    SET(IRRKLANG_INCLUDE_DIRS "${IRRKLANG_INCLUDE_DIR}")
    SET(IRRKLANG_LIBRARIES "${IRRKLANG_LIBRARY}")

    MESSAGE(STATUS "IRRKLANG_INCLUDE_DIR = ${IRRKLANG_INCLUDE_DIR}")
    MESSAGE(STATUS "IRRKLANG_LIBRARY      = ${IRRKLANG_LIBRARY}")
ENDIF()

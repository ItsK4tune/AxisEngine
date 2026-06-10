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

find_package(IrrKlang CONFIG QUIET)
if(NOT TARGET IrrKlang::IrrKlang)
    find_package(irrklang CONFIG QUIET)
endif()

if(TARGET IrrKlang::IrrKlang)
    get_target_property(IRRKLANG_INCLUDE_DIRS IrrKlang::IrrKlang INTERFACE_INCLUDE_DIRECTORIES)
    if(IRRKLANG_INCLUDE_DIRS)
        list(GET IRRKLANG_INCLUDE_DIRS 0 IRRKLANG_INCLUDE_DIR)
    endif()

    get_target_property(IRRKLANG_LIBRARY IrrKlang::IrrKlang IMPORTED_IMPLIB_RELEASE)
    if(NOT IRRKLANG_LIBRARY)
        get_target_property(IRRKLANG_LIBRARY IrrKlang::IrrKlang IMPORTED_LOCATION_RELEASE)
    endif()
    if(NOT IRRKLANG_LIBRARY)
        get_target_property(IRRKLANG_LIBRARY IrrKlang::IrrKlang IMPORTED_IMPLIB)
    endif()

    get_target_property(IRRKLANG_RUNTIME_LIBRARY IrrKlang::IrrKlang IMPORTED_LOCATION_RELEASE)
    if(NOT IRRKLANG_RUNTIME_LIBRARY)
        get_target_property(IRRKLANG_RUNTIME_LIBRARY IrrKlang::IrrKlang IMPORTED_LOCATION)
    endif()

    set(IRRKLANG_FOUND TRUE)
    set(IrrKlang_FOUND TRUE)
    set(IRRKLANG_LIBRARIES "${IRRKLANG_LIBRARY}")
    return()
endif()

# ---- default search paths ----
SET(_irrklang_HEADER_SEARCH_DIRS
    "/usr/include"
    "/usr/local/include"
    "C:/Program Files/irrKlang"
    "C:/Program Files (x86)/irrKlang"
)

SET(_irrklang_LIB_SEARCH_DIRS
    "/usr/lib"
    "/usr/local/lib"
    "C:/Program Files/irrKlang/lib"
    "C:/Program Files (x86)/irrKlang/lib"
)

SET(_irrklang_RUNTIME_SEARCH_DIRS
    "/usr/lib"
    "/usr/local/lib"
    "C:/Program Files/irrKlang/bin"
    "C:/Program Files/irrKlang/lib"
    "C:/Program Files/irrKlang/dll"
    "C:/Program Files (x86)/irrKlang/bin"
    "C:/Program Files (x86)/irrKlang/lib"
    "C:/Program Files (x86)/irrKlang/dll"
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
        "${IRRKLANG_ROOT_DIR}/lib/winx64-visualStudio"
        "${IRRKLANG_ROOT_DIR}/lib/win32-visualStudio"
        "${IRRKLANG_ROOT_DIR}/lib/Winx64-visualStudio"
        "${IRRKLANG_ROOT_DIR}/lib/Win32-visualStudio"
        "${IRRKLANG_ROOT_DIR}/bin"
        ${_irrklang_LIB_SEARCH_DIRS}
    )
    SET(_irrklang_RUNTIME_SEARCH_DIRS
        "${IRRKLANG_ROOT_DIR}/bin"
        "${IRRKLANG_ROOT_DIR}/bin/winx64-visualStudio"
        "${IRRKLANG_ROOT_DIR}/bin/win32-visualStudio"
        "${IRRKLANG_ROOT_DIR}/bin/Winx64-visualStudio"
        "${IRRKLANG_ROOT_DIR}/bin/Win32-visualStudio"
        "${IRRKLANG_ROOT_DIR}/dll"
        "${IRRKLANG_ROOT_DIR}/dll/winx64-visualStudio"
        "${IRRKLANG_ROOT_DIR}/dll/win32-visualStudio"
        "${IRRKLANG_ROOT_DIR}/dll/Winx64-visualStudio"
        "${IRRKLANG_ROOT_DIR}/dll/Win32-visualStudio"
        "${IRRKLANG_ROOT_DIR}/lib"
        "${IRRKLANG_ROOT_DIR}/lib/winx64-visualStudio"
        "${IRRKLANG_ROOT_DIR}/lib/win32-visualStudio"
        "${IRRKLANG_ROOT_DIR}/lib/Winx64-visualStudio"
        "${IRRKLANG_ROOT_DIR}/lib/Win32-visualStudio"
        ${_irrklang_RUNTIME_SEARCH_DIRS}
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

FIND_FILE(IRRKLANG_RUNTIME_LIBRARY
    NAMES irrKlang.dll libIrrKlang.so libirrklang.so libIrrKlang.dylib libirrklang.dylib
    PATHS ${_irrklang_RUNTIME_SEARCH_DIRS}
)

SET(IRRKLANG_PLUGIN_RUNTIME_LIBRARIES)
FOREACH(_irrklang_PLUGIN_NAME IN ITEMS ikpMP3.dll ikpFlac.dll)
    STRING(MAKE_C_IDENTIFIER "${_irrklang_PLUGIN_NAME}" _irrklang_PLUGIN_VAR)
    FIND_FILE(IRRKLANG_PLUGIN_${_irrklang_PLUGIN_VAR}
        NAMES "${_irrklang_PLUGIN_NAME}"
        PATHS ${_irrklang_RUNTIME_SEARCH_DIRS}
    )
    IF(IRRKLANG_PLUGIN_${_irrklang_PLUGIN_VAR})
        LIST(APPEND IRRKLANG_PLUGIN_RUNTIME_LIBRARIES "${IRRKLANG_PLUGIN_${_irrklang_PLUGIN_VAR}}")
    ENDIF()
ENDFOREACH()

# ---- handle standard CMake find package ----
INCLUDE(FindPackageHandleStandardArgs)
SET(_irrklang_REQUIRED_VARS IRRKLANG_INCLUDE_DIR IRRKLANG_LIBRARY)
IF(WIN32)
    LIST(APPEND _irrklang_REQUIRED_VARS IRRKLANG_RUNTIME_LIBRARY)
ENDIF()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    IrrKlang
    DEFAULT_MSG
    ${_irrklang_REQUIRED_VARS}
)

IF(IRRKLANG_FOUND OR IrrKlang_FOUND)
    SET(IRRKLANG_FOUND TRUE)
    SET(IrrKlang_FOUND TRUE)
    SET(IRRKLANG_INCLUDE_DIRS "${IRRKLANG_INCLUDE_DIR}")
    SET(IRRKLANG_LIBRARIES "${IRRKLANG_LIBRARY}")

    IF(NOT IrrKlang_FIND_QUIETLY)
        MESSAGE(STATUS "IRRKLANG_INCLUDE_DIR = ${IRRKLANG_INCLUDE_DIR}")
        MESSAGE(STATUS "IRRKLANG_LIBRARY      = ${IRRKLANG_LIBRARY}")
    ENDIF()
    IF(NOT TARGET IrrKlang::IrrKlang)
        ADD_LIBRARY(IrrKlang::IrrKlang SHARED IMPORTED)
        SET_TARGET_PROPERTIES(IrrKlang::IrrKlang PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${IRRKLANG_INCLUDE_DIR}"
        )
        IF(WIN32 AND IRRKLANG_RUNTIME_LIBRARY)
            SET_TARGET_PROPERTIES(IrrKlang::IrrKlang PROPERTIES
                IMPORTED_IMPLIB "${IRRKLANG_LIBRARY}"
                IMPORTED_LOCATION "${IRRKLANG_RUNTIME_LIBRARY}"
            )
        ELSE()
            SET_TARGET_PROPERTIES(IrrKlang::IrrKlang PROPERTIES
                IMPORTED_LOCATION "${IRRKLANG_LIBRARY}"
            )
        ENDIF()
    ENDIF()
ENDIF()

MARK_AS_ADVANCED(IRRKLANG_INCLUDE_DIR IRRKLANG_LIBRARY IRRKLANG_RUNTIME_LIBRARY)

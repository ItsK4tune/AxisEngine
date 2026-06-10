# FindFMOD - locates the FMOD Core API.
#
# Result variables:
#   FMOD_FOUND        - true when FMOD was found
#   FMOD_INCLUDE_DIRS - directory containing fmod.hpp
#   FMOD_LIBRARIES    - library used for linking
#
# Optional hints:
#   FMOD_ROOT_DIR environment variable or CMake cache variable

find_package(fmod CONFIG QUIET)
if(NOT TARGET FMOD::FMOD)
    find_package(FMOD CONFIG QUIET)
endif()

if(TARGET FMOD::FMOD)
    get_target_property(FMOD_INCLUDE_DIRS FMOD::FMOD INTERFACE_INCLUDE_DIRECTORIES)
    if(FMOD_INCLUDE_DIRS)
        list(GET FMOD_INCLUDE_DIRS 0 FMOD_INCLUDE_DIR)
    endif()

    get_target_property(FMOD_LIBRARY_RELEASE FMOD::FMOD IMPORTED_IMPLIB_RELEASE)
    if(NOT FMOD_LIBRARY_RELEASE)
        get_target_property(FMOD_LIBRARY_RELEASE FMOD::FMOD IMPORTED_LOCATION_RELEASE)
    endif()

    get_target_property(FMOD_LIBRARY_DEBUG FMOD::FMOD IMPORTED_IMPLIB_DEBUG)
    if(NOT FMOD_LIBRARY_DEBUG)
        get_target_property(FMOD_LIBRARY_DEBUG FMOD::FMOD IMPORTED_LOCATION_DEBUG)
    endif()

    get_target_property(FMOD_RUNTIME_LIBRARY_RELEASE FMOD::FMOD IMPORTED_LOCATION_RELEASE)
    get_target_property(FMOD_RUNTIME_LIBRARY_DEBUG FMOD::FMOD IMPORTED_LOCATION_DEBUG)

    if(NOT FMOD_RUNTIME_LIBRARY_RELEASE)
        get_target_property(FMOD_RUNTIME_LIBRARY_RELEASE FMOD::FMOD IMPORTED_LOCATION)
    endif()
    if(NOT FMOD_LIBRARY_RELEASE)
        get_target_property(FMOD_LIBRARY_RELEASE FMOD::FMOD IMPORTED_IMPLIB)
    endif()

    set(FMOD_FOUND TRUE)
    set(FMOD_LIBRARIES "${FMOD_LIBRARY_RELEASE}")
    set(FMOD_RUNTIME_LIBRARY "${FMOD_RUNTIME_LIBRARY_RELEASE}")
    return()
endif()

SET(_fmod_ENV_ROOT "$ENV{FMOD_ROOT_DIR}")
IF(NOT FMOD_ROOT_DIR AND _fmod_ENV_ROOT)
    SET(FMOD_ROOT_DIR "${_fmod_ENV_ROOT}")
ENDIF()

SET(_fmod_HEADER_SEARCH_DIRS
    "/usr/include"
    "/usr/local/include"
    "/opt/fmod/api/core/inc"
    "C:/Program Files/FMOD SoundSystem/FMOD Studio API Windows/api/core/inc"
    "C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/inc"
)

SET(_fmod_LIB_SEARCH_DIRS
    "/usr/lib"
    "/usr/local/lib"
    "/opt/fmod/api/core/lib/x86_64"
    "/opt/fmod/api/core/lib"
    "C:/Program Files/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64"
    "C:/Program Files/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x86"
    "C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64"
    "C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x86"
)

SET(_fmod_RUNTIME_SEARCH_DIRS
    "/usr/lib"
    "/usr/local/lib"
    "/opt/fmod/api/core/lib/x86_64"
    "/opt/fmod/api/core/lib"
    "C:/Program Files/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64"
    "C:/Program Files/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x86"
    "C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64"
    "C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x86"
)

IF(FMOD_ROOT_DIR)
    LIST(PREPEND _fmod_HEADER_SEARCH_DIRS
        "${FMOD_ROOT_DIR}"
        "${FMOD_ROOT_DIR}/include"
        "${FMOD_ROOT_DIR}/inc"
        "${FMOD_ROOT_DIR}/api/core/inc"
    )
    LIST(PREPEND _fmod_LIB_SEARCH_DIRS
        "${FMOD_ROOT_DIR}/lib"
        "${FMOD_ROOT_DIR}/lib/x64"
        "${FMOD_ROOT_DIR}/lib/x86"
        "${FMOD_ROOT_DIR}/api/core/lib"
        "${FMOD_ROOT_DIR}/api/core/lib/x64"
        "${FMOD_ROOT_DIR}/api/core/lib/x86"
        "${FMOD_ROOT_DIR}/api/core/lib/x86_64"
    )
    LIST(PREPEND _fmod_RUNTIME_SEARCH_DIRS
        "${FMOD_ROOT_DIR}/bin"
        "${FMOD_ROOT_DIR}/lib"
        "${FMOD_ROOT_DIR}/lib/x64"
        "${FMOD_ROOT_DIR}/lib/x86"
        "${FMOD_ROOT_DIR}/api/core/lib"
        "${FMOD_ROOT_DIR}/api/core/lib/x64"
        "${FMOD_ROOT_DIR}/api/core/lib/x86"
        "${FMOD_ROOT_DIR}/api/core/lib/x86_64"
    )
ENDIF()

FIND_PATH(FMOD_INCLUDE_DIR "fmod.hpp"
    PATHS ${_fmod_HEADER_SEARCH_DIRS}
)

FIND_LIBRARY(FMOD_LIBRARY_RELEASE
    NAMES fmod_vc fmod fmod64
    PATHS ${_fmod_LIB_SEARCH_DIRS}
)

FIND_LIBRARY(FMOD_LIBRARY_DEBUG
    NAMES fmodL_vc fmodL fmodL64
    PATHS ${_fmod_LIB_SEARCH_DIRS}
)

FIND_FILE(FMOD_RUNTIME_LIBRARY_RELEASE
    NAMES fmod.dll libfmod.so libfmod.dylib
    PATHS ${_fmod_RUNTIME_SEARCH_DIRS}
)

FIND_FILE(FMOD_RUNTIME_LIBRARY_DEBUG
    NAMES fmodL.dll libfmodL.so libfmodL.dylib
    PATHS ${_fmod_RUNTIME_SEARCH_DIRS}
)

INCLUDE(FindPackageHandleStandardArgs)

SET(_fmod_REQUIRED_VARS FMOD_INCLUDE_DIR FMOD_LIBRARY_RELEASE)
IF(WIN32)
    LIST(APPEND _fmod_REQUIRED_VARS FMOD_RUNTIME_LIBRARY_RELEASE)
ENDIF()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    FMOD
    DEFAULT_MSG
    ${_fmod_REQUIRED_VARS}
)

IF(FMOD_FOUND)
    SET(FMOD_INCLUDE_DIRS "${FMOD_INCLUDE_DIR}")
    SET(FMOD_LIBRARIES "${FMOD_LIBRARY_RELEASE}")
    SET(FMOD_RUNTIME_LIBRARY "${FMOD_RUNTIME_LIBRARY_RELEASE}")

    IF(NOT TARGET FMOD::FMOD)
        ADD_LIBRARY(FMOD::FMOD SHARED IMPORTED)
        SET_TARGET_PROPERTIES(FMOD::FMOD PROPERTIES
            IMPORTED_LOCATION "${FMOD_LIBRARY_RELEASE}"
            INTERFACE_INCLUDE_DIRECTORIES "${FMOD_INCLUDE_DIR}"
        )

        IF(WIN32 AND FMOD_RUNTIME_LIBRARY_RELEASE)
            SET_TARGET_PROPERTIES(FMOD::FMOD PROPERTIES
                IMPORTED_IMPLIB "${FMOD_LIBRARY_RELEASE}"
                IMPORTED_LOCATION "${FMOD_RUNTIME_LIBRARY_RELEASE}"
            )
        ENDIF()

        IF(FMOD_LIBRARY_DEBUG)
            IF(WIN32)
                SET(_fmod_RUNTIME_DEBUG "${FMOD_RUNTIME_LIBRARY_RELEASE}")
                IF(FMOD_RUNTIME_LIBRARY_DEBUG)
                    SET(_fmod_RUNTIME_DEBUG "${FMOD_RUNTIME_LIBRARY_DEBUG}")
                ENDIF()
                SET_TARGET_PROPERTIES(FMOD::FMOD PROPERTIES
                    IMPORTED_IMPLIB_DEBUG "${FMOD_LIBRARY_DEBUG}"
                    IMPORTED_IMPLIB_RELEASE "${FMOD_LIBRARY_RELEASE}"
                    IMPORTED_IMPLIB_RELWITHDEBINFO "${FMOD_LIBRARY_RELEASE}"
                    IMPORTED_IMPLIB_MINSIZEREL "${FMOD_LIBRARY_RELEASE}"
                    IMPORTED_LOCATION_DEBUG "${_fmod_RUNTIME_DEBUG}"
                    IMPORTED_LOCATION_RELEASE "${FMOD_RUNTIME_LIBRARY_RELEASE}"
                    IMPORTED_LOCATION_RELWITHDEBINFO "${FMOD_RUNTIME_LIBRARY_RELEASE}"
                    IMPORTED_LOCATION_MINSIZEREL "${FMOD_RUNTIME_LIBRARY_RELEASE}"
                )
            ELSE()
                SET_TARGET_PROPERTIES(FMOD::FMOD PROPERTIES
                    IMPORTED_LOCATION_DEBUG "${FMOD_LIBRARY_DEBUG}"
                    IMPORTED_LOCATION_RELEASE "${FMOD_LIBRARY_RELEASE}"
                    IMPORTED_LOCATION_RELWITHDEBINFO "${FMOD_LIBRARY_RELEASE}"
                    IMPORTED_LOCATION_MINSIZEREL "${FMOD_LIBRARY_RELEASE}"
                )
            ENDIF()
        ENDIF()
    ENDIF()
ENDIF()

MARK_AS_ADVANCED(FMOD_INCLUDE_DIR FMOD_LIBRARY_RELEASE FMOD_LIBRARY_DEBUG FMOD_RUNTIME_LIBRARY_RELEASE
                 FMOD_RUNTIME_LIBRARY_DEBUG FMOD_RUNTIME_LIBRARY)

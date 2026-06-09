# - Try to find FreeType
# Once done, this will define
#
# FREETYPE_FOUND - system has FreeType
# FREETYPE_INCLUDE_DIR - the FreeType include directories
# FREETYPE_LIBRARIES - link these to use FreeType

FIND_PATH(FREETYPE_INCLUDE_DIR ft2build.h
    PATH_SUFFIXES freetype freetype2
    PATHS
    /usr/include/freetype2
    /usr/local/include/freetype2
    /opt/local/include/freetype2
    /opt/homebrew/include/freetype2
    /usr/include
    /usr/local/include
    /opt/local/include
    /opt/homebrew/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/freetype
)

FIND_LIBRARY(FREETYPE_LIBRARY freetype
    /usr/lib64
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    /opt/homebrew/lib
    ${CMAKE_SOURCE_DIR}/lib
)

IF(FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARY)
    SET(FREETYPE_FOUND TRUE)
    SET(Freetype_FOUND TRUE)
    SET(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})
ENDIF(FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARY)

IF(FREETYPE_FOUND)
    IF(NOT FREETYPE_FIND_QUIETLY)
        MESSAGE(STATUS "Found FreeType: ${FREETYPE_LIBRARIES}")
        MESSAGE(STATUS "FreeType include: ${FREETYPE_INCLUDE_DIR}")
    ENDIF()
    IF(NOT TARGET Freetype::Freetype)
        ADD_LIBRARY(Freetype::Freetype UNKNOWN IMPORTED)
        SET_TARGET_PROPERTIES(Freetype::Freetype PROPERTIES
            IMPORTED_LOCATION "${FREETYPE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIR}"
        )
    ENDIF()
ELSE()
    IF(FREETYPE_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find FreeType library")
    ENDIF()
ENDIF()

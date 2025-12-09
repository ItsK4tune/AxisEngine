# - Try to find FreeType
# Once done, this will define
#
# FREETYPE_FOUND - system has FreeType
# FREETYPE_INCLUDE_DIR - the FreeType include directories
# FREETYPE_LIBRARIES - link these to use FreeType

FIND_PATH(FREETYPE_INCLUDE_DIR ft2build.h
    /usr/include/freetype2
    /usr/local/include/freetype2
    /opt/local/include/freetype2
    /usr/include
    /usr/local/include
    /opt/local/include
    ${CMAKE_SOURCE_DIR}/includes
)

FIND_LIBRARY(FREETYPE_LIBRARY freetype
    /usr/lib64
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    ${CMAKE_SOURCE_DIR}/lib
)

IF(FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARY)
    SET(FREETYPE_FOUND TRUE)
    SET(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})
ENDIF(FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARY)

IF(FREETYPE_FOUND)
    IF(NOT FREETYPE_FIND_QUIETLY)
        MESSAGE(STATUS "Found FreeType: ${FREETYPE_LIBRARIES}")
        MESSAGE(STATUS "FreeType include: ${FREETYPE_INCLUDE_DIR}")
    ENDIF()
ELSE()
    IF(FREETYPE_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find FreeType library")
    ENDIF()
ENDIF()

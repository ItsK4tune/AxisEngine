# - Try to find FFmpeg
# Once done, this will define:
#
#   FFMPEG_FOUND           - system has FFmpeg
#   FFMPEG_INCLUDE_DIRS    - the FFmpeg include directories
#   FFMPEG_LIBRARIES       - all FFmpeg libraries
#
#   AVCODEC_LIBRARY        - avcodec library
#   AVFORMAT_LIBRARY       - avformat library
#   AVUTIL_LIBRARY         - avutil library
#   SWSCALE_LIBRARY        - swscale library
#   SWRESAMPLE_LIBRARY     - swresample library

# --- Include directory ---
FIND_PATH(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h
    /usr/include
    /usr/local/include
    /opt/local/include
    ${CMAKE_SOURCE_DIR}/includes/ffmpeg
    ${CMAKE_SOURCE_DIR}/includes
)

# --- Libraries ---
FIND_LIBRARY(AVCODEC_LIBRARY avcodec
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/ffmpeg
    /usr/lib
    /usr/local/lib
)

FIND_LIBRARY(AVFORMAT_LIBRARY avformat
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/ffmpeg
    /usr/lib
    /usr/local/lib
)

FIND_LIBRARY(AVUTIL_LIBRARY avutil
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/ffmpeg
    /usr/lib
    /usr/local/lib
)

FIND_LIBRARY(SWSCALE_LIBRARY swscale
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/ffmpeg
    /usr/lib
    /usr/local/lib
)

FIND_LIBRARY(SWRESAMPLE_LIBRARY swresample
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/ffmpeg
    /usr/lib
    /usr/local/lib
)

# --- Validate ---
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFMPEG DEFAULT_MSG
    FFMPEG_INCLUDE_DIR
    AVCODEC_LIBRARY
    AVFORMAT_LIBRARY
    AVUTIL_LIBRARY
    SWSCALE_LIBRARY
    SWRESAMPLE_LIBRARY
)

if(FFMPEG_FOUND)
    set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
    set(FFMPEG_LIBRARIES
        ${AVCODEC_LIBRARY}
        ${AVFORMAT_LIBRARY}
        ${AVUTIL_LIBRARY}
        ${SWSCALE_LIBRARY}
        ${SWRESAMPLE_LIBRARY}
    )
endif()

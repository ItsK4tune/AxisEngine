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
    PATH_SUFFIXES ffmpeg
    PATHS
    /usr/include
    /usr/local/include
    /opt/local/include
    /opt/homebrew/include
)

# --- Libraries ---
FIND_LIBRARY(AVCODEC_LIBRARY avcodec
    /usr/lib
    /usr/local/lib
    /opt/homebrew/lib
)

FIND_LIBRARY(AVFORMAT_LIBRARY avformat
    /usr/lib
    /usr/local/lib
    /opt/homebrew/lib
)

FIND_LIBRARY(AVUTIL_LIBRARY avutil
    /usr/lib
    /usr/local/lib
    /opt/homebrew/lib
)

FIND_LIBRARY(SWSCALE_LIBRARY swscale
    /usr/lib
    /usr/local/lib
    /opt/homebrew/lib
)

FIND_LIBRARY(SWRESAMPLE_LIBRARY swresample
    /usr/lib
    /usr/local/lib
    /opt/homebrew/lib
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
    if(NOT TARGET FFMPEG::FFMPEG)
        add_library(FFMPEG::FFMPEG INTERFACE IMPORTED)
        set_target_properties(FFMPEG::FFMPEG PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${FFMPEG_LIBRARIES}"
        )
    endif()
endif()

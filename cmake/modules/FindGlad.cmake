# FindGlad - locates the GLAD OpenGL loader.
#
# Result target:
#   Glad::Glad

find_package(glad CONFIG QUIET)
if(TARGET glad::glad)
    if(NOT TARGET Glad::Glad)
        add_library(Glad::Glad INTERFACE IMPORTED)
        set_target_properties(Glad::Glad PROPERTIES
            INTERFACE_LINK_LIBRARIES glad::glad
        )
    endif()
    set(Glad_FOUND TRUE)
    return()
endif()

set(_glad_ROOT_HINTS
    ${Glad_ROOT_DIR}
    $ENV{Glad_ROOT_DIR}
    ${GLAD_ROOT_DIR}
    $ENV{GLAD_ROOT_DIR}
)

find_path(GLAD_INCLUDE_DIR
    NAMES glad/glad.h
    HINTS ${_glad_ROOT_HINTS}
    PATH_SUFFIXES include
)

find_library(GLAD_LIBRARY
    NAMES glad
    HINTS ${_glad_ROOT_HINTS}
    PATH_SUFFIXES lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Glad DEFAULT_MSG GLAD_INCLUDE_DIR GLAD_LIBRARY)

if(Glad_FOUND AND NOT TARGET Glad::Glad)
    add_library(Glad::Glad UNKNOWN IMPORTED)
    set_target_properties(Glad::Glad PROPERTIES
        IMPORTED_LOCATION "${GLAD_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GLAD_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(GLAD_INCLUDE_DIR GLAD_LIBRARY)

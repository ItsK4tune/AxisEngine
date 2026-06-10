# FindSTB - locates stb headers.
#
# Result target:
#   STB::STB

set(_stb_ROOT_HINTS
    ${STB_ROOT_DIR}
    $ENV{STB_ROOT_DIR}
)

find_path(STB_INCLUDE_DIR
    NAMES stb_image.h
    HINTS ${_stb_ROOT_HINTS}
    PATH_SUFFIXES include include/stb stb
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(STB DEFAULT_MSG STB_INCLUDE_DIR)

if(STB_FOUND AND NOT TARGET STB::STB)
    add_library(STB::STB INTERFACE IMPORTED)
    set_target_properties(STB::STB PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${STB_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(STB_INCLUDE_DIR)

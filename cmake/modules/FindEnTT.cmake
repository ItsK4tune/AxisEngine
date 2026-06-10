# FindEnTT - locates the EnTT header-only ECS library.
#
# Result target:
#   EnTT::EnTT

find_package(EnTT CONFIG QUIET)
if(TARGET EnTT::EnTT)
    set(EnTT_FOUND TRUE)
    return()
endif()

set(_entt_ROOT_HINTS
    ${EnTT_ROOT_DIR}
    $ENV{EnTT_ROOT_DIR}
    ${ENTT_ROOT_DIR}
    $ENV{ENTT_ROOT_DIR}
)

find_path(ENTT_INCLUDE_DIR
    NAMES entt/entt.hpp
    HINTS ${_entt_ROOT_HINTS}
    PATH_SUFFIXES include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EnTT DEFAULT_MSG ENTT_INCLUDE_DIR)

if(EnTT_FOUND AND NOT TARGET EnTT::EnTT)
    add_library(EnTT::EnTT INTERFACE IMPORTED)
    set_target_properties(EnTT::EnTT PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ENTT_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(ENTT_INCLUDE_DIR)

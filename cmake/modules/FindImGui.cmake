# FindImGui - locates Dear ImGui with GLFW and OpenGL backend bindings.
#
# Result target:
#   ImGui::ImGui

find_package(imgui CONFIG QUIET)
if(TARGET imgui::imgui)
    if(NOT TARGET ImGui::ImGui)
        add_library(ImGui::ImGui INTERFACE IMPORTED)
        set_target_properties(ImGui::ImGui PROPERTIES
            INTERFACE_LINK_LIBRARIES imgui::imgui
        )
    endif()

    find_path(IMGUI_BACKENDS_INCLUDE_DIR
        NAMES imgui_impl_glfw.h
        PATH_SUFFIXES include include/backends include/imgui include/imgui/backends backends imgui/backends
    )
    if(IMGUI_BACKENDS_INCLUDE_DIR)
        set_property(TARGET ImGui::ImGui APPEND PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES "${IMGUI_BACKENDS_INCLUDE_DIR}"
        )
    endif()

    set(ImGui_FOUND TRUE)
    return()
endif()

set(_imgui_ROOT_HINTS
    ${ImGui_ROOT_DIR}
    $ENV{ImGui_ROOT_DIR}
    ${IMGUI_ROOT_DIR}
    $ENV{IMGUI_ROOT_DIR}
)

find_path(IMGUI_INCLUDE_DIR
    NAMES imgui.h
    HINTS ${_imgui_ROOT_HINTS}
    PATH_SUFFIXES include include/imgui imgui
)

find_path(IMGUI_BACKENDS_INCLUDE_DIR
    NAMES imgui_impl_glfw.h
    HINTS ${_imgui_ROOT_HINTS}
    PATH_SUFFIXES include include/backends include/imgui include/imgui/backends backends imgui/backends
)

find_library(IMGUI_LIBRARY
    NAMES imgui
    HINTS ${_imgui_ROOT_HINTS}
    PATH_SUFFIXES lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ImGui DEFAULT_MSG
    IMGUI_INCLUDE_DIR
    IMGUI_BACKENDS_INCLUDE_DIR
    IMGUI_LIBRARY
)

if(ImGui_FOUND AND NOT TARGET ImGui::ImGui)
    add_library(ImGui::ImGui UNKNOWN IMPORTED)
    set_target_properties(ImGui::ImGui PROPERTIES
        IMPORTED_LOCATION "${IMGUI_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${IMGUI_INCLUDE_DIR};${IMGUI_BACKENDS_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(IMGUI_INCLUDE_DIR IMGUI_BACKENDS_INCLUDE_DIR IMGUI_LIBRARY)

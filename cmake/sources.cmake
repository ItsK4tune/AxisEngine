function(axis_append_sources_from_dir target_var source_dir)
    if(NOT EXISTS "${source_dir}")
        return()
    endif()

    file(GLOB_RECURSE _axis_sources CONFIGURE_DEPENDS
        "${source_dir}/*.c"
        "${source_dir}/*.cc"
        "${source_dir}/*.cpp"
        "${source_dir}/*.cxx"
    )

    if(_axis_sources)
        list(APPEND ${target_var} ${_axis_sources})
        set(${target_var} "${${target_var}}" PARENT_SCOPE)
    endif()
endfunction()

file(GLOB_RECURSE AXIS_ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.c"
    "${CMAKE_SOURCE_DIR}/src/*.cc"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cxx"
)

set(_AXIS_ENGINE_EXCLUDED_PATTERNS
    "/src/audio/strategy/"
    "/src/editor/"
    "/src/physics/strategy/"
    "/src/platform/strategy/"
    "/src/render/strategy/"
    "/src/external/"
)

set(_AXIS_FILTERED_ENGINE_SOURCES)
foreach(_axis_source IN LISTS AXIS_ENGINE_SOURCES)
    file(TO_CMAKE_PATH "${_axis_source}" _axis_source_norm)
    set(_axis_excluded FALSE)
    foreach(_axis_pattern IN LISTS _AXIS_ENGINE_EXCLUDED_PATTERNS)
        if(_axis_source_norm MATCHES "${_axis_pattern}")
            set(_axis_excluded TRUE)
            break()
        endif()
    endforeach()

    if(NOT _axis_excluded)
        list(APPEND _AXIS_FILTERED_ENGINE_SOURCES "${_axis_source}")
    endif()
endforeach()
set(AXIS_ENGINE_SOURCES ${_AXIS_FILTERED_ENGINE_SOURCES})

axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/audio/strategy/null")

if(AXIS_HAS_IRRKLANG_BACKEND)
    axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/audio/strategy/irrklang")
endif()

if(AXIS_HAS_FMOD_BACKEND)
    axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/audio/strategy/fmod")
endif()

if(AXIS_HAS_BULLET_BACKEND)
    axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/physics/strategy/bullet")
endif()

if(AXIS_HAS_PHYSX_BACKEND)
    axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/physics/strategy/physx")
endif()

axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/platform/strategy/glfw")

axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/render/rhi")

if(AXIS_HAS_OPENGL_BACKEND)
    axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/render/strategy/opengl")
endif()

axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/render/strategy/null")

if(AXIS_HAS_VULKAN_BACKEND)
    axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/render/strategy/vulkan")
endif()

if(AXIS_HAS_DIRECTX_BACKEND)
    axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/render/strategy/directx")
endif()

if(WIN32)
    axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/platform/strategy/windows")
elseif(APPLE)
    axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/platform/strategy/macos")
else()
    axis_append_sources_from_dir(AXIS_ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/platform/strategy/posix")
endif()

list(REMOVE_DUPLICATES AXIS_ENGINE_SOURCES)
list(SORT AXIS_ENGINE_SOURCES)

file(GLOB_RECURSE AXIS_EDITOR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/editor/*.cc"
    "${CMAKE_SOURCE_DIR}/src/editor/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/editor/*.cxx"
)

list(REMOVE_DUPLICATES AXIS_EDITOR_SOURCES)
list(SORT AXIS_EDITOR_SOURCES)

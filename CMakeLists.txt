cmake_minimum_required(VERSION 3.20)

project(AxisEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_EDITOR "Enable Editor System (ImGui)" OFF)
option(BUILD_SAMPLES "Build sample application when editor is enabled" ON)
option(ENABLE_TESTS "Build AxisEngine automated tests" OFF)
option(ENABLE_LTO "Enable interprocedural optimization for optimized builds" ON)
option(ENABLE_PCH "Use private precompiled headers to reduce project compile time" ON)
option(ENABLE_UNITY_BUILD "Batch sources into unity translation units to reduce compile time" ON)
option(ENABLE_PARALLEL_COMPILE "Compile source files concurrently when supported" ON)
set(AXIS_UNITY_BATCH_SIZE 8 CACHE STRING "Maximum source files per unity translation unit")

set(AXIS_PCH_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pch/axis_pch.hpp")

function(axis_enable_pch target)
    if(ENABLE_PCH)
        target_precompile_headers(${target} PRIVATE "${AXIS_PCH_HEADER}")
    endif()
endfunction()

function(axis_optimize_build target)
    axis_enable_pch(${target})

    if(MSVC AND ENABLE_PARALLEL_COMPILE)
        # /MP parallelizes compilation within a Visual Studio target. /FS makes
        # concurrent writes to the compiler PDB safe.
        target_compile_options(${target} PRIVATE /MP /FS /bigobj)
    endif()

    if(WIN32)
        # Prevent Windows SDK macros from leaking between files in a unity batch.
        target_compile_definitions(${target} PRIVATE NOMINMAX)
    endif()

    if(ENABLE_UNITY_BUILD)
        set_target_properties(${target} PROPERTIES
            UNITY_BUILD ON
            UNITY_BUILD_BATCH_SIZE ${AXIS_UNITY_BATCH_SIZE}
        )
    endif()
endfunction()

if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT AXIS_IPO_SUPPORTED OUTPUT AXIS_IPO_ERROR)
    if(AXIS_IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
    else()
        message(WARNING "AxisEngine LTO is unavailable: ${AXIS_IPO_ERROR}")
    endif()
endif()

set(AXIS_GRAPHICS_BACKEND "OpenGL" CACHE STRING "Graphics backend to compile")
set_property(CACHE AXIS_GRAPHICS_BACKEND PROPERTY STRINGS OpenGL)

set(AXIS_PHYSICS_BACKEND "Bullet" CACHE STRING "Physics backend to compile")
set_property(CACHE AXIS_PHYSICS_BACKEND PROPERTY STRINGS Bullet)

set(_AXIS_DEFAULT_AUDIO_BACKEND "Null")
set(AXIS_AUDIO_BACKEND "${_AXIS_DEFAULT_AUDIO_BACKEND}" CACHE STRING "Audio backend to compile")
set_property(CACHE AXIS_AUDIO_BACKEND PROPERTY STRINGS Null IrrKlang FMOD)

if(ENABLE_EDITOR)
    message(STATUS "Editor System Enabled")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

foreach(_axis_config IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${_axis_config}" _axis_config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_axis_config_upper} "${CMAKE_BINARY_DIR}/bin/${_axis_config}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_axis_config_upper} "${CMAKE_BINARY_DIR}/lib/${_axis_config}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_axis_config_upper} "${CMAKE_BINARY_DIR}/lib/${_axis_config}")
endforeach()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

find_package(ASSIMP REQUIRED)
find_package(GLFW3 REQUIRED)
find_package(GLM REQUIRED)
find_package(EnTT REQUIRED)
find_package(STB REQUIRED)
find_package(Freetype REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(Enet REQUIRED)
if(ENABLE_EDITOR)
    find_package(ImGui REQUIRED)
endif()

string(TOUPPER "${AXIS_GRAPHICS_BACKEND}" AXIS_GRAPHICS_BACKEND_UPPER)
string(TOUPPER "${AXIS_PHYSICS_BACKEND}" AXIS_PHYSICS_BACKEND_UPPER)
string(TOUPPER "${AXIS_AUDIO_BACKEND}" AXIS_AUDIO_BACKEND_UPPER)

set(AXIS_HAS_OPENGL_BACKEND 0)
set(AXIS_HAS_VULKAN_BACKEND 0)
set(AXIS_HAS_DIRECTX_BACKEND 0)
if(AXIS_GRAPHICS_BACKEND_UPPER STREQUAL "OPENGL")
    find_package(Glad REQUIRED)
    set(AXIS_HAS_OPENGL_BACKEND 1)
elseif(AXIS_GRAPHICS_BACKEND_UPPER STREQUAL "VULKAN")
    message(FATAL_ERROR "AXIS_GRAPHICS_BACKEND=Vulkan is not provided by this release. Select OpenGL.")
elseif(AXIS_GRAPHICS_BACKEND_UPPER STREQUAL "DIRECTX")
    message(FATAL_ERROR "AXIS_GRAPHICS_BACKEND=DirectX is not provided by this release. Select OpenGL.")
else()
    message(FATAL_ERROR "Unsupported AXIS_GRAPHICS_BACKEND='${AXIS_GRAPHICS_BACKEND}'. This release provides OpenGL.")
endif()

set(AXIS_HAS_BULLET_BACKEND 0)
set(AXIS_HAS_PHYSX_BACKEND 0)
if(AXIS_PHYSICS_BACKEND_UPPER STREQUAL "BULLET")
    find_package(Bullet REQUIRED)
    set(AXIS_HAS_BULLET_BACKEND 1)
elseif(AXIS_PHYSICS_BACKEND_UPPER STREQUAL "PHYSX")
    message(FATAL_ERROR "AXIS_PHYSICS_BACKEND=PhysX is not provided by this release. Select Bullet.")
else()
    message(FATAL_ERROR "Unsupported AXIS_PHYSICS_BACKEND='${AXIS_PHYSICS_BACKEND}'. This release provides Bullet.")
endif()

set(AXIS_HAS_IRRKLANG_BACKEND 0)
set(AXIS_HAS_FMOD_BACKEND 0)
set(AXIS_HAS_NULL_AUDIO_BACKEND 1)
if(AXIS_AUDIO_BACKEND_UPPER STREQUAL "NULL")
    message(STATUS "Audio backend selected: Null")
elseif(AXIS_AUDIO_BACKEND_UPPER STREQUAL "IRRKLANG")
    find_package(IrrKlang QUIET)
    if(NOT IrrKlang_FOUND)
        message(FATAL_ERROR
            "AXIS_AUDIO_BACKEND=IrrKlang requires the irrKlang SDK. "
            "Install it and set IRRKLANG_ROOT_DIR, or use -DAXIS_AUDIO_BACKEND=Null."
        )
    endif()
    set(AXIS_HAS_IRRKLANG_BACKEND 1)
elseif(AXIS_AUDIO_BACKEND_UPPER STREQUAL "FMOD")
    find_package(FMOD QUIET)
    if(NOT FMOD_FOUND)
        message(FATAL_ERROR
            "AXIS_AUDIO_BACKEND=FMOD requires the FMOD Core SDK. "
            "Install it and set FMOD_ROOT_DIR, or use -DAXIS_AUDIO_BACKEND=Null."
        )
    endif()
    set(AXIS_HAS_FMOD_BACKEND 1)
else()
    message(FATAL_ERROR "Unknown AXIS_AUDIO_BACKEND='${AXIS_AUDIO_BACKEND}'. Expected Null, IrrKlang, or FMOD.")
endif()

if(AXIS_HAS_OPENGL_BACKEND AND NOT WIN32)
    find_package(OpenGL REQUIRED)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/sources.cmake)

# Engine Core Library
add_library(axis_engine STATIC ${AXIS_ENGINE_SOURCES})
add_library(Axis::Engine ALIAS axis_engine)
set_target_properties(axis_engine PROPERTIES EXPORT_NAME Engine)

# These implementation files define macros or file-local symbols that must stay
# in isolated translation units. The remaining engine sources can safely use
# unity batches.
set_source_files_properties(
    "${CMAKE_SOURCE_DIR}/src/resource/logic/stb_image_impl.cpp"
    "${CMAKE_SOURCE_DIR}/src/ecs/logic/ui_interact_system.cpp"
    "${CMAKE_SOURCE_DIR}/src/platform/strategy/windows/wasapi_audio_capture_service.cpp"
    "${CMAKE_SOURCE_DIR}/src/audio/strategy/null/null_audio_engine.cpp"
    "${CMAKE_SOURCE_DIR}/src/audio/strategy/irrklang/irrklang_audio_engine.cpp"
    "${CMAKE_SOURCE_DIR}/src/audio/strategy/fmod/fmod_audio_engine.cpp"
    "${CMAKE_SOURCE_DIR}/src/editor/editor_bootstrap.cpp"
    "${CMAKE_SOURCE_DIR}/src/editor/panels/file_hierarchy_panel.cpp"
    "${CMAKE_SOURCE_DIR}/sample/src/sample_state.cpp"
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)

target_compile_features(axis_engine PUBLIC cxx_std_20)

target_compile_definitions(axis_engine PUBLIC
    GLM_ENABLE_EXPERIMENTAL
    AXIS_HAS_OPENGL_BACKEND=${AXIS_HAS_OPENGL_BACKEND}
    AXIS_HAS_VULKAN_BACKEND=${AXIS_HAS_VULKAN_BACKEND}
    AXIS_HAS_DIRECTX_BACKEND=${AXIS_HAS_DIRECTX_BACKEND}
    AXIS_HAS_BULLET_BACKEND=${AXIS_HAS_BULLET_BACKEND}
    AXIS_HAS_PHYSX_BACKEND=${AXIS_HAS_PHYSX_BACKEND}
    AXIS_HAS_IRRKLANG_BACKEND=${AXIS_HAS_IRRKLANG_BACKEND}
    AXIS_HAS_FMOD_BACKEND=${AXIS_HAS_FMOD_BACKEND}
    AXIS_HAS_OPENAL_BACKEND=0
    AXIS_HAS_NULL_AUDIO_BACKEND=${AXIS_HAS_NULL_AUDIO_BACKEND}
)

if(MSVC)
    target_compile_definitions(axis_engine PRIVATE _ENABLE_EXTENDED_ALIGNED_STORAGE)
    add_link_options($<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
endif()

axis_optimize_build(axis_engine)

target_include_directories(axis_engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/engine>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/engine>
)

target_link_libraries(axis_engine PUBLIC
    GLFW3::GLFW3
    ASSIMP::ASSIMP
    Freetype::Freetype
    FFMPEG::FFMPEG
    Enet::Enet
    GLM::GLM
    EnTT::EnTT
    STB::STB
)

if(AXIS_HAS_BULLET_BACKEND)
    target_link_libraries(axis_engine PUBLIC Bullet::Bullet)
endif()

if(AXIS_HAS_OPENGL_BACKEND)
    target_link_libraries(axis_engine PUBLIC Glad::Glad)
endif()

if(AXIS_HAS_IRRKLANG_BACKEND)
    target_include_directories(axis_engine PUBLIC
        $<BUILD_INTERFACE:${IRRKLANG_INCLUDE_DIR}>
    )
    target_link_libraries(axis_engine PUBLIC IrrKlang::IrrKlang)

    set(_axis_irrklang_runtime_libraries
        ${IRRKLANG_RUNTIME_LIBRARY}
        ${IRRKLANG_PLUGIN_RUNTIME_LIBRARIES}
    )
    foreach(_axis_irrklang_runtime IN LISTS _axis_irrklang_runtime_libraries)
        if(_axis_irrklang_runtime)
            add_custom_command(TARGET axis_engine POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_axis_irrklang_runtime}"
                    "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/"
                COMMENT "Copying irrKlang runtime library beside $<CONFIG> executables"
            )
        endif()
    endforeach()
endif()

if(AXIS_HAS_FMOD_BACKEND)
    target_include_directories(axis_engine PUBLIC
        $<BUILD_INTERFACE:${FMOD_INCLUDE_DIRS}>
    )
    target_link_libraries(axis_engine PUBLIC FMOD::FMOD)

    if(FMOD_RUNTIME_LIBRARY_RELEASE)
        add_custom_command(TARGET axis_engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FMOD_RUNTIME_LIBRARY_RELEASE}"
                "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/"
            COMMENT "Copying FMOD release runtime library beside $<CONFIG> executables"
        )
    endif()

    if(FMOD_RUNTIME_LIBRARY_DEBUG)
        add_custom_command(TARGET axis_engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FMOD_RUNTIME_LIBRARY_DEBUG}"
                "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/"
            COMMENT "Copying FMOD debug runtime library beside $<CONFIG> executables"
        )
    endif()
endif()

if(WIN32)
    target_link_libraries(axis_engine PUBLIC
        opengl32
        dbghelp
        winmm
        ole32
        ws2_32
    )
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    target_link_libraries(axis_engine PUBLIC
        OpenGL::GL
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )
else()
    target_link_libraries(axis_engine PUBLIC
        OpenGL::GL
        pthread
        dl
    )
endif()

# Editor Library
if(ENABLE_EDITOR)
    add_library(axis_editor STATIC ${AXIS_EDITOR_SOURCES})
    add_library(Axis::Editor ALIAS axis_editor)
    set_target_properties(axis_editor PROPERTIES EXPORT_NAME Editor)

    target_include_directories(axis_editor PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    
    target_link_libraries(axis_editor PUBLIC
        axis_engine
        ImGui::ImGui
    )
    target_compile_definitions(axis_editor PUBLIC ENABLE_EDITOR)

    # Linking Axis::Editor is the opt-in switch for the editor runtime. Retain
    # only editor_bootstrap.cpp from the static archive; that bootstrap adds
    # EditorSystem to SystemFactory before Application creates its systems.
    if(MSVC)
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            target_link_options(axis_editor INTERFACE "/INCLUDE:_axis_editor_bootstrap_anchor")
        else()
            target_link_options(axis_editor INTERFACE "/INCLUDE:axis_editor_bootstrap_anchor")
        endif()
    elseif(APPLE)
        target_link_options(axis_editor INTERFACE "LINKER:-u,_axis_editor_bootstrap_anchor")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_options(axis_editor INTERFACE "LINKER:--undefined=axis_editor_bootstrap_anchor")
    endif()

    axis_optimize_build(axis_editor)
endif()

# Installation & Export Setup
include(GNUInstallDirs)

install(TARGETS axis_engine
    EXPORT AxisEngineTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(ENABLE_EDITOR)
    install(TARGETS axis_editor
        EXPORT AxisEngineTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PATTERN "engine/asset/*" EXCLUDE
    PATTERN "strategy" EXCLUDE
    PATTERN "modules" EXCLUDE
    PATTERN "panels" EXCLUDE
    PATTERN "static_batch_manager.h" EXCLUDE
    PATTERN "texture_atlas.h" EXCLUDE
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/engine/asset/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/AxisEngine/assets
)

if(AXIS_HAS_FMOD_BACKEND AND FMOD_RUNTIME_LIBRARY_RELEASE)
    install(FILES "${FMOD_RUNTIME_LIBRARY_RELEASE}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(AXIS_HAS_FMOD_BACKEND AND FMOD_RUNTIME_LIBRARY_DEBUG)
    install(FILES "${FMOD_RUNTIME_LIBRARY_DEBUG}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(AXIS_HAS_IRRKLANG_BACKEND)
    set(_axis_irrklang_install_runtime_libraries
        ${IRRKLANG_RUNTIME_LIBRARY}
        ${IRRKLANG_PLUGIN_RUNTIME_LIBRARIES}
    )
    foreach(_axis_irrklang_runtime IN LISTS _axis_irrklang_install_runtime_libraries)
        if(_axis_irrklang_runtime)
            install(FILES "${_axis_irrklang_runtime}"
                DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        endif()
    endforeach()
endif()

install(EXPORT AxisEngineTargets
    FILE AxisEngineTargets.cmake
    NAMESPACE Axis::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AxisEngine
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/cmake/modules/
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AxisEngine/modules
)

# Generate Config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/AxisEngineConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/AxisEngineConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AxisEngine
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/AxisEngineConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AxisEngine
)

# Sample app
if(ENABLE_EDITOR AND BUILD_SAMPLES)
    file(GLOB SAMPLE_SCENARIO_SOURCES CONFIGURE_DEPENDS
        sample/src/scenarios/*.cpp
    )
    file(GLOB SAMPLE_SCRIPTABLE_SOURCES CONFIGURE_DEPENDS
        sample/src/scriptable/*.cpp
    )

    set(SAMPLE_SOURCES
        sample/src/main.cpp
        sample/src/sample_state.cpp
        ${SAMPLE_SCENARIO_SOURCES}
        ${SAMPLE_SCRIPTABLE_SOURCES}
    )

    if(WIN32)
        list(APPEND SAMPLE_SOURCES include/engine/asset/project/app.rc)
    endif()

    add_executable(axis_samples ${SAMPLE_SOURCES})

    target_include_directories(axis_samples PRIVATE
        sample/src
    )

    target_link_libraries(axis_samples PRIVATE
        axis_editor
        axis_engine
    )
    axis_optimize_build(axis_samples)

    if(WIN32)
        target_link_libraries(axis_samples PRIVATE
            dxgi
        )
    endif()

endif()

if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Scene compiler utility
add_executable(axis_compile EXCLUDE_FROM_ALL compiler/src/main.cpp)
if(MSVC)
    target_compile_options(axis_compile PRIVATE /FS)
endif()
target_link_libraries(axis_compile PRIVATE
    axis_engine
)
if(WIN32)
    target_link_libraries(axis_compile PRIVATE
        dxgi
    )
endif()

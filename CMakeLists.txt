cmake_minimum_required(VERSION 3.20)

project(AxisEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_EDITOR "Enable Editor System (ImGui)" OFF)
option(BUILD_SAMPLES "Build sample application when editor is enabled" ON)
option(ENABLE_TESTS "Build AxisEngine automated tests" OFF)

set(AXIS_GRAPHICS_BACKEND "OpenGL" CACHE STRING "Graphics backend to compile")
set_property(CACHE AXIS_GRAPHICS_BACKEND PROPERTY STRINGS OpenGL Vulkan DirectX)

set(AXIS_PHYSICS_BACKEND "Bullet" CACHE STRING "Physics backend to compile")
set_property(CACHE AXIS_PHYSICS_BACKEND PROPERTY STRINGS Bullet PhysX)

set(_AXIS_DEFAULT_AUDIO_BACKEND "Null")
set(AXIS_AUDIO_BACKEND "${_AXIS_DEFAULT_AUDIO_BACKEND}" CACHE STRING "Audio backend to compile")
set_property(CACHE AXIS_AUDIO_BACKEND PROPERTY STRINGS Null IrrKlang FMOD)

if(ENABLE_EDITOR)
    add_compile_definitions(ENABLE_EDITOR)
    message(STATUS "Editor System Enabled")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

foreach(OUTPUTCONFIG IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
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

string(TOUPPER "${AXIS_GRAPHICS_BACKEND}" AXIS_GRAPHICS_BACKEND_UPPER)
string(TOUPPER "${AXIS_PHYSICS_BACKEND}" AXIS_PHYSICS_BACKEND_UPPER)
string(TOUPPER "${AXIS_AUDIO_BACKEND}" AXIS_AUDIO_BACKEND_UPPER)

set(AXIS_HAS_OPENGL_BACKEND 0)
set(AXIS_HAS_VULKAN_BACKEND 0)
set(AXIS_HAS_DIRECTX_BACKEND 0)
set(AXIS_HAS_NULL_GRAPHICS_BACKEND 0)
if(AXIS_GRAPHICS_BACKEND_UPPER STREQUAL "OPENGL")
    find_package(Glad REQUIRED)
    set(AXIS_HAS_OPENGL_BACKEND 1)
elseif(AXIS_GRAPHICS_BACKEND_UPPER STREQUAL "VULKAN")
    find_package(Vulkan REQUIRED)
    set(AXIS_HAS_VULKAN_BACKEND 1)
elseif(AXIS_GRAPHICS_BACKEND_UPPER STREQUAL "DIRECTX")
    if(NOT WIN32)
        message(FATAL_ERROR "AXIS_GRAPHICS_BACKEND=DirectX requires Windows.")
    endif()
    set(AXIS_HAS_DIRECTX_BACKEND 1)
elseif(AXIS_GRAPHICS_BACKEND_UPPER STREQUAL "NULL")
    set(AXIS_HAS_NULL_GRAPHICS_BACKEND 1)
else()
    message(FATAL_ERROR "Unknown AXIS_GRAPHICS_BACKEND='${AXIS_GRAPHICS_BACKEND}'. Expected OpenGL, Vulkan, DirectX, or Null.")
endif()

if(ENABLE_EDITOR AND NOT AXIS_HAS_OPENGL_BACKEND)
    message(STATUS "ENABLE_EDITOR enabled with native RHI backend. ImGui will use the selected backend adapter.")
endif()

if(ENABLE_EDITOR)
    find_package(ImGui REQUIRED)
endif()

set(AXIS_HAS_BULLET_BACKEND 0)
set(AXIS_HAS_PHYSX_BACKEND 0)
if(AXIS_PHYSICS_BACKEND_UPPER STREQUAL "BULLET")
    find_package(Bullet REQUIRED)
    set(AXIS_HAS_BULLET_BACKEND 1)
elseif(AXIS_PHYSICS_BACKEND_UPPER STREQUAL "PHYSX")
    find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)
    set(AXIS_HAS_PHYSX_BACKEND 1)
else()
    message(FATAL_ERROR "Unknown AXIS_PHYSICS_BACKEND='${AXIS_PHYSICS_BACKEND}'. Expected Bullet or PhysX.")
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

set(AXIS_RHI_SHADER_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/include/engine/asset/shaders/rhi")

#Engine Core Library
add_library(axis_engine STATIC ${AXIS_ENGINE_SOURCES})
add_library(Axis::Engine ALIAS axis_engine)
if(TARGET axis_rhi_shaders)
    add_dependencies(axis_engine axis_rhi_shaders)
endif()

target_compile_features(axis_engine PUBLIC cxx_std_20)

target_compile_definitions(axis_engine PUBLIC
    AXIS_HAS_OPENGL_BACKEND=${AXIS_HAS_OPENGL_BACKEND}
    AXIS_HAS_VULKAN_BACKEND=${AXIS_HAS_VULKAN_BACKEND}
    AXIS_HAS_DIRECTX_BACKEND=${AXIS_HAS_DIRECTX_BACKEND}
    AXIS_HAS_NULL_GRAPHICS_BACKEND=${AXIS_HAS_NULL_GRAPHICS_BACKEND}
    AXIS_HAS_BULLET_BACKEND=${AXIS_HAS_BULLET_BACKEND}
    AXIS_HAS_PHYSX_BACKEND=${AXIS_HAS_PHYSX_BACKEND}
    AXIS_HAS_IRRKLANG_BACKEND=${AXIS_HAS_IRRKLANG_BACKEND}
    AXIS_HAS_FMOD_BACKEND=${AXIS_HAS_FMOD_BACKEND}
    AXIS_HAS_OPENAL_BACKEND=0
    AXIS_HAS_NULL_AUDIO_BACKEND=${AXIS_HAS_NULL_AUDIO_BACKEND}
    AXIS_RHI_SHADER_DIR="${AXIS_RHI_SHADER_OUTPUT_DIR}"
)

if(MSVC)
    target_compile_options(axis_engine PRIVATE /FS)
endif()

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

if(ENABLE_EDITOR)
    target_link_libraries(axis_engine PUBLIC ImGui::ImGui)
endif()

if(AXIS_HAS_BULLET_BACKEND)
    target_link_libraries(axis_engine PUBLIC Bullet::Bullet)
endif()

if(AXIS_HAS_PHYSX_BACKEND)
    target_link_libraries(axis_engine PUBLIC unofficial::omniverse-physx-sdk::sdk)
    if(TARGET unofficial::omniverse-physx-sdk::gpu-library)
        add_custom_command(TARGET axis_engine POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-library>
                        $<TARGET_FILE_DIR:axis_engine>)
        if(WIN32)
            add_custom_command(TARGET axis_engine POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-device-library>
                            $<TARGET_FILE_DIR:axis_engine>)
        endif()
    endif()
endif()

if(AXIS_HAS_OPENGL_BACKEND)
    target_link_libraries(axis_engine PUBLIC Glad::Glad)
endif()

if(AXIS_HAS_VULKAN_BACKEND)
    target_link_libraries(axis_engine PUBLIC Vulkan::Vulkan)
endif()

if(AXIS_HAS_DIRECTX_BACKEND)
    target_link_libraries(axis_engine PUBLIC
        d3d12
        dxgi
        dxguid
    )
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
                    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_axis_irrklang_runtime}"
                    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
                COMMENT "Copying irrKlang runtime library to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
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
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FMOD_RUNTIME_LIBRARY_RELEASE}"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
            COMMENT "Copying FMOD release runtime library to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        )
    endif()

    if(FMOD_RUNTIME_LIBRARY_DEBUG)
        add_custom_command(TARGET axis_engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FMOD_RUNTIME_LIBRARY_DEBUG}"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
            COMMENT "Copying FMOD debug runtime library to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        )
    endif()
endif()

if(WIN32)
    target_link_libraries(axis_engine PUBLIC
        dbghelp
        winmm
        ws2_32
    )
    if(AXIS_HAS_OPENGL_BACKEND)
        target_link_libraries(axis_engine PUBLIC opengl32)
    endif()
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    target_link_libraries(axis_engine PUBLIC
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )
    if(AXIS_HAS_OPENGL_BACKEND)
        target_link_libraries(axis_engine PUBLIC OpenGL::GL)
    endif()
else()
    target_link_libraries(axis_engine PUBLIC
        pthread
        dl
    )
    if(AXIS_HAS_OPENGL_BACKEND)
        target_link_libraries(axis_engine PUBLIC OpenGL::GL)
    endif()
endif()

#Editor Library
if(ENABLE_EDITOR)
    add_library(axis_editor STATIC ${AXIS_EDITOR_SOURCES})
    add_library(Axis::Editor ALIAS axis_editor)

    if(MSVC)
        target_compile_options(axis_editor PRIVATE /FS)
    endif()
    
    target_include_directories(axis_editor PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    
    target_link_libraries(axis_editor PUBLIC
        axis_engine
        ImGui::ImGui
    )
endif()

#Installation& Export Setup
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
set(AXIS_CONFIG_IMGUI_DEPENDENCY "")
if(ENABLE_EDITOR)
    set(AXIS_CONFIG_IMGUI_DEPENDENCY "find_dependency(ImGui)")
endif()

set(AXIS_CONFIG_BULLET_DEPENDENCY "")
if(AXIS_HAS_BULLET_BACKEND)
    set(AXIS_CONFIG_BULLET_DEPENDENCY "find_dependency(Bullet)")
endif()

set(AXIS_CONFIG_IRRKLANG_DEPENDENCY "")
if(AXIS_HAS_IRRKLANG_BACKEND)
    set(AXIS_CONFIG_IRRKLANG_DEPENDENCY "find_dependency(IrrKlang)")
endif()

set(AXIS_CONFIG_FMOD_DEPENDENCY "")
if(AXIS_HAS_FMOD_BACKEND)
    set(AXIS_CONFIG_FMOD_DEPENDENCY "find_dependency(FMOD)")
endif()

set(AXIS_CONFIG_GLAD_DEPENDENCY "")
if(AXIS_HAS_OPENGL_BACKEND)
    set(AXIS_CONFIG_GLAD_DEPENDENCY "find_dependency(Glad)")
endif()

set(AXIS_CONFIG_VULKAN_DEPENDENCY "")
if(AXIS_HAS_VULKAN_BACKEND)
    set(AXIS_CONFIG_VULKAN_DEPENDENCY "find_dependency(Vulkan)")
endif()

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

    set(SAMPLE_SOURCES
        sample/src/main.cpp
        sample/src/sample_state.cpp
        ${SAMPLE_SCENARIO_SOURCES}
    )

    if(WIN32)
        list(APPEND SAMPLE_SOURCES include/engine/asset/project/app.rc)
    endif()

    add_executable(axis_samples ${SAMPLE_SOURCES})
    if(MSVC)
        target_compile_options(axis_samples PRIVATE /FS)
    endif()

    target_include_directories(axis_samples PRIVATE
        sample/src
    )

    target_link_libraries(axis_samples PRIVATE
        axis_editor
        axis_engine
    )

    if(WIN32)
        target_link_libraries(axis_samples PRIVATE
            dxgi
            pdh
        )
    endif()

    if(MSVC)
        target_link_options(axis_samples PRIVATE "/WHOLEARCHIVE:axis_engine")
        target_link_options(axis_samples PRIVATE "/WHOLEARCHIVE:axis_editor")
    endif()

endif()

if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

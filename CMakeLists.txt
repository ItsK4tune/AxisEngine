cmake_minimum_required(VERSION 3.20)

project(AxisEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_EDITOR "Enable Editor System (ImGui)" OFF)
option(BUILD_SAMPLES "Build sample application when editor is enabled" ON)
option(ENABLE_TESTS "Build AxisEngine automated tests" OFF)

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
find_package(Bullet REQUIRED)
find_package(Freetype REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(Enet REQUIRED)

set(AXIS_HAS_IRRKLANG_BACKEND 0)
if(WIN32)
    find_package(IrrKlang REQUIRED)
    set(AXIS_HAS_IRRKLANG_BACKEND 1)
else()
    find_package(OpenGL REQUIRED)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/sources.cmake)

if(WIN32)
    list(APPEND AXIS_ENGINE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/audio/strategy/irrklang/irrklang_audio_engine.cpp
        ${CMAKE_SOURCE_DIR}/src/platform/strategy/windows/windows_filesystem.cpp
        ${CMAKE_SOURCE_DIR}/src/platform/strategy/windows/windows_runtime.cpp
    )
elseif(APPLE)
    list(APPEND AXIS_ENGINE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/platform/strategy/macos/macos_filesystem.cpp
        ${CMAKE_SOURCE_DIR}/src/platform/strategy/macos/macos_runtime.cpp
    )
else()
    list(APPEND AXIS_ENGINE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/platform/strategy/posix/posix_filesystem.cpp
        ${CMAKE_SOURCE_DIR}/src/platform/strategy/posix/posix_runtime.cpp
    )
endif()

# Engine Core Library
add_library(axis_engine STATIC ${AXIS_ENGINE_SOURCES})
add_library(Axis::Engine ALIAS axis_engine)

target_compile_features(axis_engine PUBLIC cxx_std_20)

target_compile_definitions(axis_engine PUBLIC
    AXIS_HAS_OPENGL_BACKEND=1
    AXIS_HAS_VULKAN_BACKEND=0
    AXIS_HAS_DIRECTX_BACKEND=0
    AXIS_HAS_BULLET_BACKEND=1
    AXIS_HAS_PHYSX_BACKEND=0
    AXIS_HAS_IRRKLANG_BACKEND=${AXIS_HAS_IRRKLANG_BACKEND}
    AXIS_HAS_FMOD_BACKEND=0
    AXIS_HAS_OPENAL_BACKEND=0
    AXIS_HAS_NULL_AUDIO_BACKEND=1
)

if(MSVC)
    target_compile_options(axis_engine PRIVATE /FS)
endif()

target_include_directories(axis_engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/bullet>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/entt>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/engine>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/imgui>
    $<BUILD_INTERFACE:${FREETYPE_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ASSIMP_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${GLFW3_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${GLM_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${BULLET_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${FFMPEG_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${ENET_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/engine>
    $<INSTALL_INTERFACE:include/imgui>
)

target_link_libraries(axis_engine PUBLIC
    GLFW3::GLFW3
    ASSIMP::ASSIMP
    Bullet::Bullet
    Freetype::Freetype
    FFMPEG::FFMPEG
    Enet::Enet
)

if(AXIS_HAS_IRRKLANG_BACKEND)
    target_include_directories(axis_engine PUBLIC
        $<BUILD_INTERFACE:${IRRKLANG_INCLUDE_DIR}>
    )
    target_link_libraries(axis_engine PUBLIC IrrKlang::IrrKlang)
endif()

if(WIN32)
    target_link_libraries(axis_engine PUBLIC
        opengl32
        dbghelp
        winmm
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

    if(MSVC)
        target_compile_options(axis_editor PRIVATE /FS)
    endif()
    
    target_include_directories(axis_editor PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    
    target_link_libraries(axis_editor PUBLIC axis_engine)
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
)

if(WIN32)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/lib/
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILES_MATCHING PATTERN "*.lib"
    )

    install(DIRECTORY ${CMAKE_SOURCE_DIR}/dlls/
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        FILES_MATCHING PATTERN "*.dll"
    )
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

    if(WIN32)
        # Copy DLLs post-build
        add_custom_command(TARGET axis_samples POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/dlls"
                "$<TARGET_FILE_DIR:axis_samples>"
        )
    endif()
endif()

if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

cmake_minimum_required(VERSION 3.20)

project(AxisEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT WIN32)
    message(FATAL_ERROR "AxisEngine currently only supports Windows.")
endif()

option(ENABLE_EDITOR "Enable Editor System (ImGui)" OFF)

if(ENABLE_EDITOR)
    add_compile_definitions(ENABLE_EDITOR)
    message(STATUS "Editor System Enabled")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

foreach(OUTPUTCONFIG IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib)
endforeach()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

find_package(ASSIMP REQUIRED)
find_package(GLFW3 REQUIRED)
find_package(GLM REQUIRED)
find_package(Bullet REQUIRED)
find_package(Freetype REQUIRED)
find_package(IrrKlang REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(Enet REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/bullet
    ${CMAKE_SOURCE_DIR}/include/entt
    ${CMAKE_SOURCE_DIR}/include/engine
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${FREETYPE_INCLUDE_DIR}
    ${ASSIMP_INCLUDE_DIR}
    ${GLFW3_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${BULLET_INCLUDE_DIR}
    ${IRRKLANG_INCLUDE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
    ${ENET_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/game/include
)

link_directories(${CMAKE_SOURCE_DIR}/lib)

include(${CMAKE_SOURCE_DIR}/cmake/sources.cmake)

# Engine Core Library
add_library(axis_engine STATIC ${AXIS_ENGINE_SOURCES})
add_library(Axis::Engine ALIAS axis_engine)

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
    $<BUILD_INTERFACE:${IRRKLANG_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${FFMPEG_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${ENET_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

set(BULLET_LIBS
    BulletDynamics$<$<CONFIG:Debug>:_Debug>
    BulletCollision$<$<CONFIG:Debug>:_Debug>
    LinearMath$<$<CONFIG:Debug>:_Debug>
)

target_link_libraries(axis_engine PUBLIC
    glfw3 
    opengl32 
    assimp-vc143-mt
    ${BULLET_LIBS}
    freetype
    irrKlang
    ${FFMPEG_LIBRARIES}
    ${ENET_LIBRARIES}
    dbghelp
    winmm
    ws2_32
)

# Editor Library
if(ENABLE_EDITOR)
    add_library(axis_editor STATIC ${AXIS_EDITOR_SOURCES})
    add_library(Axis::Editor ALIAS axis_editor)
    
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

install(EXPORT AxisEngineTargets
    FILE AxisEngineTargets.cmake
    NAMESPACE Axis::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AxisEngine
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
if(ENABLE_EDITOR)
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
    target_compile_options(axis_samples PRIVATE /FS)

    target_include_directories(axis_samples PRIVATE
        sample/src
    )

    target_link_libraries(axis_samples PRIVATE
        axis_editor
        axis_engine
        dxgi
        pdh
    )

    if(MSVC)
        target_link_options(axis_samples PRIVATE "/WHOLEARCHIVE:axis_engine")
        target_link_options(axis_samples PRIVATE "/WHOLEARCHIVE:axis_editor")
    endif()

    # Copy DLLs post-build
    add_custom_command(TARGET axis_samples POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/dlls"
            "$<TARGET_FILE_DIR:axis_samples>"
    )
endif()

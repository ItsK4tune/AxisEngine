cmake_minimum_required(VERSION 3.20)

project(AxisEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT WIN32)
    message(FATAL_ERROR "AxisEngine currently only supports Windows.")
endif()

option(ENABLE_DEBUG_SYSTEM "Enable Runtime Debug System (F1/F2/F3)" ON)

if(ENABLE_DEBUG_SYSTEM)
    add_compile_definitions(ENABLE_DEBUG_SYSTEM)
    message(STATUS "Debug System Enabled")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

find_package(ASSIMP REQUIRED)
find_package(GLFW3 REQUIRED)
find_package(GLM REQUIRED)
find_package(Bullet REQUIRED)
find_package(Freetype REQUIRED)
find_package(IrrKlang REQUIRED)
find_package(FFMPEG REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/bullet
    ${CMAKE_SOURCE_DIR}/include/entt
    ${CMAKE_SOURCE_DIR}/include/engine
    ${FREETYPE_INCLUDE_DIR}
    ${ASSIMP_INCLUDE_DIR}
    ${GLFW3_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${BULLET_INCLUDE_DIR}
    ${IRRKLANG_INCLUDE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/game/include
)

link_directories(${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE ENGINE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.c
)

file(GLOB_RECURSE GAME_SOURCES
    ${CMAKE_SOURCE_DIR}/game/src/*.cpp
    # Windows resource files containing the app icon
    ${CMAKE_SOURCE_DIR}/include/engine/asset/project/app.rc
    ${CMAKE_SOURCE_DIR}/include/engine/asset/project/resource.h
)

add_executable(AxisEngine ${ENGINE_SOURCES} ${GAME_SOURCES})

set(BULLET_LIBS
    BulletDynamics$<$<CONFIG:Debug>:_Debug>
    BulletCollision$<$<CONFIG:Debug>:_Debug>
    LinearMath$<$<CONFIG:Debug>:_Debug>
)

target_link_libraries(AxisEngine 
    glfw3 
    opengl32 
    assimp-vc143-mt
    ${BULLET_LIBS}
    freetype
    irrKlang
    ${FFMPEG_LIBRARIES}
)

file(GLOB DLLS "${CMAKE_SOURCE_DIR}/dlls/*.dll")
foreach(DLL ${DLLS})
    add_custom_command(TARGET AxisEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DLL}
        $<TARGET_FILE_DIR:AxisEngine>
    )
endforeach()
cmake_minimum_required(VERSION 3.20)

project(AxisEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_DEBUG_SYSTEM "Enable Runtime Debug System (F1/F2/F3)" OFF)

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
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/includes/bullet
    ${CMAKE_SOURCE_DIR}/includes/entt
    ${CMAKE_SOURCE_DIR}/includes/engine
    ${FFMPEG_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/game/includes
)

link_directories(${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE ENGINE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.c
)

file(GLOB_RECURSE GAME_SOURCES
    ${CMAKE_SOURCE_DIR}/game/src/*.cpp
)

set(PROJECT_RESOURCES
    ${CMAKE_SOURCE_DIR}/src/asset/project/app.rc
    ${CMAKE_SOURCE_DIR}/src/asset/project/resource.h
)

add_executable(AxisEngine ${ENGINE_SOURCES} ${GAME_SOURCES} ${PROJECT_RESOURCES})

set(BULLET_LIBS
    BulletDynamics$<$<CONFIG:Debug>:_Debug>
    BulletCollision$<$<CONFIG:Debug>:_Debug>
    LinearMath$<$<CONFIG:Debug>:_Debug>
)

if (WIN32)
    target_link_libraries(AxisEngine 
        glfw3 
        opengl32 
        assimp-vc143-mt
        ${BULLET_LIBS}
        freetype
        irrKlang
        ${FFMPEG_LIBRARIES}
    )
endif()

if (UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(AxisEngine 
        assimp-vc143-mt
        glfw 
        GL 
        dl 
        pthread 
        X11 
        Xrandr 
        Xi 
        Xxf86vm 
        Xinerama 
        Xcursor
        ${BULLET_LIBS}
        freetype
        irrKlang
    )
endif()

if (WIN32)
    file(GLOB DLLS "${CMAKE_SOURCE_DIR}/dlls/*.dll")
    foreach(DLL ${DLLS})
        add_custom_command(TARGET AxisEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DLL}
            $<TARGET_FILE_DIR:AxisEngine>
        )
    endforeach()
endif()
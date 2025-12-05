cmake_minimum_required(VERSION 3.20)

project(GameEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

find_package(ASSIMP REQUIRED)
find_package(GLFW3 REQUIRED)
find_package(GLM REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/includes
)

link_directories(${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.c
)

configure_file(configuration/root_directory.h.in configuration/root_directory.h)
include_directories(${CMAKE_BINARY_DIR}/configuration)

add_executable(GameEngine ${SOURCES})

if (WIN32)
    target_link_libraries(GameEngine glfw3 opengl32 assimp)
endif()
if (UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(GameEngine glfw GL dl pthread X11 Xrandr Xi Xxf86vm Xinerama Xcursor)
endif()
if (WIN32)
    file(GLOB DLLS "${CMAKE_SOURCE_DIR}/dlls/*.dll")

foreach(DLL ${DLLS})
    add_custom_command(TARGET GameEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DLL}
        $<TARGET_FILE_DIR:GameEngine>
    )
endforeach()
endif()
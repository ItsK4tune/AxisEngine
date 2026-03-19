cmake_minimum_required(VERSION 3.20)
project(AxisEngineTests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the engine headers
include_directories(
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/../include/engine
    ${CMAKE_SOURCE_DIR}/../include/entt
    ${CMAKE_SOURCE_DIR}/../include/glm
)

# Test sources
file(GLOB_RECURSE TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/*.cpp
)

add_executable(AxisEngineTests ${TEST_SOURCES})

# Set output directory
set_target_properties(AxisEngineTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../bin
)

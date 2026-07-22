add_executable(axis_test
    test_main.cpp
    api/test_public_headers.cpp
    unit/test_collision_matrix.cpp
    unit/test_entity_manager.cpp
      unit/test_event_manager.cpp
      unit/test_dds_texture_loader.cpp
    unit/test_pathfinding.cpp
    unit/test_runtime_contracts.cpp
    unit/test_scene_validator.cpp
    unit/test_static_batch_manager.cpp
    unit/test_texture_ownership.cpp
    unit/test_video_decoder.cpp
    unit/test_yaml_parser.cpp
    unit/test_yaml_writer.cpp
    integration/test_binary_scene_serializer.cpp
    integration/test_navigation_system.cpp
    integration/test_physics_system.cpp
    integration/test_resource_manager.cpp
    integration/test_sample_scenarios.cpp
    integration/test_scene_manager.cpp
    integration/test_scene_serializer.cpp
    integration/test_scriptable_system.cpp
    integration/test_transform_system.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario01.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario03.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario07.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario11.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario21.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario23.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario25.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario26.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario27.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario28.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario29.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario30.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario31.cpp
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios/scenario33.cpp
)

target_include_directories(axis_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/sample/src
    ${CMAKE_SOURCE_DIR}/sample/src/scenarios
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/engine
)

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/sample/resource/video/sample.mp4" AXIS_TEST_VIDEO_PATH_VALUE)
target_compile_definitions(axis_test PRIVATE
    AXIS_TESTING
    AXIS_TEST_VIDEO_PATH="${AXIS_TEST_VIDEO_PATH_VALUE}"
)

target_link_libraries(axis_test PRIVATE axis_engine)
axis_optimize_build(axis_test)

# The lightweight test registration macros derive symbols from __LINE__, which
# can collide when multiple test files are merged into one unity translation
# unit. Tests still benefit from PCH and MSVC /MP parallel compilation.
set_target_properties(axis_test PROPERTIES UNITY_BUILD OFF)

if(TARGET axis_editor)
    target_link_libraries(axis_test PRIVATE axis_editor)
endif()

add_test(NAME axis_test COMMAND axis_test)

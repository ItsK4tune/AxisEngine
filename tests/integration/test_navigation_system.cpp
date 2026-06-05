#include "test_framework.h"
#include "test_support.h"

#include <ecs/logic/entity_manager.h>
#include <ecs/unit/core_components.h>
#include <navigation/logic/navigation_system.h>
#include <navigation/unit/navmesh_component.h>
#include <navigation/unit/pathfollower_component.h>

namespace
{
entt::entity CreateFollower(Scene& scene)
{
    auto entity = EntityManager::CreateEntity(scene, "Agent", "agent");
    scene.registry.emplace<PathFollowerComponent>(entity);
    return entity;
}
}  // namespace

AXIS_TEST_CASE("NavigationSystem MoveTo sets pending target")
{
    Scene scene;
    NavigationSystem navigation;
    auto entity = CreateFollower(scene);

    navigation.MoveTo(scene, entity, {5.0f, 0.0f, 0.0f});

    const auto& follower = scene.registry.get<PathFollowerComponent>(entity);
    AXIS_CHECK(follower.pathPending);
    AXIS_CHECK(!follower.isMoving);
    AXIS_CHECK_NEAR(follower.targetPosition.x, 5.0f, 0.0001f);
}

AXIS_TEST_CASE("NavigationSystem no navmesh fails pending path cleanly")
{
    axis_test_support::HeadlessResourceFixture fixture;
    Scene scene;
    NavigationSystem navigation;
    auto entity = CreateFollower(scene);
    auto& follower = scene.registry.get<PathFollowerComponent>(entity);
    follower.pathfindingOptions.criteria = PathfindingCriteria::Shortest;

    navigation.MoveTo(scene, entity, {5.0f, 0.0f, 0.0f});
    navigation.Update(scene, 0.1f);

    AXIS_CHECK(!follower.pathPending);
    AXIS_CHECK(!follower.isMoving);
    AXIS_CHECK(follower.currentPath.empty());
}

AXIS_TEST_CASE("NavigationSystem straight line creates path and starts moving")
{
    axis_test_support::HeadlessResourceFixture fixture;
    Scene scene;
    NavigationSystem navigation;
    auto entity = CreateFollower(scene);
    auto& follower = scene.registry.get<PathFollowerComponent>(entity);
    follower.pathfindingOptions.criteria = PathfindingCriteria::StraightLine;
    follower.moveSpeed = 5.0f;

    navigation.MoveTo(scene, entity, {5.0f, 0.0f, 0.0f});
    navigation.Update(scene, 0.1f);
    navigation.Update(scene, 0.1f);

    const auto& pos = scene.registry.get<PositionComponent>(entity);
    AXIS_CHECK(!follower.pathPending);
    AXIS_CHECK(follower.isMoving);
    AXIS_CHECK(follower.currentPath.size() == 2);
    AXIS_CHECK(follower.currentPathIndex == 1);
    AXIS_CHECK(pos.value.x > 0.0f);
}

AXIS_TEST_CASE("NavigationSystem StopMoving clears state")
{
    Scene scene;
    NavigationSystem navigation;
    auto entity = CreateFollower(scene);
    auto& follower = scene.registry.get<PathFollowerComponent>(entity);
    follower.isMoving = true;
    follower.pathPending = true;
    follower.currentPath = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    follower.currentPathIndex = 1;
    follower.debugPlannedPath = follower.currentPath;
    follower.debugTraveledPath = {{0.0f, 0.0f, 0.0f}};

    navigation.StopMoving(scene, entity);

    AXIS_CHECK(!follower.isMoving);
    AXIS_CHECK(!follower.pathPending);
    AXIS_CHECK(follower.currentPath.empty());
    AXIS_CHECK(follower.debugPlannedPath.empty());
    AXIS_CHECK(follower.debugTraveledPath.empty());
    AXIS_CHECK(follower.currentPathIndex == 0);
}

AXIS_TEST_CASE("NavigationSystem GetRemainingDistance sums remaining waypoints")
{
    Scene scene;
    NavigationSystem navigation;
    auto entity = CreateFollower(scene);
    auto& follower = scene.registry.get<PathFollowerComponent>(entity);
    follower.isMoving = true;
    follower.currentPath = {{3.0f, 0.0f, 0.0f}, {3.0f, 4.0f, 0.0f}};
    follower.currentPathIndex = 0;

    const float distance = navigation.GetRemainingDistance(scene, entity);

    AXIS_CHECK_NEAR(distance, 7.0f, 0.0001f);
}

AXIS_TEST_CASE("NavigationSystem ClearWalkableTags resets lowercase walkable")
{
    NavigationSystem navigation;
    navigation.AddWalkableTag("road");

    navigation.ClearWalkableTags();

    const auto& tags = navigation.GetWalkableTags();
    AXIS_CHECK(tags.size() == 1);
    AXIS_CHECK(tags[0] == "walkable");
}

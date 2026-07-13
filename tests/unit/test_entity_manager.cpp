#include "test_framework.h"
#include <ecs/logic/entity_builder.h>
#include <ecs/unit/core_components.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>

AXIS_TEST_CASE("EntityManager creates entity with default core components")
{
    Scene scene;

    auto entity = scene.CreateEntity("Player", "player");

    AXIS_CHECK(scene.IsValid(entity));
    AXIS_CHECK(scene.HasAllComponents<InfoComponent>(entity));
    AXIS_CHECK(scene.HasAllComponents<PositionComponent>(entity));
    AXIS_CHECK(scene.HasAllComponents<RotationComponent>(entity));
    AXIS_CHECK(scene.HasAllComponents<ScaleComponent>(entity));
    AXIS_CHECK(scene.HasAllComponents<HierarchyComponent>(entity));
    AXIS_CHECK(scene.HasAllComponents<WorldTransformComponent>(entity));
    AXIS_CHECK(scene.GetComponent<InfoComponent>(entity).name == "Player");
    AXIS_CHECK(scene.GetComponent<InfoComponent>(entity).tag == "player");
}

AXIS_TEST_CASE("EntityBuilder publishes world transforms before the next transform-system update")
{
    Scene scene;
    ResourceManager resources;

    auto parent = EntityBuilder(scene, resources, "test")
                      .WithTransform({10.0f, 2.0f, -3.0f})
                      .Build();
    auto child = EntityBuilder(scene, resources, "test")
                     .WithParent(parent)
                     .WithTransform({1.0f, 4.0f, 2.0f})
                     .Build();

    const auto& parentWorld = scene.GetComponent<WorldTransformComponent>(parent);
    const auto& childWorld = scene.GetComponent<WorldTransformComponent>(child);

    AXIS_CHECK_NEAR(parentWorld.worldMatrix[3].x, 10.0f, 0.0001f);
    AXIS_CHECK_NEAR(parentWorld.prevWorldMatrix[3].x, 10.0f, 0.0001f);
    AXIS_CHECK_NEAR(childWorld.worldMatrix[3].x, 11.0f, 0.0001f);
    AXIS_CHECK_NEAR(childWorld.worldMatrix[3].y, 6.0f, 0.0001f);
    AXIS_CHECK_NEAR(childWorld.worldMatrix[3].z, -1.0f, 0.0001f);
    AXIS_CHECK_NEAR(childWorld.prevWorldMatrix[3].x, 11.0f, 0.0001f);
    AXIS_CHECK(!parentWorld.isDirty);
    AXIS_CHECK(!childWorld.isDirty);
    AXIS_CHECK(parentWorld.version > 0);
    AXIS_CHECK(childWorld.version > 0);
}

AXIS_TEST_CASE("EntityManager finds entities by name tag and scene")
{
    Scene scene;

    auto player = scene.CreateEntity("Player", "player");
    auto enemy = scene.CreateEntity("Enemy", "enemy");
    scene.GetComponent<InfoComponent>(player).sceneName = "arena";
    scene.GetComponent<InfoComponent>(enemy).sceneName = "arena";

    AXIS_CHECK(scene.FindByName("Player") == player);
    AXIS_CHECK(scene.FindByTag("enemy") == enemy);
    AXIS_CHECK(scene.FindByNameAndTag("Player", "player") == player);
    AXIS_CHECK(scene.FindByNameTagAndScene("Enemy", "enemy", "arena") == enemy);
    AXIS_CHECK(scene.FindAllBySceneName("arena").size() == 2);
}

AXIS_TEST_CASE("EntityManager reparents child and removes it from old parent")
{
    Scene scene;
    auto parentA = scene.CreateEntity("ParentA");
    auto parentB = scene.CreateEntity("ParentB");
    auto child = scene.CreateEntity("Child");

    scene.SetParent(child, parentA);
    scene.SetParent(child, parentB);

    const auto& childHierarchy = scene.GetComponent<HierarchyComponent>(child);
    const auto& parentAHierarchy = scene.GetComponent<HierarchyComponent>(parentA);
    const auto& parentBHierarchy = scene.GetComponent<HierarchyComponent>(parentB);

    AXIS_CHECK(childHierarchy.parent == parentB);
    AXIS_CHECK(parentAHierarchy.children.empty());
    AXIS_CHECK(parentBHierarchy.children.size() == 1);
    AXIS_CHECK(parentBHierarchy.children[0] == child);
}

AXIS_TEST_CASE("EntityManager keeps hierarchy stable for repeated parent assignment")
{
    Scene scene;
    auto root = scene.CreateEntity("Root");
    auto child = scene.CreateEntity("Child");

    scene.SetParent(child, root);
    scene.SetParent(child, root);

    AXIS_CHECK(scene.GetComponent<HierarchyComponent>(root).parent == entt::null);
    AXIS_CHECK(scene.GetComponent<HierarchyComponent>(child).parent == root);
    AXIS_CHECK(scene.GetComponent<HierarchyComponent>(root).children.size() == 1);
}

AXIS_TEST_CASE("EntityManager rejects self-parent without hierarchy mutation")
{
    Scene scene;
    auto entity = scene.CreateEntity("Root");

    scene.SetParent(entity, entity);

    const auto& hierarchy = scene.GetComponent<HierarchyComponent>(entity);
    AXIS_CHECK(hierarchy.parent == entt::null);
    AXIS_CHECK(hierarchy.children.empty());
}

AXIS_TEST_CASE("EntityManager rejects hierarchy cycle without hierarchy mutation")
{
    Scene scene;
    auto root = scene.CreateEntity("Root");
    auto child = scene.CreateEntity("Child");

    scene.SetParent(child, root);
    scene.SetParent(root, child);

    const auto& rootHierarchy = scene.GetComponent<HierarchyComponent>(root);
    const auto& childHierarchy = scene.GetComponent<HierarchyComponent>(child);
    AXIS_CHECK(rootHierarchy.parent == entt::null);
    AXIS_CHECK(rootHierarchy.children.size() == 1);
    AXIS_CHECK(rootHierarchy.children[0] == child);
    AXIS_CHECK(childHierarchy.parent == root);
    AXIS_CHECK(childHierarchy.children.empty());
}

AXIS_TEST_CASE("EntityManager destroy detaches surviving children")
{
    Scene scene;
    auto parent = scene.CreateEntity("Parent");
    auto child = scene.CreateEntity("Child");
    scene.SetParent(child, parent);

    scene.DestroyEntity(parent);

    AXIS_CHECK(!scene.IsValid(parent));
    AXIS_CHECK(scene.IsValid(child));
    AXIS_CHECK(scene.GetComponent<HierarchyComponent>(child).parent == entt::null);
}

AXIS_TEST_CASE("EntityManager destroy with children recursively destroys hierarchy")
{
    Scene scene;
    auto parent = scene.CreateEntity("Parent");
    auto child = scene.CreateEntity("Child");
    auto grandChild = scene.CreateEntity("GrandChild");
    scene.SetParent(child, parent);
    scene.SetParent(grandChild, child);

    scene.DestroyEntityWithChildren(parent);

    AXIS_CHECK(!scene.IsValid(parent));
    AXIS_CHECK(!scene.IsValid(child));
    AXIS_CHECK(!scene.IsValid(grandChild));
}

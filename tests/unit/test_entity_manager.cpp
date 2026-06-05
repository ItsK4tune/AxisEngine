#include "test_framework.h"

#include <ecs/logic/entity_manager.h>
#include <ecs/unit/core_components.h>

AXIS_TEST_CASE("EntityManager creates entity with default core components")
{
    Scene scene;

    auto entity = EntityManager::CreateEntity(scene, "Player", "player");

    AXIS_CHECK(scene.registry.valid(entity));
    AXIS_CHECK(scene.registry.all_of<InfoComponent>(entity));
    AXIS_CHECK(scene.registry.all_of<PositionComponent>(entity));
    AXIS_CHECK(scene.registry.all_of<RotationComponent>(entity));
    AXIS_CHECK(scene.registry.all_of<ScaleComponent>(entity));
    AXIS_CHECK(scene.registry.all_of<HierarchyComponent>(entity));
    AXIS_CHECK(scene.registry.all_of<WorldTransformComponent>(entity));
    AXIS_CHECK(scene.registry.get<InfoComponent>(entity).name == "Player");
    AXIS_CHECK(scene.registry.get<InfoComponent>(entity).tag == "player");
}

AXIS_TEST_CASE("EntityManager finds entities by name tag and scene")
{
    Scene scene;

    auto player = EntityManager::CreateEntity(scene, "Player", "player");
    auto enemy = EntityManager::CreateEntity(scene, "Enemy", "enemy");
    scene.registry.get<InfoComponent>(player).sceneName = "arena";
    scene.registry.get<InfoComponent>(enemy).sceneName = "arena";

    AXIS_CHECK(EntityManager::FindByName(scene, "Player") == player);
    AXIS_CHECK(EntityManager::FindByTag(scene, "enemy") == enemy);
    AXIS_CHECK(EntityManager::FindByNameAndTag(scene, "Player", "player") == player);
    AXIS_CHECK(EntityManager::FindByNameTagAndScene(scene, "Enemy", "enemy", "arena") == enemy);
    AXIS_CHECK(EntityManager::FindAllBySceneName(scene, "arena").size() == 2);
}

AXIS_TEST_CASE("EntityManager reparents child and removes it from old parent")
{
    Scene scene;
    auto parentA = EntityManager::CreateEntity(scene, "ParentA");
    auto parentB = EntityManager::CreateEntity(scene, "ParentB");
    auto child = EntityManager::CreateEntity(scene, "Child");

    EntityManager::SetParent(scene, child, parentA);
    EntityManager::SetParent(scene, child, parentB);

    const auto& childHierarchy = scene.registry.get<HierarchyComponent>(child);
    const auto& parentAHierarchy = scene.registry.get<HierarchyComponent>(parentA);
    const auto& parentBHierarchy = scene.registry.get<HierarchyComponent>(parentB);

    AXIS_CHECK(childHierarchy.parent == parentB);
    AXIS_CHECK(parentAHierarchy.children.empty());
    AXIS_CHECK(parentBHierarchy.children.size() == 1);
    AXIS_CHECK(parentBHierarchy.children[0] == child);
}

AXIS_TEST_CASE("EntityManager keeps hierarchy stable for repeated parent assignment")
{
    Scene scene;
    auto root = EntityManager::CreateEntity(scene, "Root");
    auto child = EntityManager::CreateEntity(scene, "Child");

    EntityManager::SetParent(scene, child, root);
    EntityManager::SetParent(scene, child, root);

    AXIS_CHECK(scene.registry.get<HierarchyComponent>(root).parent == entt::null);
    AXIS_CHECK(scene.registry.get<HierarchyComponent>(child).parent == root);
    AXIS_CHECK(scene.registry.get<HierarchyComponent>(root).children.size() == 1);
}

AXIS_TEST_CASE("EntityManager rejects self-parent without hierarchy mutation")
{
    Scene scene;
    auto entity = EntityManager::CreateEntity(scene, "Root");

    EntityManager::SetParent(scene, entity, entity);

    const auto& hierarchy = scene.registry.get<HierarchyComponent>(entity);
    AXIS_CHECK(hierarchy.parent == entt::null);
    AXIS_CHECK(hierarchy.children.empty());
}

AXIS_TEST_CASE("EntityManager rejects hierarchy cycle without hierarchy mutation")
{
    Scene scene;
    auto root = EntityManager::CreateEntity(scene, "Root");
    auto child = EntityManager::CreateEntity(scene, "Child");

    EntityManager::SetParent(scene, child, root);
    EntityManager::SetParent(scene, root, child);

    const auto& rootHierarchy = scene.registry.get<HierarchyComponent>(root);
    const auto& childHierarchy = scene.registry.get<HierarchyComponent>(child);
    AXIS_CHECK(rootHierarchy.parent == entt::null);
    AXIS_CHECK(rootHierarchy.children.size() == 1);
    AXIS_CHECK(rootHierarchy.children[0] == child);
    AXIS_CHECK(childHierarchy.parent == root);
    AXIS_CHECK(childHierarchy.children.empty());
}

AXIS_TEST_CASE("EntityManager destroy detaches surviving children")
{
    Scene scene;
    auto parent = EntityManager::CreateEntity(scene, "Parent");
    auto child = EntityManager::CreateEntity(scene, "Child");
    EntityManager::SetParent(scene, child, parent);

    EntityManager::DestroyEntity(scene, parent);

    AXIS_CHECK(!scene.registry.valid(parent));
    AXIS_CHECK(scene.registry.valid(child));
    AXIS_CHECK(scene.registry.get<HierarchyComponent>(child).parent == entt::null);
}

AXIS_TEST_CASE("EntityManager destroy with children recursively destroys hierarchy")
{
    Scene scene;
    auto parent = EntityManager::CreateEntity(scene, "Parent");
    auto child = EntityManager::CreateEntity(scene, "Child");
    auto grandChild = EntityManager::CreateEntity(scene, "GrandChild");
    EntityManager::SetParent(scene, child, parent);
    EntityManager::SetParent(scene, grandChild, child);

    EntityManager::DestroyEntityWithChildren(scene, parent);

    AXIS_CHECK(!scene.registry.valid(parent));
    AXIS_CHECK(!scene.registry.valid(child));
    AXIS_CHECK(!scene.registry.valid(grandChild));
}

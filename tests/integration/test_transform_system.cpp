#include "test_framework.h"

#include <ecs/logic/entity_manager.h>
#include <ecs/logic/transform_system.h>
#include <ecs/unit/core_components.h>

namespace
{
glm::vec3 TranslationOf(const glm::mat4& matrix)
{
    return glm::vec3(matrix[3]);
}
}  // namespace

AXIS_TEST_CASE("TransformSystem computes local world transform")
{
    Scene scene;
    auto entity = EntityManager::CreateEntityWithTransform(scene, "Entity", {2.0f, 3.0f, 4.0f});

    TransformSystem system;
    system.BindRegistry(scene);
    system.Update(scene, 0.0f);

    auto worldPos = TranslationOf(scene.registry.get<WorldTransformComponent>(entity).worldMatrix);
    AXIS_CHECK_NEAR(worldPos.x, 2.0f, 0.0001f);
    AXIS_CHECK_NEAR(worldPos.y, 3.0f, 0.0001f);
    AXIS_CHECK_NEAR(worldPos.z, 4.0f, 0.0001f);
}

AXIS_TEST_CASE("TransformSystem composes parent and child transforms")
{
    Scene scene;
    auto parent = EntityManager::CreateEntityWithTransform(scene, "Parent", {10.0f, 0.0f, 0.0f});
    auto child = EntityManager::CreateEntityWithTransform(scene, "Child", {1.0f, 0.0f, 0.0f});
    EntityManager::SetParent(scene, child, parent);

    TransformSystem system;
    system.BindRegistry(scene);
    system.MarkTransformGraphDirty();
    system.Update(scene, 0.0f);

    auto childWorldPos = TranslationOf(scene.registry.get<WorldTransformComponent>(child).worldMatrix);
    AXIS_CHECK_NEAR(childWorldPos.x, 11.0f, 0.0001f);
    AXIS_CHECK_NEAR(childWorldPos.y, 0.0f, 0.0001f);
    AXIS_CHECK_NEAR(childWorldPos.z, 0.0f, 0.0001f);
}

AXIS_TEST_CASE("TransformSystem propagates dirty parent transform to child")
{
    Scene scene;
    auto parent = EntityManager::CreateEntityWithTransform(scene, "Parent", {10.0f, 0.0f, 0.0f});
    auto child = EntityManager::CreateEntityWithTransform(scene, "Child", {1.0f, 0.0f, 0.0f});
    EntityManager::SetParent(scene, child, parent);

    TransformSystem system;
    system.BindRegistry(scene);
    system.MarkTransformGraphDirty();
    system.Update(scene, 0.0f);

    scene.registry.get<PositionComponent>(parent).value = {20.0f, 0.0f, 0.0f};
    scene.registry.get<WorldTransformComponent>(parent).isDirty = true;
    system.Update(scene, 0.0f);

    auto childWorldPos = TranslationOf(scene.registry.get<WorldTransformComponent>(child).worldMatrix);
    AXIS_CHECK_NEAR(childWorldPos.x, 21.0f, 0.0001f);
}

AXIS_TEST_CASE("TransformSystem fixed update captures previous transform state")
{
    Scene scene;
    auto entity = EntityManager::CreateEntityWithTransform(scene, "Entity", {2.0f, 0.0f, 0.0f});

    TransformSystem system;
    system.BindRegistry(scene);
    system.Update(scene, 0.0f);
    system.FixedUpdate(scene, 1.0f / 60.0f);

    auto& pos = scene.registry.get<PositionComponent>(entity);
    auto& world = scene.registry.get<WorldTransformComponent>(entity);

    AXIS_CHECK_NEAR(pos.prev.x, 2.0f, 0.0001f);
    AXIS_CHECK_NEAR(TranslationOf(world.prevWorldMatrix).x, 2.0f, 0.0001f);

    pos.value = {5.0f, 0.0f, 0.0f};
    world.isDirty = true;
    system.Update(scene, 0.0f);

    AXIS_CHECK_NEAR(pos.prev.x, 2.0f, 0.0001f);
    AXIS_CHECK_NEAR(TranslationOf(world.worldMatrix).x, 5.0f, 0.0001f);
}

#include "test_framework.h"

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
    auto entity = scene.CreateEntityWithTransform("Entity", {2.0f, 3.0f, 4.0f});

    TransformSystem system;
    system.BindRegistry(scene);
    system.Update(scene, 0.0f);

    auto worldPos = TranslationOf(scene.GetComponent<WorldTransformComponent>(entity).worldMatrix);
    AXIS_CHECK_NEAR(worldPos.x, 2.0f, 0.0001f);
    AXIS_CHECK_NEAR(worldPos.y, 3.0f, 0.0001f);
    AXIS_CHECK_NEAR(worldPos.z, 4.0f, 0.0001f);
}

AXIS_TEST_CASE("TransformSystem composes parent and child transforms")
{
    Scene scene;
    auto parent = scene.CreateEntityWithTransform("Parent", {10.0f, 0.0f, 0.0f});
    auto child = scene.CreateEntityWithTransform("Child", {1.0f, 0.0f, 0.0f});
    scene.SetParent(child, parent, false);

    TransformSystem system;
    system.BindRegistry(scene);
    system.MarkTransformGraphDirty();
    system.Update(scene, 0.0f);

    auto childWorldPos = TranslationOf(scene.GetComponent<WorldTransformComponent>(child).worldMatrix);
    AXIS_CHECK_NEAR(childWorldPos.x, 11.0f, 0.0001f);
    AXIS_CHECK_NEAR(childWorldPos.y, 0.0f, 0.0001f);
    AXIS_CHECK_NEAR(childWorldPos.z, 0.0f, 0.0001f);
}

AXIS_TEST_CASE("TransformSystem propagates dirty parent transform to child")
{
    Scene scene;
    auto parent = scene.CreateEntityWithTransform("Parent", {10.0f, 0.0f, 0.0f});
    auto child = scene.CreateEntityWithTransform("Child", {1.0f, 0.0f, 0.0f});
    scene.SetParent(child, parent, false);

    TransformSystem system;
    system.BindRegistry(scene);
    system.MarkTransformGraphDirty();
    system.Update(scene, 0.0f);

    scene.GetComponent<PositionComponent>(parent).value = {20.0f, 0.0f, 0.0f};
    scene.MarkTransformDirty(parent);
    system.Update(scene, 0.0f);

    auto childWorldPos = TranslationOf(scene.GetComponent<WorldTransformComponent>(child).worldMatrix);
    AXIS_CHECK_NEAR(childWorldPos.x, 21.0f, 0.0001f);
}

AXIS_TEST_CASE("Scene reparent preserves current and previous world transforms when requested")
{
    Scene scene;
    auto firstParent = scene.CreateEntityWithTransform("FirstParent", {10.0f, 2.0f, -3.0f}, {0.0f, 25.0f, 0.0f},
                                                       {1.5f, 1.5f, 1.5f});
    auto secondParent = scene.CreateEntityWithTransform("SecondParent", {-4.0f, 1.0f, 8.0f}, {0.0f, -40.0f, 0.0f},
                                                        {0.75f, 0.75f, 0.75f});
    auto child = scene.CreateEntityWithTransform("Child", {2.0f, 3.0f, 1.0f}, {5.0f, 15.0f, 0.0f},
                                                 {0.5f, 0.75f, 1.0f});
    scene.SetParent(child, firstParent, false);

    TransformSystem system;
    system.BindRegistry(scene);
    system.MarkTransformGraphDirty();
    system.Update(scene, 0.0f);
    system.FixedUpdate(scene, 1.0f / 60.0f);
    const glm::mat4 worldBefore = scene.GetComponent<WorldTransformComponent>(child).worldMatrix;
    const glm::mat4 previousBefore = scene.GetComponent<WorldTransformComponent>(child).prevWorldMatrix;

    scene.SetParent(child, secondParent, true);
    system.MarkTransformGraphDirty();
    system.Update(scene, 0.0f);
    const auto& world = scene.GetComponent<WorldTransformComponent>(child);

    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            AXIS_CHECK_NEAR(world.worldMatrix[column][row], worldBefore[column][row], 0.001f);
            AXIS_CHECK_NEAR(world.prevWorldMatrix[column][row], previousBefore[column][row], 0.001f);
        }
    }
}

AXIS_TEST_CASE("Scene reparent keeps local transform when world preservation is disabled")
{
    Scene scene;
    auto parent = scene.CreateEntityWithTransform("Parent", {10.0f, 0.0f, 0.0f});
    auto child = scene.CreateEntityWithTransform("Child", {2.0f, 0.0f, 0.0f});
    scene.SetParent(child, parent, false);

    TransformSystem system;
    system.BindRegistry(scene);
    system.MarkTransformGraphDirty();
    system.Update(scene, 0.0f);

    AXIS_CHECK_NEAR(scene.GetComponent<PositionComponent>(child).value.x, 2.0f, 0.0001f);
    AXIS_CHECK_NEAR(TranslationOf(scene.GetComponent<WorldTransformComponent>(child).worldMatrix).x, 12.0f, 0.0001f);
}

AXIS_TEST_CASE("TransformSystem fixed update captures previous transform state")
{
    Scene scene;
    auto entity = scene.CreateEntityWithTransform("Entity", {2.0f, 0.0f, 0.0f});

    TransformSystem system;
    system.BindRegistry(scene);
    system.Update(scene, 0.0f);
    system.FixedUpdate(scene, 1.0f / 60.0f);

    auto& pos = scene.GetComponent<PositionComponent>(entity);
    auto& world = scene.GetComponent<WorldTransformComponent>(entity);

    AXIS_CHECK_NEAR(pos.prev.x, 2.0f, 0.0001f);
    AXIS_CHECK_NEAR(TranslationOf(world.prevWorldMatrix).x, 2.0f, 0.0001f);

    pos.value = {5.0f, 0.0f, 0.0f};
    scene.MarkTransformDirty(entity);
    system.Update(scene, 0.0f);

    AXIS_CHECK_NEAR(pos.prev.x, 2.0f, 0.0001f);
    AXIS_CHECK_NEAR(TranslationOf(world.worldMatrix).x, 5.0f, 0.0001f);
}

#include <ecs/unit/core_components.h>
#include <ecs/logic/transform_system.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>

void TransformSystem::Initialize()
{
}

void TransformSystem::FixedUpdate(Scene& scene, float dt)
{
    auto view = scene.registry.view<PositionComponent, RotationComponent, ScaleComponent, WorldTransformComponent>();
    for (auto entity : view)
    {
        auto& pos = view.get<PositionComponent>(entity);
        auto& rot = view.get<RotationComponent>(entity);
        auto& scl = view.get<ScaleComponent>(entity);
        auto& world = view.get<WorldTransformComponent>(entity);

        pos.prev = pos.value;
        rot.prev = rot.value;
        scl.prev = scl.value;
        world.prevWorldMatrix = world.worldMatrix;
    }
}

void TransformSystem::Update(Scene& scene, float dt)
{
    if (scene.isLinearTransformsDirty)
    {
        scene.RebuildLinearTransforms();
    }

    auto& registry = scene.registry;
    for (auto entity : scene.linearTransforms)
    {
        auto* world = registry.try_get<WorldTransformComponent>(entity);
        if (!world) continue;

        // Propagate dirtiness from parent if necessary
        auto* hierarchy = registry.try_get<HierarchyComponent>(entity);
        glm::mat4 parentTransform(1.0f);
        if (hierarchy && hierarchy->parent != entt::null)
        {
            if (auto* pWorld = registry.try_get<WorldTransformComponent>(hierarchy->parent))
            {
                parentTransform = pWorld->worldMatrix;
            }
        }

        if (world->isDirty)
        {
            auto* pos = registry.try_get<PositionComponent>(entity);
            auto* rot = registry.try_get<RotationComponent>(entity);
            auto* scl = registry.try_get<ScaleComponent>(entity);
            if (!pos || !rot || !scl) continue;

            glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), pos->value) *
                                    glm::toMat4(rot->value) *
                                    glm::scale(glm::mat4(1.0f), scl->value);

            world->worldMatrix = parentTransform * localMatrix;
            world->isDirty = false;
            world->version++;

            // Propagate dirty flag down the hierarchy. 
            // Because linearTransforms is depth-sorted (BFS), children will be updated 
            // later in this same loop and will see their isDirty flag as true.
            if (hierarchy)
            {
                for (auto child : hierarchy->children)
                {
                    if (auto* cWorld = registry.try_get<WorldTransformComponent>(child))
                    {
                        cWorld->isDirty = true;
                    }
                }
            }
        }
    }
}

std::vector<entt::id_type> TransformSystem::GetReadComponents() const
{
    return {
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash(),
        entt::type_id<ScaleComponent>().hash(),
        entt::type_id<HierarchyComponent>().hash()
    };
}

std::vector<entt::id_type> TransformSystem::GetWriteComponents() const
{
    return {
        entt::type_id<WorldTransformComponent>().hash(),
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash(),
        entt::type_id<ScaleComponent>().hash()
    };
}

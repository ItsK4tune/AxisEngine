#include <ecs/unit/core_components.h>
#include <ecs/logic/transform_system.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <core/logic/logger.h>

void TransformSystem::Init(EngineContext ctx)
{
    m_Ctx = ctx;
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
    auto& registry = scene.registry;

    auto view = registry.view<PositionComponent, RotationComponent, ScaleComponent, WorldTransformComponent>();
    
    for (auto entity : view)
    {
        auto* hierarchy = registry.try_get<HierarchyComponent>(entity);
        if (!hierarchy || hierarchy->parent == entt::null)
        {
        UpdateWorldTransform(entity, registry, glm::mat4(1.0f), false, 0);
        }
    }
}

void TransformSystem::Render(Scene& scene)
{
}

void TransformSystem::Shutdown()
{
}

void TransformSystem::UpdateWorldTransform(entt::entity entity, entt::registry& registry, const glm::mat4& parentTransform, bool parentDirty, int depth)
{
    auto* pos = registry.try_get<PositionComponent>(entity);
    auto* rot = registry.try_get<RotationComponent>(entity);
    auto* scl = registry.try_get<ScaleComponent>(entity);
    auto* world = registry.try_get<WorldTransformComponent>(entity);

    if (!pos || !rot || !scl || !world) return;
    if (depth > 64) {
        LOGGER_ERROR("TransformSystem") << "Max hierarchy depth reached for entity " << (uint32_t)entity << ". Possible cycle!";
        return;
    }

    bool isDirty = world->isDirty || parentDirty;
    
    if (isDirty)
    {
        glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), pos->value) *
                                glm::toMat4(rot->value) *
                                glm::scale(glm::mat4(1.0f), scl->value);
        
        world->worldMatrix = parentTransform * localMatrix;
        world->isDirty = false;
        world->version++;
    }

    auto* hierarchy = registry.try_get<HierarchyComponent>(entity);
    if (hierarchy)
    {
        for (auto child : hierarchy->children)
        {
            UpdateWorldTransform(child, registry, world->worldMatrix, isDirty, depth + 1);
        }
    }
}

#include <ecs/systems/transform_system.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

void TransformSystem::Init(EngineContext ctx)
{
    m_Ctx = ctx;
}

void TransformSystem::FixedUpdate(Scene& scene, float dt)
{
    // Store previous states for interpolation
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

    // First, identify all root entities (no parent)
    auto view = registry.view<PositionComponent, RotationComponent, ScaleComponent, WorldTransformComponent>();
    
    for (auto entity : view)
    {
        auto* hierarchy = registry.try_get<HierarchyComponent>(entity);
        if (!hierarchy || hierarchy->parent == entt::null)
        {
            UpdateWorldTransform(entity, registry, glm::mat4(1.0f), false);
        }
    }
}

void TransformSystem::Render(Scene& scene)
{
}

void TransformSystem::Shutdown()
{
}

void TransformSystem::UpdateWorldTransform(entt::entity entity, entt::registry& registry, const glm::mat4& parentTransform, bool parentDirty)
{
    auto* pos = registry.try_get<PositionComponent>(entity);
    auto* rot = registry.try_get<RotationComponent>(entity);
    auto* scl = registry.try_get<ScaleComponent>(entity);
    auto* world = registry.try_get<WorldTransformComponent>(entity);

    if (!pos || !rot || !scl || !world) return;

    bool isDirty = world->isDirty || parentDirty;
    
    // Check if PSR changed (this could be optimized with signals setting world->isDirty)
    // For now, let's assume world->isDirty is managed externally or we check it here.
    
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
            UpdateWorldTransform(child, registry, world->worldMatrix, isDirty);
        }
    }
}

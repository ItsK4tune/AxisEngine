#include <ecs/unit/core_components.h>
#include <ecs/logic/transform_system.h>
#include <ecs/logic/system_factory.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_system.h>
#include <scene/type/scene_events.h>
#include <deque>

REGISTER_SYSTEM(TransformSystem)

void TransformSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<TransformSystem>(this);
    EventSystem::Instance().Subscribe<SceneChangedEvent>([this](const SceneChangedEvent& e) {
        
        e.registry->on_construct<HierarchyComponent>().connect<&TransformSystem::OnHierarchyChanged>(this);
        e.registry->on_destroy<HierarchyComponent>().connect<&TransformSystem::OnHierarchyChanged>(this);
        e.registry->on_update<HierarchyComponent>().connect<&TransformSystem::OnHierarchyChanged>(this);
        
        e.registry->on_construct<WorldTransformComponent>().connect<&TransformSystem::OnHierarchyChanged>(this);
        e.registry->on_destroy<WorldTransformComponent>().connect<&TransformSystem::OnHierarchyChanged>(this);

        e.registry->on_update<PositionComponent>().connect<&TransformSystem::OnTransformChanged>(this);
        e.registry->on_update<RotationComponent>().connect<&TransformSystem::OnTransformChanged>(this);
        e.registry->on_update<ScaleComponent>().connect<&TransformSystem::OnTransformChanged>(this);

        m_IsLinearTransformsDirty = true;
        RebuildLinearTransforms(*(Scene*)e.scene); 
    });
}

void TransformSystem::OnTransformChanged(entt::registry &reg, entt::entity entity)
{
    if (auto* world = reg.try_get<WorldTransformComponent>(entity))
    {
        world->isDirty = true;
    }
}

void TransformSystem::OnHierarchyChanged(entt::registry &reg, entt::entity entity)
{
    m_IsLinearTransformsDirty = true;
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

void TransformSystem::RebuildLinearTransforms(Scene& scene)
{
    m_LinearTransforms.clear();
    std::vector<entt::entity> roots;
    auto& registry = scene.registry;
    auto transformView = registry.view<WorldTransformComponent>();
    
    for (auto entity : transformView)
    {
        auto* hierarchy = registry.try_get<HierarchyComponent>(entity);
        if (!hierarchy || hierarchy->parent == entt::null)
        {
            roots.push_back(entity);
        }
    }

    std::deque<entt::entity> queue(roots.begin(), roots.end());
    while (!queue.empty())
    {
        entt::entity current = queue.front();
        queue.pop_front();

        m_LinearTransforms.push_back(current);

        auto* hierarchy = registry.try_get<HierarchyComponent>(current);
        if (hierarchy)
        {
            for (auto child : hierarchy->children)
            {
                if (registry.valid(child) && registry.all_of<WorldTransformComponent>(child))
                {
                    queue.push_back(child);
                }
            }
        }
    }

    m_IsLinearTransformsDirty = false;
    LOGGER_INFO("TransformSystem") << "Rebuilt linear transforms for " << m_LinearTransforms.size() << " entities";
}

void TransformSystem::Update(Scene& scene, float dt)
{
    if (m_IsLinearTransformsDirty)
    {
        RebuildLinearTransforms(scene);
    }

    auto& registry = scene.registry;
    for (auto entity : m_LinearTransforms)
    {
        auto* world = registry.try_get<WorldTransformComponent>(entity);
        if (!world) continue;

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

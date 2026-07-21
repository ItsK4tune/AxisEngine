#include <ecs/logic/transform_system.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <glm/gtc/matrix_transform.hpp>

#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <scene/type/scene_events.h>
#include <deque>
#include <queue>
#include <glm/gtx/quaternion.hpp>


void TransformSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<TransformSystem>(this);
    m_EventSubscriptions.Clear();
    if (auto* scene = sl.Resolve<Scene>())
        BindRegistry(*scene);

    m_EventSubscriptions.Add(EventManager::Instance().Subscribe<SceneChangedEvent>([this](const SceneChangedEvent& e) {
        if (e.scene)
            BindRegistry(*e.scene);
    }));
}

void TransformSystem::Shutdown()
{
    m_EventSubscriptions.Clear();
    UnbindRegistries();
    m_LinearTransforms.clear();
    m_DirtyTransforms.clear();
    m_LinearTransformIndices.clear();
    m_HasParentedTransforms = false;
    m_IsLinearTransformsDirty = true;
}

void TransformSystem::BindRegistry(Scene& scene)
{
    auto& registry = scene.GetRegistry();
    if (!m_BoundRegistries.insert(&registry).second)
        return;

    registry.on_construct<HierarchyComponent>().connect<&TransformSystem::OnHierarchyChanged>(this);
    registry.on_destroy<HierarchyComponent>().connect<&TransformSystem::OnHierarchyChanged>(this);
    registry.on_update<HierarchyComponent>().connect<&TransformSystem::OnHierarchyChanged>(this);

    registry.on_construct<WorldTransformComponent>().connect<&TransformSystem::OnHierarchyChanged>(this);
    registry.on_destroy<WorldTransformComponent>().connect<&TransformSystem::OnHierarchyChanged>(this);

    registry.on_update<PositionComponent>().connect<&TransformSystem::OnTransformChanged>(this);
    registry.on_update<RotationComponent>().connect<&TransformSystem::OnTransformChanged>(this);
    registry.on_update<ScaleComponent>().connect<&TransformSystem::OnTransformChanged>(this);

    m_IsLinearTransformsDirty = true;
    RebuildLinearTransforms(scene);
}

void TransformSystem::UnbindRegistries()
{
    for (auto* registry : m_BoundRegistries)
    {
        if (!registry)
            continue;

        registry->on_construct<HierarchyComponent>().disconnect<&TransformSystem::OnHierarchyChanged>(this);
        registry->on_destroy<HierarchyComponent>().disconnect<&TransformSystem::OnHierarchyChanged>(this);
        registry->on_update<HierarchyComponent>().disconnect<&TransformSystem::OnHierarchyChanged>(this);

        registry->on_construct<WorldTransformComponent>().disconnect<&TransformSystem::OnHierarchyChanged>(this);
        registry->on_destroy<WorldTransformComponent>().disconnect<&TransformSystem::OnHierarchyChanged>(this);

        registry->on_update<PositionComponent>().disconnect<&TransformSystem::OnTransformChanged>(this);
        registry->on_update<RotationComponent>().disconnect<&TransformSystem::OnTransformChanged>(this);
        registry->on_update<ScaleComponent>().disconnect<&TransformSystem::OnTransformChanged>(this);
    }
    m_BoundRegistries.clear();
}

void TransformSystem::OnTransformChanged(entt::registry& reg, entt::entity entity)
{
    if (auto* world = reg.try_get<WorldTransformComponent>(entity))
    {
        world->isDirty = true;
        m_DirtyTransforms.insert(entity);
    }
}

void TransformSystem::OnHierarchyChanged(entt::registry& reg, entt::entity entity)
{
    m_IsLinearTransformsDirty = true;
}

void TransformSystem::FixedUpdate(Scene& scene, float dt)
{
    auto view = scene.View<PositionComponent, RotationComponent, ScaleComponent, WorldTransformComponent>();
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
    m_LinearTransformIndices.clear();
    std::vector<entt::entity> roots;
    auto& registry = scene.GetRegistry();
    auto transformView = registry.view<WorldTransformComponent>();
    const size_t transformCount = transformView.size();
    m_LinearTransforms.reserve(transformCount);
    m_DirtyTransforms.reserve(transformCount);

    m_HasParentedTransforms = false;
    auto hierarchyView = registry.view<HierarchyComponent>();
    for (auto entity : hierarchyView)
    {
        const auto& hierarchy = hierarchyView.get<HierarchyComponent>(entity);
        if (hierarchy.parent != entt::null && registry.all_of<WorldTransformComponent>(entity))
        {
            m_HasParentedTransforms = true;
            break;
        }
    }

    // Flat procedural scenes need neither a parent-first traversal nor the
    // entity-to-linear-index table used to propagate hierarchical updates.
    if (!m_HasParentedTransforms)
    {
        for (auto entity : transformView)
        {
            m_LinearTransforms.push_back(entity);
            if (transformView.get<WorldTransformComponent>(entity).isDirty)
                m_DirtyTransforms.insert(entity);
        }
        m_IsLinearTransformsDirty = false;
        LOGGER_INFO("TransformSystem") << "Rebuilt flat transform list for " << m_LinearTransforms.size()
                                       << " entities";
        return;
    }

    roots.reserve(transformCount);
    m_LinearTransformIndices.reserve(transformCount);
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
        m_LinearTransformIndices[current] = m_LinearTransforms.size() - 1;
        if (auto* world = registry.try_get<WorldTransformComponent>(current); world && world->isDirty)
            m_DirtyTransforms.insert(current);

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

    auto& registry = scene.GetRegistry();
    scene.ConsumeDirtyTransforms(m_DirtyScratch);
    for (const auto entity : m_DirtyScratch)
    {
        if (!registry.valid(entity))
            continue;
        if (const auto* world = registry.try_get<WorldTransformComponent>(entity); world && world->isDirty)
            m_DirtyTransforms.insert(entity);
    }
    for (auto it = m_DirtyTransforms.begin(); it != m_DirtyTransforms.end();)
    {
        const auto entity = *it;
        const auto* world = registry.valid(entity) ? registry.try_get<WorldTransformComponent>(entity) : nullptr;
        if (!world || !world->isDirty)
            it = m_DirtyTransforms.erase(it);
        else
            ++it;
    }
    if (m_DirtyTransforms.empty())
        return;

    if (!m_HasParentedTransforms)
    {
        for (const auto entity : m_DirtyTransforms)
        {
            auto* world = registry.try_get<WorldTransformComponent>(entity);
            auto* pos = registry.try_get<PositionComponent>(entity);
            auto* rot = registry.try_get<RotationComponent>(entity);
            auto* scl = registry.try_get<ScaleComponent>(entity);
            if (!world || !pos || !rot || !scl || !world->isDirty)
                continue;

            world->worldMatrix = glm::translate(glm::mat4(1.0f), pos->value) * glm::toMat4(rot->value) *
                                 glm::scale(glm::mat4(1.0f), scl->value);
            world->prevWorldMatrix = glm::translate(glm::mat4(1.0f), pos->prev) * glm::toMat4(rot->prev) *
                                     glm::scale(glm::mat4(1.0f), scl->prev);
            world->isDirty = false;
            world->version++;
            scene.MarkOctreeEntityDirty(entity);
        }
        m_DirtyTransforms.clear();
        return;
    }

    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> pending;
    std::unordered_set<entt::entity> queued;
    queued.reserve(m_DirtyTransforms.size() * 2);
    for (const auto entity : m_DirtyTransforms)
    {
        if (const auto found = m_LinearTransformIndices.find(entity); found != m_LinearTransformIndices.end())
        {
            pending.push(found->second);
            queued.insert(entity);
        }
    }
    m_DirtyTransforms.clear();

    bool spatialDataChanged = false;
    while (!pending.empty())
    {
        const size_t linearIndex = pending.top();
        pending.pop();
        if (linearIndex >= m_LinearTransforms.size())
            continue;
        const entt::entity entity = m_LinearTransforms[linearIndex];
        auto* world = registry.try_get<WorldTransformComponent>(entity);
        if (!world)
            continue;

        auto* hierarchy = registry.try_get<HierarchyComponent>(entity);
        glm::mat4 parentTransform(1.0f);
        glm::mat4 parentPrevTransform(1.0f);
        if (hierarchy && hierarchy->parent != entt::null)
        {
            if (auto* pWorld = registry.try_get<WorldTransformComponent>(hierarchy->parent))
            {
                parentTransform = pWorld->worldMatrix;
                parentPrevTransform = pWorld->prevWorldMatrix;
            }
        }

        if (world->isDirty)
        {
            auto* pos = registry.try_get<PositionComponent>(entity);
            auto* rot = registry.try_get<RotationComponent>(entity);
            auto* scl = registry.try_get<ScaleComponent>(entity);
            if (!pos || !rot || !scl)
                continue;

            glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), pos->value) * glm::toMat4(rot->value) *
                                    glm::scale(glm::mat4(1.0f), scl->value);

            glm::mat4 prevLocalMatrix = glm::translate(glm::mat4(1.0f), pos->prev) * glm::toMat4(rot->prev) *
                                        glm::scale(glm::mat4(1.0f), scl->prev);

            world->worldMatrix = parentTransform * localMatrix;
            world->prevWorldMatrix = parentPrevTransform * prevLocalMatrix;
            world->isDirty = false;
            world->version++;
            spatialDataChanged = true;
            scene.MarkOctreeEntityDirty(entity);

            if (hierarchy)
            {
                for (auto child : hierarchy->children)
                {
                    if (auto* cWorld = registry.try_get<WorldTransformComponent>(child))
                    {
                        cWorld->isDirty = true;
                        if (queued.insert(child).second)
                        {
                            if (const auto found = m_LinearTransformIndices.find(child);
                                found != m_LinearTransformIndices.end())
                                pending.push(found->second);
                        }
                    }
                }
            }
        }
    }
    (void)spatialDataChanged;
}

std::vector<entt::id_type> TransformSystem::GetReadComponents() const
{
    return {entt::type_id<PositionComponent>().hash(), entt::type_id<RotationComponent>().hash(),
            entt::type_id<ScaleComponent>().hash(), entt::type_id<HierarchyComponent>().hash()};
}

std::vector<entt::id_type> TransformSystem::GetWriteComponents() const
{
    return {entt::type_id<WorldTransformComponent>().hash(), entt::type_id<PositionComponent>().hash(),
            entt::type_id<RotationComponent>().hash(), entt::type_id<ScaleComponent>().hash()};
}

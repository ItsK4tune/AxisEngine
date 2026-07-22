#include <scene/logic/scene.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/script_component.h>
#include <ecs/unit/ui_components.h>
#include <script/logic/scriptable.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <physics/interface/i_physics_world.h>
#include <physics/logic/constraint_lifecycle.h>
#include <scene/logic/scene_manager.h>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>

namespace
{
glm::mat4 LocalTransform(entt::registry& registry, entt::entity entity, bool previous)
{
    const auto* position = registry.try_get<PositionComponent>(entity);
    const auto* rotation = registry.try_get<RotationComponent>(entity);
    const auto* scale = registry.try_get<ScaleComponent>(entity);
    if (!position || !rotation || !scale)
        return glm::mat4(1.0f);

    const glm::vec3& p = previous ? position->prev : position->value;
    const glm::quat& r = previous ? rotation->prev : rotation->value;
    const glm::vec3& s = previous ? scale->prev : scale->value;
    return glm::translate(glm::mat4(1.0f), p) * glm::toMat4(r) * glm::scale(glm::mat4(1.0f), s);
}

glm::mat4 CalculateWorldTransform(entt::registry& registry, entt::entity entity, bool previous)
{
    std::vector<entt::entity> chain;
    for (entt::entity current = entity; registry.valid(current);)
    {
        chain.push_back(current);
        const auto* hierarchy = registry.try_get<HierarchyComponent>(current);
        if (!hierarchy || hierarchy->parent == entt::null || !registry.valid(hierarchy->parent))
            break;
        current = hierarchy->parent;
    }

    glm::mat4 world(1.0f);
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) world *= LocalTransform(registry, *it, previous);
    return world;
}

void ApplyLocalTransform(entt::registry& registry, entt::entity entity, const glm::mat4& matrix, bool previous)
{
    auto* position = registry.try_get<PositionComponent>(entity);
    auto* rotation = registry.try_get<RotationComponent>(entity);
    auto* scale = registry.try_get<ScaleComponent>(entity);
    if (!position || !rotation || !scale)
        return;

    glm::vec3 localScale(glm::length(glm::vec3(matrix[0])), glm::length(glm::vec3(matrix[1])),
                         glm::length(glm::vec3(matrix[2])));
    const float determinant = glm::determinant(glm::mat3(matrix));
    if (determinant < 0.0f)
        localScale.x = -localScale.x;

    glm::mat3 rotationMatrix(1.0f);
    constexpr float epsilon = 0.000001f;
    if (std::abs(localScale.x) > epsilon)
        rotationMatrix[0] = glm::vec3(matrix[0]) / localScale.x;
    if (std::abs(localScale.y) > epsilon)
        rotationMatrix[1] = glm::vec3(matrix[1]) / localScale.y;
    if (std::abs(localScale.z) > epsilon)
        rotationMatrix[2] = glm::vec3(matrix[2]) / localScale.z;

    const glm::vec3 localPosition(matrix[3]);
    const glm::quat localRotation = glm::normalize(glm::quat_cast(rotationMatrix));
    if (previous)
    {
        position->prev = localPosition;
        rotation->prev = localRotation;
        scale->prev = localScale;
    }
    else
    {
        position->value = localPosition;
        rotation->value = localRotation;
        scale->value = localScale;
    }
}
}  // namespace

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::InitializeManagers()
{
    m_Octree = std::make_unique<Octree>(AABB(glm::vec3(-1000.0f), glm::vec3(1000.0f)));
    m_OctreeDirty = true;
    m_OctreeFullRebuildRequired = true;
    m_DirtyOctreeEntities.clear();
    m_OctreeDirtyEventCount = 0;

    registry.on_update<PositionComponent>().connect<&Scene::OnOctreeDirty>(this);
    registry.on_update<RotationComponent>().connect<&Scene::OnOctreeDirty>(this);
    registry.on_update<ScaleComponent>().connect<&Scene::OnOctreeDirty>(this);
    registry.on_construct<MeshRendererComponent>().connect<&Scene::OnOctreeDirty>(this);
    registry.on_destroy<MeshRendererComponent>().connect<&Scene::OnOctreeDirty>(this);
}

void Scene::ShutdownManagers()
{
    registry.on_update<PositionComponent>().disconnect<&Scene::OnOctreeDirty>(this);
    registry.on_update<RotationComponent>().disconnect<&Scene::OnOctreeDirty>(this);
    registry.on_update<ScaleComponent>().disconnect<&Scene::OnOctreeDirty>(this);
    registry.on_construct<MeshRendererComponent>().disconnect<&Scene::OnOctreeDirty>(this);
    registry.on_destroy<MeshRendererComponent>().disconnect<&Scene::OnOctreeDirty>(this);

    m_Octree.reset();
    m_DirtyOctreeEntities.clear();
    m_OctreeDirtyEventCount = 0;
    m_DirtyTransforms.clear();
    m_OctreeDirty = true;
    m_OctreeFullRebuildRequired = true;
}

void Scene::Destroy(Entity entity)
{
    if (entity && registry.valid(entity))
    {
        DestroyEntity(entity);
    }
}

Entity Scene::FindByName(const std::string& name)
{
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            return Entity(entity, this);
    }
    return Entity();
}

Entity Scene::FindByTag(const std::string& tag)
{
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).tag == tag)
            return Entity(entity, this);
    }
    return Entity();
}

Entity Scene::FindByNameAndTag(const std::string& name, const std::string& tag)
{
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        const auto& info = view.get<InfoComponent>(entity);
        if (info.name == name && info.tag == tag)
            return Entity(entity, this);
    }
    return Entity();
}

Entity Scene::FindByNameTagAndScene(const std::string& name, const std::string& tag, const std::string& sceneName)
{
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        const auto& info = view.get<InfoComponent>(entity);
        if (info.name == name && info.tag == tag && info.sceneName == sceneName)
            return Entity(entity, this);
    }
    return Entity();
}

std::vector<Entity> Scene::FindAllByName(const std::string& name)
{
    std::vector<Entity> results;
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            results.push_back(Entity(entity, this));
    }
    return results;
}

std::vector<Entity> Scene::FindAllByTag(const std::string& tag)
{
    std::vector<Entity> results;
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).tag == tag)
            results.push_back(Entity(entity, this));
    }
    return results;
}

std::vector<Entity> Scene::FindAllBySceneName(const std::string& sceneName)
{
    std::vector<Entity> results;
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).sceneName == sceneName)
            results.push_back(Entity(entity, this));
    }
    return results;
}

Entity Scene::GetCameraByName(const std::string& name)
{
    auto view = registry.view<CameraComponent, InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            return Entity(entity, this);
    }
    return Entity();
}

Entity Scene::GetCameraByTag(const std::string& tag)
{
    auto view = registry.view<CameraComponent, InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).tag == tag)
            return Entity(entity, this);
    }
    return Entity();
}

std::vector<Entity> Scene::GetAllCameras()
{
    std::vector<Entity> cameras;
    auto view = registry.view<CameraComponent>();
    for (auto entity : view) cameras.push_back(Entity(entity, this));
    return cameras;
}

Entity Scene::GetActiveCamera()
{
    if (registry.valid(m_ActiveCamera) && registry.all_of<CameraComponent>(m_ActiveCamera) &&
        registry.get<CameraComponent>(m_ActiveCamera).isPrimary)
        return Entity(m_ActiveCamera, this);

    auto view = registry.view<CameraComponent>();
    for (auto entity : view)
    {
        if (view.get<CameraComponent>(entity).isPrimary)
        {
            m_ActiveCamera = entity;
            return Entity(entity, this);
        }
    }
    for (auto entity : view)
    {
        m_ActiveCamera = entity;
        return Entity(entity, this);
    }
    m_ActiveCamera = entt::null;
    return Entity();
}

void Scene::SetActiveCamera(Entity entity)
{
    m_ActiveCamera = entt::null;
    auto view = registry.view<CameraComponent>();
    for (auto camEntity : view)
    {
        view.get<CameraComponent>(camEntity).isPrimary = (camEntity == entity);
        if (camEntity == entity)
            m_ActiveCamera = camEntity;
    }
}

Entity Scene::GetActiveSkybox()
{
    if (registry.valid(m_ActiveSkybox) && registry.all_of<SkyboxRenderComponent>(m_ActiveSkybox) &&
        registry.get<SkyboxRenderComponent>(m_ActiveSkybox).isPrimary)
        return Entity(m_ActiveSkybox, this);

    auto view = registry.view<SkyboxRenderComponent>();
    for (auto entity : view)
    {
        if (view.get<SkyboxRenderComponent>(entity).isPrimary)
        {
            m_ActiveSkybox = entity;
            return Entity(entity, this);
        }
    }
    for (auto entity : view)
    {
        m_ActiveSkybox = entity;
        return Entity(entity, this);
    }
    m_ActiveSkybox = entt::null;
    return Entity();
}

void Scene::SetActiveSkybox(Entity entity)
{
    m_ActiveSkybox = entt::null;
    auto view = registry.view<SkyboxRenderComponent>();
    for (auto skyEntity : view)
    {
        view.get<SkyboxRenderComponent>(skyEntity).isPrimary = (skyEntity == entity);
        if (skyEntity == entity)
            m_ActiveSkybox = skyEntity;
    }
}

Entity Scene::CreateEntity(const std::string& name, const std::string& tag)
{
    entt::entity entity = registry.create();
    registry.emplace<PositionComponent>(entity);
    registry.emplace<RotationComponent>(entity);
    registry.emplace<ScaleComponent>(entity);
    registry.emplace<HierarchyComponent>(entity);
    registry.emplace<WorldTransformComponent>(entity);
    registry.emplace<InfoComponent>(entity, name, CanonicalizeEntityTag(tag));
    return Entity(entity, this);
}

Entity Scene::CreateEntityWithTransform(const std::string& name, const glm::vec3& position, const glm::vec3& rotation,
                                        const glm::vec3& scale)
{
    Entity entity = CreateEntity(name);

    if (auto* p = registry.try_get<PositionComponent>(entity))
        p->value = p->prev = position;
    if (auto* r = registry.try_get<RotationComponent>(entity))
        r->value = r->prev = glm::quat(glm::radians(rotation));
    if (auto* s = registry.try_get<ScaleComponent>(entity))
        s->value = s->prev = scale;

    return entity;
}

Entity Scene::CreateEmptyEntity(const std::string& name)
{
    return CreateEntityWithTransform(name, glm::vec3(0.0f));
}

void Scene::SetParent(Entity child, Entity parent, bool keepWorldTransform)
{
    entt::entity childHandle = child;
    entt::entity parentHandle = parent;

    if (!registry.valid(childHandle) || !registry.valid(parentHandle))
    {
        LOGGER_WARN("Scene") << "Invalid child or parent entity";
        return;
    }

    // EntityBuilder deliberately creates components on demand. Parenting is a
    // core scene operation, so make its required bookkeeping available for any
    // valid entity instead of silently rejecting otherwise valid builders.
    auto& childH = registry.get_or_emplace<HierarchyComponent>(childHandle);
    (void)registry.get_or_emplace<HierarchyComponent>(parentHandle);
    if (childHandle == parentHandle)
    {
        LOGGER_WARN("Scene") << "Attempted to set entity as its own parent: " << (uint32_t)childHandle;
        return;
    }

    if (childH.parent == parentHandle)
        return;

    entt::entity current = parentHandle;
    while (current != entt::null)
    {
        if (current == childHandle)
        {
            LOGGER_WARN("Scene") << "Cycle detected in hierarchy! Cannot set " << (uint32_t)childHandle
                                 << " as child of " << (uint32_t)parentHandle;
            return;
        }
        if (auto* pH = registry.try_get<HierarchyComponent>(current))
            current = pH->parent;
        else
            break;
    }

    const glm::mat4 childWorld =
        keepWorldTransform ? CalculateWorldTransform(registry, childHandle, false) : glm::mat4(1.0f);
    const glm::mat4 childPreviousWorld =
        keepWorldTransform ? CalculateWorldTransform(registry, childHandle, true) : glm::mat4(1.0f);
    const glm::mat4 parentWorld =
        keepWorldTransform ? CalculateWorldTransform(registry, parentHandle, false) : glm::mat4(1.0f);
    const glm::mat4 parentPreviousWorld =
        keepWorldTransform ? CalculateWorldTransform(registry, parentHandle, true) : glm::mat4(1.0f);
    if (keepWorldTransform && (std::abs(glm::determinant(glm::mat3(parentWorld))) < 0.000001f ||
                               std::abs(glm::determinant(glm::mat3(parentPreviousWorld))) < 0.000001f))
    {
        LOGGER_WARN("Scene") << "Cannot preserve world transform under a singular parent transform";
        return;
    }

    const entt::entity oldParent = childH.parent;
    if (registry.valid(oldParent) && registry.all_of<HierarchyComponent>(oldParent))
    {
        registry.patch<HierarchyComponent>(oldParent, [childHandle](HierarchyComponent& oldParentH) {
            oldParentH.children.erase(std::remove(oldParentH.children.begin(), oldParentH.children.end(), childHandle),
                                      oldParentH.children.end());
        });
    }

    registry.patch<HierarchyComponent>(
        childHandle, [parentHandle](HierarchyComponent& hierarchy) { hierarchy.parent = parentHandle; });
    registry.patch<HierarchyComponent>(parentHandle, [childHandle](HierarchyComponent& hierarchy) {
        if (std::find(hierarchy.children.begin(), hierarchy.children.end(), childHandle) == hierarchy.children.end())
            hierarchy.children.push_back(childHandle);
    });

    if (keepWorldTransform)
    {
        ApplyLocalTransform(registry, childHandle, glm::inverse(parentWorld) * childWorld, false);
        ApplyLocalTransform(registry, childHandle, glm::inverse(parentPreviousWorld) * childPreviousWorld, true);
    }

    MarkTransformDirty(childHandle);
}

void Scene::MarkTransformDirty(entt::entity entity)
{
    if (!registry.valid(entity))
        return;
    if (auto* world = registry.try_get<WorldTransformComponent>(entity))
    {
        world->isDirty = true;
        m_DirtyTransforms.insert(entity);
        MarkOctreeEntityDirty(entity);
    }
}

void Scene::MarkOctreeEntityDirty(entt::entity entity)
{
    m_OctreeDirty = true;
    ++m_OctreeDirtyEventCount;
    if (entity != entt::null)
        m_DirtyOctreeEntities.insert(entity);
}

bool Scene::ConsumeOctreeChanges(std::vector<entt::entity>& output)
{
    output.assign(m_DirtyOctreeEntities.begin(), m_DirtyOctreeEntities.end());
    m_DirtyOctreeEntities.clear();
    const bool fullRebuild = m_OctreeFullRebuildRequired;
    m_OctreeFullRebuildRequired = false;
    m_OctreeDirty = false;
    return fullRebuild;
}

void Scene::ConsumeDirtyTransforms(std::vector<entt::entity>& output)
{
    output.assign(m_DirtyTransforms.begin(), m_DirtyTransforms.end());
    m_DirtyTransforms.clear();
}

void Scene::AddChild(Entity parent, Entity child, bool keepWorldTransform)
{
    SetParent(child, parent, keepWorldTransform);
}

void Scene::DestroyEntity(Entity entity, SceneManager* manager)
{
    entt::entity handle = entity;
    if (!registry.valid(handle))
        return;

    if (auto* h = registry.try_get<HierarchyComponent>(handle))
    {
        if (registry.valid(h->parent) && registry.all_of<HierarchyComponent>(h->parent))
        {
            auto& parentH = registry.get<HierarchyComponent>(h->parent);
            parentH.children.erase(std::remove(parentH.children.begin(), parentH.children.end(), handle),
                                   parentH.children.end());
        }

        std::vector<entt::entity> childrenCopy = h->children;
        for (auto childNode : childrenCopy)
        {
            if (registry.valid(childNode) && registry.all_of<HierarchyComponent>(childNode))
            {
                auto& childH = registry.get<HierarchyComponent>(childNode);
                childH.parent = entt::null;
            }
        }
    }

    if (auto sc = registry.try_get<ScriptComponent>(handle))
    {
        if (sc->instance && sc->DestroyScript)
            sc->DestroyScript(sc);
        sc->instance.reset();
        sc->scriptableInstance = nullptr;
        sc->inputScriptableInstance = nullptr;
    }

    if (auto rb = registry.try_get<RigidBodyComponent>(handle))
    {
        IPhysicsWorld* physicsWorld =
            manager ? manager->GetPhysicsWorld() : ServiceLocator::Instance().Resolve<IPhysicsWorld>();
        if (physicsWorld)
        {
            PhysicsConstraintLifecycle::RemoveAll(registry, *physicsWorld, *rb);
            if (rb->body)
                physicsWorld->RemoveRigidBody(rb->body.get());
        }
        rb->constraints.clear();
        rb->body = nullptr;
    }

    if (auto mesh = registry.try_get<MeshRendererComponent>(handle))
    {
        mesh->model = nullptr;
        mesh->shader.reset();
    }

    if (auto anim = registry.try_get<AnimationComponent>(handle))
    {
        anim->animator = nullptr;
    }

    if (auto ui = registry.try_get<UIRendererComponent>(handle))
    {
        ui->model = nullptr;
        ui->shader = nullptr;
    }

    if (auto text = registry.try_get<UITextComponent>(handle))
    {
        text->model = nullptr;
        text->shader = nullptr;
        text->font = nullptr;
    }

    if (auto sky = registry.try_get<SkyboxRenderComponent>(handle))
    {
        sky->skybox = nullptr;
        sky->shader.reset();
    }

    if (manager)
        manager->RemoveEntity(handle);

    registry.destroy(handle);
}

void Scene::DestroyEntityWithChildren(Entity entity, SceneManager* manager)
{
    entt::entity handle = entity;
    if (!registry.valid(handle))
        return;

    std::vector<entt::entity> children;
    if (registry.all_of<HierarchyComponent>(handle))
    {
        const auto& hier = registry.get<HierarchyComponent>(handle);
        children = hier.children;
    }

    for (auto child : children)
    {
        DestroyEntityWithChildren(Entity(child, this), manager);
    }

    DestroyEntity(entity, manager);
}

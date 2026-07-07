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
#include <scene/logic/scene_manager.h>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <vector>

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

Entity Scene::FindByNameTagAndScene(const std::string& name, const std::string& tag,
                                    const std::string& sceneName)
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
    auto view = registry.view<CameraComponent>();
    for (auto entity : view)
    {
        if (view.get<CameraComponent>(entity).isPrimary)
        {
            return Entity(entity, this);
        }
    }
    for (auto entity : view)
    {
        return Entity(entity, this);
    }
    return Entity();
}

void Scene::SetActiveCamera(Entity entity)
{
    auto view = registry.view<CameraComponent>();
    for (auto camEntity : view)
    {
        view.get<CameraComponent>(camEntity).isPrimary = (camEntity == entity);
    }
}

Entity Scene::GetActiveSkybox()
{
    auto view = registry.view<SkyboxRenderComponent>();
    for (auto entity : view)
    {
        if (view.get<SkyboxRenderComponent>(entity).isPrimary)
        {
            return Entity(entity, this);
        }
    }
    for (auto entity : view)
    {
        return Entity(entity, this);
    }
    return Entity();
}

void Scene::SetActiveSkybox(Entity entity)
{
    auto view = registry.view<SkyboxRenderComponent>();
    for (auto skyEntity : view)
    {
        view.get<SkyboxRenderComponent>(skyEntity).isPrimary = (skyEntity == entity);
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
    registry.emplace<InfoComponent>(entity, name, tag);
    return Entity(entity, this);
}

Entity Scene::CreateEntityWithTransform(const std::string& name, const glm::vec3& position,
                                              const glm::vec3& rotation, const glm::vec3& scale)
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

    if (!registry.all_of<HierarchyComponent>(childHandle) || !registry.all_of<HierarchyComponent>(parentHandle))
    {
        LOGGER_WARN("Scene") << "Child or parent missing HierarchyComponent";
        return;
    }

    auto& childH = registry.get<HierarchyComponent>(childHandle);
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
            LOGGER_WARN("Scene") << "Cycle detected in hierarchy! Cannot set " << (uint32_t)childHandle << " as child of "
                                 << (uint32_t)parentHandle;
            return;
        }
        if (auto* pH = registry.try_get<HierarchyComponent>(current))
            current = pH->parent;
        else
            break;
    }

    if (registry.valid(childH.parent) && registry.all_of<HierarchyComponent>(childH.parent))
    {
        auto& oldParentH = registry.get<HierarchyComponent>(childH.parent);
        oldParentH.children.erase(std::remove(oldParentH.children.begin(), oldParentH.children.end(), childHandle),
                                  oldParentH.children.end());
    }

    childH.parent = parentHandle;
    auto& parentH = registry.get<HierarchyComponent>(parentHandle);
    parentH.children.push_back(childHandle);

    if (auto* w = registry.try_get<WorldTransformComponent>(childHandle))
        w->isDirty = true;
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
        if (rb->body)
        {
            IPhysicsWorld* physicsWorld = manager ? manager->GetPhysicsWorld()
                                                  : ServiceLocator::Instance().Resolve<IPhysicsWorld>();
            if (physicsWorld)
            {
                for (auto& constraint : rb->constraints)
                {
                    if (constraint)
                        physicsWorld->RemoveConstraint(constraint);
                }
                rb->constraints.clear();
                physicsWorld->RemoveRigidBody(rb->body.get());
            }

            rb->body = nullptr;
        }
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

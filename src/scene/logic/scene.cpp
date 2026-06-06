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
}

void Scene::ShutdownManagers()
{
    m_Octree.reset();
}

void Scene::Destroy(entt::entity entity)
{
    if (entity != entt::null && registry.valid(entity))
    {
        DestroyEntity(entity);
    }
}

entt::entity Scene::FindByName(const std::string& name)
{
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            return entity;
    }
    return entt::null;
}

entt::entity Scene::FindByTag(const std::string& tag)
{
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).tag == tag)
            return entity;
    }
    return entt::null;
}

entt::entity Scene::FindByNameAndTag(const std::string& name, const std::string& tag)
{
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        const auto& info = view.get<InfoComponent>(entity);
        if (info.name == name && info.tag == tag)
            return entity;
    }
    return entt::null;
}

entt::entity Scene::FindByNameTagAndScene(const std::string& name, const std::string& tag,
                                         const std::string& sceneName)
{
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        const auto& info = view.get<InfoComponent>(entity);
        if (info.name == name && info.tag == tag && info.sceneName == sceneName)
            return entity;
    }
    return entt::null;
}

std::vector<entt::entity> Scene::FindAllByName(const std::string& name)
{
    std::vector<entt::entity> results;
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            results.push_back(entity);
    }
    return results;
}

std::vector<entt::entity> Scene::FindAllByTag(const std::string& tag)
{
    std::vector<entt::entity> results;
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).tag == tag)
            results.push_back(entity);
    }
    return results;
}

std::vector<entt::entity> Scene::FindAllBySceneName(const std::string& sceneName)
{
    std::vector<entt::entity> results;
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).sceneName == sceneName)
            results.push_back(entity);
    }
    return results;
}

entt::entity Scene::GetCameraByName(const std::string& name)
{
    auto view = registry.view<CameraComponent, InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            return entity;
    }
    return entt::null;
}

entt::entity Scene::GetCameraByTag(const std::string& tag)
{
    auto view = registry.view<CameraComponent, InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).tag == tag)
            return entity;
    }
    return entt::null;
}

std::vector<entt::entity> Scene::GetAllCameras()
{
    std::vector<entt::entity> cameras;
    auto view = registry.view<CameraComponent>();
    for (auto entity : view) cameras.push_back(entity);
    return cameras;
}

entt::entity Scene::GetActiveCamera()
{
    auto view = registry.view<CameraComponent>();
    for (auto entity : view)
    {
        if (view.get<CameraComponent>(entity).isPrimary)
        {
            return entity;
        }
    }
    for (auto entity : view)
    {
        return entity;
    }
    return entt::null;
}

void Scene::SetActiveCamera(entt::entity entity)
{
    auto view = registry.view<CameraComponent>();
    for (auto camEntity : view)
    {
        view.get<CameraComponent>(camEntity).isPrimary = (camEntity == entity);
    }
}

entt::entity Scene::GetActiveSkybox()
{
    auto view = registry.view<SkyboxRenderComponent>();
    for (auto entity : view)
    {
        if (view.get<SkyboxRenderComponent>(entity).isPrimary)
        {
            return entity;
        }
    }
    for (auto entity : view)
    {
        return entity;
    }
    return entt::null;
}

void Scene::SetActiveSkybox(entt::entity entity)
{
    auto view = registry.view<SkyboxRenderComponent>();
    for (auto skyEntity : view)
    {
        view.get<SkyboxRenderComponent>(skyEntity).isPrimary = (skyEntity == entity);
    }
}

entt::entity Scene::CreateEntity(const std::string& name, const std::string& tag)
{
    entt::entity entity = registry.create();
    registry.emplace<PositionComponent>(entity);
    registry.emplace<RotationComponent>(entity);
    registry.emplace<ScaleComponent>(entity);
    registry.emplace<HierarchyComponent>(entity);
    registry.emplace<WorldTransformComponent>(entity);
    registry.emplace<InfoComponent>(entity, name, tag);
    return entity;
}

entt::entity Scene::CreateEntityWithTransform(const std::string& name, const glm::vec3& position,
                                              const glm::vec3& rotation, const glm::vec3& scale)
{
    entt::entity entity = CreateEntity(name);

    if (auto* p = registry.try_get<PositionComponent>(entity))
        p->value = p->prev = position;
    if (auto* r = registry.try_get<RotationComponent>(entity))
        r->value = r->prev = glm::quat(glm::radians(rotation));
    if (auto* s = registry.try_get<ScaleComponent>(entity))
        s->value = s->prev = scale;

    return entity;
}

entt::entity Scene::CreateEmptyEntity(const std::string& name)
{
    return CreateEntityWithTransform(name, glm::vec3(0.0f));
}

entt::entity Scene::CreateCube(const std::string& name, const glm::vec3& position)
{
    return CreateEntityWithTransform(name, position);
}

entt::entity Scene::CreateSphere(const std::string& name, const glm::vec3& position)
{
    return CreateEntityWithTransform(name, position);
}

entt::entity Scene::CreatePlane(const std::string& name, const glm::vec3& position)
{
    return CreateEntityWithTransform(name, position);
}

void Scene::SetParent(entt::entity child, entt::entity parent, bool keepWorldTransform)
{
    if (!registry.valid(child) || !registry.valid(parent))
    {
        LOGGER_WARN("Scene") << "Invalid child or parent entity";
        return;
    }

    if (!registry.all_of<HierarchyComponent>(child) || !registry.all_of<HierarchyComponent>(parent))
    {
        LOGGER_WARN("Scene") << "Child or parent missing HierarchyComponent";
        return;
    }

    auto& childH = registry.get<HierarchyComponent>(child);
    if (child == parent)
    {
        LOGGER_WARN("Scene") << "Attempted to set entity as its own parent: " << (uint32_t)child;
        return;
    }

    if (childH.parent == parent)
        return;

    entt::entity current = parent;
    while (current != entt::null)
    {
        if (current == child)
        {
            LOGGER_WARN("Scene") << "Cycle detected in hierarchy! Cannot set " << (uint32_t)child << " as child of "
                                 << (uint32_t)parent;
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
        oldParentH.children.erase(std::remove(oldParentH.children.begin(), oldParentH.children.end(), child),
                                  oldParentH.children.end());
    }

    childH.parent = parent;
    auto& parentH = registry.get<HierarchyComponent>(parent);
    parentH.children.push_back(child);

    if (auto* w = registry.try_get<WorldTransformComponent>(child))
        w->isDirty = true;
}

void Scene::AddChild(entt::entity parent, entt::entity child, bool keepWorldTransform)
{
    SetParent(child, parent, keepWorldTransform);
}

void Scene::DestroyEntity(entt::entity entity, SceneManager* manager)
{
    if (!registry.valid(entity))
        return;

    if (auto* h = registry.try_get<HierarchyComponent>(entity))
    {
        if (registry.valid(h->parent) && registry.all_of<HierarchyComponent>(h->parent))
        {
            auto& parentH = registry.get<HierarchyComponent>(h->parent);
            parentH.children.erase(std::remove(parentH.children.begin(), parentH.children.end(), entity),
                                   parentH.children.end());
        }

        std::vector<entt::entity> childrenCopy = h->children;
        for (auto child : childrenCopy)
        {
            if (registry.valid(child) && registry.all_of<HierarchyComponent>(child))
            {
                auto& childH = registry.get<HierarchyComponent>(child);
                childH.parent = entt::null;
            }
        }
    }

    if (auto sc = registry.try_get<ScriptComponent>(entity))
    {
        if (sc->instance && sc->DestroyScript)
            sc->DestroyScript(sc);
        sc->instance.reset();
        sc->scriptableInstance = nullptr;
        sc->inputScriptableInstance = nullptr;
    }

    if (auto rb = registry.try_get<RigidBodyComponent>(entity))
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

    if (auto mesh = registry.try_get<MeshRendererComponent>(entity))
    {
        mesh->model = nullptr;
        mesh->shader.reset();
    }

    if (auto anim = registry.try_get<AnimationComponent>(entity))
    {
        anim->animator = nullptr;
    }

    if (auto ui = registry.try_get<UIRendererComponent>(entity))
    {
        ui->model = nullptr;
        ui->shader = nullptr;
    }

    if (auto text = registry.try_get<UITextComponent>(entity))
    {
        text->model = nullptr;
        text->shader = nullptr;
        text->font = nullptr;
    }

    if (auto sky = registry.try_get<SkyboxRenderComponent>(entity))
    {
        sky->skybox = nullptr;
        sky->shader.reset();
    }

    if (manager)
        manager->RemoveEntity(entity);

    registry.destroy(entity);
}

void Scene::DestroyEntityWithChildren(entt::entity entity, SceneManager* manager)
{
    if (!registry.valid(entity))
        return;

    std::vector<entt::entity> children;
    if (registry.all_of<HierarchyComponent>(entity))
    {
        const auto& hier = registry.get<HierarchyComponent>(entity);
        children = hier.children;
    }

    for (auto child : children)
    {
        DestroyEntityWithChildren(child, manager);
    }

    DestroyEntity(entity, manager);
}

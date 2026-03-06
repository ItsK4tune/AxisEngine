#include <ecs/components/info_component.h>
#include <ecs/components/render_components.h>
#include <ecs/entity_manager.h>
#include <core/scripting/scriptable.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <systems/physics/interfaces/i_physics_world.h>
#include <scene/scene_manager.h>
#include <core/utils/logger.h>

void EntityManager::Destroy(Scene& scene, entt::entity entity)
{
    if (entity != entt::null && scene.registry.valid(entity))
    {
        DestroyEntity(scene, entity);
    }
}

entt::entity EntityManager::FindByName(Scene& scene, const std::string& name)
{
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            return entity;
    }
    return entt::null;
}

entt::entity EntityManager::FindByTag(Scene& scene, const std::string& tag)
{
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).tag == tag)
            return entity;
    }
    return entt::null;
}

entt::entity EntityManager::FindByNameAndTag(Scene& scene, const std::string& name, const std::string& tag)
{
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        const auto& info = view.get<InfoComponent>(entity);
        if (info.name == name && info.tag == tag)
            return entity;
    }
    return entt::null;
}

entt::entity EntityManager::FindByNameTagAndScene(Scene& scene, const std::string& name, const std::string& tag, const std::string& sceneName)
{
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        const auto& info = view.get<InfoComponent>(entity);
        if (info.name == name && info.tag == tag && info.sceneName == sceneName)
            return entity;
    }
    return entt::null;
}

std::vector<entt::entity> EntityManager::FindAllByName(Scene& scene, const std::string& name)
{
    std::vector<entt::entity> results;
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            results.push_back(entity);
    }
    return results;
}

std::vector<entt::entity> EntityManager::FindAllByTag(Scene& scene, const std::string& tag)
{
    std::vector<entt::entity> results;
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).tag == tag)
            results.push_back(entity);
    }
    return results;
}

std::vector<entt::entity> EntityManager::FindAllBySceneName(Scene& scene, const std::string& sceneName)
{
    std::vector<entt::entity> results;
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).sceneName == sceneName)
            results.push_back(entity);
    }
    return results;
}

entt::entity EntityManager::GetCameraByName(Scene& scene, const std::string& name)
{
    auto view = scene.registry.view<CameraComponent, InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            return entity;
    }
    return entt::null;
}

entt::entity EntityManager::GetCameraByTag(Scene& scene, const std::string& tag)
{
    auto view = scene.registry.view<CameraComponent, InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).tag == tag)
            return entity;
    }
    return entt::null;
}

std::vector<entt::entity> EntityManager::GetAllCameras(Scene& scene)
{
    std::vector<entt::entity> cameras;
    auto view = scene.registry.view<CameraComponent>();
    for (auto entity : view)
        cameras.push_back(entity);
    return cameras;
}

entt::entity EntityManager::GetActiveCamera(Scene& scene)
{
    auto view = scene.registry.view<CameraComponent>();
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

void EntityManager::SetActiveCamera(Scene& scene, entt::entity entity)
{
    auto view = scene.registry.view<CameraComponent>();
    for (auto camEntity : view)
    {
        view.get<CameraComponent>(camEntity).isPrimary = (camEntity == entity);
    }
}

entt::entity EntityManager::GetActiveSkybox(Scene& scene)
{
    auto view = scene.registry.view<SkyboxRenderComponent>();
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

void EntityManager::SetActiveSkybox(Scene& scene, entt::entity entity)
{
    auto view = scene.registry.view<SkyboxRenderComponent>();
    for (auto skyEntity : view)
    {
        view.get<SkyboxRenderComponent>(skyEntity).isPrimary = (skyEntity == entity);
    }
}

entt::entity EntityManager::CreateEntity(Scene& scene, const std::string &name, const std::string &tag)
{
    entt::entity entity = scene.registry.create();
    scene.registry.emplace<PositionComponent>(entity);
    scene.registry.emplace<RotationComponent>(entity);
    scene.registry.emplace<ScaleComponent>(entity);
    scene.registry.emplace<HierarchyComponent>(entity);
    scene.registry.emplace<WorldTransformComponent>(entity);

    scene.registry.emplace<TransformComponent>(entity);
    scene.registry.emplace<InfoComponent>(entity, name, tag);
    LOGGER_INFO("Scene") << "Created entity: " << name << " (Tag: " << tag << ")";
    return entity;
}

entt::entity EntityManager::CreateEntityWithTransform(Scene& scene, const std::string &name, const glm::vec3 &position, const glm::vec3 &rotation, const glm::vec3 &scale)
{
    entt::entity entity = CreateEntity(scene, name);

    if (auto* p = scene.registry.try_get<PositionComponent>(entity)) p->value = p->prev = position;
    if (auto* r = scene.registry.try_get<RotationComponent>(entity)) r->value = r->prev = glm::quat(glm::radians(rotation));
    if (auto* s = scene.registry.try_get<ScaleComponent>(entity)) s->value = s->prev = scale;

    auto &transform = scene.registry.get<TransformComponent>(entity);
    transform.position = position;
    transform.rotation = glm::quat(glm::radians(rotation));
    transform.scale = scale;

    return entity;
}

entt::entity EntityManager::CreateEmptyEntity(Scene& scene, const std::string &name)
{
    return CreateEntityWithTransform(scene, name, glm::vec3(0.0f));
}

entt::entity EntityManager::CreateCube(Scene& scene, const std::string &name, const glm::vec3 &position)
{
    return CreateEntityWithTransform(scene, name, position);
}

entt::entity EntityManager::CreateSphere(Scene& scene, const std::string &name, const glm::vec3 &position)
{
    return CreateEntityWithTransform(scene, name, position);
}

entt::entity EntityManager::CreatePlane(Scene& scene, const std::string &name, const glm::vec3 &position)
{
    return CreateEntityWithTransform(scene, name, position);
}

void EntityManager::SetParent(Scene& scene, entt::entity child, entt::entity parent, bool keepWorldTransform)
{
    if (!scene.registry.valid(child) || !scene.registry.valid(parent))
    {
        LOGGER_ERROR("Scene") << "Invalid child or parent entity";
        return;
    }

    if (!scene.registry.all_of<HierarchyComponent>(child) || !scene.registry.all_of<HierarchyComponent>(parent))
    {
        LOGGER_ERROR("Scene") << "Child or parent missing HierarchyComponent";
        return;
    }

    auto &childH = scene.registry.get<HierarchyComponent>(child);
    if (childH.parent == parent) return;

    // Remove from old parent
    if (scene.registry.valid(childH.parent) && scene.registry.all_of<HierarchyComponent>(childH.parent))
    {
        auto &oldParentH = scene.registry.get<HierarchyComponent>(childH.parent);
        oldParentH.children.erase(std::remove(oldParentH.children.begin(), oldParentH.children.end(), child), oldParentH.children.end());
    }

    childH.parent = parent;
    auto& parentH = scene.registry.get<HierarchyComponent>(parent);
    parentH.children.push_back(child);

    if (auto* w = scene.registry.try_get<WorldTransformComponent>(child)) w->isDirty = true;
    
    // Support legacy
    if (scene.registry.all_of<TransformComponent>(child) && scene.registry.all_of<TransformComponent>(parent))
    {
        auto &childTransform = scene.registry.get<TransformComponent>(child);
        childTransform.SetParent(child, parent, scene.registry, keepWorldTransform);
    }
}

void EntityManager::AddChild(Scene& scene, entt::entity parent, entt::entity child, bool keepWorldTransform)
{
    SetParent(scene, child, parent, keepWorldTransform);
}

void EntityManager::DestroyEntity(Scene& scene, entt::entity entity, SceneManager *manager)
{
    if (!scene.registry.valid(entity))
        return;

    if (auto *h = scene.registry.try_get<HierarchyComponent>(entity))
    {
        if (scene.registry.valid(h->parent) && scene.registry.all_of<HierarchyComponent>(h->parent))
        {
            auto &parentH = scene.registry.get<HierarchyComponent>(h->parent);
            parentH.children.erase(std::remove(parentH.children.begin(), parentH.children.end(), entity), parentH.children.end());
        }

        std::vector<entt::entity> childrenCopy = h->children;
        for (auto child : childrenCopy)
        {
            if (scene.registry.valid(child) && scene.registry.all_of<HierarchyComponent>(child))
            {
                auto &childH = scene.registry.get<HierarchyComponent>(child);
                childH.parent = entt::null;
            }
        }
    }

    if (auto sc = scene.registry.try_get<ScriptComponent>(entity))
    {
        if (sc->instance && sc->DestroyScript)
            sc->DestroyScript(sc);
        sc->instance.reset();
    }

    if (auto rb = scene.registry.try_get<RigidBodyComponent>(entity))
    {
        if (rb->body)
        {
            if (manager && manager->GetPhysicsWorld())
                manager->GetPhysicsWorld()->RemoveRigidBody(rb->body.get());

            rb->body = nullptr;
        }
    }

    if (auto mesh = scene.registry.try_get<MeshRendererComponent>(entity))
    {
        mesh->model = nullptr;
        mesh->shader.reset();
    }

    if (auto anim = scene.registry.try_get<AnimationComponent>(entity))
    {
        anim->animator = nullptr;
    }

    if (auto ui = scene.registry.try_get<UIRendererComponent>(entity))
    {
        ui->model = nullptr;
        ui->shader = nullptr;
    }

    if (auto text = scene.registry.try_get<UITextComponent>(entity))
    {
        text->model = nullptr;
        text->shader = nullptr;
        text->font = nullptr;
    }

    if (auto sky = scene.registry.try_get<SkyboxRenderComponent>(entity))
    {
        sky->skybox = nullptr;
        sky->shader.reset();
    }

    scene.registry.destroy(entity);
}

void EntityManager::DestroyEntityWithChildren(Scene& scene, entt::entity entity, SceneManager *manager)
{
    if (!scene.registry.valid(entity))
        return;

    std::vector<entt::entity> children;
    if (scene.registry.all_of<TransformComponent>(entity))
    {
        const auto &transform = scene.registry.get<TransformComponent>(entity);
        children = transform.children;
    }

    for (auto child : children)
    {
        DestroyEntityWithChildren(scene, child, manager);
    }

    DestroyEntity(scene, entity, manager);
}

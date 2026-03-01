#include <scene/scene.h>
#include <interface/physics/i_physics_world.h>
#include <scene/scene_manager.h>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <algorithm>
#include <utils/logger.h>

Scene::Scene()
{
    registry.on_destroy<ScriptComponent>().connect<&Scene::OnScriptComponentDestroyed>(this);
}

Scene::~Scene()
{
    registry.on_destroy<ScriptComponent>().disconnect<&Scene::OnScriptComponentDestroyed>(this);
}

void Scene::OnScriptComponentDestroyed(entt::registry &reg, entt::entity entity)
{
    if (auto sc = reg.try_get<ScriptComponent>(entity))
    {
        if (sc->instance)
        {
            try {
                if (sc->DestroyScript)
                    sc->DestroyScript(sc);
                sc->instance = nullptr;
            } catch (...) {
                LOGGER_ERROR("Scene") << "OnScriptComponentDestroyed: CRASH during script cleanup for entity " << (uint32_t)entity;
            }
        }
    }
}

entt::entity Scene::CreateEntity(const std::string &name, const std::string &tag)
{
    entt::entity entity = registry.create();
    registry.emplace<TransformComponent>(entity);
    registry.emplace<InfoComponent>(entity, name, tag);
    LOGGER_INFO("Scene") << "Created entity: " << name << " (Tag: " << tag << ")";
    return entity;
}

entt::entity Scene::CreateEntityWithTransform(const std::string &name, const glm::vec3 &position, const glm::vec3 &rotation, const glm::vec3 &scale)
{
    entt::entity entity = CreateEntity(name);

    auto &transform = registry.get<TransformComponent>(entity);
    transform.position = position;
    transform.rotation = glm::quat(rotation);
    transform.scale = scale;

    return entity;
}

entt::entity Scene::CreateEmptyEntity(const std::string &name)
{
    return CreateEntityWithTransform(name, glm::vec3(0.0f));
}

entt::entity Scene::CreateCube(const std::string &name, const glm::vec3 &position)
{
    return CreateEntityWithTransform(name, position);
}

entt::entity Scene::CreateSphere(const std::string &name, const glm::vec3 &position)
{
    return CreateEntityWithTransform(name, position);
}

entt::entity Scene::CreatePlane(const std::string &name, const glm::vec3 &position)
{
    return CreateEntityWithTransform(name, position);
}

void Scene::SetParent(entt::entity child, entt::entity parent, bool keepWorldTransform)
{
    if (!registry.valid(child) || !registry.valid(parent))
    {
        LOGGER_ERROR("Scene") << "Invalid child or parent entity";
        return;
    }

    if (!registry.all_of<TransformComponent>(child) || !registry.all_of<TransformComponent>(parent))
    {
        LOGGER_ERROR("Scene") << "Child or parent missing TransformComponent";
        return;
    }

    auto &childTransform = registry.get<TransformComponent>(child);
    childTransform.SetParent(child, parent, registry, keepWorldTransform);
}

void Scene::AddChild(entt::entity parent, entt::entity child, bool keepWorldTransform)
{
    SetParent(child, parent, keepWorldTransform);
}

void Scene::DestroyEntity(entt::entity entity, SceneManager *manager)
{
    if (!registry.valid(entity))
        return;

    if (auto *transform = registry.try_get<TransformComponent>(entity))
    {
        if (registry.valid(transform->parent) && registry.all_of<TransformComponent>(transform->parent))
        {
            auto &parentTrans = registry.get<TransformComponent>(transform->parent);
            parentTrans.RemoveChild(entity);
        }

        std::vector<entt::entity> childrenCopy = transform->children;
        for (auto child : childrenCopy)
        {
            if (registry.valid(child) && registry.all_of<TransformComponent>(child))
            {
                auto &childTrans = registry.get<TransformComponent>(child);
                childTrans.parent = entt::null;
            }
        }
    }

    if (auto sc = registry.try_get<ScriptComponent>(entity))
    {
        if (sc->instance && sc->DestroyScript)
            sc->DestroyScript(sc);
        sc->instance = nullptr;
    }

    if (auto rb = registry.try_get<RigidBodyComponent>(entity))
    {
        if (rb->body)
        {
            if (manager)
                manager->GetPhysicsWorld().RemoveRigidBody(rb->body.get());

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

    registry.destroy(entity);
}

entt::entity Scene::GetActiveCamera()
{
    if (registry.valid(m_ActiveCamera) && registry.all_of<CameraComponent>(m_ActiveCamera))
    {
        if (registry.get<CameraComponent>(m_ActiveCamera).isPrimary)
        {
            return m_ActiveCamera;
        }
    }

    auto view = registry.view<const CameraComponent>();
    for (auto entity : view)
    {
        const auto &cam = view.get<const CameraComponent>(entity);
        if (cam.isPrimary)
        {
            m_ActiveCamera = entity;
            return entity;
        }
    }

    m_ActiveCamera = entt::null;
    return entt::null;
}

void Scene::DestroyEntityWithChildren(entt::entity entity, SceneManager *manager)
{
    if (!registry.valid(entity))
        return;

    std::vector<entt::entity> children;
    if (registry.all_of<TransformComponent>(entity))
    {
        const auto &transform = registry.get<TransformComponent>(entity);
        children = transform.children;
    }

    for (auto child : children)
    {
        DestroyEntityWithChildren(child, manager);
    }

    DestroyEntity(entity, manager);
}

void Scene::SetActiveCamera(entt::entity entity)
{
    m_ActiveCamera = entity;
}

entt::entity Scene::GetActiveSkybox() const
{
    if (registry.valid(m_ActiveSkybox) && registry.all_of<SkyboxRenderComponent>(m_ActiveSkybox))
        return m_ActiveSkybox;
    return entt::null;
}

void Scene::SetActiveSkybox(entt::entity entity)
{
    if (registry.valid(entity) && registry.all_of<SkyboxRenderComponent>(entity))
    {
        m_ActiveSkybox = entity;
    }
}

void Scene::InitializeManagers()
{
    m_Octree = std::make_unique<Octree>(AABB(glm::vec3(-1000.0f), glm::vec3(1000.0f)));
}

void Scene::ShutdownManagers()
{
    m_Octree.reset();
}

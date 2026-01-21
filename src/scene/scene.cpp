#include <scene/scene.h>
#include <scene/scene_manager.h>
#include <scene/light_manager.h>
#include <scene/camera_manager.h>
#include <scene/entity_factory.h>
#include <vector>
#include <algorithm>

entt::entity Scene::createEntity()
{
    entt::entity entity = registry.create();
    registry.emplace<TransformComponent>(entity);
    return entity;
}

void Scene::destroyEntity(entt::entity entity, SceneManager *manager)
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
                manager->GetPhysicsWorld().GetWorld()->removeRigidBody(rb->body);
            delete rb->body;
            rb->body = nullptr;
        }
    }

    if (auto mesh = registry.try_get<MeshRendererComponent>(entity))
    {
        mesh->model = nullptr;
        mesh->shader = nullptr;
    }

    if (auto anim = registry.try_get<AnimationComponent>(entity))
    {
        if (anim->animator)
        {
            delete anim->animator;
            anim->animator = nullptr;
        }
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

    if (auto interactive = registry.try_get<UIInteractiveComponent>(entity))
    {
        interactive->onClick = nullptr;
        interactive->onHoverEnter = nullptr;
        interactive->onHoverExit = nullptr;
    }

    if (auto sky = registry.try_get<SkyboxRenderComponent>(entity))
    {
        sky->skybox = nullptr;
        sky->shader = nullptr;
    }

    registry.destroy(entity);
}

entt::entity Scene::GetActiveCamera()
{
    auto view = registry.view<const CameraComponent>();
    for (auto entity : view)
    {
        const auto &cam = view.get<const CameraComponent>(entity);
        if (cam.isPrimary)
        {
            return entity;
        }
    }
    return entt::null;
}

void Scene::InitializeManagers()
{
    lightManager = std::make_unique<LightManager>(*this);
    cameraManager = std::make_unique<CameraManager>(*this);
    entityFactory = std::make_unique<EntityFactory>(*this);
}

void Scene::ShutdownManagers()
{
    entityFactory.reset();
    cameraManager.reset();
    lightManager.reset();
}

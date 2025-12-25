#include <engine/core/scene.h>
#include <engine/core/scene_manager.h>

entt::entity Scene::createEntity()
{
    return registry.create();
}

void Scene::destroyEntity(entt::entity entity, SceneManager* manager)
{
    if (!registry.valid(entity))
        return;

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
                manager->m_Physics.GetWorld()->removeRigidBody(rb->body);
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
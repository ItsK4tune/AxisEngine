#include <ecs/system.h>

void SkyboxRenderSystem::Render(Scene &scene)
{
    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity == entt::null)
        return;

    auto &camera = scene.registry.get<CameraComponent>(camEntity);

    glDepthFunc(GL_LEQUAL);

    auto view = scene.registry.view<SkyboxRenderComponent>();
    for (auto entity : view)
    {
        auto &component = view.get<SkyboxRenderComponent>(entity);
        if (!component.skybox || !component.shader)
            continue;

        component.shader->use();

        glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(camera.viewMatrix));
        component.shader->setMat4("view", viewNoTranslation);
        component.shader->setMat4("projection", camera.projectionMatrix);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, component.skybox->GetTextureID());
        component.shader->setInt("skybox", 0);

        component.skybox->Draw(*component.shader);
    }

    glDepthFunc(GL_LESS);
}

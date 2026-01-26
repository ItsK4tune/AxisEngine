#include <ecs/system.h>

void SkyboxRenderSystem::Render(Scene &scene)
{
    if (!m_Enabled) return;

    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity == entt::null)
        return;

    auto &camera = scene.registry.get<CameraComponent>(camEntity);

    glDepthFunc(GL_LEQUAL);

    auto activeSkybox = scene.GetActiveSkybox();
    if (activeSkybox != entt::null)
    {
        auto &component = scene.registry.get<SkyboxRenderComponent>(activeSkybox);
        if (component.skybox && component.shader)
        {
            component.shader->use();

            glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(camera.viewMatrix));
            component.shader->setMat4("view", viewNoTranslation);
            component.shader->setMat4("projection", camera.projectionMatrix);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, component.skybox->GetTextureID());
            component.shader->setInt("skybox", 0);

            component.skybox->Draw(*component.shader);
        }
    }

    glDepthFunc(GL_LESS);
}

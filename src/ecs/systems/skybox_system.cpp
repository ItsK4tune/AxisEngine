#include <ecs/systems/skybox_system.h>
#include <interface/graphic/i_graphics_context.h>
#include <interface/graphic/i_render_state_manager.h>
#include <interface/graphic/i_texture_manager.h>
#include <graphic/core/shader.h>
#include <graphic/renderer/skybox.h>

void SkyboxRenderSystem::Init(IGraphicsContext& context)
{
    m_Context = &context;
}

void SkyboxRenderSystem::Render(Scene &scene)
{
    if (!m_Enabled || !m_Context) return;

    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity == entt::null)
        return;

    auto &camera = scene.registry.get<CameraComponent>(camEntity);

    auto& rsm = m_Context->GetRenderStateManager();
    auto& tm = m_Context->GetTextureManager();

    rsm.DepthFunc(Graphics::CompareFunc::Lequal);

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

            tm.ActiveTexture(Graphics::TextureUnit::Texture0);
            tm.BindTexture(Graphics::TextureType::TextureCubeMap, component.skybox->GetTextureID());
            component.shader->setInt("skybox", 0);

            component.skybox->Draw(*component.shader);
        }
    }

    rsm.DepthFunc(Graphics::CompareFunc::Less);
}

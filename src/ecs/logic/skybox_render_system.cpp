#include <ecs/unit/core_components.h>
#include <ecs/logic/entity_manager.h>
#include <ecs/logic/skybox_render_system.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/render_components.h>
#include <resource/unit/shader.h>
#include <render/unit/skybox.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_texture_manager.h>
#include <core/logic/logger.h>
#include <core/type/app_config.h>

#include <platform/logic/io_handler.h>
#include <ecs/interface/i_render_service.h>
#include <render/interface/i_render_target_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>

REGISTER_SYSTEM(SkyboxRenderSystem)

void SkyboxRenderSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<ISkyboxService>(this);
    sl.Register<SkyboxRenderSystem>(this);
    m_Context = &sl.Require<IGraphicsContext>();
    
    auto& configManager = sl.Require<ConfigManager>();
    m_Intensity = configManager.GetConfig().skyboxIntensity;

    m_ConfigSubId = EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (e.bitmask & ConfigChangedEvent::Graphics) {
            m_Intensity = ServiceLocator::Instance().Require<ConfigManager>().GetConfig().skyboxIntensity;
        }
    });

    m_RenderDataSubId = EventSystem::Instance().Subscribe<FrameRenderDataEvent>([this](const FrameRenderDataEvent& e) {
        m_LastFrameData.mainFBO = e.data.mainFBO;
    });

    m_RenderService = sl.Resolve<IRenderService>();
}

std::vector<entt::id_type> SkyboxRenderSystem::GetReadComponents() const
{
    return {entt::type_id<SkyboxRenderComponent>().hash()};
}

std::vector<entt::id_type> SkyboxRenderSystem::GetWriteComponents() const
{
    return {};
}

void SkyboxRenderSystem::RenderAlphaPass(Scene &scene, int width, int height, float alpha)
{
    if (!m_Enabled || !m_Context) return;

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) return;

    auto &camera = scene.registry.get<CameraComponent>(camEntity);
    RenderAlphaPassWithCamera(scene, camera.viewMatrix, camera.projectionMatrix, width, height, m_LastFrameData.mainFBO);
}

void SkyboxRenderSystem::RenderAlphaPassWithCamera(Scene &scene, const glm::mat4& view, const glm::mat4& proj, int width, int height, uint32_t targetFBO)
{
    if (!m_Enabled || !m_Context) return;

    auto& rsm = m_Context->GetRenderStateManager();
    auto& tm = m_Context->GetTextureManager();
    auto& rtm = m_Context->GetRenderTargetManager();
    
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, targetFBO);

    rsm.SetViewport(0, 0, width, height);
    rsm.SetDepthFunc(CompareFunc::Lequal);

    auto activeSkybox = EntityManager::GetActiveSkybox(scene);
    if (activeSkybox != entt::null)
    {
        auto &component = scene.registry.get<SkyboxRenderComponent>(activeSkybox);
        auto lockedShader = component.shader.lock();
        if (component.skybox && lockedShader)
        {
            lockedShader->use();
            
            // Set matrices explicitly for skybox
            lockedShader->setMat4("view", glm::mat4(glm::mat3(view))); // Remove translation
            lockedShader->setMat4("projection", proj);

            tm.ActiveTexture(TextureUnit::Texture0);
            tm.BindTexture(TextureType::TextureCubeMap, component.skybox->GetTextureID());
            lockedShader->setInt("skybox", 0);
            lockedShader->setFloat("intensity", m_Intensity);

            component.skybox->Draw(*lockedShader);
        }
    }

    rsm.SetDepthFunc(CompareFunc::Less);
}

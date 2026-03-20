#include <ecs/unit/core_components.h>
#include <ecs/logic/entity_manager.h>
#include <ecs/logic/skybox_render_system.h>
#include <ecs/unit/render_components.h>
#include <resource/unit/shader.h>
#include <render/unit/skybox.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_texture_manager.h>
#include <core/logic/logger.h>
#include <core/type/app_config.h>

#include <platform/logic/io_handler.h>
#include <core/logic/service_locator.h>
#include <core/app/runtime_core.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>

void SkyboxRenderSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    m_Context = &sl.Require<IGraphicsContext>();
    
    auto& configManager = sl.Require<ConfigManager>();
    m_Intensity = configManager.GetConfig().skyboxIntensity;

    m_ConfigSubId = EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (e.bitmask & ConfigChangedEvent::Graphics) {
            m_Intensity = ServiceLocator::Instance().Require<ConfigManager>().GetConfig().skyboxIntensity;
        }
    });
}

std::vector<entt::id_type> SkyboxRenderSystem::GetReadComponents() const
{
    return {entt::type_id<SkyboxRenderComponent>().hash()};
}

std::vector<entt::id_type> SkyboxRenderSystem::GetWriteComponents() const
{
    return {};
}

void SkyboxRenderSystem::Render(Scene &scene)
{
    if (!m_Enabled || !m_Context) return;

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null)
        return;

    auto &camera = scene.registry.get<CameraComponent>(camEntity);

    auto& rsm = m_Context->GetRenderStateManager();
    auto& tm = m_Context->GetTextureManager();

    rsm.SetDepthFunc(CompareFunc::Lequal);

    auto activeSkybox = EntityManager::GetActiveSkybox(scene);
    if (activeSkybox != entt::null)
    {
        auto &component = scene.registry.get<SkyboxRenderComponent>(activeSkybox);
        auto lockedShader = component.shader.lock();
        if (component.skybox && lockedShader)
        {
            lockedShader->use();

            tm.ActiveTexture(TextureUnit::Texture0);
            tm.BindTexture(TextureType::TextureCubeMap, component.skybox->GetTextureID());
            lockedShader->setInt("skybox", 0);
            lockedShader->setFloat("intensity", m_Intensity);

            component.skybox->Draw(*lockedShader);
        }
    }

    rsm.SetDepthFunc(CompareFunc::Less);
}

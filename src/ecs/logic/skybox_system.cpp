#include <ecs/unit/core_components.h>
#include <ecs/manager/entity_manager.h>
#include <ecs/logic/skybox_system.h>
#include <ecs/unit/render_components.h>
#include <render/logic/shader.h>
#include <render/logic/skybox.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_texture_manager.h>
#include <core/logic/logger.h>
#include <core/type/app_config.h>

#include <platform/logic/io_handler.h>
#include <core/unit/engine_context.h>
#include <core/logic/engine_core.h>

void SkyboxRenderSystem::Initialize(EngineContext ctx)
{
    m_Ctx = ctx;
    m_Context = &ctx.io->GetGraphicsContext();
    m_Config = &ctx.runtime->GetConfig();
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
            lockedShader->setFloat("intensity", m_Config->skyboxIntensity);

            component.skybox->Draw(*lockedShader);
        }
    }

    rsm.SetDepthFunc(CompareFunc::Less);
}

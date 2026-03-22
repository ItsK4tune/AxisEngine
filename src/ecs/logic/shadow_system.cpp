#include <ecs/logic/shadow_system.h>
#include <ecs/interface/i_render_service.h>
#include <render/interface/i_graphics_context.h>
#include <resource/logic/resource_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <core/logic/config_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <render/logic/frustum_culler.h>
#include <render/unit/render_queue.h>

void ShadowSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& resources = sl.Require<ResourceManager>();
    auto& config = sl.Require<ConfigManager>().GetConfig();

    m_ShadowRenderer.Initialize(context, resources);
    m_ShadowRenderer.GetShadow().Initialize(context, config.shadowMapResolution, config.shadowMapResolution);
    
    m_ShadowRenderer.SetEnableShadows(config.shadowsEnabled);
    m_ShadowRenderer.SetShadowMode(config.shadowMode);
    m_ShadowRenderer.SetShadowBias(config.shadowBias);
    m_ShadowRenderer.SetShadowSoftness(config.shadowSoftness);
    m_ShadowRenderer.SetShadowProjectionSize(config.shadowProjectionSize);
    m_ShadowRenderer.SetShadowFrustumCulling(config.shadowFrustumCullingEnabled);
    m_ShadowRenderer.SetShadowDistanceCulling(config.shadowDistanceCulling);

    EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (!(e.bitmask & (ConfigChangedEvent::Graphics | ConfigChangedEvent::All)))
            return;

        const auto& cfg = e.config;
        m_ShadowRenderer.SetEnableShadows(cfg.shadowsEnabled);
        m_ShadowRenderer.SetShadowMode(cfg.shadowMode);
        m_ShadowRenderer.SetShadowBias(cfg.shadowBias);
        m_ShadowRenderer.SetShadowSoftness(cfg.shadowSoftness);
        m_ShadowRenderer.SetShadowProjectionSize(cfg.shadowProjectionSize);
        m_ShadowRenderer.SetShadowFrustumCulling(cfg.shadowFrustumCullingEnabled);
        m_ShadowRenderer.SetShadowDistanceCulling(cfg.shadowDistanceCulling);

        if (cfg.shadowMapResolution != m_ShadowRenderer.GetShadow().GetShadowWidth()) {
            auto& sl_inner = ServiceLocator::Instance();
            m_ShadowRenderer.GetShadow().Initialize(sl_inner.Require<IGraphicsContext>(), cfg.shadowMapResolution, cfg.shadowMapResolution);
        }
    });
}

void ShadowSystem::Shutdown()
{
    m_ShadowRenderer.Shutdown();
}

void ShadowSystem::Render(Scene& scene)
{
    if (!m_Enabled || !m_ShadowRenderer.IsShadowsEnabled())
        return;

    auto& sl = ServiceLocator::Instance();
    auto* rs = sl.Resolve<IRenderService>();
    if (!rs) return;

    m_ShadowRenderer.RenderShadows(scene, rs->GetRenderQueueObj().GetShadowQueue());
}

std::vector<entt::id_type> ShadowSystem::GetReadComponents() const
{
    return {
        entt::type_id<MeshRendererComponent>().hash(),
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash(),
        entt::type_id<ScaleComponent>().hash(),
        entt::type_id<WorldTransformComponent>().hash(),
        entt::type_id<GPUDirLight>().hash(),
        entt::type_id<GPUPointLight>().hash(),
        entt::type_id<GPUSpotLight>().hash()
    };
}

std::vector<entt::id_type> ShadowSystem::GetWriteComponents() const
{
    return {};
}

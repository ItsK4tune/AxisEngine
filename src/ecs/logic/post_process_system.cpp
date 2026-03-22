#include <ecs/logic/post_process_system.h>
#include <render/interface/i_graphics_context.h>
#include <resource/logic/resource_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <core/logic/config_manager.h>
#include <ecs/interface/i_render_service.h>

void PostProcessSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& resources = sl.Require<ResourceManager>();
    auto& config = sl.Require<ConfigManager>().GetConfig();

    m_Pipeline.Initialize(context, config.width, config.height, resources);

    EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (!(e.bitmask & (ConfigChangedEvent::Window | ConfigChangedEvent::Graphics | ConfigChangedEvent::All)))
            return;

        const auto& cfg = e.config;
        m_Pipeline.SetBloomEnabled(cfg.bloomEnabled);
        m_Pipeline.SetBloomThreshold(cfg.bloomThreshold);
        m_Pipeline.SetBloomIntensity(cfg.bloomIntensity);
        m_Pipeline.SetBloomRadius(cfg.bloomRadius);
        m_Pipeline.SetExposure(cfg.exposure);
        m_Pipeline.SetGamma(cfg.gamma);
        m_Pipeline.SetTonemappingMode((int)cfg.tonemappingMode);

        if (cfg.width != m_Pipeline.GetWidth() || cfg.height != m_Pipeline.GetHeight()) {
             m_Pipeline.Resize(cfg.width, cfg.height);
        }
    });
}

void PostProcessSystem::Shutdown()
{
    m_Pipeline.Shutdown();
}

void PostProcessSystem::RenderAlpha(Scene& scene, int width, int height, float alpha)
{
    if (!m_Enabled) return;
    
    if (width != m_Pipeline.GetWidth() || height != m_Pipeline.GetHeight()) {
        m_Pipeline.Resize(width, height);
    }
}

void PostProcessSystem::Render(Scene& scene)
{
    if (!m_Enabled)
         return;

    auto& sl = ServiceLocator::Instance();
    auto* rs = sl.Resolve<IRenderService>();
    
    // Pass 3 is for "Final / Misc". We apply AA and then EndCapture.
    if (rs) {
        m_Pipeline.ApplyAntiAliasing(rs->GetAntiAliasingMode(), rs->GetPrevViewProj(), rs->GetCurrViewProj(), rs->GetJitterOffset());
    }
    
    m_Pipeline.EndCapture();
}

std::vector<entt::id_type> PostProcessSystem::GetReadComponents() const
{
    return {};
}

std::vector<entt::id_type> PostProcessSystem::GetWriteComponents() const
{
    return {};
}

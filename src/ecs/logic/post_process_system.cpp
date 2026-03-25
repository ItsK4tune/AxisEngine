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

    m_RenderService = sl.Resolve<IRenderService>();
}

void PostProcessSystem::Shutdown()
{
    m_Pipeline.Shutdown();
}

void PostProcessSystem::RenderCapturePass(Scene &scene, int width, int height)
{
    if (!m_Enabled) return;
    if (width != m_Pipeline.GetWidth() || height != m_Pipeline.GetHeight()) {
        m_Pipeline.Resize(width, height);
    }
    m_Pipeline.BeginCapture();
}

void PostProcessSystem::RenderAlphaPass(Scene &scene, int width, int height, float alpha)
{
}

void PostProcessSystem::Render(Scene& scene)
{
    if (!m_Enabled)
         return;

    if (m_RenderService) {
        m_Pipeline.ApplyAntiAliasing(m_RenderService->GetAntiAliasingMode(), m_RenderService->GetPrevViewProj(), m_RenderService->GetCurrViewProj(), m_RenderService->GetJitterOffset());
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

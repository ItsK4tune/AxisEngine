#include <ecs/logic/post_process_system.h>
#include <ecs/logic/system_factory.h>
#include <render/interface/i_graphics_context.h>
#include <resource/logic/resource_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_manager.h>
#include <core/type/event_types.h>
#include <core/logic/config_manager.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/unit/post_process_component.h>
#include <scene/logic/scene.h>

REGISTER_SYSTEM(PostProcessSystem)

void PostProcessSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<PostProcessSystem>(this);
    sl.Register<PostProcessPipeline>(&m_Pipeline);
    
    auto* context = sl.Resolve<IGraphicsContext>();
    auto* resources = sl.Resolve<ResourceManager>();
    auto* configManager = sl.Resolve<ConfigManager>();
    
    if (!context || !resources || !configManager) {
        LOGGER_WARN("PostProcessSystem") << "Skipping full initialization (missing Graphics, Resources, or Config)";
        return;
    }

    const auto& config = configManager->GetConfig();

    m_Pipeline.Initialize(*context, config.width, config.height, *resources);

    EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
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

    EventManager::Instance().Subscribe<WindowResizedEvent>([this](const WindowResizedEvent& e) {
        if (e.width != m_Pipeline.GetWidth() || e.height != m_Pipeline.GetHeight()) {
            m_Pipeline.Resize(e.width, e.height);
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
    
    // Lazy resolve RenderService if needed
    if (!m_RenderService) {
        m_RenderService = ServiceLocator::Instance().Resolve<IRenderService>();
    }

    if (width != m_Pipeline.GetWidth() || height != m_Pipeline.GetHeight()) {
        m_Pipeline.Resize(width, height);
    }
    m_Pipeline.BeginCapture();
    
    uint32_t captureFBO = m_Pipeline.GetCaptureFBO();
    if (m_RenderService) {
        m_RenderService->SetMainFBO(captureFBO);
    }
    
    static bool firstCaptureLog = true;
    if (firstCaptureLog && captureFBO != 0) {
        LOGGER_INFO("PostProcessSystem") << "BeginCapture initialized: FBO=" << captureFBO;
        firstCaptureLog = false;
    }

    FrameRenderData data;
    data.mainFBO = captureFBO;
    data.width = width;
    data.height = height;
    data.alpha = 1.0f; // Post process usually full alpha
    EventManager::Instance().Publish<FrameRenderDataEvent>({data});
}

void PostProcessSystem::RenderAlphaPass(Scene &scene, int width, int height, float alpha)
{
}

void PostProcessSystem::Render(Scene& scene)
{
    m_Pipeline.ClearEffects();
    if (!m_Enabled)
         return;

    // Lazy resolve RenderService if needed
    if (!m_RenderService) {
        m_RenderService = ServiceLocator::Instance().Resolve<IRenderService>();
    }

    if (m_RenderService) {
        m_Pipeline.ApplyAntiAliasing(m_RenderService->GetAntiAliasingMode(), m_RenderService->GetPrevViewProj(), m_RenderService->GetCurrViewProj(), m_RenderService->GetJitterOffset());
    }

    // Collect effects from components
    auto view = scene.registry.view<PostProcessComponent>();
    auto* res = ServiceLocator::Instance().Resolve<ResourceManager>();
    if (!res) return;
    
    for (auto entity : view) {
        auto& pp = view.get<PostProcessComponent>(entity);
        if (!pp.enabled) continue;
        for (const auto& eff : pp.effects) {
            auto shader = res->GetShader(eff.shaderName);
            if (shader) {
                m_Pipeline.AddEffect(shader, eff.x, eff.y, eff.w, eff.h, eff.priority);
            }
        }
    }
    
    m_Pipeline.EndCapture();

    if (m_RenderService) {
        m_RenderService->SetMainFBO(0);
    }

    FrameRenderData data;
    data.mainFBO = 0;
    data.width = m_Pipeline.GetWidth();
    data.height = m_Pipeline.GetHeight();
    data.alpha = 1.0f;
    EventManager::Instance().Publish<FrameRenderDataEvent>({data});
}

std::vector<entt::id_type> PostProcessSystem::GetReadComponents() const
{
    return {};
}

std::vector<entt::id_type> PostProcessSystem::GetWriteComponents() const
{
    return {};
}

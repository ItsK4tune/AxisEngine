#include <ecs/logic/post_process_system.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/post_process_component.h>
#include <render/interface/i_graphics_context.h>
#include <resource/logic/resource_manager.h>
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
    m_EventSubscriptions.Clear();

    if (!context || !resources || !configManager)
    {
        LOGGER_WARN("PostProcessSystem") << "Skipping full initialization (missing Graphics, Resources, or Config)";
        return;
    }

    auto config = configManager->GetConfig();

    m_Pipeline.Initialize(*context, config.width, config.height, *resources);

    m_EventSubscriptions.Add(
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

            if (cfg.width != m_Pipeline.GetWidth() || cfg.height != m_Pipeline.GetHeight())
            {
                m_Pipeline.Resize(cfg.width, cfg.height);
            }
        }));

    m_EventSubscriptions.Add(
        EventManager::Instance().Subscribe<WindowResizedEvent>([this](const WindowResizedEvent& e) {
            if (e.width != m_Pipeline.GetWidth() || e.height != m_Pipeline.GetHeight())
            {
                m_Pipeline.Resize(e.width, e.height);
            }
        }));

    m_RenderService = sl.Resolve<IRenderService>();
}

void PostProcessSystem::Shutdown()
{
    m_EventSubscriptions.Clear();
    m_Pipeline.Shutdown();
}

void PostProcessSystem::RenderCapturePass(Scene& scene, int width, int height)
{
    // Lazy resolve RenderService if needed
    if (!m_RenderService)
    {
        m_RenderService = ServiceLocator::Instance().Resolve<IRenderService>();
    }

    if (width != m_Pipeline.GetWidth() || height != m_Pipeline.GetHeight())
    {
        m_Pipeline.Resize(width, height);
    }
    m_Pipeline.BeginCapture();

    uint32_t captureFBO = m_Pipeline.GetCaptureFBO();
    if (m_RenderService)
    {
        m_RenderService->SetMainFBO(captureFBO);
    }

    static bool firstCaptureLog = true;
    if (firstCaptureLog && captureFBO != 0)
    {
        LOGGER_INFO("PostProcessSystem") << "BeginCapture initialized: FBO=" << captureFBO;
        firstCaptureLog = false;
    }

    FrameRenderData data;
    data.mainFBO = captureFBO;
    data.width = width;
    data.height = height;
    data.alpha = 1.0f;  // Post process usually full alpha
    EventManager::Instance().Publish<FrameRenderDataEvent>({data});
}

void PostProcessSystem::RenderAlphaPass(Scene& scene, int width, int height, float alpha)
{
}

void PostProcessSystem::Render(Scene& scene)
{
    m_Pipeline.ClearEffects();

    // Lazy resolve RenderService if needed
    if (!m_RenderService)
    {
        m_RenderService = ServiceLocator::Instance().Resolve<IRenderService>();
    }

    if (m_RenderService)
    {
        m_Pipeline.ApplyAntiAliasing(m_RenderService->GetAntiAliasingMode(), m_RenderService->GetPrevViewProj(),
                                     m_RenderService->GetCurrViewProj(), m_RenderService->GetJitterOffset());
    }

    // Only collect custom component effects if system is enabled
    if (m_EffectsEnabled)
    {
        auto view = scene.registry.view<PostProcessComponent, InfoComponent>();
        auto* res = ServiceLocator::Instance().Resolve<ResourceManager>();
        if (res)
        {
            for (auto entity : view)
            {
                auto [pp, info] = view.get<PostProcessComponent, InfoComponent>(entity);
                if (!pp.enabled || !info.isActive)
                    continue;
                for (const auto& eff : pp.effects)
                {
                    if (!eff.enabled)
                        continue;
                    auto shader = res->GetShader(eff.shaderName);
                    if (shader)
                    {
                        m_Pipeline.AddEffect(shader, eff.x, eff.y, eff.w, eff.h, eff.priority, eff.affectUI);
                    }
                }
            }
        }
    }

    m_Pipeline.EndCapture();

    if (m_RenderService && !m_Pipeline.HasUIEffects())
    {
        m_RenderService->SetMainFBO(0);
    }

    FrameRenderData data;
    data.mainFBO = m_Pipeline.HasUIEffects() ? m_Pipeline.GetCaptureFBO() : 0;
    data.width = m_Pipeline.GetWidth();
    data.height = m_Pipeline.GetHeight();
    data.alpha = 1.0f;
    EventManager::Instance().Publish<FrameRenderDataEvent>({data});
}

void PostProcessSystem::RenderUIPass(Scene& scene, float width, float height, IRenderStateManager& renderState)
{
    if (!m_EffectsEnabled || !m_Pipeline.HasUIEffects())
        return;

    m_Pipeline.RenderUIEffects();

    if (m_RenderService)
    {
        m_RenderService->SetMainFBO(0);
    }
}

std::vector<entt::id_type> PostProcessSystem::GetReadComponents() const
{
    return {};
}

std::vector<entt::id_type> PostProcessSystem::GetWriteComponents() const
{
    return {};
}

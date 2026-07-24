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
#include <resource/interface/i_resource_libraries.h>
#include <scene/logic/scene.h>
#include <algorithm>

void PostProcessSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<PostProcessSystem>(this);
    sl.Register<PostProcessPipeline>(&m_Pipeline);
    sl.Register<IPostProcessRegistry>(this);

    auto* context = sl.Resolve<IGraphicsContext>();
    auto* resources = sl.Resolve<IShaderLibrary>();
    auto* configManager = sl.Resolve<ConfigManager>();
    m_EventSubscriptions.Clear();

    if (!context || !resources || !configManager)
    {
        LOGGER_WARN("PostProcessSystem") << "Skipping full initialization (missing Graphics, Resources, or Config)";
        return;
    }

    auto config = configManager->GetConfig();

    m_Pipeline.Initialize(*context, config.window.width, config.window.height, *resources);
    m_Pipeline.SetBloomEnabled(config.render.bloomEnabled);
    m_Pipeline.SetBloomThreshold(config.render.bloomThreshold);
    m_Pipeline.SetBloomIntensity(config.render.bloomIntensity);
    m_Pipeline.SetBloomRadius(config.render.bloomRadius);
    m_Pipeline.SetTAAFeedback(config.render.taaFeedback);
    m_Pipeline.SetExposure(config.render.exposure);
    m_Pipeline.SetGamma(config.render.gamma);
    m_Pipeline.SetTonemappingMode(static_cast<int>(config.render.tonemappingMode));

    m_EventSubscriptions.Add(
        EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
            if (!HasConfigChanged(e, ConfigChangedEvent::Window | ConfigChangedEvent::Graphics))
                return;

            const auto& cfg = e.config;
            m_Pipeline.SetBloomEnabled(cfg.render.bloomEnabled);
            m_Pipeline.SetBloomThreshold(cfg.render.bloomThreshold);
            m_Pipeline.SetBloomIntensity(cfg.render.bloomIntensity);
            m_Pipeline.SetBloomRadius(cfg.render.bloomRadius);
            m_Pipeline.SetTAAFeedback(cfg.render.taaFeedback);
            m_Pipeline.SetExposure(cfg.render.exposure);
            m_Pipeline.SetGamma(cfg.render.gamma);
            m_Pipeline.SetTonemappingMode((int)cfg.render.tonemappingMode);

            if (cfg.window.width != m_Pipeline.GetWidth() || cfg.window.height != m_Pipeline.GetHeight())
            {
                m_Pipeline.Resize(cfg.window.width, cfg.window.height);
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
    std::lock_guard lock(m_RegistryMutex);
    m_RegisteredEffects.clear();
    RebuildRegistrySnapshotLocked();
}

void PostProcessSystem::RenderCapturePass(Scene& scene, int width, int height)
{
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

    FrameRenderData data;
    data.mainFBO = captureFBO;
    data.width = width;
    data.height = height;
    data.alpha = 1.0f;
    EventManager::Instance().Publish<FrameRenderDataEvent>({data});
}

void PostProcessSystem::Render(Scene& scene)
{
    m_Pipeline.ClearEffects();

    if (!m_RenderService)
    {
        m_RenderService = ServiceLocator::Instance().Resolve<IRenderService>();
    }

    if (m_RenderService)
    {
        const auto antiAliasingMode = m_RenderService->GetAntiAliasingMode();
        if (m_LastAntiAliasingMode != static_cast<int>(antiAliasingMode))
        {
            m_Pipeline.ResetTemporalHistory();
            m_LastAntiAliasingMode = static_cast<int>(antiAliasingMode);
        }
        m_Pipeline.ApplyAntiAliasing(antiAliasingMode, m_RenderService->GetPrevViewProj(),
                                     m_RenderService->GetCurrViewProj());
    }

    auto* res = ServiceLocator::Instance().Resolve<IShaderLibrary>();
    if (m_EffectsEnabled && res)
    {
        const auto registered = m_RegisteredEffectsSnapshot.load(std::memory_order_acquire);
        for (const auto& effect : *registered)
        {
            const auto& descriptor = effect.descriptor;
            if (auto shader = res->GetShader(descriptor.shaderName))
                m_Pipeline.AddEffect(shader, descriptor.x, descriptor.y, descriptor.width, descriptor.height,
                                     descriptor.priority, descriptor.affectUI, descriptor.inputs);
        }
    }

    // Only collect custom component effects if system is enabled
    if (m_EffectsEnabled)
    {
        auto view = scene.View<PostProcessComponent, InfoComponent>();
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
                        m_Pipeline.AddEffect(shader, eff.x, eff.y, eff.w, eff.h, eff.priority, eff.affectUI,
                                             eff.inputs);
                    }
                }
            }
        }
    }

    m_Pipeline.EndCapture();

    if (m_RenderService)
        m_RenderService->SetMainFBO(m_Pipeline.GetFinalFBO());

    FrameRenderData data;
    // UI must be composited into the actual post-HDR current target. FBO 0 is
    // only the scene-capture target and may no longer own the current color.
    data.mainFBO = m_Pipeline.GetFinalFBO();
    data.width = m_Pipeline.GetWidth();
    data.height = m_Pipeline.GetHeight();
    data.alpha = 1.0f;
    EventManager::Instance().Publish<FrameRenderDataEvent>({data});
}

void PostProcessSystem::RenderUIPass(Scene& scene, float width, float height, IRenderStateManager& renderState)
{
    if (m_EffectsEnabled && m_Pipeline.HasUIEffects())
        m_Pipeline.RenderUIEffects(m_PresentToBackbuffer);
    else if (m_PresentToBackbuffer)
        m_Pipeline.Present();

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

PostProcessEffectHandle PostProcessSystem::RegisterEffect(PostProcessEffectDescriptor descriptor)
{
    constexpr uint32_t validInputs = static_cast<uint32_t>(PostProcessInput::All);
    if (descriptor.owner.empty() || descriptor.name.empty() || descriptor.shaderName.empty() ||
        descriptor.width < 0 || descriptor.height < 0 ||
        (static_cast<uint32_t>(descriptor.inputs) & ~validInputs) != 0)
        return 0;
    std::lock_guard lock(m_RegistryMutex);
    const auto duplicate = std::find_if(m_RegisteredEffects.begin(), m_RegisteredEffects.end(),
                                        [&](const auto& entry) {
                                            return entry.second.owner == descriptor.owner &&
                                                   entry.second.name == descriptor.name;
                                        });
    if (duplicate != m_RegisteredEffects.end())
        return 0;
    const PostProcessEffectHandle handle = m_NextEffectHandle++;
    m_RegisteredEffects.emplace(handle, std::move(descriptor));
    RebuildRegistrySnapshotLocked();
    return handle;
}

bool PostProcessSystem::UnregisterEffect(PostProcessEffectHandle handle)
{
    if (handle == 0)
        return false;
    std::lock_guard lock(m_RegistryMutex);
    const bool removed = m_RegisteredEffects.erase(handle) != 0;
    if (removed)
        RebuildRegistrySnapshotLocked();
    return removed;
}

size_t PostProcessSystem::UnregisterOwner(std::string_view owner)
{
    std::lock_guard lock(m_RegistryMutex);
    const size_t removed = std::erase_if(m_RegisteredEffects,
                                         [owner](const auto& entry) { return entry.second.owner == owner; });
    if (removed > 0)
        RebuildRegistrySnapshotLocked();
    return removed;
}

std::vector<RegisteredPostProcessEffect> PostProcessSystem::GetRegisteredEffects() const
{
    const auto snapshot = m_RegisteredEffectsSnapshot.load(std::memory_order_acquire);
    return *snapshot;
}

void PostProcessSystem::RebuildRegistrySnapshotLocked()
{
    std::vector<RegisteredPostProcessEffect> effects;
    effects.reserve(m_RegisteredEffects.size());
    for (const auto& [handle, descriptor] : m_RegisteredEffects) effects.push_back({handle, descriptor});
    std::sort(effects.begin(), effects.end(), [](const auto& left, const auto& right) {
        if (left.descriptor.priority != right.descriptor.priority)
            return left.descriptor.priority < right.descriptor.priority;
        if (left.descriptor.owner != right.descriptor.owner)
            return left.descriptor.owner < right.descriptor.owner;
        if (left.descriptor.name != right.descriptor.name)
            return left.descriptor.name < right.descriptor.name;
        return left.handle < right.handle;
    });
    m_RegisteredEffectsSnapshot.store(
        std::make_shared<const std::vector<RegisteredPostProcessEffect>>(std::move(effects)),
        std::memory_order_release);
}

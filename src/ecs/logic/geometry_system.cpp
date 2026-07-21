#include <ecs/logic/geometry_system.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_shadow_service.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/decal_component.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/terrain_component.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/logic/material_renderer.h>
#include <render/logic/render_core.h>
#include <render/unit/render_queue.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <algorithm>


namespace
{
bool HasActiveTerrain(Scene& scene)
{
    auto view = scene.View<TerrainComponent>();
    for (auto entity : view)
    {
        if (auto* info = scene.TryGetComponent<InfoComponent>(entity); info && !info->isActive)
            continue;
        return true;
    }
    return false;
}
}  // namespace

void GeometrySystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<IGeometryService>(this);
    sl.Register<GeometrySystem>(this);
    m_EventSubscriptions.Clear();
    m_GraphicsContext = sl.Resolve<IGraphicsContext>();
    m_ConfigManager = sl.Resolve<ConfigManager>();

    if (!m_GraphicsContext || !m_ConfigManager)
    {
        LOGGER_WARN("GeometrySystem") << "Skipping full initialization (missing GraphicsContext or ConfigManager)";
        return;
    }

    auto& context = *m_GraphicsContext;
    auto config = m_ConfigManager->GetConfig();

    m_GBuffer.SetRenderScale(config.graphics.renderScale);
    m_GBuffer.SetSampleCount((std::max)(1, config.graphics.msaaSamples));
    m_GBuffer.SetEntityIdEnabled(config.optimization.gbufferEntityIdEnabled);
    m_GBuffer.Initialize(context, config.window.width, config.window.height);
    m_IsDeferredCached = false;

    m_RenderService = sl.Resolve<IRenderService>();
    m_ShadowService = sl.Resolve<IShadowService>();

    m_EventSubscriptions.Add(
        EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
            if (!HasConfigChanged(e, ConfigChangedEvent::Graphics | ConfigChangedEvent::Window))
                return;

            const auto& cfg = e.config;
            if (cfg.window.width != m_GBuffer.GetWidth() || cfg.window.height != m_GBuffer.GetHeight())
            {
                m_GBuffer.Resize(cfg.window.width, cfg.window.height);
            }
        }));

    m_EventSubscriptions.Add(
        EventManager::Instance().Subscribe<WindowResizedEvent>([this](const WindowResizedEvent& e) {
            if (e.width != m_GBuffer.GetWidth() || e.height != m_GBuffer.GetHeight())
            {
                m_GBuffer.Resize(e.width, e.height);
            }
        }));
}

void GeometrySystem::Shutdown()
{
    m_EventSubscriptions.Clear();
    m_GBuffer.Shutdown();
}

void GeometrySystem::Render(Scene& scene)
{
    m_IsDeferredCached = false;

    if (!m_Enabled)
        return;

    if (!m_RenderService)
        m_RenderService = ServiceLocator::Instance().Resolve<IRenderService>();
    auto* rs = m_RenderService;
    if (!rs)
        return;

    entt::entity camEntity = scene.GetActiveCamera();
    auto* cam = scene.TryGetComponent<CameraComponent>(camEntity);
    if (!cam)
        return;

    const auto& defOpaqueQueue = rs->GetRenderQueueObj().GetDeferredOpaqueQueue();
    bool hasDeferredWork = !defOpaqueQueue.empty() || HasActiveTerrain(scene);
    if (!hasDeferredWork)
        return;

    const auto config = m_ConfigManager->GetConfigSnapshot();
    int width = config->window.width;
    int height = config->window.height;
    float currentScale = config->graphics.renderScale;
    const int currentSamples = (std::max)(1, config->graphics.msaaSamples);
    const bool hasDeferredDecals = !scene.View<DecalComponent>().empty();
    const bool entityIdEnabled = config->optimization.gbufferEntityIdEnabled || hasDeferredDecals ||
                                 m_EntityIdRequested;
    m_EntityIdRequested = false;

    if (width != m_GBuffer.GetWidth() || height != m_GBuffer.GetHeight() ||
        currentScale != m_GBuffer.GetRenderScale() || currentSamples != m_GBuffer.GetSampleCount() ||
        entityIdEnabled != m_GBuffer.IsEntityIdEnabled())
    {
        m_GBuffer.SetRenderScale(currentScale);
        m_GBuffer.SetSampleCount(currentSamples);
        m_GBuffer.SetEntityIdEnabled(entityIdEnabled);
        m_GBuffer.Resize(width, height);
    }

    if (!m_GraphicsContext)
        return;
    m_IsDeferredCached = true;

    auto& context = *m_GraphicsContext;
    auto& rsm = context.GetRenderStateManager();
    auto& dc = context.GetDrawContext();
    auto& rtm = context.GetRenderTargetManager();

    BindGBufferForWriting();
    rsm.SetViewport(0, 0, (int)(width * m_GBuffer.GetRenderScale()), (int)(height * m_GBuffer.GetRenderScale()));
    dc.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    // glClearColor is not the correct clear path for the R32UI picking attachment.
    // Temporarily omit it, clear the regular MRT/depth targets, then clear ID with
    // the typed integer API and restore the complete geometry draw-buffer map.
    FramebufferAttachment clearAttachments[] = {FramebufferAttachment::None, FramebufferAttachment::Color1,
                                                FramebufferAttachment::Color2, FramebufferAttachment::None,
                                                FramebufferAttachment::Color4, FramebufferAttachment::Color5};
    rtm.DrawBuffers(6, clearAttachments);
    dc.Clear(BufferBit::Color | BufferBit::Depth);
    FramebufferAttachment geometryAttachments[] = {
        FramebufferAttachment::None, FramebufferAttachment::Color1, FramebufferAttachment::Color2,
        entityIdEnabled ? FramebufferAttachment::Color3 : FramebufferAttachment::None,
        FramebufferAttachment::Color4, FramebufferAttachment::Color5};
    rtm.DrawBuffers(6, geometryAttachments);
    if (entityIdEnabled)
    {
        const unsigned int clearEntityId[4] = {0, 0, 0, 0};
        rtm.ClearColorAttachmentUInt(3, clearEntityId);
    }

    if (config->culling.depthTestEnabled)
    {
        rsm.Enable(ServerCapability::DepthTest);
        rsm.SetDepthFunc(CompareFunc::Less);
    }
    else
    {
        rsm.Disable(ServerCapability::DepthTest);
    }

    if (config->culling.cullFaceEnabled)
    {
        rsm.Enable(ServerCapability::CullFace);
        rsm.SetCullFace(CullMode::Back);
    }
    else
    {
        rsm.Disable(ServerCapability::CullFace);
    }

    auto& sl = ServiceLocator::Instance();
    auto* shadowSys = sl.Resolve<IShadowService>();
    ShadowRenderer* shadowRenderer = shadowSys ? &shadowSys->GetRenderer() : nullptr;

    if (!defOpaqueQueue.empty())
    {
        shadowRenderer = shadowSys ? &shadowSys->GetRenderer() : nullptr;
        RenderCore* core = sl.Resolve<RenderCore>();
        if (core)
        {
            rs->ExecuteQueue(defOpaqueQueue, RenderQueuePass::DeferredGeometry, shadowRenderer,
                             &core->GetMaterialRenderer(), nullptr);
        }
    }

    UnbindGBuffer();
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, rs->GetMainFBO());
}

void GeometrySystem::BindGBufferForWriting()
{
    m_GBuffer.BindForWriting();
}

void GeometrySystem::UnbindGBuffer()
{
    m_GBuffer.Unbind();
}

void GeometrySystem::BeginDecalPass()
{
    if (!m_GraphicsContext)
        return;
    auto& context = *m_GraphicsContext;
    auto& rtm = context.GetRenderTargetManager();
    auto& rsm = context.GetRenderStateManager();

    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_GBuffer.GetFBO());
    rsm.SetViewport(0, 0, (int)(m_GBuffer.GetWidth() * m_GBuffer.GetRenderScale()),
                    (int)(m_GBuffer.GetHeight() * m_GBuffer.GetRenderScale()));

    // The decal shader samples resolved depth and normal. Leaving either texture
    // attached to this draw framebuffer at the same time is an OpenGL feedback
    // loop with undefined results (observed as invisible decals on AMD drivers).
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth, TextureType::Texture2D, 0,
                             0);
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color1, TextureType::Texture2D, 0,
                             0);

    FramebufferAttachment attachments[] = {
        FramebufferAttachment::Color2,  // Albedo (location 0)
        FramebufferAttachment::None,    // Normal is sampled and intentionally preserved
        FramebufferAttachment::Color4,  // Emissive (location 2)
        FramebufferAttachment::Color5   // PBRParams (location 3)
    };
    rtm.DrawBuffers(4, attachments);
}

void GeometrySystem::EndDecalPass(uint32_t mainFBO)
{
    if (!m_GraphicsContext)
        return;
    auto& context = *m_GraphicsContext;
    auto& rtm = context.GetRenderTargetManager();
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_GBuffer.GetFBO());
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color1, TextureType::Texture2D,
                             m_GBuffer.GetNormalTexture(), 0);
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth, TextureType::Texture2D,
                             m_GBuffer.GetDepthTexture(), 0);
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);

    FramebufferAttachment att = FramebufferAttachment::Color0;
    rtm.DrawBuffers(1, &att);
}

std::vector<entt::id_type> GeometrySystem::GetReadComponents() const
{
    return {entt::type_id<MeshRendererComponent>().hash(),   entt::type_id<PositionComponent>().hash(),
            entt::type_id<RotationComponent>().hash(),       entt::type_id<ScaleComponent>().hash(),
            entt::type_id<WorldTransformComponent>().hash(), entt::type_id<MaterialComponent>().hash()};
}

std::vector<entt::id_type> GeometrySystem::GetWriteComponents() const
{
    return {};
}

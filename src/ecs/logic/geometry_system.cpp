#include <ecs/logic/geometry_system.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_shadow_service.h>
#include <ecs/logic/entity_manager.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/logic/frustum_culler.h>
#include <render/logic/material_renderer.h>
#include <render/logic/render_core.h>
#include <render/unit/render_queue.h>
#include <resource/logic/resource_manager.h>
#include <algorithm>

REGISTER_SYSTEM(GeometrySystem)

void GeometrySystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<IGeometryService>(this);
    sl.Register<GeometrySystem>(this);
    m_GraphicsContext = sl.Resolve<IGraphicsContext>();
    m_ConfigManager = sl.Resolve<ConfigManager>();

    if (!m_GraphicsContext || !m_ConfigManager)
    {
        LOGGER_WARN("GeometrySystem") << "Skipping full initialization (missing GraphicsContext or ConfigManager)";
        return;
    }

    auto& context = *m_GraphicsContext;
    auto* resources = sl.Resolve<ResourceManager>();
    if (!resources)
        return;

    const auto& config = m_ConfigManager->GetConfig();

    m_GBufferShader = resources->GetShader("deferred_lit");

    m_GBuffer.SetRenderScale(config.renderScale);
    m_GBuffer.Initialize(context, config.width, config.height);
    m_IsDeferredCached = true;

    m_RenderService = sl.Resolve<IRenderService>();
    m_ShadowService = sl.Resolve<IShadowService>();

    EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (!(e.bitmask & (ConfigChangedEvent::Graphics | ConfigChangedEvent::Window | ConfigChangedEvent::All)))
            return;

        const auto& cfg = e.config;
        if (cfg.width != m_GBuffer.GetWidth() || cfg.height != m_GBuffer.GetHeight())
        {
            m_GBuffer.Resize(cfg.width, cfg.height);
        }
    });

    EventManager::Instance().Subscribe<WindowResizedEvent>([this](const WindowResizedEvent& e) {
        if (e.width != m_GBuffer.GetWidth() || e.height != m_GBuffer.GetHeight())
        {
            m_GBuffer.Resize(e.width, e.height);
        }
    });
}

void GeometrySystem::Shutdown()
{
    m_GBuffer.Shutdown();
}

void GeometrySystem::Render(Scene& scene)
{
    if (!m_Enabled)
        return;

    if (!m_RenderService)
        m_RenderService = ServiceLocator::Instance().Resolve<IRenderService>();
    auto* rs = m_RenderService;
    if (!rs)
    {
        static bool rsWarned = false;
        if (!rsWarned)
        {
            LOGGER_ERROR("GeometrySystem") << "RenderService not found!";
            rsWarned = true;
        }
        return;
    }

    entt::entity camEntity = ::EntityManager::GetActiveCamera(scene);
    auto* cam = scene.registry.try_get<CameraComponent>(camEntity);
    if (!cam)
    {
        static bool camWarned = false;
        if (!camWarned)
        {
            LOGGER_WARN("GeometrySystem") << "No active camera found!";
            camWarned = true;
        }
        return;
    }

    bool isDeferred = true;
    m_IsDeferredCached = true;

    const auto& config = m_ConfigManager->GetConfig();
    int width = config.width;
    int height = config.height;
    float currentScale = config.renderScale;

    if (width != m_GBuffer.GetWidth() || height != m_GBuffer.GetHeight() || currentScale != m_GBuffer.GetRenderScale())
    {
        m_GBuffer.SetRenderScale(currentScale);
        m_GBuffer.Resize(width, height);
    }

    if (!m_GraphicsContext)
        return;
    auto& context = *m_GraphicsContext;
    auto& rsm = context.GetRenderStateManager();
    auto& dc = context.GetDrawContext();
    auto& rtm = context.GetRenderTargetManager();

    BindGBufferForWriting();
    rsm.SetViewport(0, 0, (int)(width * m_GBuffer.GetRenderScale()), (int)(height * m_GBuffer.GetRenderScale()));
    dc.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    dc.Clear(BufferBit::Color | BufferBit::Depth);

    rsm.Enable(ServerCapability::DepthTest);
    rsm.SetDepthFunc(CompareFunc::Less);
    rsm.Enable(ServerCapability::CullFace);
    rsm.SetCullFace(CullMode::Back);

    auto& sl = ServiceLocator::Instance();
    auto* shadowSys = sl.Resolve<IShadowService>();
    ShadowRenderer* shadowRenderer = shadowSys ? &shadowSys->GetRenderer() : nullptr;

    const auto& defOpaqueQueue = rs->GetRenderQueueObj().GetDeferredOpaqueQueue();
    if (!defOpaqueQueue.empty())
    {
        shadowRenderer = shadowSys ? &shadowSys->GetRenderer() : nullptr;
        RenderCore* core = sl.Resolve<RenderCore>();
        if (core)
        {
            rs->ExecuteQueue(defOpaqueQueue, false, shadowRenderer, &core->GetMaterialRenderer(), nullptr);
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

    FramebufferAttachment attachments[] = {
        FramebufferAttachment::Color2,  // Albedo (location 0)
        FramebufferAttachment::Color1,  // Normal (location 1)
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
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);

    FramebufferAttachment att = FramebufferAttachment::Color0;
    rtm.DrawBuffers(1, &att);
}

std::vector<entt::id_type> GeometrySystem::GetReadComponents() const
{
    return {entt::type_id<MeshRendererComponent>().hash(),   entt::type_id<PositionComponent>().hash(),
            entt::type_id<RotationComponent>().hash(),       entt::type_id<ScaleComponent>().hash(),
            entt::type_id<WorldTransformComponent>().hash(), entt::type_id<AxisMaterialComponent>().hash()};
}

std::vector<entt::id_type> GeometrySystem::GetWriteComponents() const
{
    return {};
}

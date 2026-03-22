#include <ecs/logic/geometry_system.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_shadow_service.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_buffer_manager.h>
#include <resource/logic/resource_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <core/logic/config_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <render/logic/frustum_culler.h>
#include <render/unit/render_queue.h>
#include <algorithm>

void GeometrySystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& resources = sl.Require<ResourceManager>();
    auto& config = sl.Require<ConfigManager>().GetConfig();

    resources.LoadShader("gbuffer", "include/engine/asset/shaders/gbuffer.vs", "include/engine/asset/shaders/gbuffer.fs");
    m_GBufferShader = resources.GetShader("gbuffer");

    m_GBuffer.SetRenderScale(config.renderScale);
    m_GBuffer.Initialize(context, config.width, config.height);

    auto* renderSys = sl.Resolve<IRenderService>();
    if (renderSys) {
        m_MaterialRenderer.Initialize(context, renderSys->GetWhiteTexture(), renderSys->GetBlackTexture(), renderSys->GetFlatNormalTexture());
    }

    EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (!(e.bitmask & (ConfigChangedEvent::Graphics | ConfigChangedEvent::Window | ConfigChangedEvent::All)))
            return;

        const auto& cfg = e.config;
        if (cfg.width != m_GBuffer.GetWidth() || cfg.height != m_GBuffer.GetHeight()) {
             auto& sl_inner = ServiceLocator::Instance();
             m_GBuffer.Resize(cfg.width, cfg.height);
        }
    });
}

void GeometrySystem::Shutdown()
{
    m_GBuffer.Shutdown();
}

void GeometrySystem::RenderAlpha(Scene& scene, int width, int height, float alpha)
{
    if (!m_Enabled)
         return;

    auto& sl = ServiceLocator::Instance();
    auto* rs = sl.Resolve<IRenderService>();
    if (!rs) return;

    auto renderPath = rs->GetRenderPath();
    bool isDeferred = (renderPath == RenderPath::Deferred);

    const auto& config = sl.Require<ConfigManager>().GetConfig();
    float currentScale = config.renderScale;

    if (width != m_GBuffer.GetWidth() || height != m_GBuffer.GetHeight() || currentScale != m_GBuffer.GetRenderScale()) {
        m_GBuffer.SetRenderScale(currentScale);
        m_GBuffer.Resize(width, height);
    }

    auto& context = sl.Require<IGraphicsContext>();
    auto& rsm = context.GetRenderStateManager();
    auto& dc = context.GetDrawContext();
    auto& rtm = context.GetRenderTargetManager();

    if (isDeferred) {
        // Fill G-Buffer (Opaque Pass)
        BindGBufferForWriting();
        
        rsm.SetViewport(0, 0, (int)(width * m_GBuffer.GetRenderScale()), (int)(height * m_GBuffer.GetRenderScale()));
        dc.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        dc.Clear(BufferBit::Color | BufferBit::Depth);
    } else {
        // Full Forward Rendering Path
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, rs->GetMainFBO());
        rsm.SetViewport(0, 0, width, height);
        
        const auto& config = sl.Require<ConfigManager>().GetConfig();
        dc.ClearColor(config.clearColor[0], config.clearColor[1], config.clearColor[2], config.clearColor[3]);
        dc.Clear(BufferBit::Color | BufferBit::Depth);
    }

    rsm.Enable(ServerCapability::DepthTest);
    rsm.SetDepthFunc(CompareFunc::Less);
    rsm.Enable(ServerCapability::CullFace);
    rsm.SetCullFace(CullMode::Back);

    const auto& opaqueQueue = rs->GetRenderQueueObj().GetOpaqueQueue();
    if (!opaqueQueue.empty())
    {
        auto* shadowSys = sl.Resolve<IShadowService>();
        ShadowRenderer* shadowRenderer = shadowSys ? &shadowSys->GetRenderer() : nullptr;
        
        // Use override shader ONLY for deferred
        Shader* overrideShader = isDeferred ? m_GBufferShader.get() : nullptr;
        
        rs->ExecuteQueue(scene, opaqueQueue, false, shadowRenderer, &m_MaterialRenderer, overrideShader); 
    }
    
    if (isDeferred) {
        UnbindGBuffer();

        // Re-bind main FBO if any (e.g. for lighting pass or post-process)
        uint32_t mainFBO = rs->GetMainFBO();
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);
    }
}

void GeometrySystem::BindGBufferForWriting()
{
    m_GBuffer.BindForWriting();
}

void GeometrySystem::UnbindGBuffer()
{
    m_GBuffer.Unbind();
    // Reset to main frame buffer if needed
}

void GeometrySystem::BeginDecalPass()
{
    auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
    auto& rtm = context.GetRenderTargetManager();
    auto& rsm = context.GetRenderStateManager();
    
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_GBuffer.GetFBO());
    rsm.SetViewport(0, 0, (int)(m_GBuffer.GetWidth() * m_GBuffer.GetRenderScale()), (int)(m_GBuffer.GetHeight() * m_GBuffer.GetRenderScale()));
    
    // We only want to write to Albedo (Color2 in GBuffer)
    FramebufferAttachment att = FramebufferAttachment::Color2;
    rtm.DrawBuffers(1, &att);
}

void GeometrySystem::EndDecalPass(uint32_t mainFBO)
{
    auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
    auto& rtm = context.GetRenderTargetManager();
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);
    
    // Reset DrawBuffers for main FBO
    FramebufferAttachment att = FramebufferAttachment::Color0;
    rtm.DrawBuffers(1, &att);
}

bool GeometrySystem::IsDeferredRenderingEnabled() const
{
    auto* rs = ServiceLocator::Instance().Resolve<IRenderService>();
    return rs && rs->GetRenderPath() == RenderPath::Deferred;
}

std::vector<entt::id_type> GeometrySystem::GetReadComponents() const
{
    return {
        entt::type_id<MeshRendererComponent>().hash(),
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash(),
        entt::type_id<ScaleComponent>().hash(),
        entt::type_id<WorldTransformComponent>().hash(),
        entt::type_id<MaterialComponent>().hash()
    };
}

std::vector<entt::id_type> GeometrySystem::GetWriteComponents() const
{
    return {};
}

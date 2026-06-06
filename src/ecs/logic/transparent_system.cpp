#include <ecs/logic/transparent_system.h>
#include <core/logic/service_locator.h>
#include <ecs/interface/i_geometry_service.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_shadow_service.h>
#include <ecs/logic/decal_system.h>
#include <ecs/logic/particle_system.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/logic/render_core.h>
#include <render/unit/gbuffer.h>
#include <render/unit/render_queue.h>
#include <resource/logic/resource_manager.h>

REGISTER_SYSTEM(TransparentSystem)

void TransparentSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<TransparentSystem>(this);
}

void TransparentSystem::RenderAlphaPass(Scene& scene, int width, int height, float alpha)
{
    if (!m_Enabled)
        return;

    auto& sl = ServiceLocator::Instance();
    auto* rs = sl.Resolve<IRenderService>();
    if (!rs)
        return;

    auto* context = sl.Resolve<IGraphicsContext>();
    if (!context)
        return;

    auto& rsm = context->GetRenderStateManager();
    auto& rtm = context->GetRenderTargetManager();
    uint32_t mainFBO = rs->GetMainFBO();
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);

    auto* shadowSys = sl.Resolve<IShadowService>();
    ShadowRenderer* shadowRenderer = shadowSys ? &shadowSys->GetRenderer() : nullptr;
    auto* core = sl.Resolve<RenderCore>();

    if (core)
    {
        // Forced-forward opaque runs after skybox. Depth-ignored 3D overlays are drawn after transparent objects.
        rsm.Disable(ServerCapability::Blend);
        rsm.Enable(ServerCapability::DepthTest);
        rsm.SetDepthMask(true);

        FramebufferAttachment colorAttachment = FramebufferAttachment::Color0;
        rtm.DrawBuffers(1, &colorAttachment);

        const auto& regularForwardQueue = rs->GetRenderQueueObj().GetForwardOpaqueQueue();
        if (!regularForwardQueue.empty())
        {
            rs->ExecuteQueue(regularForwardQueue, RenderQueuePass::ForwardOpaque, shadowRenderer,
                             &core->GetMaterialRenderer(), nullptr);
        }
    }

    if (auto* decalSystem = sl.Resolve<DecalSystem>())
    {
        decalSystem->RenderAlphaPass(scene, width, height, alpha);
    }
}

void TransparentSystem::RenderTransparentPass(Scene& scene, int width, int height, float alpha)
{
    if (!m_Enabled)
        return;

    auto& sl = ServiceLocator::Instance();
    auto* rs = sl.Resolve<IRenderService>();
    if (!rs)
        return;

    auto* context = sl.Resolve<IGraphicsContext>();
    if (!context)
        return;

    auto& rsm = context->GetRenderStateManager();
    auto& rtm = context->GetRenderTargetManager();
    uint32_t mainFBO = rs->GetMainFBO();
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);

    auto* shadowSys = sl.Resolve<IShadowService>();
    ShadowRenderer* shadowRenderer = shadowSys ? &shadowSys->GetRenderer() : nullptr;
    auto* core = sl.Resolve<RenderCore>();

    rsm.Enable(ServerCapability::Blend);
    rsm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
    rsm.Enable(ServerCapability::DepthTest);
    rsm.SetDepthMask(false);

    if (core)
    {
        rs->ExecuteQueue(rs->GetRenderQueueObj().GetTransparentQueue(), RenderQueuePass::Transparent, shadowRenderer,
                         &core->GetMaterialRenderer());
    }

    if (auto* particleSystem = sl.Resolve<ParticleSystem>())
    {
        particleSystem->RenderParticles(scene, width, height, alpha);
    }

    rsm.Disable(ServerCapability::Blend);
    rsm.SetDepthMask(true);

    const auto& depthOverlayQueue = rs->GetRenderQueueObj().GetDepthOverlayQueue();
    if (depthOverlayQueue.empty())
        return;

    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, rs->GetMainFBO());
    FramebufferAttachment colorAttachment = FramebufferAttachment::Color0;
    rtm.DrawBuffers(1, &colorAttachment);
    rsm.SetViewport(0, 0, width, height);
    rsm.Disable(ServerCapability::Blend);
    context->Clear(BufferBit::Depth);
    rsm.Enable(ServerCapability::DepthTest);
    rsm.SetDepthFunc(CompareFunc::Less);
    rsm.SetDepthMask(true);

    if (core)
    {
        rs->ExecuteQueue(depthOverlayQueue, RenderQueuePass::DepthOverlay, shadowRenderer,
                         &core->GetMaterialRenderer(), nullptr);
    }
}

std::vector<entt::id_type> TransparentSystem::GetReadComponents() const
{
    return {entt::type_id<MeshRendererComponent>().hash(),   entt::type_id<PositionComponent>().hash(),
            entt::type_id<RotationComponent>().hash(),       entt::type_id<ScaleComponent>().hash(),
            entt::type_id<WorldTransformComponent>().hash(), entt::type_id<MaterialComponent>().hash()};
}

std::vector<entt::id_type> TransparentSystem::GetWriteComponents() const
{
    return {};
}

#include <ecs/logic/transparent_system.h>
#include <ecs/logic/system_factory.h>
#include <ecs/interface/i_shadow_service.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_geometry_service.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_draw_context.h>
#include <resource/logic/resource_manager.h>
#include <render/unit/gbuffer.h>
#include <render/logic/render_core.h>
#include <render/unit/render_queue.h>
#include <render/interface/i_render_target_manager.h>
#include <core/logic/service_locator.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_draw_context.h>

REGISTER_SYSTEM(TransparentSystem)

void TransparentSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<TransparentSystem>(this);
}

void TransparentSystem::RenderTransparentPass(Scene& scene, int width, int height, float alpha)
{
    if (!m_Enabled)
         return;

    auto& sl = ServiceLocator::Instance();
    auto* rs = sl.Resolve<IRenderService>();
    if (!rs) return;

    auto* context = sl.Resolve<IGraphicsContext>();
    if (!context) return;

    auto& rsm = context->GetRenderStateManager();
    auto& rtm = context->GetRenderTargetManager();
    uint32_t mainFBO = rs->GetMainFBO();
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);

    rsm.Enable(ServerCapability::Blend);
    rsm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
    rsm.Enable(ServerCapability::DepthTest);
    rsm.SetDepthMask(false);

    auto* shadowSys = sl.Resolve<IShadowService>();
    ShadowRenderer* shadowRenderer = shadowSys ? &shadowSys->GetRenderer() : nullptr;
    auto* core = sl.Resolve<RenderCore>();
    if (core) {
        rs->ExecuteQueue(rs->GetRenderQueueObj().GetTransparentQueue(), true, shadowRenderer, &core->GetMaterialRenderer());
    }

    rsm.Disable(ServerCapability::Blend);
    rsm.SetDepthMask(true);
}

std::vector<entt::id_type> TransparentSystem::GetReadComponents() const
{
    return {
        entt::type_id<MeshRendererComponent>().hash(),
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash(),
        entt::type_id<ScaleComponent>().hash(),
        entt::type_id<WorldTransformComponent>().hash(),
        entt::type_id<AxisMaterialComponent>().hash()
    };
}

std::vector<entt::id_type> TransparentSystem::GetWriteComponents() const
{
    return {};
}

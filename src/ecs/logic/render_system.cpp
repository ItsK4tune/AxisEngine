#include <ecs/logic/render_system.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/ui_components.h>
#include <ecs/interface/i_render_runtime_control.h>


RenderSystem::RenderSystem()
{
}
RenderSystem::~RenderSystem() = default;

void RenderSystem::Initialize()
{
    m_RenderService = ServiceLocator::Instance().Resolve<IRenderRuntimeControl>();
}

void RenderSystem::Update(Scene& scene, float dt)
{
    if (m_RenderService)
    {
        m_RenderService->ResetQueuesBuilt();
        m_RenderService->IncrementFrame();
        m_RenderService->ResetRenderedCount();
        m_RenderService->AddTime(dt);
    }
}

std::vector<entt::id_type> RenderSystem::GetReadComponents() const
{
    return {};
}

std::vector<entt::id_type> RenderSystem::GetWriteComponents() const
{
    return {};
}

void RenderSystem::SetFilterLayerMask(uint32_t mask)
{
    if (m_RenderService)
        m_RenderService->SetFilterLayerMask(mask);
}
void RenderSystem::SetFaceCulling(bool enabled, CullMode mode)
{
    if (m_RenderService)
        m_RenderService->SetFaceCulling(enabled, mode);
}
void RenderSystem::SetDepthTest(bool enabled, CompareFunc func)
{
    if (m_RenderService)
        m_RenderService->SetDepthTest(enabled, func);
}
void RenderSystem::SetInstanceBatching(bool enable)
{
    if (m_RenderService)
        m_RenderService->SetInstanceBatching(enable);
}
void RenderSystem::SetFrustumCulling(bool enable)
{
    if (m_RenderService)
        m_RenderService->SetFrustumCulling(enable);
}
void RenderSystem::SetSpatialCullingMode(SpatialCullingMode mode)
{
    if (m_RenderService)
        m_RenderService->SetSpatialCullingMode(mode);
}
void RenderSystem::SetRenderOrderEnabled(bool enable)
{
    if (m_RenderService)
        m_RenderService->SetRenderOrderEnabled(enable);
}

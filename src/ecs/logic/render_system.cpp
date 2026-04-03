#include <ecs/logic/render_system.h>
#include <render/logic/render_service_impl.h>
#include <ecs/logic/system_factory.h>
#include <core/logic/service_locator.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/ui_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/reflection_components.h>
#include <core/logic/logger.h>

REGISTER_SYSTEM(RenderSystem)

RenderSystem::RenderSystem() {}
RenderSystem::~RenderSystem() = default;

void RenderSystem::Initialize()
{
    m_RenderService = ServiceLocator::Instance().Resolve<RenderServiceImpl>();
}

void RenderSystem::Shutdown()
{
}

void RenderSystem::Update(Scene& scene, float dt)
{
    if (m_RenderService) 
    {
        m_RenderService->FetchRenderPath();
        m_RenderService->ResetQueuesBuilt();
        m_RenderService->IncrementFrame();
        m_RenderService->ResetRenderedCount();
        m_RenderService->AddTime(dt);
    }
}

void RenderSystem::Render(Scene& scene)
{
    // Minimal implementation as in old code
}

void RenderSystem::RenderAlphaPass(Scene &scene, int width, int height, float alpha)
{
}

void RenderSystem::RenderTransparentPass(Scene &scene, int width, int height, float alpha)
{
}

void RenderSystem::RenderUIPass(Scene &scene, float width, float height, IRenderStateManager &renderState)
{
}

std::vector<entt::id_type> RenderSystem::GetReadComponents() const
{
    return {
        entt::type_id<MeshRendererComponent>().hash(),
        entt::type_id<CameraComponent>().hash(),
        entt::type_id<InfoComponent>().hash(),
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash(),
        entt::type_id<ScaleComponent>().hash(),
        entt::type_id<WorldTransformComponent>().hash(),
        entt::type_id<AxisMaterialComponent>().hash(),
        entt::type_id<SkyboxRenderComponent>().hash(),
        entt::type_id<GPUDirLight>().hash(),
        entt::type_id<GPUPointLight>().hash(),
        entt::type_id<GPUSpotLight>().hash(),
        entt::type_id<AnimationComponent>().hash(),
        entt::type_id<StreamingComponent>().hash(),
        entt::type_id<UIRendererComponent>().hash()
    };
}

std::vector<entt::id_type> RenderSystem::GetWriteComponents() const
{
    return {
        entt::type_id<OcclusionComponent>().hash()
    };
}

void RenderSystem::SetFilterLayerMask(uint32_t mask) { if (m_RenderService) m_RenderService->SetFilterLayerMask(mask); }
void RenderSystem::SetFaceCulling(bool enabled, CullMode mode) { if (m_RenderService) m_RenderService->SetFaceCulling(enabled, mode); }
void RenderSystem::SetDepthTest(bool enabled, CompareFunc func) { if (m_RenderService) m_RenderService->SetDepthTest(enabled, func); }
void RenderSystem::SetInstanceBatching(bool enable) { if (m_RenderService) m_RenderService->SetInstanceBatching(enable); }
void RenderSystem::SetFrustumCulling(bool enable) { if (m_RenderService) m_RenderService->SetFrustumCulling(enable); }
void RenderSystem::SetRenderOrderEnabled(bool enable) { if (m_RenderService) m_RenderService->SetRenderOrderEnabled(enable); }

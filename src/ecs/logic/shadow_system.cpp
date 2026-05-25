#include <ecs/logic/shadow_system.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/render_components.h>
#include <render/interface/i_graphics_context.h>
#include <render/logic/frustum_culler.h>
#include <render/unit/render_queue.h>
#include <resource/logic/resource_manager.h>

REGISTER_SYSTEM(ShadowSystem)

void ShadowSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<IShadowService>(this);
    sl.Register<ShadowSystem>(this);

    auto* context = sl.Resolve<IGraphicsContext>();
    auto* resources = sl.Resolve<ResourceManager>();
    auto* configManager = sl.Resolve<ConfigManager>();

    if (!context || !resources || !configManager)
    {
        LOGGER_WARN("ShadowSystem") << "Skipping full initialization (missing Graphics, Resources, or Config)";
        return;
    }

    const auto& config = configManager->GetConfig();

    m_ShadowRenderer.Initialize(*context, *resources);
    m_ShadowRenderer.GetShadow().Initialize(*context, config.shadowMapResolution, config.shadowMapResolution);

    m_ShadowRenderer.SetEnableShadows(config.shadowsEnabled);
    m_ShadowRenderer.SetShadowMode(config.shadowMode);
    m_ShadowRenderer.SetShadowBias(config.shadowBias);
    m_ShadowRenderer.SetShadowSoftness(config.shadowSoftness);
    m_ShadowRenderer.SetShadowProjectionSize(config.shadowProjectionSize);
    m_ShadowRenderer.SetShadowFrustumCulling(config.shadowFrustumCullingEnabled);
    m_ShadowRenderer.SetShadowDistanceCulling(config.shadowDistanceCulling);

    EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (!(e.bitmask & (ConfigChangedEvent::Graphics | ConfigChangedEvent::All)))
            return;

        const auto& cfg = e.config;
        m_ShadowRenderer.SetEnableShadows(cfg.shadowsEnabled);
        m_ShadowRenderer.SetShadowMode(cfg.shadowMode);
        m_ShadowRenderer.SetShadowBias(cfg.shadowBias);
        m_ShadowRenderer.SetShadowSoftness(cfg.shadowSoftness);
        m_ShadowRenderer.SetShadowProjectionSize(cfg.shadowProjectionSize);
        m_ShadowRenderer.SetShadowFrustumCulling(cfg.shadowFrustumCullingEnabled);
        m_ShadowRenderer.SetShadowDistanceCulling(cfg.shadowDistanceCulling);

        if (cfg.shadowMapResolution != m_ShadowRenderer.GetShadow().GetShadowWidth())
        {
            auto& sl_inner = ServiceLocator::Instance();
            auto* context_inner = sl_inner.Resolve<IGraphicsContext>();
            if (context_inner)
            {
                m_ShadowRenderer.GetShadow().Initialize(*context_inner, cfg.shadowMapResolution,
                                                        cfg.shadowMapResolution);
            }
        }
    });
}

void ShadowSystem::Shutdown()
{
    m_ShadowRenderer.Shutdown();
}

void ShadowSystem::Render(Scene& scene)
{
    if (!m_Enabled || !m_ShadowRenderer.IsShadowsEnabled())
        return;

    auto& sl = ServiceLocator::Instance();
    auto* rs = sl.Resolve<IRenderService>();
    if (!rs)
        return;

    PrepareShadowLights(scene);

    RenderSceneData sceneData;
    sceneData.shadowQueue = rs->GetRenderQueueObj().GetShadowQueue();
    sceneData.lights = rs->GetRenderQueueObj().GetLights();
    sceneData.cameraPosition = rs->GetCameraPosition();
    sceneData.viewMatrix = rs->GetViewMatrix();
    sceneData.projMatrix = rs->GetProjectionMatrix();
    sceneData.nearPlane = rs->GetNearPlane();
    sceneData.farPlane = rs->GetFarPlane();

    m_ShadowRenderer.PerformShadowPass(sceneData);
}

void ShadowSystem::PrepareShadowLights(Scene& scene)
{
    if (!m_Enabled || !m_ShadowRenderer.IsShadowsEnabled())
        return;

    auto& sl = ServiceLocator::Instance();
    auto* rs = sl.Resolve<IRenderService>();
    if (!rs)
        return;

    auto& queueLights = rs->GetRenderQueueObj().GetLights();

    int dirShadowCount = 0;
    int pointShadowCount = 0;
    int spotShadowCount = 0;
    const int shadowMode = m_ShadowRenderer.GetShadowMode();
    const int maxDirectionalShadows =
        shadowMode == 2 ? Shadow::MAX_DIR_LIGHTS_SHADOW : (shadowMode == 1 ? 1 : 0);

    for (auto& rl : queueLights)
    {
        rl.shadowMapIndex = -1;

        if (!rl.castShadows)
            continue;
        if (shadowMode == 0)
            continue;

        if (rl.type == RenderLightType::Directional && dirShadowCount < maxDirectionalShadows)
        {
            float projSize = m_ShadowRenderer.GetShadowProjectionSize();
            glm::vec3 lightPos = -rl.direction * projSize;
            float nearPlane = 1.0f;
            float farPlane = projSize * 3.0f;
            glm::vec3 up = std::abs(glm::dot(rl.direction, glm::vec3(0, 1, 0))) > 0.99f
                               ? glm::vec3(0, 0, 1)
                               : glm::vec3(0, 1, 0);
            glm::mat4 lightProjection = glm::ortho(-projSize, projSize, -projSize, projSize, nearPlane, farPlane);
            rl.viewProj = lightProjection * glm::lookAt(lightPos, glm::vec3(0.0f), up);
            rl.shadowMapIndex = dirShadowCount++;
        }
        else if (rl.type == RenderLightType::Point && pointShadowCount < Shadow::MAX_POINT_LIGHTS_SHADOW)
        {
            rl.shadowMapIndex = pointShadowCount++;
        }
        else if (rl.type == RenderLightType::Spot && spotShadowCount < Shadow::MAX_SPOT_LIGHTS_SHADOW)
        {
            float fov = glm::acos(rl.outerCutoff) * 2.0f;
            if (fov <= 0.0f || !std::isfinite(fov))
                fov = glm::radians(45.0f);
            glm::mat4 spotProjection = glm::perspective(fov, 1.0f, 0.1f, m_ShadowRenderer.GetFarPlaneSpot());
            glm::vec3 up =
                std::abs(glm::dot(rl.direction, glm::vec3(0, 1, 0))) > 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
            rl.viewProj = spotProjection * glm::lookAt(rl.position, rl.position + rl.direction, up);
            rl.shadowMapIndex = spotShadowCount++;
        }
    }
}

std::vector<entt::id_type> ShadowSystem::GetReadComponents() const
{
    return {entt::type_id<MeshRendererComponent>().hash(),   entt::type_id<PositionComponent>().hash(),
            entt::type_id<RotationComponent>().hash(),       entt::type_id<ScaleComponent>().hash(),
            entt::type_id<WorldTransformComponent>().hash(), entt::type_id<GPUDirLight>().hash(),
            entt::type_id<GPUPointLight>().hash(),           entt::type_id<GPUSpotLight>().hash()};
}

std::vector<entt::id_type> ShadowSystem::GetWriteComponents() const
{
    return {};
}

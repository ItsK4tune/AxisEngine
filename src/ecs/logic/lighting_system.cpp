#include <ecs/logic/lighting_system.h>
#include <core/logic/logger.h>
#include <ecs/logic/system_factory.h>
#include <core/logic/service_locator.h>
#include <render/interface/i_graphics_context.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/light_probe_components.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <render/unit/gbuffer.h>
#include <render/unit/shadow.h>
#include <render/logic/shadow_renderer.h>
#include <render/logic/light_renderer.h>
#include <platform/logic/io_handler.h>
#include <ecs/interface/i_render_service.h>
#include <render/unit/render_queue.h>
#include <ecs/interface/i_geometry_service.h>
#include <ecs/interface/i_shadow_service.h>
#include <resource/logic/resource_manager.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <core/logic/config_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <scene/logic/scene.h>
#include <render/logic/render_core.h>
#include <resource/logic/shader_manager.h>
#include <engine/ecs/unit/light_probe_components.h>
#include <engine/ecs/unit/reflection_components.h>
#include <algorithm>
#include <string>

REGISTER_SYSTEM(LightingSystem)

void LightingSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<ILightingService>(this);
    sl.Register<LightingSystem>(this);
    m_GraphicsContext = sl.Resolve<IGraphicsContext>();
    m_RenderService = sl.Resolve<IRenderService>();
    m_GeoService = sl.Resolve<IGeometryService>();
    m_ShadowService = sl.Resolve<IShadowService>();

    auto& context = *m_GraphicsContext;
    auto& resources = sl.Require<ResourceManager>();

    m_LightRenderer.Initialize(context);
    
    m_DeferredLightShader = resources.GetShader("deferred_light");
}

void LightingSystem::Shutdown()
{
}

void LightingSystem::Render(Scene& scene)
{
    if (!m_Enabled) return;

    auto& sl = ServiceLocator::Instance();
    if (!m_GeoService) m_GeoService = sl.Resolve<IGeometryService>();
    if (!m_RenderService) m_RenderService = sl.Resolve<IRenderService>();
    if (!m_ShadowService) m_ShadowService = sl.Resolve<IShadowService>();

    auto* rs = m_RenderService;
    if (!rs) return;

    auto* io = sl.Resolve<IOHandler>();
    int width = io ? io->GetMonitorManager().GetWidth() : 800;
    int height = io ? io->GetMonitorManager().GetHeight() : 600;

    if (rs->GetRenderPath() == RenderPath::Forward) {
        RenderSceneData sceneData;
        sceneData.lights = rs->GetRenderQueueObj().GetLights();
        sceneData.cameraPosition = rs->GetCameraPosition();
        sceneData.viewMatrix = rs->GetViewMatrix();
        sceneData.projMatrix = rs->GetProjectionMatrix();
        sceneData.nearPlane = rs->GetNearPlane();
        sceneData.farPlane = rs->GetFarPlane();

        m_LightRenderer.UploadLightData(sceneData, nullptr);
        m_GraphicsContext->GetRenderStateManager().SetViewport(0, 0, width, height);
        return;
    }

    RenderDeferredLighting(scene, width, height);
}

void LightingSystem::RenderAlphaPass(Scene& scene, int width, int height, float alpha)
{
    Render(scene);
}

void LightingSystem::UploadLightData(const RenderSceneData& sceneData, Shader* shader)
{
    m_LightRenderer.UploadLightData(sceneData, shader);
}


void LightingSystem::RenderDeferredLighting(Scene& scene, int width, int height)
{
    if (!m_DeferredLightShader) return;

    auto& context = *m_GraphicsContext;
    auto& tm = context.GetTextureManager();
    auto& rsm = context.GetRenderStateManager();
    auto& dc = context.GetDrawContext();
    auto& rtm = context.GetRenderTargetManager();
    auto& bm = context.GetBufferManager();

    auto* geoSys = m_GeoService;
    if (!geoSys) return;
    auto& gBuffer = geoSys->GetGBuffer();

    auto* shadowSys = m_ShadowService;
    auto* rs = m_RenderService;
    uint32_t mainFBO = rs ? rs->GetMainFBO() : 0;
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);


    rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, gBuffer.GetFBO());
    rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, mainFBO);
    rtm.BlitFramebuffer(0, 0, gBuffer.GetScaledWidth(), gBuffer.GetScaledHeight(), 0, 0, width, height, BufferBit::Depth, TextureFilter::Nearest);
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);

    rsm.SetViewport(0, 0, width, height);
    rsm.Disable(ServerCapability::DepthTest);
    rsm.SetDepthMask(false);
    rsm.Disable(ServerCapability::CullFace);
    rsm.Disable(ServerCapability::Blend);

    RenderSceneData sceneData;
    sceneData.lights = rs->GetRenderQueueObj().GetLights();
    sceneData.cameraPosition = rs->GetCameraPosition();
    sceneData.viewMatrix = rs->GetViewMatrix();
    sceneData.projMatrix = rs->GetProjectionMatrix();
    sceneData.nearPlane = rs->GetNearPlane();
    sceneData.farPlane = rs->GetFarPlane();

    m_LightRenderer.UploadLightData(sceneData, m_DeferredLightShader.get());
    m_DeferredLightShader->use();
    

    tm.ActiveTexture(TextureUnit::Texture0);
    tm.BindTexture(TextureType::Texture2D, gBuffer.GetPositionTexture());
    m_DeferredLightShader->setInt("gPosition", 0);
    
    tm.ActiveTexture(TextureUnit::Texture1);
    tm.BindTexture(TextureType::Texture2D, gBuffer.GetNormalTexture());
    m_DeferredLightShader->setInt("gNormal", 1);
    
    tm.ActiveTexture(TextureUnit::Texture2);
    tm.BindTexture(TextureType::Texture2D, gBuffer.GetAlbedoSpecTexture());
    m_DeferredLightShader->setInt("gAlbedoSpec", 2);

    tm.ActiveTexture(TextureUnit::Texture3);
    tm.BindTexture(TextureType::Texture2D, gBuffer.GetIDTexture());
    m_DeferredLightShader->setInt("gID", 3);

    tm.ActiveTexture(TextureUnit::Texture4);
    tm.BindTexture(TextureType::Texture2D, gBuffer.GetEmissiveTexture());
    m_DeferredLightShader->setInt("gEmissive", 4);

    tm.ActiveTexture(TextureUnit::Texture5);
    tm.BindTexture(TextureType::Texture2D, gBuffer.GetPBRParamsTexture());
    m_DeferredLightShader->setInt("gPBRParams", 5);

    if (shadowSys) {
        bool enableShadows = shadowSys->GetRenderer().IsShadowsEnabled();
        if (enableShadows) {
            shadowSys->GetShadow().BindTexture_Dir(0, 10);
            shadowSys->GetShadow().BindTexture_Point(0, 11);
            shadowSys->GetShadow().BindTexture_Spot(0, 12);
            m_DeferredLightShader->setInt("shadowMapDir", 10);
            m_DeferredLightShader->setInt("shadowMapPoint", 11);
            m_DeferredLightShader->setInt("shadowMapSpot", 12);
        }
    }

    // Bind nearest reflection probe
    auto reflectionView = scene.registry.view<PositionComponent, ReflectionProbeComponent>();
    entt::entity nearestProbe = entt::null;
    float minDistanceSq = std::numeric_limits<float>::max();
    glm::vec3 camPos = ServiceLocator::Instance().Require<IRenderService>().GetCameraPosition();

    for (auto entity : reflectionView) {
        auto& pos = reflectionView.get<PositionComponent>(entity).value;
        float distSq = glm::distance2(pos, camPos);
        if (distSq < minDistanceSq) {
            minDistanceSq = distSq;
            nearestProbe = entity;
        }
    }

    if (nearestProbe != entt::null) {
        auto& probe = reflectionView.get<ReflectionProbeComponent>(nearestProbe);
        auto& pos = reflectionView.get<PositionComponent>(nearestProbe).value;
        
        tm.ActiveTexture(TextureUnit::Texture15);
        tm.BindTexture(TextureType::TextureCubeMap, probe.cubemapID);
        m_DeferredLightShader->setInt("reflectionProbe", 15);
        m_DeferredLightShader->setVec3("u_ProbePos", pos);
        m_DeferredLightShader->setVec3("u_ProbeBoxMin", pos + probe.boxMin);
        m_DeferredLightShader->setVec3("u_ProbeBoxMax", pos + probe.boxMax);
        m_DeferredLightShader->setBool("u_HasProbe", true);
    } else {
        m_DeferredLightShader->setBool("u_HasProbe", false);
    }

    // Bind nearest light probe
    auto lpView = scene.registry.view<PositionComponent, LightProbeComponent>();
    entt::entity nearestLP = entt::null;
    minDistanceSq = std::numeric_limits<float>::max();

    for (auto entity : lpView) {
        auto& pos = lpView.get<PositionComponent>(entity).value;
        float distSq = glm::distance2(pos, camPos);
        if (distSq < minDistanceSq) {
            minDistanceSq = distSq;
            nearestLP = entity;
        }
    }

    if (nearestLP != entt::null) {
        auto& lp = lpView.get<LightProbeComponent>(nearestLP);
        m_DeferredLightShader->setVec3Array("u_SH", &lp.sh[0], 9);
        m_DeferredLightShader->setFloat("u_LightProbeIntensity", lp.intensity);
        m_DeferredLightShader->setBool("u_HasLightProbe", true);
    } else {
        m_DeferredLightShader->setBool("u_HasLightProbe", false);
    }

    auto& core = ServiceLocator::Instance().Require<RenderCore>();
    bm.BindVertexArray(core.GetQuadVAO());
    dc.DrawArrays(Primitive::TriangleStrip, 0, 4);
    bm.BindVertexArray(0);

    rsm.SetDepthMask(true);
    rsm.Enable(ServerCapability::DepthTest);
}


std::vector<entt::id_type> LightingSystem::GetReadComponents() const
{
    return {
        entt::type_id<GPUDirLight>().hash(),
        entt::type_id<GPUPointLight>().hash(),
        entt::type_id<GPUSpotLight>().hash(),
        entt::type_id<ReflectionProbeComponent>().hash(),
        entt::type_id<LightProbeComponent>().hash()
    };
}

std::vector<entt::id_type> LightingSystem::GetWriteComponents() const
{
    return {};
}

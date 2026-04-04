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
    auto& rsm = context.GetRenderStateManager();
    auto& dc = context.GetDrawContext();
    auto& rtm = context.GetRenderTargetManager();
    auto& bm = context.GetBufferManager();

    // Clear main FBO before lighting pass if needed, though we usually overwrite
    auto* rs = m_RenderService;
    uint32_t mainFBO = rs ? rs->GetMainFBO() : 0;
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);
    // rtm.Clear(BufferBit::Color); // Usually not needed as we fill fragments

    auto& tm = context.GetTextureManager();
    auto* geoSys = m_GeoService;
    auto* shadowSys = m_ShadowService;
    if (!geoSys) return;
    auto& gBuffer = geoSys->GetGBuffer();

    rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, gBuffer.GetFBO());
    rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, mainFBO);
    rtm.BlitFramebuffer(0, 0, gBuffer.GetScaledWidth(), gBuffer.GetScaledHeight(), 0, 0, width, height, BufferBit::Depth, TextureFilter::Nearest);
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);
    
    context.SetViewport(0, 0, width, height);

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

    tm.ActiveTexture(TextureUnit::Texture6);
    tm.BindTexture(TextureType::Texture2D, gBuffer.GetDepthTexture());
    m_DeferredLightShader->setInt("gDepth", 6);

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

    // 1. Separate Global and Local probes
    struct ProbeEntry {
        entt::entity entity;
        float distSq;
        glm::vec3 pos;
        float volume;
    };
    std::vector<ProbeEntry> allProbes;
    auto reflectionView = scene.registry.view<PositionComponent, ReflectionProbeComponent>();
    glm::vec3 camPos = rs->GetCameraPosition();

    ProbeEntry globalProbe = { entt::null, 0.0f, glm::vec3(0.0f), -1.0f };

    for (auto entity : reflectionView) {
        auto& pos = reflectionView.get<PositionComponent>(entity).value;
        auto& pr = reflectionView.get<ReflectionProbeComponent>(entity);
        float vol = (pr.boxMax.x - pr.boxMin.x) * (pr.boxMax.y - pr.boxMin.y) * (pr.boxMax.z - pr.boxMin.z);
        
        allProbes.push_back({entity, glm::distance2(pos, camPos), pos, vol});
        
        if (vol > globalProbe.volume) {
            globalProbe = allProbes.back();
        }
    }

    // 2. Filter out global from locals and sort locals by distance
    std::vector<ProbeEntry> localProbes;
    for (auto& p : allProbes) {
        if (p.entity != globalProbe.entity) {
            localProbes.push_back(p);
        }
    }

    std::sort(localProbes.begin(), localProbes.end(), [](const ProbeEntry& a, const ProbeEntry& b) {
        return a.distSq < b.distSq;
    });

    // 3. Assemble final list: [0] is Global, [1-3] are Locals
    std::vector<ProbeEntry> finalProbes;
    if (globalProbe.entity != entt::null) finalProbes.push_back(globalProbe);
    for (int i = 0; i < localProbes.size() && finalProbes.size() < 4; ++i) {
        finalProbes.push_back(localProbes[i]);
    }

    int probeCount = (int)finalProbes.size();
    m_DeferredLightShader->setInt("u_ProbeCount", probeCount);

    for (int i = 0; i < probeCount; ++i) {
        auto entity = finalProbes[i].entity;
        auto& probe = reflectionView.get<ReflectionProbeComponent>(entity);
        auto& pos = finalProbes[i].pos;

        tm.ActiveTexture(static_cast<TextureUnit>(static_cast<int>(TextureUnit::Texture15) + i));
        tm.BindTexture(TextureType::TextureCubeMap, probe.cubemapID);
        
        std::string base = "u_Probes[" + std::to_string(i) + "].";
        m_DeferredLightShader->setInt("reflectionProbes[" + std::to_string(i) + "]", 15 + i);
        m_DeferredLightShader->setVec3(base + "pos", pos);
        m_DeferredLightShader->setVec3(base + "boxMin", pos + probe.boxMin);
        m_DeferredLightShader->setVec3(base + "boxMax", pos + probe.boxMax);
        m_DeferredLightShader->setFloat(base + "blendDistance", probe.blendDistance);
        m_DeferredLightShader->setBool(base + "boxProjection", probe.boxProjection);
    }

    // Bind nearest light probe
    auto lpView = scene.registry.view<PositionComponent, LightProbeComponent>();
    entt::entity nearestLP = entt::null;
    float minDistanceSq = std::numeric_limits<float>::max();

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

    auto planarView = scene.registry.view<PlanarReflectionComponent>();
    uint32_t planarTex = 0;
    for (auto entity : planarView) {
        auto& prc = planarView.get<PlanarReflectionComponent>(entity);
        if (prc.reflectionTextureID) {
            planarTex = prc.reflectionTextureID;
            break;
        }
    }

    if (planarTex > 0) {
        tm.ActiveTexture(TextureUnit::Texture19);
        tm.BindTexture(TextureType::Texture2D, planarTex);
        m_DeferredLightShader->setInt("u_PlanarReflection", 19);
        m_DeferredLightShader->setBool("u_HasPlanar", true);
        
        auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
        int w = io ? io->GetMonitorManager().GetWidth() : 800;
        int h = io ? io->GetMonitorManager().GetHeight() : 600;
        m_DeferredLightShader->setVec2("u_ScreenSize", glm::vec2(w, h));
    } else {
        m_DeferredLightShader->setBool("u_HasPlanar", false);
    }

    auto& core = ServiceLocator::Instance().Require<RenderCore>();
    bm.BindVertexArray(core.GetQuadVAO());
    dc.DrawArrays(Primitive::TriangleStrip, 0, 4);
    bm.BindVertexArray(0);

    // Unbind reflection probes to prevent texture leaking to subsequent passes (e.g. Transparent Pass)
    for (int i = 0; i < 4; ++i) {
        tm.ActiveTexture(static_cast<TextureUnit>(static_cast<int>(TextureUnit::Texture15) + i));
        tm.BindTexture(TextureType::TextureCubeMap, 0);
    }

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

#include <ecs/logic/lighting_system.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_geometry_service.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_shadow_service.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/light_probe_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/render_components.h>
#include <platform/logic/io_handler.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/logic/light_renderer.h>
#include <render/logic/render_core.h>
#include <render/logic/shadow_renderer.h>
#include <render/unit/gbuffer.h>
#include <render/unit/render_queue.h>
#include <render/unit/shadow.h>
#include <resource/logic/resource_manager.h>
#include <resource/logic/shader_manager.h>
#include <scene/logic/scene.h>
#include <algorithm>
#include <limits>
#include <string>
#include <glm/gtx/norm.hpp>


void LightingSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<ILightingService>(this);
    sl.Register<LightingSystem>(this);
    m_GraphicsContext = sl.Resolve<IGraphicsContext>();
    m_RenderService = sl.Resolve<IRenderService>();
    m_GeoService = sl.Resolve<IGeometryService>();
    m_ShadowService = sl.Resolve<IShadowService>();

    if (!m_GraphicsContext)
    {
        LOGGER_WARN("LightingSystem") << "Skipping full initialization (missing GraphicsContext)";
        return;
    }

    auto* resources = sl.Resolve<ResourceManager>();
    if (!resources)
        return;

    m_LightRenderer.Initialize(*m_GraphicsContext);

    m_DeferredLightShader = resources->GetShader("deferred_light");
}

void LightingSystem::Render(Scene& scene)
{
    if (!m_Enabled)
        return;

    auto& sl = ServiceLocator::Instance();
    if (!m_GeoService)
        m_GeoService = sl.Resolve<IGeometryService>();
    if (!m_RenderService)
        m_RenderService = sl.Resolve<IRenderService>();
    if (!m_ShadowService)
        m_ShadowService = sl.Resolve<IShadowService>();

    auto* rs = m_RenderService;
    if (!rs)
        return;

    auto* io = sl.Resolve<IOHandler>();
    int width = io ? io->GetMonitorManager().GetWidth() : 800;
    int height = io ? io->GetMonitorManager().GetHeight() : 600;

    RenderDeferredLighting(scene, width, height);
}

void LightingSystem::UploadLightData(const RenderSceneData& sceneData, Shader* shader)
{
    m_LightRenderer.UploadLightData(sceneData, shader);
}

void LightingSystem::RenderDeferredLighting(Scene& scene, int width, int height)
{
    if (!m_DeferredLightShader || !m_GraphicsContext)
        return;

    auto& context = *m_GraphicsContext;
    auto& rsm = context.GetRenderStateManager();
    auto& dc = context.GetDrawContext();
    auto& rtm = context.GetRenderTargetManager();
    auto& bm = context.GetBufferManager();

    auto* rs = m_RenderService;
    uint32_t mainFBO = rs ? rs->GetMainFBO() : 0;
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);

    auto& tm = context.GetTextureManager();
    auto* geoSys = m_GeoService;
    auto* shadowSys = m_ShadowService;
    if (!geoSys)
        return;
    if (!geoSys->IsDeferredRenderingEnabled())
        return;
    auto& gBuffer = geoSys->GetGBuffer();

    rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, gBuffer.GetFBO());
    rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, mainFBO);
    rtm.BlitFramebuffer(0, 0, gBuffer.GetScaledWidth(), gBuffer.GetScaledHeight(), 0, 0, width, height,
                        BufferBit::Depth, TextureFilter::Nearest);
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);

    context.SetViewport(0, 0, width, height);

    rsm.SetViewport(0, 0, width, height);
    rsm.Disable(ServerCapability::DepthTest);
    rsm.SetDepthMask(false);
    rsm.Disable(ServerCapability::CullFace);
    rsm.Disable(ServerCapability::Blend);

    RenderSceneData sceneData;
    sceneData.lightView = &rs->GetRenderQueueObj().GetLights();
    sceneData.cameraPosition = rs->GetCameraPosition();
    sceneData.viewMatrix = rs->GetViewMatrix();
    sceneData.projMatrix = rs->GetProjectionMatrix();
    sceneData.nearPlane = rs->GetNearPlane();
    sceneData.farPlane = rs->GetFarPlane();
    sceneData.viewportWidth = width;
    sceneData.viewportHeight = height;

    m_LightRenderer.UploadLightData(sceneData, m_DeferredLightShader.get());
    m_DeferredLightShader->use();
    m_LightRenderer.ConfigureDeferredShader(*m_DeferredLightShader);

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

    if (shadowSys)
    {
        bool enableShadows = shadowSys->GetRenderer().IsShadowsEnabled();
        if (enableShadows)
        {
            shadowSys->GetShadow().BindTexture_Dir(0, 10);
            shadowSys->GetShadow().BindTexture_Point(0, 11);
            shadowSys->GetShadow().BindTexture_Spot(0, 12);
            m_DeferredLightShader->setInt("u_ShadowMapDir", 10);
            m_DeferredLightShader->setInt("u_ShadowMapPoint", 11);
            m_DeferredLightShader->setInt("u_ShadowMapSpot", 12);
            m_DeferredLightShader->setFloat("u_ShadowBias", shadowSys->GetRenderer().GetShadowBias());
            m_DeferredLightShader->setInt("u_ShadowSoftness", shadowSys->GetRenderer().GetShadowSoftness());
        }
    }

    // Queue construction already snapshots probes; consume it here instead of
    // scanning the ECS a second time during deferred lighting.
    const auto& reflectionProbes = rs->GetRenderQueueObj().GetReflectionProbes();
    glm::vec3 camPos = rs->GetCameraPosition();

    int probeCount = 0;
    for (const auto& renderProbe : reflectionProbes)
        if (renderProbe.gpuIndex >= 0)
            probeCount = (std::max)(probeCount, renderProbe.gpuIndex + 1);

    m_DeferredLightShader->setInt("u_ProbeCount", probeCount);

    for (const auto& renderProbe : reflectionProbes)
    {
        const int i = renderProbe.gpuIndex;
        if (i < 0 || i >= 4 || !renderProbe.component)
            continue;
        auto& probe = *renderProbe.component;
        const auto& pos = renderProbe.position;

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
    const auto& lightProbes = rs->GetRenderQueueObj().GetLightProbes();
    const RenderLightProbe* nearestLP = nullptr;
    float minDistanceSq = std::numeric_limits<float>::max();

    for (const auto& probe : lightProbes)
    {
        float distSq = glm::distance2(probe.position, camPos);
        if (distSq < minDistanceSq)
        {
            minDistanceSq = distSq;
            nearestLP = &probe;
        }
    }

    if (nearestLP)
    {
        m_DeferredLightShader->setVec3Array("u_SH", nearestLP->coefficients, 9);
        m_DeferredLightShader->setFloat("u_LightProbeIntensity", nearestLP->intensity);
        m_DeferredLightShader->setVec3("u_LightProbeTint", nearestLP->tint);
        m_DeferredLightShader->setBool("u_HasLightProbe", true);
    }
    else
    {
        m_DeferredLightShader->setBool("u_HasLightProbe", false);
    }

    const auto& planarReflections = rs->GetRenderQueueObj().GetPlanarReflections();
    const int planarCount = static_cast<int>((std::min)(planarReflections.size(), size_t{4}));

    m_DeferredLightShader->setInt("u_PlanarCount", planarCount);
    if (planarCount > 0)
    {
        for (int i = 0; i < planarCount; ++i)
        {
            tm.ActiveTexture(static_cast<TextureUnit>(static_cast<int>(TextureUnit::Texture19) + i));
            tm.BindTexture(TextureType::Texture2D, planarReflections[static_cast<size_t>(i)].textureId);

            std::string texBase = "u_PlanarReflections[" + std::to_string(i) + "]";
            std::string normBase = "u_PlanarNormals[" + std::to_string(i) + "]";
            m_DeferredLightShader->setInt(texBase, 19 + i);
            m_DeferredLightShader->setVec3(normBase, planarReflections[static_cast<size_t>(i)].normal);
        }
    }

    auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    int w = io ? io->GetMonitorManager().GetWidth() : 800;
    int h = io ? io->GetMonitorManager().GetHeight() : 600;
    m_DeferredLightShader->setVec2("u_ScreenSize", glm::vec2(w, h));

    auto* core = ServiceLocator::Instance().Resolve<RenderCore>();
    if (core)
    {
        bm.BindVertexArray(core->GetQuadVAO());
        dc.DrawArrays(Primitive::TriangleStrip, 0, 4);
        bm.BindVertexArray(0);
    }

    // Unbind G-Buffer and reflection probes to prevent texture leaking to subsequent passes (e.g. Transparent Pass)
    for (int i = 0; i <= 6; ++i)
    {
        tm.ActiveTexture(static_cast<TextureUnit>(static_cast<int>(TextureUnit::Texture0) + i));
        tm.BindTexture(TextureType::Texture2D, 0);
    }
    for (int i = 0; i < 4; ++i)
    {
        tm.ActiveTexture(static_cast<TextureUnit>(static_cast<int>(TextureUnit::Texture15) + i));
        tm.BindTexture(TextureType::TextureCubeMap, 0);
    }
    if (core)
    {
        core->GetMaterialRenderer().ResetTextureState();
    }

    rsm.SetDepthMask(true);
    auto* cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    const auto config = cm ? cm->GetConfigSnapshot() : nullptr;
    if (config && !config->culling.depthTestEnabled)
    {
        rsm.Disable(ServerCapability::DepthTest);
    }
    else
    {
        rsm.Enable(ServerCapability::DepthTest);
    }
}

void LightingSystem::ApplyOptimizationConfig(const OptimizationConfig& config)
{
    m_LightRenderer.SetTiledLightCulling(config.tiledLightCullingEnabled, config.tiledLightTileSize);
}

std::vector<entt::id_type> LightingSystem::GetReadComponents() const
{
    return {entt::type_id<GPUDirLight>().hash(), entt::type_id<GPUPointLight>().hash(),
            entt::type_id<GPUSpotLight>().hash(), entt::type_id<ReflectionProbeComponent>().hash(),
            entt::type_id<LightProbeComponent>().hash()};
}

std::vector<entt::id_type> LightingSystem::GetWriteComponents() const
{
    return {};
}

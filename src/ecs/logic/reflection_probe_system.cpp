#include <ecs/logic/reflection_probe_system.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <ecs/interface/i_lighting_service.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_skybox_service.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/reflection_components.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/logic/render_core.h>
#include <render/type/render_view_params.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <glm/gtc/matrix_transform.hpp>

#include <core/logic/logger.h>
#include <ecs/interface/i_shadow_service.h>
#include <ecs/logic/system_factory.h>
#include <render/logic/shadow_renderer.h>
#include <render/unit/render_queue.h>
#include <glm/gtx/norm.hpp>
#include <algorithm>


void ReflectionProbeSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    auto* context = sl.Resolve<IGraphicsContext>();
    if (!context)
    {
        LOGGER_WARN("ReflectionProbeSystem") << "Skipping full initialization (missing GraphicsContext)";
        return;
    }

    auto& rtm = context->GetRenderTargetManager();

    m_CaptureFBO = rtm.GenFramebuffer();
    m_DepthRB = rtm.CreateRenderbuffer();

    LOGGER_INFO("ReflectionProbeSystem") << "Initialized with FBO: " << m_CaptureFBO;
}

void ReflectionProbeSystem::Shutdown()
{
    Reset();
    auto& sl = ServiceLocator::Instance();
    auto* context = sl.Resolve<IGraphicsContext>();
    if (context)
    {
        auto& rtm = context->GetRenderTargetManager();
        if (m_CaptureFBO)
            rtm.DeleteFramebuffer(m_CaptureFBO);
        if (m_DepthRB)
            rtm.DeleteRenderbuffer(m_DepthRB);
    }
    m_CaptureFBO = 0;
    m_DepthRB = 0;
    m_DepthResolution = 0;
    m_NextProbeIndex = 0;
    m_Candidates.clear();
}

void ReflectionProbeSystem::ApplyOptimizationConfig(const OptimizationConfig& config)
{
    SetCaptureBudget(config.reflectionCaptureBudgetEnabled,
                     static_cast<size_t>(config.maxReflectionProbeFacesPerFrame));
}

void ReflectionProbeSystem::Reset()
{
    auto& sl = ServiceLocator::Instance();
    auto* context = sl.Resolve<IGraphicsContext>();
    auto* scene = sl.Resolve<Scene>();
    if (!context || !scene)
        return;

    auto& tm = context->GetTextureManager();
    auto view = scene->View<ReflectionProbeComponent>();
    for (auto entity : view)
    {
        auto& probe = view.get<ReflectionProbeComponent>(entity);
        if (probe.cubemapID)
            tm.DeleteTexture(probe.cubemapID);
        probe.cubemapID = 0;
        probe.lastResolution = 0;
        probe.lastGpuIndex = -1;
        probe.isDirty = true;
        probe.currentFace = 0;
    }
}

void ReflectionProbeSystem::RenderCapturePass(Scene& scene, int width, int height)
{
    auto view = scene.View<PositionComponent, ReflectionProbeComponent>();
    m_Candidates.clear();
    m_Candidates.reserve(view.size_hint());
    for (auto entity : view)
    {
        auto& probe = view.get<ReflectionProbeComponent>(entity);
        if (probe.isDirty || probe.type == ReflectionProbeType::Dynamic)
            m_Candidates.push_back(entity);
    }

    if (m_Candidates.empty())
    {
        m_NextProbeIndex = 0;
        return;
    }

    const size_t captureCount =
        m_CaptureBudgetEnabled ? m_MaxFacesPerFrame : m_Candidates.size() * size_t{6};
    const size_t first = m_NextProbeIndex % m_Candidates.size();
    for (size_t offset = 0; offset < captureCount; ++offset)
    {
        const entt::entity entity = m_Candidates[(first + offset) % m_Candidates.size()];
        auto& probe = view.get<ReflectionProbeComponent>(entity);
        if (probe.type == ReflectionProbeType::Static && !probe.isDirty)
            continue;
        if (probe.lastResolution != probe.resolution)
            probe.currentFace = 0;

        if (!CaptureProbe(scene, entity, static_cast<int>(probe.currentFace), width, height))
            continue;
        probe.currentFace = (probe.currentFace + 1) % 6;
        if (probe.type == ReflectionProbeType::Static && probe.currentFace == 0)
            probe.isDirty = false;
    }
    m_NextProbeIndex = (first + captureCount) % m_Candidates.size();
}

unsigned int ReflectionProbeSystem::CreateCubemap(int resolution)
{
    auto& sl = ServiceLocator::Instance();
    auto* context = sl.Resolve<IGraphicsContext>();
    if (!context)
        return 0;
    auto& tm = context->GetTextureManager();

    unsigned int id = tm.GenTexture();
    tm.BindTexture(TextureType::TextureCubeMap, id);

    for (unsigned int i = 0; i < 6; ++i)
    {
        tm.TexImage2D((TextureType)((int)TextureType::CubeMapPositiveX + i), 0, InternalFormat::RGBA16F, resolution,
                      resolution, 0, TextureFormat::RGBA, DataType::Float, nullptr);
    }

    tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::MinFilter, (int)TextureFilter::LinearMipmapLinear);
    tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::MagFilter, (int)TextureFilter::Linear);
    tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::WrapS, (int)TextureWrap::ClampToEdge);
    tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::WrapT, (int)TextureWrap::ClampToEdge);
    tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::WrapR, (int)TextureWrap::ClampToEdge);

    tm.GenerateMipmap(TextureType::TextureCubeMap);
    tm.BindTexture(TextureType::TextureCubeMap, 0);

    return id;
}

bool ReflectionProbeSystem::CaptureProbe(Scene& scene, entt::entity entity, int faceIndex, int viewportWidth,
                                         int viewportHeight)
{
    auto& sl = ServiceLocator::Instance();
    auto* context_ptr = sl.Resolve<IGraphicsContext>();
    auto* renderService_ptr = sl.Resolve<IRenderService>();
    if (!context_ptr || !renderService_ptr)
        return false;

    auto& context = *context_ptr;
    auto& rtm = context.GetRenderTargetManager();
    auto& tm = context.GetTextureManager();
    auto& renderService = *renderService_ptr;

    auto& pos = scene.GetComponent<PositionComponent>(entity).value;
    auto& probe = scene.GetComponent<ReflectionProbeComponent>(entity);
    probe.resolution = glm::clamp(probe.resolution, 16, 4096);

    if (probe.cubemapID == 0 || probe.lastResolution != probe.resolution)
    {
        // Resolution changed or first allocation — (re)create cubemap at correct size
        if (probe.cubemapID != 0)
        {
            tm.DeleteTexture(probe.cubemapID);
            probe.cubemapID = 0;
        }
        probe.cubemapID = CreateCubemap(probe.resolution);
        probe.lastResolution = probe.resolution;
        probe.isDirty = true;
    }

    rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, m_CaptureFBO);
    rtm.BindRenderbuffer(m_DepthRB);
    if (m_DepthResolution != probe.resolution)
    {
        rtm.RenderbufferStorage(InternalFormat::DepthComponent24, probe.resolution, probe.resolution);
        m_DepthResolution = probe.resolution;
    }
    rtm.FramebufferRenderbuffer(FramebufferTarget::DrawFramebuffer, FramebufferAttachment::Depth, m_DepthRB);

    auto* lightingService = sl.Resolve<ILightingService>();

    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);
    glm::mat4 views[] = {glm::lookAt(pos, pos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)),
                         glm::lookAt(pos, pos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)),
                         glm::lookAt(pos, pos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
                         glm::lookAt(pos, pos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)),
                         glm::lookAt(pos, pos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)),
                         glm::lookAt(pos, pos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0))};

    uint32_t oldFBO = renderService.GetMainFBO();
    // Save Camera State to prevent POV Jumps
    glm::vec3 originalCamPos = renderService.GetCameraPosition();
    glm::mat4 originalView = renderService.GetViewMatrix();
    glm::mat4 originalProj = renderService.GetProjectionMatrix();
    float originalNear = renderService.GetNearPlane();
    float originalFar = renderService.GetFarPlane();

    context.SetViewport(0, 0, probe.resolution, probe.resolution);

    // Capture specific face
    rtm.FramebufferTexture2D(FramebufferTarget::DrawFramebuffer, FramebufferAttachment::Color0,
                             (TextureType)((int)TextureType::CubeMapPositiveX + faceIndex), probe.cubemapID, 0);

    context.Clear(BufferBit::Color | BufferBit::Depth);

    // 1. Build queues from probe perspective (updates UBO)
    RenderViewParams probeParams;
    probeParams.view = views[faceIndex];
    probeParams.projection = proj;
    probeParams.cameraPos = pos;
    probeParams.nearPlane = 0.1f;
    probeParams.farPlane = 1000.0f;
    probeParams.lodFactor = 1.0f;
    probeParams.width = probe.resolution;
    probeParams.height = probe.resolution;
    probeParams.cullingMask = 0xFFFFFFFF;
    probeParams.isCapturingProbe = true;
    probeParams.excludeEntity = entity;
    renderService.PushRenderViewContext();
    renderService.BuildRenderQueuesWithCamera(scene, probeParams);

    // 2. Render Skybox
    if (auto* skyService = sl.Resolve<ISkyboxService>())
    {
        skyService->RenderAlphaPassWithCamera(scene, views[faceIndex], proj, probe.resolution, probe.resolution,
                                              m_CaptureFBO);
    }

    // 3. Update lighting for this face
    if (lightingService)
    {
        RenderSceneData sceneData;
        sceneData.viewMatrix = views[faceIndex];
        sceneData.projMatrix = proj;
        sceneData.cameraPosition = pos;
        sceneData.lightView = &renderService.GetRenderQueueObj().GetLights();
        lightingService->UploadLightData(sceneData);
    }

    // 4. Render the scene
    auto& defOpaque = renderService.GetRenderQueueObj().GetDeferredOpaqueQueue();
    auto& fwdOpaque = renderService.GetRenderQueueObj().GetForwardOpaqueQueue();
    auto& transparent = renderService.GetRenderQueueObj().GetTransparentQueue();

    auto* core = sl.Resolve<RenderCore>();
    ShadowRenderer* shadowRenderer = nullptr;

    // Final Render Queue Execution
    if (core)
    {
        renderService.ExecuteQueue(defOpaque, RenderQueuePass::DeferredGeometry, shadowRenderer,
                                   &core->GetMaterialRenderer(), nullptr);
        renderService.ExecuteQueue(fwdOpaque, RenderQueuePass::ForwardOpaque, shadowRenderer,
                                   &core->GetMaterialRenderer(), nullptr);
        renderService.ExecuteQueue(transparent, RenderQueuePass::Transparent, shadowRenderer,
                                   &core->GetMaterialRenderer());
    }
    renderService.PopRenderViewContext();

    // Generate mipmaps only after full update or on every few faces
    if (faceIndex == 5)
    {
        tm.BindTexture(TextureType::TextureCubeMap, probe.cubemapID);
        tm.GenerateMipmap(TextureType::TextureCubeMap);
        tm.BindTexture(TextureType::TextureCubeMap, 0);
    }

    rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, oldFBO);
    // Restore original viewport
    context.SetViewport(0, 0, viewportWidth, viewportHeight);

    // RESTORE CAMERA STATE (UBO only, avoid rebuilt queue overhead)
    GPUCameraData restoreCam;
    std::memcpy(restoreCam.projection, &originalProj[0][0], 16 * sizeof(float));
    std::memcpy(restoreCam.view, &originalView[0][0], 16 * sizeof(float));
    std::memcpy(restoreCam.viewPos, &originalCamPos[0], 3 * sizeof(float));
    restoreCam.viewPos[3] = 1.0f;
    glm::mat4 rInvProj = glm::inverse(originalProj);
    glm::mat4 rInvView = glm::inverse(originalView);
    std::memcpy(restoreCam.invProjection, &rInvProj[0][0], 16 * sizeof(float));
    std::memcpy(restoreCam.invView, &rInvView[0][0], 16 * sizeof(float));
    std::memcpy(restoreCam.stableProjection, &originalProj[0][0], 16 * sizeof(float));
    std::memcpy(restoreCam.invStableProjection, &rInvProj[0][0], 16 * sizeof(float));

    // Upload original camera state back to GPU
    renderService.UploadCameraUBO(restoreCam);

    // Also restore the internal camera variables on the CPU side to prevent drifting/jittering
    renderService.RestoreCameraState(originalView, originalProj, originalCamPos, originalNear, originalFar);
    return true;
}

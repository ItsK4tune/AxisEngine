#include <ecs/logic/planar_reflection_system.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_skybox_service.h>
#include <ecs/interface/i_lighting_service.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/reflection_components.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/type/render_view_params.h>
#include <render/unit/render_queue.h>
#include <scene/logic/scene_manager.h>
#include <glm/gtc/matrix_transform.hpp>

REGISTER_SYSTEM(PlanarReflectionSystem)

void PlanarReflectionSystem::Initialize()
{
    m_Context = ServiceLocator::Instance().Resolve<IGraphicsContext>();
    m_RenderService = ServiceLocator::Instance().Resolve<IRenderService>();
}

void PlanarReflectionSystem::Shutdown()
{
    if (!m_Context || !m_Context->SupportsLegacyRenderPipeline())
        return;

    auto& rtm = m_Context->GetRenderTargetManager();
    auto& tm = m_Context->GetTextureManager();

    // Get the global scene/registry to clean up resources
    auto& sl = ServiceLocator::Instance();
    auto& scene = sl.Require<Scene>();
    auto view = scene.View<PlanarReflectionComponent>();
    for (auto entity : view)
    {
        auto& comp = view.get<PlanarReflectionComponent>(entity);
        if (comp.reflectionFBO)
            rtm.DeleteFramebuffer(comp.reflectionFBO);
        if (comp.reflectionTextureID)
            tm.DeleteTexture(comp.reflectionTextureID);
        comp.reflectionFBO = 0;
        comp.reflectionTextureID = 0;
    }
}

void PlanarReflectionSystem::Render(Scene& scene)
{
    if (!m_Enabled || !m_Context || !m_Context->SupportsLegacyRenderPipeline() || !m_RenderService)
        return;

    auto view = scene.View<PlanarReflectionComponent, PositionComponent, RotationComponent>();
    if (view.begin() == view.end())
        return;

    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity == entt::null)
        return;

    auto& cam = scene.GetComponent<CameraComponent>(camEntity);
    auto* camPosComp = scene.TryGetComponent<PositionComponent>(camEntity);
    if (!camPosComp)
        return;
    glm::vec3 camPos = camPosComp->value;

    auto& rtm = m_Context->GetRenderTargetManager();
    auto& rsm = m_Context->GetRenderStateManager();
    auto& tm = m_Context->GetTextureManager();
    auto& dc = m_Context->GetDrawContext();

    uint32_t currentFBO = m_RenderService->GetMainFBO();
    const glm::mat4 savedView = m_RenderService->GetViewMatrix();
    const glm::mat4 savedProjection = m_RenderService->GetProjectionMatrix();
    const glm::vec3 savedCameraPos = m_RenderService->GetCameraPosition();

    int screenWidth = m_RenderService->GetLastWidth();
    int screenHeight = m_RenderService->GetLastHeight();
    if (screenWidth <= 0 || screenHeight <= 0)
        return;

    for (auto entity : view)
    {
        auto& prc = view.get<PlanarReflectionComponent>(entity);
        auto& pos = view.get<PositionComponent>(entity);
        auto& rot = view.get<RotationComponent>(entity);

        // Dynamic resizing to match screen aspect ratio and resolution
        if (prc.reflectionFBO == 0 || prc.resolution != (uint32_t)screenWidth ||
            prc.resolution_y != (uint32_t)screenHeight)
        {
            if (prc.reflectionFBO != 0)
            {
                rtm.DeleteFramebuffer(prc.reflectionFBO);
                tm.DeleteTexture(prc.reflectionTextureID);
            }

            prc.resolution = screenWidth;
            prc.resolution_y = screenHeight;

            prc.reflectionTextureID = tm.GenTexture();
            tm.BindTexture(TextureType::Texture2D, prc.reflectionTextureID);
            tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, prc.resolution, prc.resolution_y, 0,
                          TextureFormat::RGBA, DataType::Float, nullptr);

            tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, (int)TextureWrap::ClampToEdge);
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, (int)TextureWrap::ClampToEdge);
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::Linear);
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, (int)TextureFilter::Linear);

            uint32_t rbo = rtm.CreateRenderbuffer();
            rtm.BindRenderbuffer(rbo);
            rtm.RenderbufferStorage(InternalFormat::DepthComponent24, prc.resolution, prc.resolution_y);

            prc.reflectionFBO = rtm.CreateFramebuffer();
            rtm.BindFramebuffer(FramebufferTarget::Framebuffer, prc.reflectionFBO);
            rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color0,
                                     TextureType::Texture2D, prc.reflectionTextureID, 0);
            rtm.FramebufferRenderbuffer(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth, rbo);
            rtm.BindFramebuffer(FramebufferTarget::Framebuffer, currentFBO);
            prc.isDirty = false;
        }

        glm::vec3 normal = glm::mat3_cast(rot.value) * glm::vec3(0, 1, 0);
        prc.normal = normal;
        float d = -glm::dot(normal, pos.value);

        // Reflect camera pos
        glm::vec3 refCamPos = camPos - 2.0f * (glm::dot(normal, camPos) + d) * normal;

        // Reflected view matrix
        glm::mat4 reflectionMat = glm::mat4(1.0f);
        reflectionMat[0][0] = 1 - 2 * normal.x * normal.x;
        reflectionMat[0][1] = -2 * normal.x * normal.y;
        reflectionMat[0][2] = -2 * normal.x * normal.z;
        reflectionMat[1][0] = -2 * normal.y * normal.x;
        reflectionMat[1][1] = 1 - 2 * normal.y * normal.y;
        reflectionMat[1][2] = -2 * normal.y * normal.z;
        reflectionMat[2][0] = -2 * normal.z * normal.x;
        reflectionMat[2][1] = -2 * normal.z * normal.y;
        reflectionMat[2][2] = 1 - 2 * normal.z * normal.z;
        reflectionMat[3][0] = -2 * d * normal.x;
        reflectionMat[3][1] = -2 * d * normal.y;
        reflectionMat[3][2] = -2 * d * normal.z;

        glm::mat4 refViewMat = cam.viewMatrix * reflectionMat;

        uint32_t tempFB = m_RenderService->GetMainFBO();
        m_RenderService->SetMainFBO(prc.reflectionFBO);

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, prc.reflectionFBO);
        rsm.SetViewport(0, 0, prc.resolution, prc.resolution_y);
        dc.ClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        dc.Clear(BufferBit::Color | BufferBit::Depth);

        rsm.SetCullFace(CullMode::Front);  // Reverse winding since view is mirrored

        // 3. Calculate Oblique Projection Matrix to clip everything behind the mirror
        // See: http://terathon.com/lengyel/Lengyel-Oblique.pdf
        glm::mat4 obliqueProj = cam.projectionMatrix;

        // Plane in view space
        glm::vec3 viewPlaneNormal = glm::normalize(glm::mat3(refViewMat) * normal);
        glm::vec3 viewPlanePoint = glm::vec3(refViewMat * glm::vec4(pos.value, 1.0f));
        float viewPlaneDist = -glm::dot(viewPlaneNormal, viewPlanePoint);
        glm::vec4 clipPlane = glm::vec4(viewPlaneNormal, viewPlaneDist);

        // Calculate the "q" point (farthest point from the plane in the projection volume)
        glm::vec4 q = glm::inverse(obliqueProj) *
                      glm::vec4((clipPlane.x > 0.0f ? 1.0f : (clipPlane.x < 0.0f ? -1.0f : 0.0f)),
                                (clipPlane.y > 0.0f ? 1.0f : (clipPlane.y < 0.0f ? -1.0f : 0.0f)), 1.0f, 1.0f);

        // Scale clip plane
        glm::vec4 c = clipPlane * (2.0f / glm::dot(clipPlane, q));

        // Replace the third row of the projection matrix (the near plane)
        obliqueProj[0][2] = c.x;
        obliqueProj[1][2] = c.y;
        obliqueProj[2][2] = c.z + 1.0f;  // For OpenGL -1 to 1 depth range
        obliqueProj[3][2] = c.w;

        // Render to reflection texture
        RenderViewParams reflParams;
        reflParams.view = refViewMat;
        reflParams.projection = obliqueProj;
        reflParams.cameraPos = refCamPos;
        reflParams.nearPlane = cam.nearPlane;
        reflParams.farPlane = cam.farPlane;
        reflParams.lodFactor = 1.0f;
        reflParams.width = prc.resolution;
        reflParams.height = prc.resolution_y;
        reflParams.cullingMask = cam.cullingMask;
        reflParams.isCapturingProbe = true;
        reflParams.excludeEntity = entity;
        m_RenderService->BuildRenderQueuesWithCamera(scene, reflParams);

        if (auto* skyboxService = ServiceLocator::Instance().Resolve<ISkyboxService>())
        {
            skyboxService->RenderAlphaPassWithCamera(scene, refViewMat, obliqueProj, prc.resolution, prc.resolution_y,
                                                     prc.reflectionFBO);
        }

        if (auto* lightingService = ServiceLocator::Instance().Resolve<ILightingService>())
        {
            RenderSceneData sceneData;
            sceneData.viewMatrix = refViewMat;
            sceneData.projMatrix = obliqueProj;
            sceneData.cameraPosition = refCamPos;
            sceneData.lights = m_RenderService->GetRenderQueueObj().GetLights();
            lightingService->UploadLightData(sceneData);
        }

        const auto& defQ = m_RenderService->GetRenderQueueObj().GetDeferredOpaqueQueue();
        m_RenderService->ExecuteQueue(defQ, RenderQueuePass::DeferredGeometry, nullptr, nullptr, nullptr);

        const auto& fwdQ = m_RenderService->GetRenderQueueObj().GetForwardOpaqueQueue();
        m_RenderService->ExecuteQueue(fwdQ, RenderQueuePass::ForwardOpaque, nullptr, nullptr, nullptr);

        rsm.SetCullFace(CullMode::Back);
        m_RenderService->SetMainFBO(tempFB);
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, tempFB);
        rsm.SetViewport(0, 0, screenWidth, screenHeight);

        prc.isRendered = true;
    }

    m_RenderService->RestoreCameraState(savedView, savedProjection, savedCameraPos, cam.nearPlane, cam.farPlane);
    m_RenderService->BuildRenderQueues(scene, 1.0f, screenWidth, screenHeight);
}

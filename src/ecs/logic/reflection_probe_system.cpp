#include <ecs/logic/reflection_probe_system.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/core_components.h>
#include <core/logic/service_locator.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <ecs/interface/i_render_service.h>
#include <scene/logic/scene.h>
#include <render/logic/render_core.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <core/logic/logger.h>
#include <ecs/logic/system_factory.h>

REGISTER_SYSTEM(ReflectionProbeSystem)

void ReflectionProbeSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& rtm = context.GetRenderTargetManager();
    
    m_CaptureFBO = rtm.GenFramebuffer();
    m_DepthRB = rtm.CreateRenderbuffer();
    
    LOGGER_INFO("ReflectionProbeSystem") << "Initialized with FBO: " << m_CaptureFBO;
}

void ReflectionProbeSystem::Shutdown()
{
    auto& sl = ServiceLocator::Instance();
    auto* context = sl.Resolve<IGraphicsContext>();
    if (context) {
        auto& rtm = context->GetRenderTargetManager();
        rtm.DeleteFramebuffer(m_CaptureFBO);
        rtm.DeleteRenderbuffer(m_DepthRB);
    }
}

void ReflectionProbeSystem::Update(Scene& scene, float dt)
{
    auto view = scene.registry.view<PositionComponent, ReflectionProbeComponent>();
    for (auto entity : view) {
        auto& probe = view.get<ReflectionProbeComponent>(entity);
        if (probe.isDirty || probe.type == ReflectionProbeType::Dynamic) {
            CaptureProbe(scene, entity);
            if (probe.type == ReflectionProbeType::Static) probe.isDirty = false;
        }
    }
}

unsigned int ReflectionProbeSystem::CreateCubemap(int resolution)
{
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& tm = context.GetTextureManager();
    
    unsigned int id = tm.GenTexture();
    tm.BindTexture(TextureType::TextureCubeMap, id);
    
    for (unsigned int i = 0; i < 6; ++i) {
        tm.TexImage2D((TextureType)((int)TextureType::CubeMapPositiveX + i), 0, InternalFormat::RGBA16F, resolution, resolution, 0, TextureFormat::RGB, DataType::Float, nullptr);
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

void ReflectionProbeSystem::CaptureProbe(Scene& scene, entt::entity entity)
{
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& rtm = context.GetRenderTargetManager();
    auto& tm = context.GetTextureManager();
    auto& renderService = sl.Require<IRenderService>();
    
    auto& pos = scene.registry.get<PositionComponent>(entity).value;
    auto& probe = scene.registry.get<ReflectionProbeComponent>(entity);
    
    if (probe.cubemapID == 0) {
        probe.cubemapID = CreateCubemap(probe.resolution);
    }
    
    rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, m_CaptureFBO);
    rtm.BindRenderbuffer(m_DepthRB);
    rtm.RenderbufferStorage(InternalFormat::DepthComponent24, probe.resolution, probe.resolution);
    rtm.FramebufferRenderbuffer(FramebufferTarget::DrawFramebuffer, FramebufferAttachment::Depth, m_DepthRB);
    
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, probe.radius);
    
    glm::mat4 views[] = {
        glm::lookAt(pos, pos + glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(pos, pos + glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(pos, pos + glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(pos, pos + glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(pos, pos + glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(pos, pos + glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0))
    };
    
    uint32_t oldFBO = renderService.GetMainFBO();
    context.SetViewport(0, 0, probe.resolution, probe.resolution);
    
    for (int i = 0; i < 6; ++i) {
        rtm.FramebufferTexture2D(FramebufferTarget::DrawFramebuffer, FramebufferAttachment::Color0, 
                                 (TextureType)((int)TextureType::CubeMapPositiveX + i), 
                                 probe.cubemapID, 0);
        
        context.Clear(BufferBit::Color | BufferBit::Depth);
        
        // Temporarily set render system camera to probe perspective
        // (This is a simplified approach, IRL we might need a dedicated capture pass)
        renderService.BuildRenderQueues(scene, 1.0f, probe.resolution, probe.resolution);
        // Note: Render will use current camera in IRenderService. For true cubemap capture, 
        // we'd need to push a temporary camera state into IRenderService or similar.
    }
    
    tm.BindTexture(TextureType::TextureCubeMap, probe.cubemapID);
    tm.GenerateMipmap(TextureType::TextureCubeMap);
    tm.BindTexture(TextureType::TextureCubeMap, 0);
    
    rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, oldFBO);
}

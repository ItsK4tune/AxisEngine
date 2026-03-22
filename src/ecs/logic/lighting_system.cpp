#include <ecs/logic/lighting_system.h>
#include <core/logic/service_locator.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <render/unit/gbuffer.h>
#include <render/unit/shadow.h>
#include <render/logic/shadow_renderer.h>
#include <render/logic/light_renderer.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_geometry_service.h>
#include <ecs/interface/i_shadow_service.h>
#include <resource/logic/resource_manager.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <core/logic/config_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <algorithm>
#include <string>

void LightingSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& resources = sl.Require<ResourceManager>();

    m_LightRenderer.Initialize(context);
    
    resources.LoadShader("deferred_light", "include/engine/asset/shaders/fxaa.vs", "include/engine/asset/shaders/deferred_light.fs");
    m_DeferredLightShader = resources.GetShader("deferred_light");

    InitQuad();
}

void LightingSystem::Shutdown()
{
    auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
    auto& bm = context.GetBufferManager();
    if (m_QuadVAO.id != 0) bm.DeleteVertexArray(m_QuadVAO.id);
    if (m_QuadVBO.id != 0) bm.DeleteBuffer(m_QuadVBO.id);
}

void LightingSystem::RenderAlpha(Scene& scene, int width, int height, float alpha)
{
    if (!m_Enabled)
         return;

    auto* rs = ServiceLocator::Instance().Resolve<IRenderService>();
    if (!rs) return;

    // Always update light buffers so Forward shaders have data
    m_LightRenderer.UploadLightData(scene, nullptr);

    if (rs->GetRenderPath() == RenderPath::Forward) {
        auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
        context.GetRenderStateManager().SetViewport(0, 0, width, height);
        return;
    }

    // Perform Deferred Lighting Pass
    RenderDeferredLighting(scene, width, height);
}

void LightingSystem::RenderDeferredLighting(Scene& scene, int width, int height)
{
    if (!m_DeferredLightShader) return;

    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& tm = context.GetTextureManager();
    auto& rsm = context.GetRenderStateManager();
    auto& dc = context.GetDrawContext();
    auto& rtm = context.GetRenderTargetManager();
    auto& bm = context.GetBufferManager();

    // Resolve G-Buffer from GeometrySystem
    auto* geoSys = sl.Resolve<IGeometryService>();
    if (!geoSys) return;
    auto& gBuffer = geoSys->GetGBuffer();

    // Resolve Shadow textures from ShadowSystem
    auto* shadowSys = sl.Resolve<IShadowService>();
    
    // Ensure we are rendering to the main FBO
    auto* rs = sl.Resolve<IRenderService>();
    uint32_t mainFBO = rs ? rs->GetMainFBO() : 0;
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);

    // Blit depth from G-Buffer to the current FBO (so Skybox and Transparents can use it)
    rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, gBuffer.GetFBO());
    rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, mainFBO);
    rtm.BlitFramebuffer(0, 0, gBuffer.GetScaledWidth(), gBuffer.GetScaledHeight(), 0, 0, width, height, BufferBit::Depth, TextureFilter::Nearest);
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, mainFBO);

    rsm.SetViewport(0, 0, width, height);
    rsm.Disable(ServerCapability::DepthTest);
    rsm.SetDepthMask(false);
    rsm.Disable(ServerCapability::CullFace);
    rsm.Disable(ServerCapability::Blend);

    m_LightRenderer.UploadLightData(scene, m_DeferredLightShader.get());
    m_DeferredLightShader->use();
    
    // Bind G-Buffer textures
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

    if (shadowSys) {
        bool enableShadows = shadowSys->GetRenderer().IsShadowsEnabled();
        if (enableShadows) {
            for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i) {
                shadowSys->GetShadow().BindTexture_Dir(i, 10 + i);
                m_DeferredLightShader->setInt("shadowMapDir[" + std::to_string(i) + "]", 10 + i);
            }
        }
    }

    bm.BindVertexArray(m_QuadVAO.id);
    dc.DrawArrays(Primitive::TriangleStrip, 0, 4);
    bm.BindVertexArray(0);

    rsm.SetDepthMask(true);
    rsm.Enable(ServerCapability::DepthTest);
}

void LightingSystem::InitQuad()
{
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
    auto& bm = context.GetBufferManager();
    m_QuadVAO.id = bm.CreateVertexArray();
    m_QuadVBO.id = bm.CreateBuffer();

    bm.BindVertexArray(m_QuadVAO.id);
    bm.BindBuffer(BufferType::ArrayBuffer, m_QuadVBO.id);
    bm.BufferData(BufferType::ArrayBuffer, sizeof(quadVertices), &quadVertices, BufferUsage::StaticDraw);
    
    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, 5 * sizeof(float), (void*)0);
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 2, DataType::Float, false, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

std::vector<entt::id_type> LightingSystem::GetReadComponents() const
{
    return {
        entt::type_id<GPUDirLight>().hash(),
        entt::type_id<GPUPointLight>().hash(),
        entt::type_id<GPUSpotLight>().hash()
    };
}

std::vector<entt::id_type> LightingSystem::GetWriteComponents() const
{
    return {};
}

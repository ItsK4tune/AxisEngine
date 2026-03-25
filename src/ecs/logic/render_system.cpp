#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/ui_components.h>
#include <core/logic/logger.h>
#include <core/type/event_types.h>
#include <render/unit/frustum.h>
#include <string>
#include <algorithm>
#include <vector>
#include <thread>
#include <core/logic/job_system.h>
#include <ecs/logic/render_system.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <cstring>
#include <resource/logic/resource_manager.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_query_manager.h>
#include <ecs/logic/entity_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <core/logic/service_locator.h>
#include <render/logic/render_core.h>
#include <core/logic/event_system.h>
#include <core/type/app_config.h>
#include <core/logic/config_manager.h>

#ifdef ENABLE_DEBUG_SYSTEM
#include <ecs/logic/debug/debug_system.h>
#endif

#include <platform/logic/io_handler.h>
#include <render/logic/shadow_renderer.h>
#include <render/logic/material_renderer.h>
#include <render/logic/render_core.h>

void RenderSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& configManager = sl.Require<ConfigManager>();
    const AppConfig& config = configManager.GetConfig();
    auto& shaderLib = sl.Require<ResourceManager>();
    

    this->SetInstanceBatching(config.instanceBatchingEnabled);
    this->SetFrustumCulling(config.frustumCullingEnabled);
    this->SetOcclusionCulling(config.occlusionCullingEnabled);
    this->SetDistanceCulling(config.distanceCulling);
    this->SetAntiAliasingMode((AntiAliasingMode)config.antialiasing);
    this->SetRenderOrderEnabled(config.renderOrderEnabled);
    this->SetFilterLayerMask(config.filterLayerMask);
    this->SetFaceCulling(config.cullFaceEnabled);
    this->SetDepthTest(config.depthTestEnabled);


    EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (!(e.bitmask & (ConfigChangedEvent::Graphics | ConfigChangedEvent::Window | ConfigChangedEvent::All)))
            return;

        const AppConfig& cfg = e.config;
        this->SetInstanceBatching(cfg.instanceBatchingEnabled);
        this->SetFrustumCulling(cfg.frustumCullingEnabled);
        this->SetOcclusionCulling(cfg.occlusionCullingEnabled);
        this->SetDistanceCulling(cfg.distanceCulling);
        this->SetAntiAliasingMode((AntiAliasingMode)cfg.antialiasing);
        this->SetRenderOrderEnabled(cfg.renderOrderEnabled);
        this->SetFilterLayerMask(cfg.filterLayerMask);
        this->SetFaceCulling(cfg.cullFaceEnabled);
        this->SetDepthTest(cfg.depthTestEnabled);
    });

    auto& core = sl.Require<RenderCore>();
    
    auto& bm = context.GetBufferManager();
    m_CameraUBO = std::make_unique<GPUUBO>(context, bm.CreateBuffer());
    bm.BindBuffer(BufferType::UniformBuffer, m_CameraUBO->Get());
    bm.BufferData(BufferType::UniformBuffer, sizeof(GPUCameraData), nullptr, BufferUsage::DynamicDraw);

    m_GlobalLightUBO = std::make_unique<GPUUBO>(context, bm.CreateBuffer());
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalLightUBO->Get());
    bm.BufferData(BufferType::UniformBuffer, sizeof(GPUGlobalLightData), nullptr, BufferUsage::DynamicDraw);

    m_GlobalDataUBO = std::make_unique<GPUUBO>(context, bm.CreateBuffer());
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalDataUBO->Get());
    bm.BufferData(BufferType::UniformBuffer, sizeof(GPUGlobalData), nullptr, BufferUsage::DynamicDraw);

    bm.BindBufferBase(BufferType::UniformBuffer, 20, m_CameraUBO->Get());
    bm.BindBufferBase(BufferType::UniformBuffer, 21, m_GlobalLightUBO->Get());
    bm.BindBufferBase(BufferType::UniformBuffer, 22, m_GlobalDataUBO->Get());

    m_OcclusionCuller.Initialize(context, shaderLib.GetShader("occlusion"));
    m_UnlitShader = shaderLib.GetShader("unlitShader");
}

void RenderSystem::Update(Scene& scene, float dt)
{
    m_CachedRenderPath = ServiceLocator::Instance().Require<ConfigManager>().GetConfig().renderPath;

    m_QueuesBuilt = false;
    m_FrameIndex++;
    m_RenderedCount = 0;

    m_GlobalData.time += dt;
    m_GlobalData.deltaTime = dt;
}

void RenderSystem::Render(Scene& scene)
{
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& bm = context.GetBufferManager();

    static bool firstLog = true;
    if (firstLog) {
        LOGGER_INFO("RenderSystem") << "Final Render Pass: m_MainFBO=" << m_MainFBO;
        firstLog = false;
    }

    // --- DEBUG: Draw a red triangle on screen to verify pipeline ---
    if (m_UnlitShader) {
        static uint32_t debugVAO = 0;
        static uint32_t debugVBO = 0;
        if (debugVAO == 0) {
            float tri[] = { -0.5f, -0.5f, 0, 0.5f, -0.5f, 0, 0, 0.5f, 0 };
            debugVAO = bm.GenVertexArray();
            debugVBO = bm.GenBuffer();
            bm.BindVertexArray(debugVAO);
            bm.BindBuffer(BufferType::ArrayBuffer, debugVBO);
            bm.BufferData(BufferType::ArrayBuffer, sizeof(tri), tri, BufferUsage::StaticDraw);
            bm.EnableVertexAttribArray(0);
            bm.VertexAttribPointer(0, 3, DataType::Float, false, 0, 0);
        }
        auto& rsm = context.GetRenderStateManager();
        auto& rtm = context.GetRenderTargetManager();
        auto& dc = context.GetDrawContext();
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_MainFBO);
        rsm.Disable(ServerCapability::DepthTest);
        rsm.Disable(ServerCapability::CullFace);
        m_UnlitShader->use();
        m_UnlitShader->setVec4("tintColor", glm::vec4(1, 0, 0, 1)); // Correct uniform name
        m_UnlitShader->setBool("debug_noTexture", true); // Ensure no texture sampling
        m_UnlitShader->setBool("u_isWireframe", false);
        m_UnlitShader->setMat4("model", glm::mat4(1.0f));
        m_UnlitShader->setMat4("view", glm::mat4(1.0f));
        m_UnlitShader->setMat4("projection", glm::mat4(1.0f));
        bm.BindVertexArray(debugVAO);
        dc.DrawArrays(Primitive::Triangles, 0, 3);
        bm.BindVertexArray(0);
    }
}

void RenderSystem::RenderUIPass(Scene &scene, float width, float height, IRenderStateManager &renderState)
{

}

void RenderSystem::Shutdown()
{
    LOGGER_INFO("RenderSystem") << "Shutting down RenderSystem";
    m_OcclusionCuller.Shutdown();
}

void RenderSystem::SetFaceCulling(bool enabled, CullMode mode)
{
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto &rsm = context.GetRenderStateManager();

    if (enabled)
    {
        rsm.Enable(ServerCapability::CullFace);
        rsm.SetCullFace(mode);
    }
    else
    {
        rsm.Disable(ServerCapability::CullFace);
    }
}


void RenderSystem::SetDepthTest(bool enabled, CompareFunc func)
{
    auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
    auto &rsm = context.GetRenderStateManager();

    if (enabled)
    {
        rsm.Enable(ServerCapability::DepthTest);
        rsm.SetCullFace(CullMode::Back);
        rsm.SetDepthFunc(CompareFunc::Less);
        rsm.SetDepthMask(true);
        rsm.SetDepthFunc(func);
    }
    else
    {
        rsm.Disable(ServerCapability::DepthTest);
    }
}

void RenderSystem::BuildRenderQueues(Scene &scene, float alpha, int width, int height)
{
    if (m_QueuesBuilt && m_LastAlpha == alpha && m_LastWidth == width && m_LastHeight == height) {
        return;
    }

    MaterialRenderer::InvalidateSkyboxCache();


    auto& sl = ServiceLocator::Instance();
    m_CachedRenderPath = sl.Require<ConfigManager>().GetConfig().renderPath;

    auto& context = sl.Require<IGraphicsContext>();
    if ((m_LastWidth != width || m_LastHeight != height) && width > 0 && height > 0) {

    }
    m_LastWidth = width;
    m_LastHeight = height;

    if (alpha <= 0.0f || alpha > 1.0f) {
        LOGGER_WARN("RenderSystem") << "Invalid alpha value: " << alpha;
    }

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) {
        m_QueuesBuilt = true; m_LastAlpha = alpha;
        return;
    }

    CameraComponent* cam = &scene.registry.get<CameraComponent>(camEntity);
    PositionComponent* camPosComp = scene.registry.try_get<PositionComponent>(camEntity);
    glm::vec3 camPos = camPosComp ? camPosComp->value : glm::vec3(0.0f);

    if (width <= 0 || height <= 0) return;


    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!std::isfinite(cam->projectionMatrix[i][j]) || !std::isfinite(cam->viewMatrix[i][j])) {
                LOGGER_ERROR("RenderSystem") << "NaN detected in camera matrices. Skipping frame.";
                return;
            }
        }
    }

    if (!m_QueuesBuilt) { m_PrevViewProj = m_CurrViewProj; }

    m_JitteredProjection = cam->projectionMatrix;
    m_JitterOffset = glm::vec2(0.0f);

    if (m_AAMode == AntiAliasingMode::TAA) {
        auto HaltonSequence = [](int index, int base) -> float {
            float result = 0.0f; float f = 1.0f; int i = index;
            while (i > 0) { f = f / base; result = result + f * (i % base); i = i / base; }
            return result;
        };
        const int sampleCount = 8;
        int frameIdx = m_FrameIndex % sampleCount;
        float jitterX = HaltonSequence(frameIdx + 1, 2) - 0.5f;
        float jitterY = HaltonSequence(frameIdx + 1, 3) - 0.5f;
        m_JitterOffset = glm::vec2(jitterX, jitterY);
        
        glm::mat4 jitterMatrix = glm::mat4(1.0f);
        jitterMatrix[3][0] = jitterX * 2.0f / (float)width;
        jitterMatrix[3][1] = jitterY * 2.0f / (float)height;
        m_JitteredProjection = jitterMatrix * m_JitteredProjection;
        
        if (!m_QueuesBuilt) m_FrameIndex++;
    }

    m_CurrViewProj = m_JitteredProjection * cam->viewMatrix;
    

    bool stable = true;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!std::isfinite(m_CurrViewProj[i][j])) stable = false;
        }
    }
    if (!stable) {
        m_CurrViewProj = cam->projectionMatrix * cam->viewMatrix;
    }


    GPUCameraData camData;
    std::memcpy(camData.projection, &m_JitteredProjection[0][0], 16 * sizeof(float));
    std::memcpy(camData.view, &cam->viewMatrix[0][0], 16 * sizeof(float));
    std::memcpy(camData.viewPos, &camPos[0], 3 * sizeof(float));
    camData.pad0 = 0.0f;

    auto& bm = context.GetBufferManager();
    bm.BindBuffer(BufferType::UniformBuffer, m_CameraUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUCameraData), &camData);


    m_GlobalData.resolution[0] = (float)width;
    m_GlobalData.resolution[1] = (float)height;
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalDataUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUGlobalData), &m_GlobalData);

    glm::mat4 stableVP = cam->projectionMatrix * cam->viewMatrix;
    m_FrustumCuller.BuildFrustum(stableVP);

    if (m_OcclusionCullingEnabled) {
        m_OcclusionCuller.UpdateResults(scene);
    }

    m_RenderQueueObj.Build(scene, alpha, m_FrustumCuller, m_FrustumCullingEnabled, m_OcclusionCullingEnabled, 
                           m_DistanceCullingSq, m_FilterLayerMask, cam->cullingMask, camPos);

    static bool firstFrame = true;
    if (firstFrame) {
        LOGGER_INFO("RenderSystem") << "BuildRenderQueues: Opaque=" << m_RenderQueueObj.GetOpaqueQueue().size() 
                                    << ", Transparent=" << m_RenderQueueObj.GetTransparentQueue().size()
                                    << ", Camera=" << (uint32_t)camEntity;
        
        // Log camera matrices to see if they are zeros
        LOGGER_INFO("RenderSystem") << "Camera View[0]: " << cam->viewMatrix[0][0] << ", " << cam->viewMatrix[0][1] << ", " << cam->viewMatrix[0][2];
        LOGGER_INFO("RenderSystem") << "Camera Proj[0]: " << cam->projectionMatrix[0][0] << ", " << cam->projectionMatrix[0][1] << ", " << cam->projectionMatrix[0][2];
        
        firstFrame = false;
    }

    m_QueuesBuilt = true;
    m_LastAlpha = alpha;
    m_LastWidth = width;
    m_LastHeight = height;
}

void RenderSystem::ExecuteQueue(Scene& scene, const std::vector<RenderItem>& queue, bool isTransparentPass, ShadowRenderer* shadowRenderer, MaterialRenderer* materialRenderer, Shader* overrideShader)
{
    if (!materialRenderer) {
        auto& core = ServiceLocator::Instance().Require<RenderCore>();
        materialRenderer = &core.GetMaterialRenderer();
    }

    Shader* lastShader = nullptr;

    for (size_t i = 0; i < queue.size(); i++) {
        const auto& item = queue[i];
        entt::entity entity = item.entity;
        Model* model = item.activeModel;
        MaterialComponent* material = item.activeMaterial;
        Shader* shader = item.activeShader;

        if (overrideShader) {
            shader = overrideShader;
        } else if (!shader) {
            shader = m_UnlitShader.get();
        }

        if (!shader || !model) continue;

        if (shader != lastShader) {
            shader->use();
            lastShader = shader;


            if (shadowRenderer && shadowRenderer->IsShadowsEnabled()) {
                auto& shadow = shadowRenderer->GetShadow();
                for (int j = 0; j < Shadow::MAX_DIR_LIGHTS_SHADOW; j++) {
                    shadow.BindTexture_Dir(j, 10 + j);
                    shader->setInt(("shadowMapDir[" + std::to_string(j) + "]").c_str(), 10 + j);
                }
                shader->setFloat("u_ShadowBias", shadowRenderer->GetShadowBias());
                shader->setInt("u_ShadowSoftness", shadowRenderer->GetShadowSoftness());
            }
        }

        glm::mat4 mtx = item.worldMatrix;
        bool isAnimated = false;
        if (scene.registry.all_of<AnimationComponent>(entity)) {
            auto& anim = scene.registry.get<AnimationComponent>(entity);
            if (anim.animator) {
                isAnimated = true;
                auto bones = anim.animator->GetFinalBoneMatrices();
                shader->setMat4Array("finalBonesMatrices", bones);
            }
        }

        if (!isAnimated) {
            mtx *= model->GetRootTransform();
        }

        shader->setMat4("model", mtx);
        

        glm::vec4 tc(1.0f);
        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(entity)) {
            tc = renderer->color;
        }
        shader->setVec4("tintColor", tc);
        shader->setUInt("entityID", (uint32_t)entity);
        shader->setBool("isInstanced", false);

        bool matBound = materialRenderer->SetupMaterialUniforms(shader, material, scene, m_DebugNoTexture, m_Wireframe);
        model->Draw(*shader, !matBound);
    }
}

unsigned int RenderSystem::GetWhiteTexture() const { return ServiceLocator::Instance().Require<RenderCore>().GetWhiteTexture(); }
unsigned int RenderSystem::GetBlackTexture() const { return ServiceLocator::Instance().Require<RenderCore>().GetBlackTexture(); }
unsigned int RenderSystem::GetFlatNormalTexture() const { return ServiceLocator::Instance().Require<RenderCore>().GetFlatNormalTexture(); }

std::vector<entt::id_type> RenderSystem::GetReadComponents() const
{
    return {
        entt::type_id<MeshRendererComponent>().hash(),
        entt::type_id<CameraComponent>().hash(),
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash(),
        entt::type_id<ScaleComponent>().hash(),
        entt::type_id<WorldTransformComponent>().hash(),
        entt::type_id<MaterialComponent>().hash(),
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
void RenderSystem::RenderAlphaPass(Scene& scene, int width, int height, float alpha)
{

}

void RenderSystem::RenderTransparentPass(Scene& scene, int width, int height, float alpha)
{

}

void RenderSystem::UpdateGlobalLightData(const GPUGlobalLightData& data)
{
    auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
    auto& bm = context.GetBufferManager();
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalLightUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUGlobalLightData), &data);
}

void RenderSystem::SubmitCommand(const RenderDrawCommand& cmd)
{
    m_RenderCommandBuffer.Submit(cmd);
}

void RenderSystem::FlushCommands()
{
    m_RenderCommandBuffer.Sort();
    const auto& commands = m_RenderCommandBuffer.GetCommands();
    
    if (commands.empty()) return;

    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& rsm = context.GetRenderStateManager();
    auto& bm = context.GetBufferManager();
    auto& dc = context.GetDrawContext();
    auto& tm = context.GetTextureManager();

    uint32_t lastShader = 0;
    uint32_t lastVAO = 0;

    for (const auto& cmd : commands) {
        if (cmd.shaderId != lastShader) {
            if (cmd.shader) cmd.shader->use();
            lastShader = cmd.shaderId;
        }
        
        if (cmd.vao != lastVAO) {
            bm.BindVertexArray(cmd.vao);
            lastVAO = cmd.vao;
        }
        
        if (cmd.shader) {
            cmd.shader->setMat4("model", cmd.modelMatrix);
            if (cmd.texture0 != 0) {
                tm.ActiveTexture(TextureUnit::Texture0);
                tm.BindTexture(TextureType::Texture2D, cmd.texture0);
                cmd.shader->setInt("u_Texture0", 0);
            }
            cmd.shader->setVec4("u_TintColor", cmd.tintColor);
            
            for (const auto& kv : cmd.uintUniforms) {
                cmd.shader->setUInt(kv.first, kv.second);
            }
            for (const auto& kv : cmd.floatUniforms) {
                cmd.shader->setFloat(kv.first, kv.second);
            }
        }
        
        if (cmd.ebo != 0) {
            bm.BindBuffer(BufferType::ElementArrayBuffer, cmd.ebo);
            dc.DrawElements(Primitive::Triangles, cmd.count, DataType::UnsignedInt, 0);
        } else {
            dc.DrawArrays(Primitive::Triangles, 0, cmd.count);
        }
    }
    
    m_RenderCommandBuffer.Clear();
    bm.BindVertexArray(0);
}



#include <ecs/logic/particle_system.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <platform/logic/io_handler.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <resource/logic/resource_manager.h>
#include <algorithm>
#include <vector>


void ParticleSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<ParticleSystem>(this);
    m_Context = sl.Resolve<IGraphicsContext>();
    if (m_Context)
    {
        auto& tm = m_Context->GetTextureManager();
        m_DefaultTexture = tm.GenTexture();
        tm.BindTexture(TextureType::Texture2D, m_DefaultTexture);
        unsigned char white[] = {255, 255, 255, 255};
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 1, 1, 0, TextureFormat::RGBA,
                      DataType::UnsignedByte, white);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::Nearest);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, (int)TextureFilter::Nearest);

        auto& bm = m_Context->GetBufferManager();
        const float quadVertices[] = {
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,  0.5f, 0.0f, 1.0f, 1.0f};
        m_ParticleVAO = bm.GenVertexArray();
        m_ParticleVBO = bm.GenBuffer();
        m_ParticleInstanceUpload = std::make_unique<TransientBufferRing>();
        m_ParticleInstanceUpload->Initialize(bm, BufferType::ArrayBuffer, 1024 * sizeof(ParticleInstanceData));

        bm.BindVertexArray(m_ParticleVAO);
        bm.BindBuffer(BufferType::ArrayBuffer, m_ParticleVBO);
        bm.BufferData(BufferType::ArrayBuffer, sizeof(quadVertices), quadVertices, BufferUsage::StaticDraw);
        bm.EnableVertexAttribArray(0);
        bm.VertexAttribPointer(0, 3, DataType::Float, false, 5 * sizeof(float), nullptr);
        bm.EnableVertexAttribArray(1);
        bm.VertexAttribPointer(1, 2, DataType::Float, false, 5 * sizeof(float),
                               reinterpret_cast<void*>(3 * sizeof(float)));
        bm.BindBuffer(BufferType::ArrayBuffer, m_ParticleInstanceUpload->GetBuffer());
        bm.EnableVertexAttribArray(2);
        bm.VertexAttribPointer(2, 4, DataType::Float, false, sizeof(ParticleInstanceData),
                               reinterpret_cast<void*>(offsetof(ParticleInstanceData, color)));
        bm.VertexAttribDivisor(2, 1);
        bm.EnableVertexAttribArray(3);
        bm.VertexAttribPointer(3, 3, DataType::Float, false, sizeof(ParticleInstanceData),
                               reinterpret_cast<void*>(offsetof(ParticleInstanceData, offset)));
        bm.VertexAttribDivisor(3, 1);
        bm.EnableVertexAttribArray(4);
        bm.VertexAttribPointer(4, 1, DataType::Float, false, sizeof(ParticleInstanceData),
                               reinterpret_cast<void*>(offsetof(ParticleInstanceData, scale)));
        bm.VertexAttribDivisor(4, 1);
        bm.BindVertexArray(0);
    }
}

void ParticleSystem::ApplyOptimizationConfig(const OptimizationConfig& config)
{
    SetSpawnBudget(config.particleSpawnBudgetEnabled,
                   static_cast<unsigned int>(config.particleMaxSpawnPerFrame));
    SetParticleBatching(config.particleBatchingEnabled);
}

void ParticleSystem::Shutdown()
{
    m_ParticleBatches.clear();
    m_ActiveParticleBatchCount = 0;
    m_ParticleInstanceUpload.reset();
    if (m_Context)
    {
        auto& bm = m_Context->GetBufferManager();
        if (m_ParticleVAO != 0)
            bm.DeleteVertexArrays(1, &m_ParticleVAO);
        if (m_ParticleVBO != 0)
            bm.DeleteBuffers(1, &m_ParticleVBO);
    }
    m_ParticleVAO = 0;
    m_ParticleVBO = 0;
    if (m_Context && m_DefaultTexture != 0)
        m_Context->GetTextureManager().DeleteTextures(1, &m_DefaultTexture);
    m_DefaultTexture = 0;
    m_Context = nullptr;
}

void ParticleSystem::Update(Scene& scene, float dt)
{
    if (!m_Enabled)
        return;

    auto view = scene.View<ParticleEmitterComponent, PositionComponent, InfoComponent>();
    m_DestroyScratch.clear();
    for (auto entity : view)
    {
        auto& info = view.get<InfoComponent>(entity);
        if (!info.isActive)
            continue;

        auto& emitterComp = view.get<ParticleEmitterComponent>(entity);
        const auto& pos = scene.GetComponent<PositionComponent>(entity);
        emitterComp.emitter.SetSpawnBudget(m_SpawnBudgetEnabled, m_MaxSpawnPerFrame);

        bool isSpawning = true;
        if (emitterComp.emissionDuration > 0.0f)
        {
            emitterComp.emissionDuration -= dt;
            if (emitterComp.emissionDuration <= 0.0f)
            {
                isSpawning = false;
            }
        }

        emitterComp.emitter.Update(dt, pos.value, isSpawning);

        if (!isSpawning && emitterComp.emitter.GetActiveParticleCount() == 0)
        {
            if (scene.HasAnyComponent<InfoComponent>(entity))
            {
                auto& info = scene.GetComponent<InfoComponent>(entity);
                if (info.name.find("Impact_Particle") != std::string::npos)
                {
                    m_DestroyScratch.push_back(entity);
                }
            }
        }
    }

    for (auto entity : m_DestroyScratch)
    {
        scene.Destroy(entity);
    }
}

void ParticleSystem::RenderTransparentPass(Scene& scene, int width, int height, float alpha)
{
    RenderParticles(scene, width, height, alpha);
}

void ParticleSystem::RenderParticles(Scene& scene, int width, int height, float alpha)
{
    if (!m_Enabled || !m_Context)
        return;

    auto& sl = ServiceLocator::Instance();
    auto* rs = sl.Resolve<IRenderService>();
    auto* resources = sl.Resolve<ResourceManager>();
    if (!resources)
        return;
    uint32_t fbo = rs ? rs->GetMainFBO() : 0;
    m_Context->GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, fbo);

    auto& rsm = m_Context->GetRenderStateManager();

    rsm.Enable(ServerCapability::Blend);
    rsm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::One);
    rsm.Enable(ServerCapability::DepthTest);
    rsm.SetDepthFunc(CompareFunc::Lequal);
    rsm.SetDepthMask(false);
    rsm.Disable(ServerCapability::CullFace);

    auto view = scene.View<ParticleEmitterComponent, InfoComponent>();
    auto defaultShader = resources->GetShader("particle");
    m_ActiveParticleBatchCount = 0;
    for (auto entity : view)
    {
        auto& info = view.get<InfoComponent>(entity);
        if (!info.isActive)
            continue;

        auto& emitterComp = view.get<ParticleEmitterComponent>(entity);
        {
            uint32_t activeCount = emitterComp.emitter.GetActiveParticleCount();
            if (activeCount == 0)
                continue;
            std::shared_ptr<Shader> activeShader = defaultShader;
            if (!emitterComp.customShader.empty())
            {
                if (auto custom = resources->GetShader(emitterComp.customShader))
                {
                    activeShader = custom;
                }
            }
            if (!activeShader)
                continue;

            if (!emitterComp.emitter.texture && !emitterComp.textureName.empty())
            {
                emitterComp.emitter.texture = resources->GetTextureAuto(emitterComp.textureName);
            }

            if (!m_ParticleBatchingEnabled)
            {
                activeShader->use();
                if (!emitterComp.emitter.texture)
                {
                    m_Context->GetTextureManager().ActiveTexture(TextureUnit::Texture0);
                    m_Context->GetTextureManager().BindTexture(TextureType::Texture2D, m_DefaultTexture);
                    activeShader->setInt("u_AlbedoMap", 0);
                }
                emitterComp.emitter.Render(activeShader.get());
                continue;
            }

            const unsigned int textureId = emitterComp.emitter.texture ? emitterComp.emitter.texture->id
                                                                        : m_DefaultTexture;
            size_t batchIndex = 0;
            for (; batchIndex < m_ActiveParticleBatchCount; ++batchIndex)
            {
                if (m_ParticleBatches[batchIndex].shader.get() == activeShader.get() &&
                    m_ParticleBatches[batchIndex].texture == textureId)
                    break;
            }
            if (batchIndex == m_ActiveParticleBatchCount)
            {
                if (batchIndex == m_ParticleBatches.size())
                    m_ParticleBatches.emplace_back();
                auto& batch = m_ParticleBatches[batchIndex];
                batch.shader = activeShader;
                batch.texture = textureId;
                batch.instances.clear();
                ++m_ActiveParticleBatchCount;
            }
            const auto& emitterInstances = emitterComp.emitter.GetInstanceData();
            auto& batchInstances = m_ParticleBatches[batchIndex].instances;
            batchInstances.insert(batchInstances.end(), emitterInstances.begin(), emitterInstances.end());
        }
    }

    if (m_ParticleBatchingEnabled && m_ParticleInstanceUpload && m_ParticleVAO != 0)
    {
        auto& bm = m_Context->GetBufferManager();
        auto& tm = m_Context->GetTextureManager();
        auto& dc = m_Context->GetDrawContext();
        for (size_t batchIndex = 0; batchIndex < m_ActiveParticleBatchCount; ++batchIndex)
        {
            auto& batch = m_ParticleBatches[batchIndex];
            if (!batch.shader || batch.instances.empty())
                continue;
            batch.shader->use();
            tm.ActiveTexture(TextureUnit::Texture0);
            tm.BindTexture(TextureType::Texture2D, batch.texture);
            batch.shader->setInt("u_AlbedoMap", 0);
            const auto slice = m_ParticleInstanceUpload->Upload(
                batch.instances.data(), batch.instances.size() * sizeof(ParticleInstanceData));
            bm.BindVertexArray(m_ParticleVAO);
            bm.BindBuffer(BufferType::ArrayBuffer, slice.buffer);
            bm.VertexAttribPointer(2, 4, DataType::Float, false, sizeof(ParticleInstanceData),
                                   reinterpret_cast<void*>(slice.offset + offsetof(ParticleInstanceData, color)));
            bm.VertexAttribPointer(3, 3, DataType::Float, false, sizeof(ParticleInstanceData),
                                   reinterpret_cast<void*>(slice.offset + offsetof(ParticleInstanceData, offset)));
            bm.VertexAttribPointer(4, 1, DataType::Float, false, sizeof(ParticleInstanceData),
                                   reinterpret_cast<void*>(slice.offset + offsetof(ParticleInstanceData, scale)));
            dc.DrawArraysInstanced(Primitive::Triangles, 0, 6, static_cast<int>(batch.instances.size()));
            m_ParticleInstanceUpload->Commit();
        }
        bm.BindVertexArray(0);
    }

    rsm.SetDepthMask(true);
    rsm.SetDepthFunc(CompareFunc::Less);
    rsm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
    rsm.Disable(ServerCapability::Blend);
    rsm.Enable(ServerCapability::CullFace);
    rsm.SetCullFace(CullMode::Back);
}

std::vector<entt::id_type> ParticleSystem::GetReadComponents() const
{
    return {entt::type_id<ParticleEmitterComponent>().hash(), entt::type_id<PositionComponent>().hash(),
            entt::type_id<RotationComponent>().hash(), entt::type_id<CameraComponent>().hash()};
}

std::vector<entt::id_type> ParticleSystem::GetWriteComponents() const
{
    return {entt::type_id<ParticleEmitterComponent>().hash()};
}

#include <ecs/unit/core_components.h>
#include <algorithm>
#include <ecs/logic/entity_manager.h>
#include <ecs/unit/media_components.h>
#include <ecs/logic/particle_system.h>
#include <execution>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <core/logic/logger.h>
#include <vector>

#include <platform/logic/io_handler.h>
#include <core/logic/service_locator.h>
#include <resource/logic/resource_manager.h>
#include <ecs/interface/i_render_service.h>
#include <render/interface/i_render_target_manager.h>

void ParticleSystem::Initialize()
{
    m_Context = &ServiceLocator::Instance().Require<IGraphicsContext>();
    

    auto& tm = m_Context->GetTextureManager();
    m_DefaultTexture = tm.GenTexture();
    tm.BindTexture(TextureType::Texture2D, m_DefaultTexture);
    unsigned char white[] = { 255, 255, 255, 255 };
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 1, 1, 0, TextureFormat::RGBA, DataType::UnsignedByte, white);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::Nearest);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, (int)TextureFilter::Nearest);
}

void ParticleSystem::Update(Scene &scene, float dt)
{
    if (!m_Enabled)
        return;

    auto view = scene.registry.view<ParticleEmitterComponent, PositionComponent>();

    std::vector<entt::entity> toDestroy;
    for (auto entity : view)
    {
        auto [emitterComp, pos] = view.get<ParticleEmitterComponent, PositionComponent>(entity);

        bool isSpawning = true;
        if (emitterComp.lifetime > 0.0f)
        {
            emitterComp.lifetime -= dt;
            if (emitterComp.lifetime <= 0.0f)
            {
                isSpawning = false;
            }
        }

        emitterComp.emitter.Update(dt, pos.value, isSpawning);


        if (!isSpawning && emitterComp.emitter.GetActiveParticleCount() == 0)
        {
            if (scene.registry.any_of<InfoComponent>(entity)) {
                auto& info = scene.registry.get<InfoComponent>(entity);
                if (info.name.find("Impact_Particle") != std::string::npos) {
                    toDestroy.push_back(entity);
                }
            }
        }
    }

    for (auto entity : toDestroy) {
        scene.registry.destroy(entity);
    }
}

void ParticleSystem::RenderTransparentPass(Scene &scene, int width, int height, float alpha)
{
    if (!m_Enabled || !m_Context)
        return;

    auto& sl = ServiceLocator::Instance();
    auto* rs = sl.Resolve<IRenderService>();
    uint32_t fbo = rs ? rs->GetMainFBO() : 0;
    m_Context->GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, fbo);
    
    auto& rsm = m_Context->GetRenderStateManager();

    rsm.Enable(ServerCapability::Blend);
    rsm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::One);
    rsm.Enable(ServerCapability::DepthTest);
    rsm.SetDepthFunc(CompareFunc::Lequal);
    rsm.SetDepthMask(false);
    rsm.Disable(ServerCapability::CullFace);

    auto& resources = ServiceLocator::Instance().Require<ResourceManager>();
    auto view = scene.registry.view<ParticleEmitterComponent>();
    auto defaultShader = resources.GetShader("particle"); // Assuming a default particle shader exists
    for (auto entity : view)
    {
        auto &emitterComp = view.get<ParticleEmitterComponent>(entity);
        if (emitterComp.isActive)
        {
            std::shared_ptr<Shader> activeShader = defaultShader;
            if (!emitterComp.customShader.empty()) {
                if (auto custom = resources.GetShader(emitterComp.customShader)) {
                    activeShader = custom;
                }
            }
            if (!activeShader) continue;
            activeShader->use();
            
            uint32_t activeCount = emitterComp.emitter.GetActiveParticleCount();
            if (activeCount > 0) {
                if (!emitterComp.emitter.Texture) {
                    m_Context->GetTextureManager().ActiveTexture(TextureUnit::Texture0);
                    m_Context->GetTextureManager().BindTexture(TextureType::Texture2D, m_DefaultTexture);
                    activeShader->setInt("sprite", 0);
                }
                emitterComp.emitter.Render(activeShader.get());
            }
        }
    }

    rsm.SetDepthMask(true);
    rsm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
    rsm.Disable(ServerCapability::Blend);
}

std::vector<entt::id_type> ParticleSystem::GetReadComponents() const
{
    return {
        entt::type_id<ParticleEmitterComponent>().hash(),
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash(),
        entt::type_id<CameraComponent>().hash()
    };
}

std::vector<entt::id_type> ParticleSystem::GetWriteComponents() const
{
    return {
        entt::type_id<ParticleEmitterComponent>().hash()
    };
}

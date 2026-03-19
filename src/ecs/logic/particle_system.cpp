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

void ParticleSystem::Initialize()
{
    m_Context = &ServiceLocator::Instance().Require<IGraphicsContext>();
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

        // Auto-cleanup for Impact particles
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

void ParticleSystem::Render(Scene &scene)
{
    if (!m_Enabled || !m_Context)
        return;

    auto& rsm = m_Context->GetRenderStateManager();

    rsm.Enable(ServerCapability::Blend);
    rsm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::One);
    rsm.Enable(ServerCapability::DepthTest);
    rsm.SetDepthMask(false);

    auto& resources = ServiceLocator::Instance().Require<ResourceManager>();
    auto shader = resources.GetShader("particle");
    if (!shader)
    {
        LOGGER_ERROR("ParticleSystem") << "'particle' shader not found!";
        return;
    }

    shader->use();

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity != entt::null)
    {
        auto &cam = scene.registry.get<CameraComponent>(camEntity);
        shader->setMat4("projection", cam.projectionMatrix);
        shader->setMat4("view", cam.viewMatrix);
    }

    auto view = scene.registry.view<ParticleEmitterComponent>();
    for (auto entity : view)
    {
        auto &emitterComp = view.get<ParticleEmitterComponent>(entity);
        if (emitterComp.isActive)
        {
            emitterComp.emitter.Render(shader.get());
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

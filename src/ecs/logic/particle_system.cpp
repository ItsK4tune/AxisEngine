#include <ecs/unit/core_components.h>
#include <algorithm>
#include <ecs/manager/entity_manager.h>
#include <ecs/unit/media_components.h>
#include <ecs/logic/particle_system.h>
#include <execution>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <core/logic/logger.h>
#include <vector>

void ParticleSystem::Initialize(IGraphicsContext& context)
{
    m_Context = &context;
}

void ParticleSystem::Update(Scene &scene, float dt)
{
    if (!m_Enabled)
        return;

    auto view = scene.registry.view<ParticleEmitterComponent, PositionComponent>();

    std::vector<entt::entity> entities(view.begin(), view.end());

    std::for_each(std::execution::par, entities.begin(), entities.end(), [&view, dt](entt::entity entity) {
        auto [emitterComp, pos] = view.get<ParticleEmitterComponent, PositionComponent>(entity);

        if (emitterComp.isActive)
        {
            emitterComp.emitter.Update(dt, pos.value);
        }
    });
}

void ParticleSystem::Render(Scene &scene)
{
    if (!m_Enabled || !m_Context)
        return;

    auto& rsm = m_Context->GetRenderStateManager();

    rsm.Enable(ServerCapability::Blend);
    rsm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::One);
    rsm.SetDepthMask(false);

    auto shader = m_Ctx.resources->GetShader("particle");
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

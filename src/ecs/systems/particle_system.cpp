#include <ecs/system.h>
#include <ecs/component.h>
#include <glad/glad.h>

void ParticleSystem::Update(Scene &scene, float dt)
{
    if (!m_Enabled)
        return;

    auto view = scene.registry.view<ParticleEmitterComponent, TransformComponent>();

    for (auto entity : view)
    {
        auto [emitterComp, transform] = view.get<ParticleEmitterComponent, TransformComponent>(entity);

        if (!emitterComp.isActive)
            continue;

        emitterComp.emitter.Update(dt, transform.position);
    }
}

void ParticleSystem::Render(Scene &scene, ResourceManager &res)
{
    if (!m_Enabled)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    auto shader = res.GetShader("particle");
    if (!shader)
    {
        std::cerr << "[ParticleSystem] 'particle' shader not found!" << std::endl;
        return;
    }

    shader->use();

    entt::entity camEntity = scene.GetActiveCamera();
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
            emitterComp.emitter.Render(shader);
        }
    }

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}

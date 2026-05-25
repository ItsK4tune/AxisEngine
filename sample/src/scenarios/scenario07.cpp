#include "sample_scenario_common.h"

void SampleState::LoadScene7()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    float angleStep = 360.0f / static_cast<float>(m_S7EmitterCount > 0 ? m_S7EmitterCount : 1);

    for (int i = 0; i < m_S7EmitterCount; ++i)
    {
        float angle = (static_cast<float>(i) * angleStep) * 3.14159f / 180.0f;
        float radius = 15.0f;
        glm::vec3 pos(cos(angle) * radius, 0.1f, sin(angle) * radius);

        auto emitterEntity = EntityBuilder(scene, res, "scenario")
            .WithName("Emitter_" + std::to_string(i))
            .WithTransform(pos, glm::vec3(0.0f))
            .Build();

        auto& pe = scene.registry.emplace<ParticleEmitterComponent>(emitterEntity);
        pe.isActive = true;
        pe.emitter.SpawnRate = m_S7SpawnRate;
        pe.emitter.LifeTime = m_S7LifeTime;
        pe.emitter.StartSize = m_S7StartSize;
        pe.emitter.EndSize = m_S7EndSize;

        glm::vec3 dir = glm::normalize(glm::vec3(pos.x, 10.0f, pos.z));
        pe.emitter.MinVelocity = dir * m_S7MinSpeed - glm::vec3(0.5f, 0.0f, 0.5f);
        pe.emitter.MaxVelocity = dir * m_S7MaxSpeed + glm::vec3(0.5f, m_S7VerticalSpeed, 0.5f);

        float r = 0.5f + 0.5f * sin(angle);
        float g = 0.5f + 0.5f * sin(angle + 2.0f);
        float b = 0.5f + 0.5f * sin(angle + 4.0f);
        pe.emitter.StartColor = glm::vec4(r, g, b, 1.0f);
        pe.emitter.EndColor = glm::vec4(r, g, b, 0.0f);

        pe.emitter.Initialize(1000);
    }
}

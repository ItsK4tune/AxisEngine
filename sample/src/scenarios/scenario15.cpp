#include "sample_scenario_common.h"

void SampleState::LoadScene15()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    float angleStep = 360.0f / static_cast<float>(m_S15EmitterCount > 0 ? m_S15EmitterCount : 1);

    for (int i = 0; i < m_S15EmitterCount; ++i)
    {
        float angle = (static_cast<float>(i) * angleStep) * 3.14159f / 180.0f;
        float radius = 15.0f;
        glm::vec3 pos(cos(angle) * radius, 0.1f, sin(angle) * radius);
        glm::vec3 dir = glm::normalize(glm::vec3(pos.x, 10.0f, pos.z));
        glm::vec3 minVel = dir * m_S15MinSpeed - glm::vec3(0.5f, 0.0f, 0.5f);
        glm::vec3 maxVel = dir * m_S15MaxSpeed + glm::vec3(0.5f, m_S15VerticalSpeed, 0.5f);

        float r = 0.5f + 0.5f * sin(angle);
        float g = 0.5f + 0.5f * sin(angle + 2.0f);
        float b = 0.5f + 0.5f * sin(angle + 4.0f);

        EntityBuilder(scene, res, "scenario")
            .WithName("Emitter_" + std::to_string(i))
            .WithTransform(pos, glm::vec3(0.0f))
            .WithParticleEmitter(m_S15SpawnRate, m_S15LifeTime, m_S15StartSize, m_S15EndSize, minVel, maxVel, glm::vec4(r, g, b, 1.0f), glm::vec4(r, g, b, 0.0f), 1000)
            .Build();
    }
}

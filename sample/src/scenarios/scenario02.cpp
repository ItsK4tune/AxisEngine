#include "sample_scenario_common.h"

void SampleState::LoadScene2()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithPBRRenderable("planeModel", "deferred_lit", glm::vec3(0.0f), glm::vec3(0.0f),
                           glm::vec3(100.0f, 1.0f, 100.0f), 0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("LightingProbeCapsule")
        .WithPBRRenderable("capsuleSmoothModel", "deferred_lit", glm::vec3(0.0f, 5.0f, 0.0f),
                           glm::vec3(0.0f), glm::vec3(5.0f, 10.0f, 5.0f), 0.2f, 0.4f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight_Key")
        .WithDirectionalLightAt(glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(-45.0f, 35.0f, 0.0f),
                                glm::normalize(glm::vec3(-0.5f, -1.0f, -0.35f)), m_S2DirectionalColor,
                                m_S2DirectionalIntensity)
        .Build();

    for (int i = 0; i < 499; ++i)
    {
        float radius = 10.0f + static_cast<float>(rand() % 40);
        float angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
        float h = 5.0f + static_cast<float>(rand() % 100) / 10.0f;
        glm::vec3 pPos(cos(angle) * radius, h, sin(angle) * radius);
        glm::vec3 pColor = m_S2PointColor * (0.5f + static_cast<float>(rand() % 50) / 100.0f);
        EntityBuilder(scene, res, "scenario")
            .WithName("PointLight_" + std::to_string(i))
            .WithPointLightAt(pPos, pColor, m_S2PointIntensity, 15.0f)
            .Build();

        float sRadius = 15.0f + static_cast<float>(rand() % 35);
        float sAngle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
        glm::vec3 sPos(cos(sAngle) * sRadius, 15.0f, sin(sAngle) * sRadius);
        glm::vec3 sColor = m_S2SpotColor * (0.5f + static_cast<float>(rand() % 50) / 100.0f);
        EntityBuilder(scene, res, "scenario")
            .WithName("SpotLight_" + std::to_string(i))
            .WithSpotLightAt(sPos, glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), sColor, m_S2SpotIntensity)
            .Build();
    }

    for (auto entity : GetEntitiesWithNamePrefix("SpotLight_"))
    {
        glm::vec3 pos = entity.GetPosition();
        glm::vec3 dir = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f) - pos);
        entity.SetLightDirection(dir);
        entity.SetSpotLightCutOff(20.0f, 30.0f);
        entity.SetLightAttenuation(0.07f, 0.017f);
        entity.SetRotation(RotationFromNegativeY(dir));
    }
}

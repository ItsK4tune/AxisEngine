#include "sample_scenario_common.h"

void SampleState::LoadScene3()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(100.0f, 1.0f, 100.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("Cylinder")
        .WithTransform(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(5.0f, 10.0f, 5.0f))
        .WithMesh("cylinderModel", "deferred_lit")
        .WithPBRMaterial(0.2f, 0.4f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight_Key")
        .WithTransform(glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(-45.0f, 35.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.5f, -1.0f, -0.35f)), m_S3DirectionalColor,
                              m_S3DirectionalIntensity)
        .Build();

    for (int i = 0; i < 499; ++i)
    {
        float radius = 10.0f + static_cast<float>(rand() % 40);
        float angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
        float h = 1.0f + static_cast<float>(rand() % 100) / 10.0f;
        glm::vec3 pPos(cos(angle) * radius, h, sin(angle) * radius);
        glm::vec3 pColor = m_S3PointColor * (0.5f + static_cast<float>(rand() % 50) / 100.0f);
        EntityBuilder(scene, res, "scenario")
            .WithName("PointLight_" + std::to_string(i))
            .WithTransform(pPos)
            .WithPointLight(pColor, m_S3PointIntensity, 15.0f)
            .Build();

        float sRadius = 15.0f + static_cast<float>(rand() % 35);
        float sAngle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
        glm::vec3 sPos(cos(sAngle) * sRadius, 15.0f, sin(sAngle) * sRadius);
        glm::vec3 sColor = m_S3SpotColor * (0.5f + static_cast<float>(rand() % 50) / 100.0f);
        EntityBuilder(scene, res, "scenario")
            .WithName("SpotLight_" + std::to_string(i))
            .WithTransform(sPos, glm::vec3(0.0f))
            .WithSpotLight(glm::vec3(0.0f, -1.0f, 0.0f), sColor, m_S3SpotIntensity)
            .Build();
    }

    auto spotView = scene.registry.view<PositionComponent, RotationComponent, SpotLightComponent, InfoComponent>();
    for (auto entity : spotView)
    {
        auto& info = spotView.get<InfoComponent>(entity);
        if (info.name.rfind("SpotLight_", 0) != 0)
            continue;
        auto& pos = spotView.get<PositionComponent>(entity);
        auto& rot = spotView.get<RotationComponent>(entity);
        auto& spot = spotView.get<SpotLightComponent>(entity);
        spot.direction = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f) - pos.value);
        spot.cutOff = glm::cos(glm::radians(20.0f));
        spot.outerCutOff = glm::cos(glm::radians(30.0f));
        spot.linear = 0.07f;
        spot.quadratic = 0.017f;
        rot.value = RotationFromNegativeY(spot.direction);
    }
}

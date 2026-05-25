#include "sample_scenario_common.h"

void SampleState::LoadScene22()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("WarmKeyLight")
        .WithTransform(glm::vec3(-14.0f, 8.0f, 10.0f), glm::vec3(0.0f), glm::vec3(0.8f))
        .WithMesh("sphereModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.3f, 1.0f)
        .WithPointLight(glm::vec3(1.0f, 0.72f, 0.45f), 18.0f, 35.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("CoolFillLight")
        .WithTransform(glm::vec3(12.0f, 14.0f, -12.0f), glm::vec3(0.0f), glm::vec3(0.7f))
        .WithMesh("sphereModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.3f, 1.0f)
        .WithPointLight(glm::vec3(0.45f, 0.7f, 1.0f), 10.0f, 28.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("PBRFloor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    for (int row = 0; row < 5; ++row)
    {
        for (int col = 0; col < 5; ++col)
        {
            float metallic = row / 4.0f;
            float roughness = 0.05f + col * 0.225f;
            auto sphere = EntityBuilder(scene, res, "scenario")
                .WithName("PBR_" + std::to_string(row) + "_" + std::to_string(col))
                .WithTransform(glm::vec3((col - 2) * 5.0f, 2.0f, (row - 2) * 5.0f), glm::vec3(0.0f), glm::vec3(2.0f))
                .WithMesh("sphereModel", "deferred_lit")
                .WithPBRMaterial(metallic, roughness, 1.0f)
                .Build();
            if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(sphere))
                renderer->color = glm::vec4(0.92f, 0.92f, 0.94f, 1.0f);
        }
    }
}

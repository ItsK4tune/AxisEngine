#include "sample_scenario_common.h"

void SampleState::LoadScene23()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("LODDirLight")
        .WithTransform(glm::vec3(20.0f, 35.0f, 25.0f), glm::vec3(-45.0f, -35.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.6f, -1.0f, -0.5f)), glm::vec3(1.0f), 1.4f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("LODFloor")
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(90.0f, 1.0f, 120.0f))
        .WithMesh("planeModel", "deferred_lit_shadow")
        .WithPBRMaterial(0.0f, 0.85f, 1.0f)
        .Build();

    auto midModel = res.GetModel("cubeModel");
    auto farModel = res.GetModel("capsuleModel");
    for (int i = 0; i < 12; ++i)
    {
        float x = -22.0f + (i % 4) * 14.5f;
        float z = 28.0f - (i / 4) * 28.0f;
        auto entity = EntityBuilder(scene, res, "scenario")
            .WithName("LOD_" + std::to_string(i))
            .WithTransform(glm::vec3(x, 5.0f, z), glm::vec3(0.0f, i * 15.0f, 0.0f), glm::vec3(2.8f))
            .WithMesh("sphereModel", "deferred_lit_shadow")
            .WithPBRMaterial(0.1f, 0.45f, 1.0f)
            .Build();

        auto& lod = scene.registry.emplace<LODComponent>(entity);
        lod.lodDistancesSq = {900.0f, 2500.0f};
        lod.lodModels = {midModel, farModel};
        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(entity))
            renderer->color = glm::vec4(0.25f + 0.05f * i, 0.8f, 0.4f, 1.0f);
    }

    // Solid reference entity (no LOD) so user can compare LOD swaps against a stable mesh
    auto solidRef = EntityBuilder(scene, res, "scenario")
        .WithName("LOD_SolidReference")
        .WithTransform(glm::vec3(0.0f, 12.0f, 0.0f), glm::vec3(0.0f), glm::vec3(3.5f))
        .WithMesh("sphereModel", "deferred_lit_shadow")
        .WithPBRMaterial(0.8f, 0.2f, 1.0f)
        .Build();
    if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(solidRef))
        renderer->color = glm::vec4(1.0f, 0.35f, 0.1f, 1.0f);
}

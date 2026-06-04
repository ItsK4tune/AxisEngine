#include "sample_scenario_common.h"

void SampleState::LoadScene9()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("LODDirLight")
        .WithDirectionalLightAt(glm::vec3(20.0f, 35.0f, 25.0f), glm::vec3(-45.0f, -35.0f, 0.0f),
                                glm::normalize(glm::vec3(-0.6f, -1.0f, -0.5f)), glm::vec3(1.0f), 1.4f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("LODFloor")
        .WithPBRRenderable("planeModel", "deferred_lit_shadow", glm::vec3(0.0f), glm::vec3(0.0f),
                           glm::vec3(90.0f, 1.0f, 120.0f), 0.0f, 0.85f, 1.0f)
        .Build();

    for (int i = 0; i < 12; ++i)
    {
        float x = -22.0f + (i % 4) * 14.5f;
        float z = 28.0f - (i / 4) * 28.0f;
        EntityBuilder(scene, res, "scenario")
            .WithName("LOD_" + std::to_string(i))
            .WithPBRRenderable("sphereModel", "deferred_lit_shadow", glm::vec3(x, 5.0f, z),
                               glm::vec3(0.0f, i * 15.0f, 0.0f), 2.8f, 0.1f, 0.45f, 1.0f)
            .WithLOD({"cubeModel", "capsuleModel"}, {30.0f, 50.0f})
            .WithRendererColor(glm::vec4(0.25f + 0.05f * i, 0.8f, 0.4f, 1.0f))
            .Build();
    }

    // Solid reference entity (no LOD) so user can compare LOD swaps against a stable mesh
    EntityBuilder(scene, res, "scenario")
        .WithName("LOD_SolidReference")
        .WithPBRRenderable("sphereModel", "deferred_lit_shadow", glm::vec3(0.0f, 12.0f, 0.0f),
                           glm::vec3(0.0f), 3.5f, 0.8f, 0.2f, 1.0f)
        .WithRendererColor(glm::vec4(1.0f, 0.35f, 0.1f, 1.0f))
        .Build();
}

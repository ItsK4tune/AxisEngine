#include "sample_scenario_common.h"

void SampleState::LoadScene8()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    m_S8LayerMask = 0x7;

    EntityBuilder(scene, res, "scenario")
        .WithName("LayerDirLight")
        .WithTransform(glm::vec3(20.0f, 35.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.6f, -1.0f, -0.6f)), glm::vec3(1.0f), 1.3f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("LayerFloor")
        .WithLayer(0x1)
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.8f, 1.0f)
        .Build();

    for (int i = 0; i < 8; ++i)
    {
        auto cube = EntityBuilder(scene, res, "scenario")
            .WithName("LayerRedCube_" + std::to_string(i))
            .WithLayer(0x2)
            .WithTransform(glm::vec3(-18.0f + i * 5.0f, 2.0f, -8.0f), glm::vec3(0.0f, i * 20.0f, 0.0f), glm::vec3(2.0f))
            .WithPBRMesh("cubeModel", "deferred_lit", 0.0f, 0.55f, 1.0f)
            .Build();
        scene.registry.get<MeshRendererComponent>(cube).color = glm::vec4(1.0f, 0.15f, 0.1f, 1.0f);

        auto sphere = EntityBuilder(scene, res, "scenario")
            .WithName("LayerBlueSphere_" + std::to_string(i))
            .WithLayer(0x4)
            .WithTransform(glm::vec3(-18.0f + i * 5.0f, 2.0f, 8.0f), glm::vec3(0.0f), glm::vec3(2.0f))
            .WithPBRMesh("sphereModel", "deferred_lit", 0.0f, 0.35f, 1.0f)
            .Build();
        scene.registry.get<MeshRendererComponent>(sphere).color = glm::vec4(0.1f, 0.35f, 1.0f, 1.0f);
    }
}

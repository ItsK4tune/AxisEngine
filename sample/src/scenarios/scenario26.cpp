#include "sample_scenario_common.h"

void SampleState::LoadScene26()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("BatchLight")
        .WithTransform(glm::vec3(25.0f, 45.0f, 25.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.6f, -1.0f, -0.6f)), glm::vec3(1.0f), 1.2f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("BatchFloor")
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(180.0f, 1.0f, 180.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.85f, 1.0f)
        .Build();

    int side = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(m_S26InstanceCount))));
    float spacing = 2.7f;
    float offset = -side * spacing * 0.5f;
    for (int i = 0; i < m_S26InstanceCount; ++i)
    {
        int x = i % side;
        int z = i / side;
        auto entity = EntityBuilder(scene, res, "scenario")
            .WithName("BatchCube_" + std::to_string(i))
            .WithTransform(glm::vec3(offset + x * spacing, 1.3f, offset + z * spacing), glm::vec3(0.0f), glm::vec3(1.0f))
            .WithMesh("cubeModel", "deferred_lit")
            .WithPBRMaterial(0.05f, 0.55f, 1.0f)
            .Build();

        if (m_S26UniqueTint)
        {
            float hue = static_cast<float>(i % 97) / 97.0f;
            scene.registry.get<MeshRendererComponent>(entity).color =
                glm::vec4(0.35f + 0.55f * hue, 0.55f, 1.0f - 0.5f * hue, 1.0f);
        }
    }
}

#include "sample_scenario_common.h"

void SampleState::LoadScene27()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(m_S27EntityCount))));
    float spacing = (m_S27EntitySize * 1.6f > 1.5f) ? (m_S27EntitySize * 1.6f) : 1.5f;
    for (int i = 0; i < m_S27EntityCount; ++i)
    {
        int xIdx = i % columns;
        int zIdx = i / columns;
        glm::vec3 pos((xIdx - columns * 0.5f) * spacing, m_S27EntitySize * 0.5f, (zIdx - columns * 0.5f) * spacing);
        EntityBuilder(scene, res, "scenario")
            .WithName("DataEntity_" + std::to_string(i))
            .WithTransform(pos, glm::vec3(0.0f, i * 13.0f, 0.0f), glm::vec3(m_S27EntitySize))
            .WithPBRMesh((i % 2 == 0) ? "cubeModel" : "sphereModel", "deferred_lit", 0.2f, 0.5f, 1.0f)
            .Build();
    }

    m_S27Status = "Ready. Save/load entity count and size, then reload to apply.";
}

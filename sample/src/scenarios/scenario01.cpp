#include "sample_scenario_common.h"

void SampleState::LoadScene1()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    int count = 0;
    int size = static_cast<int>(std::ceil(std::pow(m_S1EntityCount, 1.0f / 3.0f))) + 2;
    float spacing = 3.5f;
    float offset = -(size * spacing) * 0.5f;

    std::string modelName = "sphereModel";
    if (m_S1MeshType == 1)
        modelName = "cubeModel";
    else if (m_S1MeshType == 2)
        modelName = "cylinderModel";
    else if (m_S1MeshType == 3)
        modelName = "capsuleModel";

    for (int x = 0; x < size && count < m_S1EntityCount; ++x)
    {
        for (int y = 0; y < size && count < m_S1EntityCount; ++y)
        {
            for (int z = 0; z < size && count < m_S1EntityCount; ++z)
            {
                glm::vec3 pos(offset + x * spacing + static_cast<float>(rand() % 100) / 200.0f,
                              y * spacing + static_cast<float>(rand() % 100) / 200.0f,
                              offset + z * spacing + static_cast<float>(rand() % 100) / 200.0f);

                auto entity =
                    EntityBuilder(scene, res, "scenario")
                        .WithName("Entity_" + std::to_string(count))
                        .WithPBRRenderable(modelName, "deferred_unlit", pos, glm::vec3(0.0f), 1.0f, 0.0f, 0.5f, 1.0f)
                        .Build();

                if (m_S1UniqueTint)
                {
                    float hue = static_cast<float>(count % 157) / 157.0f;
                    if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(entity))
                    {
                        renderer->color = glm::vec4(0.35f + 0.55f * hue, 0.75f - 0.25f * hue, 1.0f - 0.45f * hue, 1.0f);
                    }
                }

                count++;
            }
        }
    }
}

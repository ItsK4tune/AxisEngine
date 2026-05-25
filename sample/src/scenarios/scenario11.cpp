#include "sample_scenario_common.h"

void SampleState::LoadScene11()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    // 1. Static Floor & Lights
    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DecalDirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), m_S11LightColor, m_S11LightIntensity)
        .Build();

    // 2. Spawn a large central wall to project decals onto
    EntityBuilder(scene, res, "scenario")
        .WithName("DecalWall")
        .WithTransform(glm::vec3(0.0f, 10.0f, -10.0f), glm::vec3(0.0f), glm::vec3(50.0f, 20.0f, 2.0f))
        .WithMesh("cubeModel", "deferred_lit")
        .WithPBRMaterial(0.1f, 0.9f, 1.0f)
        .Build();

    // 3. Use tint-only decals. The decal shader falls back to a white source when no texture is assigned.
    uint32_t decalTexId = 0;

    // 4. Spawn Decal Components in a grid on the wall
    for (int i = 0; i < m_S11DecalCount; ++i)
    {
        float x = -20.0f + static_cast<float>(i % 10) * 4.5f;
        float y = 3.0f + static_cast<float>(i / 10) * 4.0f;
        float z = -9.0f; 

        float size = m_S11DecalSize;
        auto decalEnt = EntityBuilder(scene, res, "scenario")
            .WithName("Decal_" + std::to_string(i))
            .WithTransform(glm::vec3(x, y, z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(size, size, size))
            .Build();

        auto& decal = scene.registry.emplace<DecalComponent>(decalEnt);
        decal.albedoMap = decalTexId;
        decal.opacity = m_S11Opacity;
        decal.roughness = 0.5f;
        decal.metallic = 0.0f;
        
        if (m_S11RainbowMode)
        {
            float hue = static_cast<float>(i) / static_cast<float>(m_S11DecalCount);
            float r = 0.5f + 0.5f * sin(hue * 6.28318f);
            float g = 0.5f + 0.5f * sin(hue * 6.28318f + 2.09439f);
            float b = 0.5f + 0.5f * sin(hue * 6.28318f + 4.18879f);
            decal.tintColor = glm::vec4(r, g, b, 1.0f);
        }
        else
        {
            decal.tintColor = glm::vec4(m_S11Color, 1.0f);
        }
    }
}

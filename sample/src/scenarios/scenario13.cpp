#include "sample_scenario_common.h"

void SampleState::LoadScene13()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    // 1. Static Floor & Lights
    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.8f, 1.0f)
        .WithRigidShape(ShapeType::Box, glm::vec3(1.0f, 0.05f, 1.0f))
        .WithRigidBody(0.0f, true)
        .Build();

    m_S13DirLightEntity =
        EntityBuilder(scene, res, "scenario")
            .WithName("DecalDirLight")
            .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
            .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), m_S13LightColor, m_S13LightIntensity)
            .Build();
    Entity(m_S13DirLightEntity, &scene).SetLightCastShadow(true);

    m_S13PointLightEntity = EntityBuilder(scene, res, "scenario")
                                .WithName("DecalPointLight")
                                .WithPointLightAt(glm::vec3(-12.0f, 14.0f, 4.0f), m_S13PointLightColor,
                                                  m_S13PointLightIntensity, m_S13PointLightRadius)
                                .Build();
    {
        Entity light(m_S13PointLightEntity, &scene);
        light.SetLightActive(m_S13UsePointLight);
        light.SetLightCastShadow(true);
        light.SetLightAttenuation(0.045f, 0.0075f);
    }

    m_S13PointLightMarkerEntity = EntityBuilder(scene, res, "scenario")
                                       .WithName("DecalPointLightMarker")
                                       .WithTransform(glm::vec3(-12.0f, 14.0f, 4.0f), glm::vec3(0.0f), glm::vec3(1.2f))
                                       .WithPBRMesh("sphereModel", "deferred_unlit", 0.0f, 0.25f, 1.0f)
                                       .WithMaterialEmission(m_S13PointLightColor * 3.0f)
                                       .Build();
    Entity(m_S13PointLightMarkerEntity, &scene).SetColor(glm::vec4(m_S13PointLightColor, 1.0f));

    // 2. Spawn a large central wall to project decals onto
    EntityBuilder(scene, res, "scenario")
        .WithName("DecalWall")
        .WithTransform(glm::vec3(0.0f, 10.0f, -10.0f), glm::vec3(0.0f), glm::vec3(50.0f, 20.0f, 2.0f))
        .WithPBRMesh("cubeModel", "deferred_lit", 0.1f, 0.9f, 1.0f)
        .WithRigidShape(ShapeType::Box)
        .WithRigidBody(0.0f, true)
        .Build();

    m_S13ShadowCasterEntity =
        EntityBuilder(scene, res, "scenario")
            .WithName("DecalShadowCaster")
            .WithActive(m_S13ShowShadowCaster)
            .WithTransform(glm::vec3(-4.0f, 10.0f, -4.2f), glm::vec3(0.0f, 18.0f, 0.0f), glm::vec3(3.0f, 12.0f, 1.5f))
            .WithPBRMesh("cubeModel", "deferred_lit_shadow", 0.0f, 0.55f, 1.0f)
            .WithRigidShape(ShapeType::Box)
            .WithRigidBody(0.0f, true)
            .Build();
    {
        Entity renderer(m_S13ShadowCasterEntity, &scene);
        renderer.SetColor(glm::vec4(0.12f, 0.12f, 0.14f, 1.0f));
        renderer.SetCastShadow(true);
        renderer.SetReceiveShadow(false);
    }

    // 3. Use tint-only decals. The decal shader falls back to a white source when no texture is assigned.
    uint32_t decalTexId = 0;

    // 4. Spawn Decal Components in a grid on the wall
    for (int i = 0; i < m_S13DecalCount; ++i)
    {
        float x = -20.0f + static_cast<float>(i % 10) * 4.5f;
        float y = 3.0f + static_cast<float>(i / 10) * 4.0f;
        float z = -9.0f;

        float size = m_S13DecalSize;
        glm::vec4 tint = glm::vec4(m_S13Color, 1.0f);
        if (m_S13RainbowMode)
        {
            float hue = static_cast<float>(i) / static_cast<float>(m_S13DecalCount);
            float r = 0.5f + 0.5f * sin(hue * 6.28318f);
            float g = 0.5f + 0.5f * sin(hue * 6.28318f + 2.09439f);
            float b = 0.5f + 0.5f * sin(hue * 6.28318f + 4.18879f);
            tint = glm::vec4(r, g, b, 1.0f);
        }

        EntityBuilder(scene, res, "scenario")
            .WithName("Decal_" + std::to_string(i))
            .WithTransform(glm::vec3(x, y, z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(size, size, size))
            .WithDecal(decalTexId, m_S13Opacity, 0.5f, 0.0f, m_S13LightingMode, tint)
            .Build();
    }
}

#include "sample_scenario_common.h"

void SampleState::LoadScene2()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(100.0f, 1.0f, 100.0f))
        .WithPBRMesh("planeModel", "deferred_lit_shadow", 0.0f, 0.8f, 1.0f)
        .Build();

    for (int i = 0; i < 1000; ++i)
    {
        float x = static_cast<float>(rand() % 2000) / 10.0f - 100.0f;
        float z = static_cast<float>(rand() % 2000) / 10.0f - 100.0f;
        float h = 1.0f + static_cast<float>(rand() % 50) / 10.0f;

        auto cube = EntityBuilder(scene, res, "scenario")
            .WithName("Cube_" + std::to_string(i))
            .WithTransform(glm::vec3(x, h * 0.5f + 0.5f, z), glm::vec3(0.0f, rand() % 360, 0.0f), glm::vec3(1.0f, h, 1.0f))
            .WithPBRMesh("cubeModel", "deferred_lit_shadow", 0.1f, 0.6f, 1.0f)
            .Build();
    }

    auto dir = EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithPBRRenderable("sphereModel", "deferred_lit", glm::vec3(20.0f, 40.0f, 20.0f),
                           glm::vec3(-45.0f, -45.0f, 0.0f), 2.0f, 0.0f, 0.5f, 1.0f)
        .WithMeshRenderOptions(false, false)
        .WithRendererColor(glm::vec4(m_S2DirectionalColor * (m_S2DirectionalIntensity * 3.0f), 1.0f))
        .WithMaterialEmission(m_S2DirectionalColor * (m_S2DirectionalIntensity * 3.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), m_S2DirectionalColor, m_S2DirectionalIntensity)
        .Build();
    m_S2DirLightEntity = dir;

    auto point = EntityBuilder(scene, res, "scenario")
        .WithName("PointLight")
        .WithPBRRenderable("sphereModel", "deferred_lit", glm::vec3(0.0f, 8.0f, 0.0f), glm::vec3(0.0f),
                           0.5f, 0.0f, 0.5f, 1.0f)
        .WithMeshRenderOptions(false, false)
        .WithRendererColor(glm::vec4(m_S2PointColor * (m_S2PointIntensity * 2.0f), 1.0f))
        .WithMaterialEmission(m_S2PointColor * (m_S2PointIntensity * 2.0f))
        .WithPointLight(m_S2PointColor, m_S2PointIntensity, 30.0f)
        .Build();
    scene.registry.get<PointLightComponent>(point).isCastShadow = true;

    auto spot = EntityBuilder(scene, res, "scenario")
        .WithName("SpotLight")
        .WithPBRRenderable("sphereModel", "deferred_lit",
                           glm::vec3(m_S2SpotOrbitRadius, m_S2SpotMotionHeight, 0.0f),
                           glm::vec3(-90.0f, 0.0f, 0.0f), 0.8f, 0.0f, 0.5f, 1.0f)
        .WithMeshRenderOptions(false, false)
        .WithRendererColor(glm::vec4(m_S2SpotColor * (m_S2SpotIntensity * 1.25f), 1.0f))
        .WithMaterialEmission(m_S2SpotColor * (m_S2SpotIntensity * 1.25f))
        .WithSpotLight(glm::vec3(0.0f, -1.0f, 0.0f), m_S2SpotColor, m_S2SpotIntensity)
        .Build();
    auto& spotLight = scene.registry.get<SpotLightComponent>(spot);
    spotLight.cutOff = glm::cos(glm::radians(22.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(32.5f));
    spotLight.linear = 0.045f;
    spotLight.quadratic = 0.0075f;
    scene.registry.get<RotationComponent>(spot).value =
        RotationFromNegativeY(glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f) - glm::vec3(m_S2SpotOrbitRadius, m_S2SpotMotionHeight, 0.0f)));
    scene.registry.get<SpotLightComponent>(spot).isCastShadow = true;
}

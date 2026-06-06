#include "sample_scenario_common.h"

void SampleState::LoadScene11()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    m_S11ReflectionSpheres.clear();
    m_S11ReflectionProbes.clear();
    m_S11ActiveCase = 0;

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("ReflectionFillLight")
        .WithTransform(glm::vec3(0.0f, 12.0f, 12.0f), glm::vec3(0.0f), glm::vec3(0.8f))
        .WithPBRMesh("sphereModel", "deferred_lit", 0.0f, 0.3f, 1.0f)
        .WithPointLight(glm::vec3(1.0f, 0.92f, 0.75f), 12.0f, 45.0f)
        .Build();

    auto floor = EntityBuilder(scene, res, "scenario")
        .WithName("ReflectiveFloor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.02f, 0.35f, 1.0f)
        .Build();
    if (auto* floorRenderer = scene.TryGetComponent<MeshRendererComponent>(floor))
        floorRenderer->color = glm::vec4(0.55f, 0.55f, 0.52f, 1.0f);

    auto planarMirror = EntityBuilder(scene, res, "scenario")
        .WithName("PlanarMirror")
        .WithTransform(glm::vec3(0.0f, 0.00f, -50.0f), glm::vec3(90.0f, 0.0f, 0.0f), glm::vec3(38.0f, 1.0f, 24.0f))
        .WithPBRMesh("planeModel", "deferred_reflect", 0.0f, 0.04f, 1.0f)
        .WithPlanarReflection(1024)
        .WithReflective(0.85f, 3.0f, 0.08f)
        .Build();
    if (auto* mirrorRenderer = scene.TryGetComponent<MeshRendererComponent>(planarMirror))
    {
        mirrorRenderer->color = glm::vec4(0.78f, 0.84f, 0.9f, 1.0f);
        mirrorRenderer->receiveShadow = false;
    }
    m_S11PlanarMirror = planarMirror;

    auto wallRed = EntityBuilder(scene, res, "scenario")
        .WithName("WallRed")
        .WithTransform(glm::vec3(-32.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(2.0f, 10.0f, 20.0f))
        .WithPBRMesh("cubeModel", "deferred_lit", 0.0f, 0.65f, 1.0f)
        .Build();
    auto* rRed = scene.TryGetComponent<MeshRendererComponent>(wallRed);
    if (rRed) rRed->color = glm::vec4(1.0f, 0.1f, 0.1f, 1.0f);

    auto wallBlue = EntityBuilder(scene, res, "scenario")
        .WithName("WallBlue")
        .WithTransform(glm::vec3(32.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(2.0f, 10.0f, 20.0f))
        .WithPBRMesh("cubeModel", "deferred_lit", 0.0f, 0.65f, 1.0f)
        .Build();
    auto* rBlue = scene.TryGetComponent<MeshRendererComponent>(wallBlue);
    if (rBlue) rBlue->color = glm::vec4(0.1f, 0.1f, 1.0f, 1.0f);

    struct CaseDef
    {
        const char* caseName;
        const char* probeName;
        const char* sphereName;
        glm::vec3 pos;
        glm::vec4 tint;
        int probeResolution;
        float reflectivity;
        float fresnelBias;
        float fresnelPower;
    };

    const CaseDef cases[] = {
        {"UltraLow", "ProbeUltraLow", "ReflectiveUltraLow", glm::vec3(-24.0f, 6.0f, -8.0f),
         glm::vec4(0.96f, 0.84f, 0.68f, 1.0f), 64, 0.08f, 0.01f, 0.5f},
        {"Low", "ProbeLow", "ReflectiveLow", glm::vec3(-14.0f, 6.0f, -3.0f), glm::vec4(0.95f, 0.85f, 0.7f, 1.0f), 128,
         0.2f, 0.02f, 1.0f},
        {"Mid", "CenterProbe", "ReflectiveMid", glm::vec3(-2.0f, 6.0f, 0.0f), glm::vec4(0.85f, 0.85f, 0.9f, 1.0f),
         m_S11ProbeResolution, m_S11Reflectivity, m_S11FresnelBias, m_S11FresnelPower},
        {"High", "ProbeHigh", "ReflectiveHigh", glm::vec3(10.0f, 6.0f, 4.0f), glm::vec4(0.75f, 0.9f, 1.0f, 1.0f), 1024,
         0.92f, 0.08f, 8.0f},
        {"Extreme", "ProbeExtreme", "ReflectiveExtreme", glm::vec3(20.0f, 6.0f, 8.0f),
         glm::vec4(0.9f, 0.9f, 0.98f, 1.0f), 2048, 1.0f, 0.24f, 16.0f},
    };

    for (const auto& c : cases)
    {
        auto probeEntity = EntityBuilder(scene, res, "scenario")
            .WithName(c.probeName)
            .WithTransform(c.pos + glm::vec3(0.0f, 0.0f, -6.0f), glm::vec3(0.0f), glm::vec3(1.0f))
            .WithReflectionProbe(ReflectionProbeType::Dynamic, c.probeResolution, true)
            .Build();
        if (auto* probeComp = scene.TryGetComponent<ReflectionProbeComponent>(probeEntity))
        {
            probeComp->boxMin = glm::vec3(-24.0f, -6.0f, -24.0f);
            probeComp->boxMax = glm::vec3(24.0f, 16.0f, 24.0f);
            probeComp->blendDistance = 6.0f;
        }
        m_S11ReflectionProbes.push_back(probeEntity);

        auto sphere = EntityBuilder(scene, res, "scenario")
            .WithName(c.sphereName)
            .WithTransform(c.pos, glm::vec3(0.0f), glm::vec3(4.0f))
            .WithPBRMesh("sphereModel", "deferred_reflect", 0.05f, 0.95f, 1.0f)
            .WithReflective(c.reflectivity, c.fresnelPower, c.fresnelBias)
            .Build();

        if (auto* sphereRenderer = scene.TryGetComponent<MeshRendererComponent>(sphere))
            sphereRenderer->color = c.tint;

        if (auto* refComp = scene.TryGetComponent<ReflectiveComponent>(sphere))
        {
            refComp->targetProbe = c.probeName;
        }
        m_S11ReflectionSpheres.push_back(sphere);
    }
}

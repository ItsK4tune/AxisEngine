#include "sample_scenario_common.h"

void SampleState::LoadScene12()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("S12_Floor")
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(46.0f, 1.0f, 46.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("S12_DimDirectional")
        .WithTransform(glm::vec3(0.0f, 28.0f, 0.0f), glm::vec3(-45.0f, -30.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f)), glm::vec3(1.0f), 0.18f)
        .Build();

    auto probe = EntityBuilder(scene, res, "scenario")
                     .WithName("S12_WarmLightProbe")
                     .WithTransform(glm::vec3(-8.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.2f))
                     .WithPBRMesh("sphereModel", "deferred_unlit", 0.0f, 0.5f, 1.0f)
                     .WithLightProbe(m_S12ProbeRadius, m_S12ProbeIntensity)
                     .Build();
    {
        Entity p(probe, &scene);
        p.SetLightProbeSH(0, glm::vec3(0.95f, 0.56f, 0.25f));
        p.SetLightProbeSH(1, glm::vec3(0.22f, 0.06f, 0.02f));
        p.SetLightProbeSH(2, glm::vec3(0.12f, 0.16f, 0.22f));
        p.SetColor(glm::vec4(1.0f, 0.48f, 0.15f, 1.0f));
    }

    auto coolProbe = EntityBuilder(scene, res, "scenario")
                          .WithName("S12_CoolLightProbe")
                          .WithTransform(glm::vec3(9.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.2f))
                          .WithPBRMesh("sphereModel", "deferred_unlit", 0.0f, 0.5f, 1.0f)
                          .WithLightProbe(m_S12ProbeRadius, m_S12ProbeIntensity)
                          .Build();
    {
        Entity cp(coolProbe, &scene);
        cp.SetLightProbeSH(0, glm::vec3(0.18f, 0.42f, 0.95f));
        cp.SetLightProbeSH(1, glm::vec3(-0.04f, 0.04f, 0.22f));
        cp.SetLightProbeSH(2, glm::vec3(0.04f, 0.08f, 0.25f));
        cp.SetColor(glm::vec4(0.25f, 0.55f, 1.0f, 1.0f));
    }

    for (int i = 0; i < 8; ++i)
    {
        auto entity = EntityBuilder(scene, res, "scenario")
                          .WithName("S12_ProbeLitObject_" + std::to_string(i))
                          .WithTransform(glm::vec3(-14.0f + i * 4.0f, 3.0f, 8.0f * std::sin(i * 0.7f)), glm::vec3(0.0f),
                                         glm::vec3(1.8f))
                          .WithPBRMesh((i % 2) ? "cubeModel" : "sphereModel", "deferred_lit", 0.0f, 0.5f, 1.0f)
                          .Build();
        Entity(entity, &scene).SetColor(glm::vec4(0.85f, 0.85f, 0.85f, 1.0f));
    }
}

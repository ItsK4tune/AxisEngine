#include "sample_scenario_common.h"

void SampleState::LoadScene10()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(60.0f, 1.0f, 60.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.9f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(0.2f, 0.2f, 0.2f), 0.5f)
        .Build();

    struct GlowingObject {
        glm::vec3 pos;
        glm::vec3 emissionColor;
        float scale;
    };

    std::vector<GlowingObject> glowers = {
        { glm::vec3(-10.0f, 2.0f, 0.0f), glm::vec3(10.0f, 0.0f, 0.0f), 2.0f },
        { glm::vec3(0.0f, 2.0f, -10.0f), glm::vec3(0.0f, 10.0f, 0.0f), 2.0f },
        { glm::vec3(10.0f, 2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 10.0f), 2.0f },
        { glm::vec3(0.0f, 3.0f, 10.0f), glm::vec3(10.0f, 10.0f, 0.0f), 3.0f },
        { glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(20.0f, 20.0f, 20.0f), 1.5f }
    };

    int index = 0;
    for (const auto& g : glowers)
    {
        auto ent = EntityBuilder(scene, res, "scenario")
            .WithName("Glower_" + std::to_string(index++))
            .WithTransform(g.pos, glm::vec3(0.0f), glm::vec3(g.scale))
            .WithPBRMesh("sphereModel", "deferred_lit", 0.0f, 0.1f, 1.0f)
            .Build();

        Entity(ent, &scene).SetEmission(g.emissionColor);

        EntityBuilder(scene, res, "scenario")
            .WithName("GlowLight_" + std::to_string(index))
            .WithTransform(g.pos)
            .WithPointLight(glm::normalize(g.emissionColor), glm::length(g.emissionColor) * 0.5f, 15.0f)
            .Build();
    }

    m_PPBloomEnabled = true;
    m_PPBloomThreshold = 0.8f;
    m_PPBloomIntensity = 2.0f;
    m_PPBloomRadius = 0.005f;
    m_PPHdrEnabled = true;
    m_PPExposure = 1.0f;
    m_PPGamma = 2.2f;
    m_PPTonemappingMode = 1;
    m_PPVignetteEnabled = true;
    m_PPFilmGrainEnabled = true;
    m_PPGrayEnabled = false;
    m_PPDitherEnabled = false;
    m_PPPartialEffectEnabled = true;
    m_PPPartialEffectType = 2;
    m_PPPartialX = 180;
    m_PPPartialY = 140;
    m_PPPartialW = 520;
    m_PPPartialH = 300;

    EntityBuilder(scene, res, "scenario")
        .WithName("Scenario10PostProcess")
        .WithPostProcess(PostProcessComponent{true, {}})
        .Build();

    ConfigurePostProcessing(m_PPHdrEnabled, m_PPBloomEnabled, m_PPBloomThreshold, m_PPBloomIntensity, m_PPBloomRadius, m_PPExposure, m_PPGamma, m_PPTonemappingMode);
}

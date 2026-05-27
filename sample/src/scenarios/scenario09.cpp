#include "sample_scenario_common.h"

void SampleState::LoadScene9()
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

        auto& mat = scene.registry.get<AxisMaterialComponent>(ent);
        mat.desc.emission = g.emissionColor;
        mat.gpu.dirty = true;

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

    auto ppEntity = EntityBuilder(scene, res, "scenario")
        .WithName("Scenario9PostProcess")
        .Build();
    auto& pp = scene.registry.emplace<PostProcessComponent>(ppEntity);
    pp.enabled = true;

    auto* sysMgr = Resolve<SystemManager>();
    if (sysMgr)
    {
        auto* ppSys = dynamic_cast<PostProcessSystem*>(sysMgr->GetSystem("PostProcessSystem"));
        if (ppSys)
        {
            auto& pipeline = ppSys->GetPipeline();
            pipeline.SetHDREnabled(m_PPHdrEnabled);
            pipeline.SetBloomEnabled(m_PPBloomEnabled);
            pipeline.SetBloomThreshold(m_PPBloomThreshold);
            pipeline.SetBloomIntensity(m_PPBloomIntensity);
            pipeline.SetBloomRadius(m_PPBloomRadius);
            pipeline.SetExposure(m_PPExposure);
            pipeline.SetGamma(m_PPGamma);
            pipeline.SetTonemappingMode(m_PPTonemappingMode);
        }
    }
}

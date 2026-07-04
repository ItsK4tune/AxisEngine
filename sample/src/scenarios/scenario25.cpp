#include "sample_scenario_common.h"

void SampleState::LoadScene25()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithScene(kScenario25BaseSceneName)
        .WithName("Ground")
        .WithTransient(false)
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithScene(kScenario25BaseSceneName)
        .WithName("DirLight")
        .WithTransient(false)
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    m_S25RandomEntities.clear();
    m_S25RandomEntityCount = 0;
    m_S25Status = "Base scene ready. Dynamic entities stay under scenario.";
}

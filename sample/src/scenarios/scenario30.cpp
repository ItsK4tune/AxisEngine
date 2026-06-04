#include "sample_scenario_common.h"

void SampleState::LoadScene30()
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

    EntityBuilder(scene, res, "scenario")
        .WithName("NetworkOrb")
        .WithTransform(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(3.0f))
        .WithPBRMesh("sphereModel", "deferred_lit", 0.1f, 0.9f, 0.1f)
        .Build();

    m_S30Messages.clear();
    m_S30SpawnCounter = 0;
    m_S30Status = "Network scenario initialized. Click Start to bind/connect.";
}

#include "sample_scenario_common.h"

void SampleState::LoadScene27()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("ShadowReceiverFloor")
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(85.0f, 1.0f, 85.0f))
        .WithMesh("planeModel", "deferred_lit_shadow")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    auto casterA = EntityBuilder(scene, res, "scenario")
        .WithName("ShadowCasterCube")
        .WithTransform(glm::vec3(-6.0f, 5.0f, 0.0f), glm::vec3(0.0f, 25.0f, 0.0f), glm::vec3(4.0f, 8.0f, 4.0f))
        .WithMesh("cubeModel", "deferred_lit_shadow")
        .WithPBRMaterial(0.1f, 0.5f, 1.0f)
        .Build();
    {
        auto& r = scene.registry.get<MeshRendererComponent>(casterA);
        r.receiveShadow = false;
        r.castShadow = true;
    }

    auto casterB = EntityBuilder(scene, res, "scenario")
        .WithName("ShadowCasterSphere")
        .WithTransform(glm::vec3(7.0f, 5.0f, -3.0f), glm::vec3(0.0f), glm::vec3(4.0f))
        .WithMesh("sphereModel", "deferred_lit_shadow")
        .WithPBRMaterial(0.0f, 0.35f, 1.0f)
        .Build();
    {
        auto& r = scene.registry.get<MeshRendererComponent>(casterB);
        r.receiveShadow = false;
        r.castShadow = true;
    }

    auto light = EntityBuilder(scene, res, "scenario")
        .WithName("DeferredShadowDirLight")
        .WithTransform(glm::vec3(25.0f, 40.0f, 20.0f), glm::vec3(-50.0f, -40.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.55f, -1.0f, -0.35f)), glm::vec3(1.0f), 1.4f)
        .Build();
    if (auto* dir = scene.registry.try_get<DirectionalLightComponent>(light))
        dir->isCastShadow = true;
}

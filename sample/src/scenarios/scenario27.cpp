#include "sample_scenario_common.h"

void SampleState::LoadScene27()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("ShadowReceiverFloor")
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(85.0f, 1.0f, 85.0f))
        .WithPBRMesh("planeModel", "deferred_lit_shadow", 0.0f, 0.8f, 1.0f)
        .Build();

    auto casterA =
        EntityBuilder(scene, res, "scenario")
            .WithName("DeferredShadowCasterCube")
            .WithTransform(glm::vec3(-16.0f, 5.0f, -1.0f), glm::vec3(0.0f, 25.0f, 0.0f),
                           glm::vec3(4.0f, 8.0f, 4.0f))
            .WithPBRMesh("cubeModel", "deferred_lit_shadow", 0.1f, 0.5f, 1.0f)
            .Build();
    {
        auto& r = scene.registry.get<MeshRendererComponent>(casterA);
        r.receiveShadow = false;
        r.castShadow = true;
        r.color = glm::vec4(0.18f, 0.46f, 1.0f, 1.0f);
    }

    auto casterB = EntityBuilder(scene, res, "scenario")
                       .WithName("DeferredShadowCasterSphere")
                       .WithTransform(glm::vec3(-9.0f, 5.0f, 4.0f), glm::vec3(0.0f), glm::vec3(4.0f))
                       .WithPBRMesh("sphereModel", "deferred_lit_shadow", 0.0f, 0.35f, 1.0f)
                       .Build();
    {
        auto& r = scene.registry.get<MeshRendererComponent>(casterB);
        r.receiveShadow = false;
        r.castShadow = true;
        r.color = glm::vec4(0.10f, 0.70f, 1.0f, 1.0f);
    }

    auto forwardCasterA =
        EntityBuilder(scene, res, "scenario")
            .WithName("ForwardShadowCasterCube")
            .WithTransform(glm::vec3(10.0f, 6.0f, -1.0f), glm::vec3(0.0f, -18.0f, 0.0f),
                           glm::vec3(3.5f, 7.0f, 3.5f))
            .WithPBRMesh("cubeModel", "forward_pbr_lit_shadow", 0.0f, 0.45f, 1.0f)
            .Build();
    {
        auto& r = scene.registry.get<MeshRendererComponent>(forwardCasterA);
        r.renderMode = RenderMode::ForceForward;
        r.receiveShadow = false;
        r.castShadow = true;
        r.color = glm::vec4(1.0f, 0.18f, 0.14f, 1.0f);
    }

    auto forwardCasterB =
        EntityBuilder(scene, res, "scenario")
            .WithName("ForwardShadowCasterCapsule")
            .WithTransform(glm::vec3(18.0f, 4.0f, 4.0f), glm::vec3(0.0f, 35.0f, 0.0f),
                           glm::vec3(2.8f, 5.2f, 2.8f))
            .WithPBRMesh("capsuleSmoothModel", "forward_unlit", 0.0f, 0.5f, 1.0f)
            .Build();
    {
        auto& r = scene.registry.get<MeshRendererComponent>(forwardCasterB);
        r.renderMode = RenderMode::ForceForward;
        r.receiveShadow = false;
        r.castShadow = true;
        r.color = glm::vec4(1.0f, 0.36f, 0.22f, 1.0f);
    }

    auto light = EntityBuilder(scene, res, "scenario")
                     .WithName("DeferredShadowDirLight")
                     .WithTransform(glm::vec3(25.0f, 40.0f, 20.0f), glm::vec3(-50.0f, -40.0f, 0.0f))
                     .WithDirectionalLight(glm::normalize(glm::vec3(-0.55f, -1.0f, -0.35f)), glm::vec3(1.0f), 1.4f)
                     .Build();
    if (auto* dir = scene.registry.try_get<DirectionalLightComponent>(light))
        dir->isCastShadow = true;
}

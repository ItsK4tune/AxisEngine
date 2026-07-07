#include "sample_scenario_common.h"

void SampleState::LoadScene19()
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

    // Models must be loaded before clips because AnimationManager binds clips to the model skeleton.
    res.LoadModel("defeatedModel", "sample/resource/object/defeated.fbx");
    res.LoadModel("spinModel", "sample/resource/object/spin.fbx");
    res.LoadAnimation("defeated", "sample/resource/object/defeated.fbx", "defeatedModel");
    res.LoadAnimation("spin", "sample/resource/object/spin.fbx", "spinModel");

    auto defeatedEntity = EntityBuilder(scene, res, "scenario")
        .WithName("DefeatedFbxCharacter")
        .WithTransform(glm::vec3(-5.0f, 3.0f, 0.0f), glm::vec3(0.0f), glm::vec3(4.0f))
        .WithMesh("defeatedModel", "deferred_lit")
        .WithAnimation("defeated")
        .Build();

    auto spinEntity = EntityBuilder(scene, res, "scenario")
        .WithName("SpinFbxCharacter")
        .WithTransform(glm::vec3(5.0f, 3.0f, 0.0f), glm::vec3(0.0f), glm::vec3(4.0f))
        .WithMesh("spinModel", "deferred_lit")
        .WithAnimation("spin")
        .Build();

    Entity(defeatedEntity, &scene).AddAnimationClip("spin", res.GetAnimation("spin"));
    Entity(spinEntity, &scene).AddAnimationClip("defeated", res.GetAnimation("defeated"));
}

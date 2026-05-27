#include "sample_scenario_common.h"

void SampleState::LoadScene14()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.1f, 0.1f, 0.1f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    auto viCube = EntityBuilder(scene, res, "scenario")
        .WithName("viCube")
        .WithTransform(glm::vec3(-10.0f, 3.0f, 0.0f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(4.0f))
        .WithPBRMesh("cubeModel", "deferred_lit", 1.0f, 0.1f, 0.1f)
        .Build();
    auto* rVi = scene.registry.try_get<MeshRendererComponent>(viCube);
    if (rVi) rVi->color = glm::vec4(1.0f, 0.1f, 0.1f, 1.0f);

    auto enCube = EntityBuilder(scene, res, "scenario")
        .WithName("enCube")
        .WithTransform(glm::vec3(10.0f, 3.0f, 0.0f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(4.0f))
        .WithPBRMesh("cubeModel", "deferred_lit", 0.1f, 0.1f, 1.0f)
        .Build();
    auto* rEn = scene.registry.try_get<MeshRendererComponent>(enCube);
    if (rEn) rEn->color = glm::vec4(0.1f, 0.1f, 1.0f, 1.0f);

    auto& l10n = GetSystem<LocalizationSystem>();
    l10n.LoadLanguage("sample/resource/l10n/vi.axs", "vi");
    l10n.LoadLanguage("sample/resource/l10n/en.axs", "en");
    l10n.SetLanguage("en");
}

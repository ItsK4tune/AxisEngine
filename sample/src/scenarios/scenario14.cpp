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

    auto& l10n = GetSystem<LocalizationSystem>();
    l10n.LoadLanguage("sample/resource/l10n/vi.axs", "vi");
    l10n.LoadLanguage("sample/resource/l10n/en.axs", "en");
    l10n.SetLanguage("en");

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

    auto panel = EntityBuilder(scene, res, "scenario")
        .WithName("L10nPreviewPanel")
        .WithUIAnchored(glm::vec2(1.0f, 0.0f), glm::vec2(-620.0f, 24.0f), glm::vec2(560.0f, 300.0f), 24)
        .WithUIRenderer("l10n_preview_panel", glm::vec4(0.08f, 0.10f, 0.13f, 0.94f))
        .WithUIFlex(FlexDirection::Column, 12.0f)
        .Build();
    scene.registry.get<UIFlexLayoutComponent>(panel).autoSize = false;

    EntityBuilder(scene, res, "scenario")
        .WithName("L10nPanelTitle")
        .WithUIChild(panel, glm::vec2(20.0f, 20.0f), glm::vec2(520.0f, 48.0f), 25)
        .WithUIText(l10n.Get("app.title"), "time", 1.05f, glm::vec4(0.65f, 0.95f, 1.0f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 500.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("L10nCurrentLanguageText")
        .WithUIChild(panel, glm::vec2(20.0f, 74.0f), glm::vec2(520.0f, 42.0f), 25)
        .WithUIText(l10n.GetFormat("l10n.preview.current_language", l10n.GetLanguage()), "time", 0.86f,
                    glm::vec4(0.95f, 0.95f, 0.95f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 500.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("L10nScenarioLabelText")
        .WithUIChild(panel, glm::vec2(20.0f, 120.0f), glm::vec2(520.0f, 42.0f), 25)
        .WithUIText(l10n.Get("menu.select_scenario"), "time", 0.86f, glm::vec4(0.85f, 0.90f, 0.96f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 500.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("L10nEntityCountText")
        .WithUIChild(panel, glm::vec2(20.0f, 166.0f), glm::vec2(520.0f, 42.0f), 25)
        .WithUIText(l10n.GetFormat("scenario.active_entities", std::to_string((int)scene.registry.view<InfoComponent>().size())),
                    "time", 0.86f, glm::vec4(0.85f, 0.90f, 0.96f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 500.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("L10nReloadText")
        .WithUIChild(panel, glm::vec2(20.0f, 212.0f), glm::vec2(520.0f, 58.0f), 25)
        .WithUIText(l10n.Get("l10n.preview.description") + "\n" + l10n.Get("menu.reload_scenario"), "time", 0.82f,
                    glm::vec4(1.0f, 0.88f, 0.48f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 500.0f)
        .Build();
}

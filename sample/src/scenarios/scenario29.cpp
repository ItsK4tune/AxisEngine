#include "sample_scenario_common.h"

void SampleState::LoadScene29()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    auto& l10n = GetSystem<LocalizationSystem>();
    l10n.LoadLanguage("sample/resource/l10n/vi.axs", "vi");
    l10n.LoadLanguage("sample/resource/l10n/en.axs", "en");
    l10n.SetLanguage("en");

    // A beautiful centered panel for testing localization (size doubled to 1200x880, shifted to the right)
    auto panel = EntityBuilder(scene, res, "scenario")
        .WithName("L10nPreviewPanel")
        .WithUIAnchored(glm::vec2(0.5f, 0.5f), glm::vec2(-500.0f, -440.0f), glm::vec2(1200.0f, 880.0f), 24)
        .WithUIRenderer("l10n_preview_panel", glm::vec4(0.08f, 0.10f, 0.14f, 0.96f))
        .Build();

    // Title (doubled size and Y offsets)
    EntityBuilder(scene, res, "scenario")
        .WithName("L10nPanelTitle")
        .WithUIChild(panel, glm::vec2(60.0f, 60.0f), glm::vec2(1080.0f, 96.0f), 25)
        .WithUIText(l10n.Get("app.title"), "time", 0.64f, glm::vec4(0.65f, 0.95f, 1.0f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 1040.0f)
        .Build();

    // Current Language
    EntityBuilder(scene, res, "scenario")
        .WithName("L10nCurrentLanguageText")
        .WithUIChild(panel, glm::vec2(60.0f, 160.0f), glm::vec2(1080.0f, 72.0f), 25)
        .WithUIText(l10n.GetFormat("l10n.preview.current_language", l10n.GetLanguage()), "time", 0.44f,
                    glm::vec4(0.95f, 0.95f, 0.95f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 1040.0f)
        .Build();

    // Active Entities
    EntityBuilder(scene, res, "scenario")
        .WithName("L10nEntityCountText")
        .WithUIChild(panel, glm::vec2(60.0f, 240.0f), glm::vec2(1080.0f, 72.0f), 25)
        .WithUIText(l10n.GetFormat("scenario.active_entities", std::to_string((int)GetEntityCount())),
                    "time", 0.44f, glm::vec4(0.85f, 0.90f, 0.96f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 1040.0f)
        .Build();

    // Test Welcome
    EntityBuilder(scene, res, "scenario")
        .WithName("L10nTestWelcomeText")
        .WithUIChild(panel, glm::vec2(60.0f, 320.0f), glm::vec2(1080.0f, 72.0f), 25)
        .WithUIText(l10n.Get("l10n.test.welcome"), "time", 0.44f, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 1040.0f)
        .Build();

    // Test Parameterized
    EntityBuilder(scene, res, "scenario")
        .WithName("L10nTestParameterizedText")
        .WithUIChild(panel, glm::vec2(60.0f, 400.0f), glm::vec2(1080.0f, 72.0f), 25)
        .WithUIText(l10n.GetFormat("l10n.test.parameterized", "42", "HelloWorld"), "time", 0.44f,
                    glm::vec4(0.85f, 0.90f, 0.96f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 1040.0f)
        .Build();

    // Test MultiLine & Reload Info
    EntityBuilder(scene, res, "scenario")
        .WithName("L10nTestMultiLineText")
        .WithUIChild(panel, glm::vec2(60.0f, 480.0f), glm::vec2(1080.0f, 260.0f), 25)
        .WithUIText(l10n.Get("l10n.test.multi_line") + "\n\n" + l10n.Get("menu.reload_scenario"), "time", 0.42f,
                    glm::vec4(1.0f, 0.88f, 0.48f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 1040.0f)
        .Build();
}

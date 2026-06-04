#include "sample_scenario_common.h"

void SampleState::LoadScene16()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    // ─── Part 1: Scenario 28 UI Showcase ───
    auto root = EntityBuilder(scene, res, "scenario")
                    .WithName("UIRootPanel")
                    .WithUIAnchored(glm::vec2(1.0f, 0.0f), glm::vec2(-1280.0f, 24.0f), glm::vec2(620.0f, 520.0f), 10)
                    .WithUIRenderer("ui_panel_28", glm::vec4(0.08f, 0.10f, 0.13f, 0.94f))
                    .WithUIFlex(FlexDirection::Column, 14.0f)
                    .Build();
    scene.registry.get<UIFlexLayoutComponent>(root).autoSize = false;

    EntityBuilder(scene, res, "scenario")
        .WithName("UIRootTitle")
        .WithUIChild(root, glm::vec2(-5.0f, 30.0f), glm::vec2(560.0f, 54.0f), 11)
        .WithUIText("UI Showcase - transform / text / texture / pivot / rotation / flip", "time", 1.05f,
                    glm::vec4(0.65f, 0.95f, 1.0f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 540.0f)
        .Build();

    auto content = EntityBuilder(scene, res, "scenario")
                       .WithName("UIContentRow")
                       .WithUIChild(root, glm::vec2(0.0f), glm::vec2(560.0f, 200.0f), 12)
                       .WithUIFlex(FlexDirection::Row, 18.0f)
                       .Build();
    scene.registry.get<UIFlexLayoutComponent>(content).autoSize = false;

    m_S16TextureEntity = EntityBuilder(scene, res, "scenario")
                             .WithName("UILogoTile")
                             .WithUIChild(content, glm::vec2(0.0f), glm::vec2(180.0f, 180.0f), 13)
                             .WithUITextureResource("ui_logo_28", SamplePath("include/engine/asset/project/logo.png"),
                                                    glm::vec4(1.0f), "ui_logo_tile_28")
                             .WithUIFlip(false, false)
                             .Build();

    m_S16CardEntity = EntityBuilder(scene, res, "scenario")
                          .WithName("UIRotatedCard")
                          .WithUIChild(content, glm::vec2(0.0f), glm::vec2(220.0f, 180.0f), 13)
                          .WithUIPivot(glm::vec2(0.5f, 0.5f))
                          .WithUIRenderer("ui_card_28", glm::vec4(0.18f, 0.24f, 0.31f, 0.96f))
                          .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("UICardText")
        .WithUIChild(m_S16CardEntity, glm::vec2(12.0f, 16.0f), glm::vec2(196.0f, 78.0f), 14)
        .WithUIText("Pivoted card\nRotation slider updates this card.\nThe logo tile can be hidden.", "time", 0.9f,
                    glm::vec4(0.95f, 0.95f, 0.95f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 190.0f)
        .Build();

    auto footer = EntityBuilder(scene, res, "scenario")
                      .WithName("UIFooterRow")
                      .WithUIChild(root, glm::vec2(0.0f), glm::vec2(560.0f, 96.0f), 12)
                      .WithUIFlex(FlexDirection::Row, 10.0f)
                      .Build();
    scene.registry.get<UIFlexLayoutComponent>(footer).autoSize = false;

    EntityBuilder(scene, res, "scenario")
        .WithName("UICreditText")
        .WithUIChild(footer, glm::vec2(0.0f), glm::vec2(332.0f, 84.0f), 13)
        .WithUIText("This scene exercises static UI transform, child hierarchy, rotated UI, and wrapped text.", "time",
                    0.84f, glm::vec4(0.82f, 0.88f, 0.95f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 310.0f)
        .Build();

    auto iconBadge = EntityBuilder(scene, res, "scenario")
                         .WithName("UIIconBadge")
                         .WithUIChild(footer, glm::vec2(0.0f), glm::vec2(84.0f, 84.0f), 13)
                         .WithUITextureResource("ui_icon_28", SamplePath("include/engine/asset/project/icon.png"),
                                                glm::vec4(1.0f), "ui_icon_badge_28")
                         .Build();

    // ─── Part 2: Scenario 29 Responsive UI (Merged into 28) ───
    m_S16RootPanel = EntityBuilder(scene, res, "scenario")
                         .WithName("ResponsiveRootPanel")
                         .WithUIStretch(glm::vec2(0.60f, 0.08f), glm::vec2(0.98f, 0.92f))
                         .WithUIZIndex(8)
                         .WithUIRenderer("ui_panel_29", glm::vec4(0.08f, 0.09f, 0.11f, m_S16PanelAlpha))
                         .WithUIFlex(FlexDirection::Row, 14.0f)
                         .Build();
    scene.registry.get<UIFlexLayoutComponent>(m_S16RootPanel).autoSize = false;

    auto leftColumn = EntityBuilder(scene, res, "scenario")
                          .WithName("ResponsiveLeftColumn")
                          .WithUIChild(m_S16RootPanel, glm::vec2(0.0f), glm::vec2(280.0f, 260.0f), 9)
                          .WithUIFlex(FlexDirection::Column, 10.0f)
                          .Build();
    scene.registry.get<UIFlexLayoutComponent>(leftColumn).autoSize = false;

    EntityBuilder(scene, res, "scenario")
        .WithName("ResponsiveTitle")
        .WithUIChild(leftColumn, glm::vec2(0.0f), glm::vec2(260.0f, 48.0f), 10)
        .WithUIText("Responsive UI", "time", 1.0f, glm::vec4(0.9f, 0.95f, 1.0f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, false, 0.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("ResponsiveBody")
        .WithUIChild(leftColumn, glm::vec2(0.0f), glm::vec2(260.0f, 150.0f), 10)
        .WithUIText(
            "Resize the window and compare compact, expanded, and stacked layouts. This block should reflow from "
            "the anchored root without needing reload.",
            "time", 0.86f, glm::vec4(0.84f, 0.9f, 0.94f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 280.0f)
        .Build();

    auto metricsRow = EntityBuilder(scene, res, "scenario")
                          .WithName("ResponsiveMetricsRow")
                          .WithUIChild(leftColumn, glm::vec2(0.0f), glm::vec2(260.0f, 60.0f), 10)
                          .WithUIFlex(FlexDirection::Row, 8.0f)
                          .Build();
    scene.registry.get<UIFlexLayoutComponent>(metricsRow).autoSize = false;

    const char* metricLabels[] = {"Anchors", "Offsets", "Flex"};
    for (int i = 0; i < 3; ++i)
    {
        EntityBuilder(scene, res, "scenario")
            .WithName(std::string("ResponsiveMetric_") + metricLabels[i])
            .WithUIChild(metricsRow, glm::vec2(0.0f), glm::vec2(80.0f, 52.0f), 11)
            .WithUIRenderer(std::string("ui_metric_") + metricLabels[i],
                            glm::vec4(0.16f + 0.06f * i, 0.20f, 0.28f, 0.95f))
            .WithUIText(metricLabels[i], "time", 0.82f, glm::vec4(1.0f))
            .Build();
    }

    auto rightColumn = EntityBuilder(scene, res, "scenario")
                           .WithName("ResponsiveRightColumn")
                           .WithUIChild(m_S16RootPanel, glm::vec2(0.0f), glm::vec2(240.0f, 260.0f), 9)
                           .WithUIFlex(FlexDirection::Column, 12.0f)
                           .Build();
    scene.registry.get<UIFlexLayoutComponent>(rightColumn).autoSize = false;

    auto preview = EntityBuilder(scene, res, "scenario")
                       .WithName("ResponsivePreview")
                       .WithUIChild(rightColumn, glm::vec2(0.0f), glm::vec2(220.0f, 170.0f), 10)
                       .WithUITextureResource("ui_logo_29", SamplePath("include/engine/asset/project/logo.png"),
                                              glm::vec4(0.18f, 0.21f, 0.24f, 0.95f), "ui_preview_29")
                       .WithUIFlip(false, false)
                       .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("ResponsivePreviewCaption")
        .WithUIChild(rightColumn, glm::vec2(0.0f), glm::vec2(220.0f, 64.0f), 11)
        .WithUIText("This block anchors and resizes with the viewport.\nLayout mode changes its flow.", "time", 0.8f,
                    glm::vec4(0.92f, 0.92f, 0.96f, 1.0f))
        .WithUITextAlignment(TextAlignment::Center, true, 200.0f)
        .Build();
}

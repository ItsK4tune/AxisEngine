#include "sample_scenario_common.h"

void SampleState::LoadScene16()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    // ─── Part 1: Scenario 28 UI Showcase ───
    // Increased height to 580 to prevent footer overlaps, and increased flex spacing to 24
    auto root = EntityBuilder(scene, res, "scenario")
                    .WithName("UIRootPanel")
                    .WithUIAnchored(glm::vec2(1.0f, 0.0f), glm::vec2(-1472.0f, 24.0f), glm::vec2(620.0f, 580.0f), 10)
                    .WithUIRenderer("ui_panel_28", glm::vec4(0.08f, 0.10f, 0.13f, 0.94f))
                    .WithUIFlex(FlexDirection::Column, 24.0f)
                    .Build();
    Entity(root, &scene).SetUIFlexAutoSize(false);

    EntityBuilder(scene, res, "scenario")
        .WithName("UIRootTitle")
        .WithUIChild(root, glm::vec2(-5.0f, 30.0f), glm::vec2(560.0f, 54.0f), 11)
        .WithUIText("UI Showcase - transform / text / texture / pivot / rotation / flip", "time", 0.525f,
                    glm::vec4(0.65f, 0.95f, 1.0f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 540.0f)
        .Build();

    auto content = EntityBuilder(scene, res, "scenario")
                       .WithName("UIContentRow")
                       .WithUIChild(root, glm::vec2(0.0f), glm::vec2(560.0f, 200.0f), 12)
                       .WithUIFlex(FlexDirection::Row, 18.0f)
                       .Build();
    Entity(content, &scene).SetUIFlexAutoSize(false);

    m_S16TextureEntity =
        EntityBuilder(scene, res, "scenario")
            .WithName("UILogoTile")
            .WithUIChild(content, glm::vec2(0.0f), glm::vec2(180.0f, 180.0f), 13)
            .WithUITextureResource("ui_logo_28", "asset://project/logo.png", glm::vec4(1.0f), "ui_logo_tile_28")
            .WithUIFlip(false, true)
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
        .WithUIText("Pivoted card\nRotation slider updates this card.\nThe logo tile can be hidden.", "time", 0.45f,
                    glm::vec4(0.95f, 0.95f, 0.95f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 190.0f)
        .Build();

    auto footer = EntityBuilder(scene, res, "scenario")
                      .WithName("UIFooterRow")
                      .WithUIChild(root, glm::vec2(0.0f), glm::vec2(560.0f, 96.0f), 12)
                      .WithUIFlex(FlexDirection::Row, 10.0f)
                      .Build();
    Entity(footer, &scene).SetUIFlexAutoSize(false);

    EntityBuilder(scene, res, "scenario")
        .WithName("UICreditText")
        .WithUIChild(footer, glm::vec2(0.0f), glm::vec2(440.0f, 120.0f), 13)
        .WithUIText("This scene exercises static UI transform, child hierarchy, rotated UI, and wrapped text.", "time",
                    0.42f, glm::vec4(0.82f, 0.88f, 0.95f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 420.0f)
        .Build();

    auto iconBadge =
        EntityBuilder(scene, res, "scenario")
            .WithName("UIIconBadge")
            .WithUIChild(footer, glm::vec2(0.0f), glm::vec2(84.0f, 84.0f), 13)
            .WithUITextureResource("ui_icon_28", "asset://project/icon.png", glm::vec4(1.0f), "ui_icon_badge_28")
            .Build();

    // ─── Part 2: Scenario 29 Responsive UI (Merged into 28) ───
    m_S16RootPanel = EntityBuilder(scene, res, "scenario")
                         .WithName("ResponsiveRootPanel")
                         .WithUIStretch(glm::vec2(0.60f, 0.08f), glm::vec2(0.98f, 0.92f))
                         .WithUIZIndex(8)
                         .WithUIRenderer("ui_panel_29", glm::vec4(0.08f, 0.09f, 0.11f, m_S16PanelAlpha))
                         .WithUIFlex(FlexDirection::Row, 14.0f)
                         .Build();
    Entity(m_S16RootPanel, &scene).SetUIFlexAutoSize(false);

    // Increased height to 300 to prevent text overlapping and overflow
    auto leftColumn = EntityBuilder(scene, res, "scenario")
                          .WithName("ResponsiveLeftColumn")
                          .WithUIChild(m_S16RootPanel, glm::vec2(0.0f), glm::vec2(280.0f, 300.0f), 9)
                          .WithUIFlex(FlexDirection::Column, 10.0f)
                          .Build();
    Entity(leftColumn, &scene).SetUIFlexAutoSize(false);

    EntityBuilder(scene, res, "scenario")
        .WithName("ResponsiveTitle")
        .WithUIChild(leftColumn, glm::vec2(0.0f), glm::vec2(260.0f, 48.0f), 10)
        .WithUIText("Responsive UI", "time", 0.50f, glm::vec4(0.9f, 0.95f, 1.0f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, false, 0.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("ResponsiveBody")
        .WithUIChild(leftColumn, glm::vec2(0.0f), glm::vec2(260.0f, 160.0f), 10)
        .WithUIText(
            "Resize the window and compare compact, expanded, and stacked layouts. This block should reflow from "
            "the anchored root without needing reload.",
            "time", 0.43f, glm::vec4(0.84f, 0.9f, 0.94f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 280.0f)
        .Build();

    auto metricsRow = EntityBuilder(scene, res, "scenario")
                          .WithName("ResponsiveMetricsRow")
                          .WithUIChild(leftColumn, glm::vec2(0.0f), glm::vec2(260.0f, 60.0f), 10)
                          .WithUIFlex(FlexDirection::Row, 8.0f)
                          .Build();
    Entity(metricsRow, &scene).SetUIFlexAutoSize(false);

    const char* metricLabels[] = {"Anchors", "Offsets", "Flex"};
    for (int i = 0; i < 3; ++i)
    {
        EntityBuilder(scene, res, "scenario")
            .WithName(std::string("ResponsiveMetric_") + metricLabels[i])
            .WithUIChild(metricsRow, glm::vec2(0.0f), glm::vec2(80.0f, 52.0f), 11)
            .WithUIRenderer(std::string("ui_metric_") + metricLabels[i],
                            glm::vec4(0.16f + 0.06f * i, 0.20f, 0.28f, 0.95f))
            .WithUIText(metricLabels[i], "time", 0.41f, glm::vec4(1.0f))
            .Build();
    }

    // Increased height to 300 to prevent text overlapping and overflow
    auto rightColumn = EntityBuilder(scene, res, "scenario")
                           .WithName("ResponsiveRightColumn")
                           .WithUIChild(m_S16RootPanel, glm::vec2(0.0f), glm::vec2(240.0f, 300.0f), 9)
                           .WithUIFlex(FlexDirection::Column, 12.0f)
                           .Build();
    Entity(rightColumn, &scene).SetUIFlexAutoSize(false);

    auto preview = EntityBuilder(scene, res, "scenario")
                       .WithName("ResponsivePreview")
                       .WithUIChild(rightColumn, glm::vec2(0.0f), glm::vec2(220.0f, 170.0f), 10)
                       .WithUITextureResource("ui_logo_29", "asset://project/logo.png",
                                              glm::vec4(0.18f, 0.21f, 0.24f, 0.95f), "ui_preview_29")
                       .WithUIFlip(false, true)
                       .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("ResponsivePreviewCaption")
        .WithUIChild(rightColumn, glm::vec2(0.0f), glm::vec2(220.0f, 80.0f), 11)
        .WithUIText("This block anchors and resizes with the viewport.\nLayout mode changes its flow.", "time", 0.4f,
                    glm::vec4(0.92f, 0.92f, 0.96f, 1.0f))
        .WithUITextAlignment(TextAlignment::Center, true, 200.0f)
        .Build();

    // ─── Part 3: UI Animation / Movement Showcase ───
    auto showcasePanel = EntityBuilder(scene, res, "scenario")
                             .WithName("S16_ShowcasePanel")
                             .WithUIAnchored(glm::vec2(1.0f, 0.0f), glm::vec2(-1472.0f, m_S16ShowcaseX),
                                             glm::vec2(m_S16ShowcaseW, m_S16ShowcaseH), 10)
                             .WithUIRenderer("ui_showcase_panel", glm::vec4(0.07f, 0.09f, 0.13f, 0.94f))
                             .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("S16_ShowcaseText")
        .WithUIChild(showcasePanel, glm::vec2(20.0f, 20.0f), glm::vec2(580.0f, 180.0f), 11)
        .WithUIText("UI Showcase: Animate transform, scale & word wrap dynamically!", "time", 0.44f,
                    glm::vec4(1.0f, 0.85f, 0.35f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 560.0f)
        .Build();
}

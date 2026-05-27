#include "sample_scenario_common.h"

void SampleState::LoadScene29()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    m_S29RootPanel = EntityBuilder(scene, res, "scenario")
                          .WithName("ResponsiveRootPanel")
                          .WithUIStretch(glm::vec2(0.58f, 0.08f), glm::vec2(0.98f, 0.92f))
                          .WithUIZIndex(8)
                          .WithUIRenderer("ui_panel_29", glm::vec4(0.08f, 0.09f, 0.11f, m_S29PanelAlpha))
                          .WithUIFlex(FlexDirection::Row, 14.0f)
                          .Build();
    scene.registry.get<UIFlexLayoutComponent>(m_S29RootPanel).autoSize = false;

    auto leftColumn = EntityBuilder(scene, res, "scenario")
                          .WithName("ResponsiveLeftColumn")
                          .WithUIChild(m_S29RootPanel, glm::vec2(0.0f), glm::vec2(280.0f, 260.0f), 9)
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
            .WithUIRenderer(std::string("ui_metric_") + metricLabels[i], glm::vec4(0.16f + 0.06f * i, 0.20f, 0.28f, 0.95f))
            .WithUIText(metricLabels[i], "time", 0.82f, glm::vec4(1.0f))
            .Build();
    }

    auto rightColumn = EntityBuilder(scene, res, "scenario")
                           .WithName("ResponsiveRightColumn")
                           .WithUIChild(m_S29RootPanel, glm::vec2(0.0f), glm::vec2(240.0f, 260.0f), 9)
                           .WithUIFlex(FlexDirection::Column, 12.0f)
                           .Build();
    scene.registry.get<UIFlexLayoutComponent>(rightColumn).autoSize = false;

    auto preview = EntityBuilder(scene, res, "scenario")
                       .WithName("ResponsivePreview")
                       .WithUIChild(rightColumn, glm::vec2(0.0f), glm::vec2(220.0f, 170.0f), 10)
                       .WithUITextureResource("ui_logo_29", SamplePath("include/engine/asset/project/logo.png"),
                                              glm::vec4(0.18f, 0.21f, 0.24f, 0.95f), "ui_preview_29")
                       .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("ResponsivePreviewCaption")
        .WithUIChild(rightColumn, glm::vec2(0.0f), glm::vec2(220.0f, 64.0f), 11)
        .WithUIText("This block anchors and resizes with the viewport.\nLayout mode changes its flow.",
                    "time", 0.8f, glm::vec4(0.92f, 0.92f, 0.96f, 1.0f))
        .WithUITextAlignment(TextAlignment::Center, true, 200.0f)
        .Build();
}

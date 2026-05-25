#include "sample_scenario_common.h"

namespace
{
std::shared_ptr<Texture> LoadUiTexture(ResourceManager& res, const std::string& name, const std::string& path)
{
    if (!res.GetTexture(name))
        res.LoadTexture(name, SamplePath(path.c_str()), false, true);
    return res.GetTexture(name);
}
}  // namespace

void SampleState::LoadScene28()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    auto logo = LoadUiTexture(res, "ui_logo_28", "include/engine/asset/project/logo.png");
    auto icon = LoadUiTexture(res, "ui_icon_28", "include/engine/asset/project/icon.png");

    auto root = EntityBuilder(scene, res, "scenario")
        .WithName("UIRootPanel")
        .WithUITransform(glm::vec2(-644.0f, 24.0f), glm::vec2(620.0f, 520.0f), 10)
        .WithUIAnchors(glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 0.0f))
        .WithUIRenderer("ui_panel_28", glm::vec4(0.08f, 0.10f, 0.13f, 0.94f))
        .WithUIFlex(FlexDirection::Column, 14.0f)
        .Build();
    scene.registry.get<UIFlexLayoutComponent>(root).autoSize = false;

    EntityBuilder(scene, res, "scenario")
        .WithName("UIRootTitle")
        .WithParent(root)
        .WithUITransform(glm::vec2(0.0f), glm::vec2(560.0f, 54.0f), 11)
        .WithUIText("UI Showcase - transform / text / texture / pivot / rotation", "time", 1.05f,
                    glm::vec4(0.65f, 0.95f, 1.0f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 540.0f)
        .Build();

    auto content = EntityBuilder(scene, res, "scenario")
        .WithName("UIContentRow")
        .WithParent(root)
        .WithUITransform(glm::vec2(0.0f), glm::vec2(560.0f, 200.0f), 12)
        .WithUIFlex(FlexDirection::Row, 18.0f)
        .Build();
    scene.registry.get<UIFlexLayoutComponent>(content).autoSize = false;

    m_S28TextureEntity = EntityBuilder(scene, res, "scenario")
                               .WithName("UILogoTile")
                               .WithParent(content)
                               .WithUITransform(glm::vec2(0.0f), glm::vec2(180.0f, 180.0f), 13)
                               .WithUIRenderer("ui_logo_tile_28", glm::vec4(1.0f))
                               .Build();
    if (auto* renderer = scene.registry.try_get<UIRendererComponent>(m_S28TextureEntity))
        renderer->texture = logo;

    m_S28CardEntity = EntityBuilder(scene, res, "scenario")
                          .WithName("UIRotatedCard")
                          .WithParent(content)
                          .WithUITransform(glm::vec2(0.0f), glm::vec2(220.0f, 180.0f), 13)
                          .WithUIPivot(glm::vec2(0.5f, 0.5f))
                          .WithUIRenderer("ui_card_28", glm::vec4(0.18f, 0.24f, 0.31f, 0.96f))
                          .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("UICardText")
        .WithParent(m_S28CardEntity)
        .WithUITransform(glm::vec2(12.0f, 16.0f), glm::vec2(196.0f, 78.0f), 14)
        .WithUIText("Pivoted card\nRotation slider updates this card.\nThe logo tile can be hidden.", "time", 0.9f,
                    glm::vec4(0.95f, 0.95f, 0.95f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 190.0f)
        .Build();

    auto footer = EntityBuilder(scene, res, "scenario")
        .WithName("UIFooterRow")
        .WithParent(root)
        .WithUITransform(glm::vec2(0.0f), glm::vec2(560.0f, 96.0f), 12)
        .WithUIFlex(FlexDirection::Row, 10.0f)
        .Build();
    scene.registry.get<UIFlexLayoutComponent>(footer).autoSize = false;

    EntityBuilder(scene, res, "scenario")
        .WithName("UICreditText")
        .WithParent(footer)
        .WithUITransform(glm::vec2(0.0f), glm::vec2(332.0f, 84.0f), 13)
        .WithUIText("This scene exercises static UI transform, child hierarchy, rotated UI, and wrapped text.",
                    "time", 0.84f, glm::vec4(0.82f, 0.88f, 0.95f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 310.0f)
        .Build();

    auto iconBadge = EntityBuilder(scene, res, "scenario")
        .WithName("UIIconBadge")
        .WithParent(footer)
        .WithUITransform(glm::vec2(0.0f), glm::vec2(84.0f, 84.0f), 13)
        .WithUIRenderer("ui_icon_badge_28", glm::vec4(1.0f))
        .Build();
    if (auto* renderer = scene.registry.try_get<UIRendererComponent>(iconBadge))
        renderer->texture = icon;
}

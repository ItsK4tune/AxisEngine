#pragma once

#include <resource/unit/font.h>
#include <resource/unit/shader.h>
#include <resource/unit/ui_model.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <functional>
#include <memory>
#include <string>

enum class TextAlignment
{
    Left,
    Center,
    Right
};
enum class VerticalAlignment
{
    Top,
    Center,
    Bottom
};
enum class FlexDirection
{
    Row,
    Column
};

struct UITransformComponent
{
    glm::vec2 position = glm::vec2(0.0f);
    glm::bvec2 positionIsPercent = glm::bvec2(false);

    glm::vec2 size = glm::vec2(100.0f, 100.0f);
    glm::bvec2 sizeIsPercent = glm::bvec2(false);

    float rotation = 0.0f;
    int zIndex = 0;

    glm::vec2 pivot = glm::vec2(0.5f);
    bool flipX = false;
    bool flipY = false;

    glm::vec2 anchorMin = glm::vec2(0.5f);
    glm::bvec2 anchorMinIsPercent = glm::bvec2(false);

    glm::vec2 anchorMax = glm::vec2(0.5f);
    glm::bvec2 anchorMaxIsPercent = glm::bvec2(false);

    glm::vec2 offsetMin = glm::vec2(-50.0f, -50.0f);
    glm::bvec2 offsetMinIsPercent = glm::bvec2(false);

    glm::vec2 offsetMax = glm::vec2(50.0f, 50.0f);
    glm::bvec2 offsetMaxIsPercent = glm::bvec2(false);
};

struct UIRendererComponent
{
    std::shared_ptr<UIModel> model = nullptr;
    std::shared_ptr<Shader> shader = nullptr;
    std::string shaderName = "";
    std::shared_ptr<Texture> texture = nullptr;
    std::string textureName;
    glm::vec4 color = glm::vec4(1.0f);
};

struct UITextComponent
{
    std::string text = "";
    std::string fontName = "";
    std::shared_ptr<UIModel> model = nullptr;
    std::shared_ptr<Shader> shader = nullptr;
    std::string shaderName = "";
    std::shared_ptr<Font> font = nullptr;
    int fontSize = 60;
    glm::vec4 color = glm::vec4(1.0f);
    float scale = 1.0f;

    TextAlignment alignment = TextAlignment::Left;
    bool wordWrap = false;
    float maxWidth = 0.0f;
    bool wrapByWord = true;
};

struct UIFlexLayoutComponent
{
    FlexDirection direction = FlexDirection::Column;
    float spacing = 5.0f;
    bool autoSize = false;
    glm::vec4 padding = glm::vec4(0.0f);
};

struct UIInteractiveComponent
{
    bool interactable = true;
    bool hovered = false;
    bool pressed = false;
    bool clicked = false;
    float holdTime = 0.0f;

    std::function<void(entt::entity)> onHoverEnter;
    std::function<void(entt::entity)> onHoverStay;
    std::function<void(entt::entity)> onHoverExit;
    std::function<void(entt::entity)> onPressed;
    std::function<void(entt::entity)> onReleased;
    std::function<void(entt::entity)> onClick;
};

struct UIAnimationComponent
{
    bool enabled = true;
    bool animateColor = true;
    bool animateScale = false;

    glm::vec4 normalColor = glm::vec4(1.0f);
    glm::vec4 hoverColor = glm::vec4(1.0f);
    glm::vec4 pressedColor = glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);

    float normalScale = 1.0f;
    float hoverScale = 1.0f;
    float pressedScale = 0.98f;
    float currentScale = 1.0f;
    float visualScale = 1.0f;
    float transitionSpeed = 12.0f;
};

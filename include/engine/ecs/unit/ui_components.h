#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <resource/unit/shader.h>
#include <resource/unit/ui_model.h>
#include <resource/unit/font.h>

// --- UI Enums ---

enum class TextAlignment { Left, Center, Right };
enum class VerticalAlignment { Top, Center, Bottom };
enum class FlexDirection { Row, Column };

// --- UI Transform ---

struct UITransformComponent
{
    glm::vec2 position = glm::vec2(0.0f); // Local position if anchors are same, or offset from anchor center
    glm::vec2 size = glm::vec2(100.0f, 100.0f);
    float rotation = 0.0f;
    int zIndex = 0;

    glm::vec2 pivot = glm::vec2(0.5f); // Point around which it rotates/scales (0-1)
    
    // UI Anchors (0.0 to 1.0)
    glm::vec2 anchorMin = glm::vec2(0.5f); 
    glm::vec2 anchorMax = glm::vec2(0.5f);
    
    // Pixel Offsets from Anchors
    glm::vec2 offsetMin = glm::vec2(-50.0f, -50.0f); 
    glm::vec2 offsetMax = glm::vec2(50.0f, 50.0f);

    bool usePercentage = false; // Legacy support
};

// --- UI Renderer ---

struct UIRendererComponent
{
    std::shared_ptr<UIModel> model = nullptr;
    std::shared_ptr<Shader> shader = nullptr;
    std::shared_ptr<Texture> texture = nullptr;
    glm::vec4 color = glm::vec4(1.0f);
};

// --- UI Text ---

struct UITextComponent
{
    std::string text = "";
    std::string fontName = "";
    std::shared_ptr<UIModel> model = nullptr;
    std::shared_ptr<Shader> shader = nullptr;
    std::shared_ptr<Font> font = nullptr;
    glm::vec4 color = glm::vec4(1.0f);
    float scale = 1.0f;

    TextAlignment alignment = TextAlignment::Left;
    bool wordWrap = false;
    float maxWidth = 0.0f; // 0 means no wrap
};

// --- UI Layout ---

struct UIFlexLayoutComponent
{
    FlexDirection direction = FlexDirection::Column;
    float spacing = 5.0f;
    bool autoSize = false;
    glm::vec4 padding = glm::vec4(0.0f); // Left, Top, Right, Bottom
};

#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <render/logic/shader.h>
#include <render/logic/ui_model.h>
#include <render/logic/font.h>

// --- UI Transform ---

struct UITransformComponent
{
    glm::vec2 position = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(100.0f, 100.0f);
    float rotation = 0.0f;
    int zIndex = 0;

    bool usePercentage = false;
    glm::vec2 anchor = glm::vec2(0.0f);
};

// --- UI Renderer ---

struct UIRendererComponent
{
    std::shared_ptr<UIModel> model = nullptr;
    std::shared_ptr<Shader> shader = nullptr;
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
    glm::vec3 color = glm::vec3(1.0f);
    float scale = 1.0f;
};

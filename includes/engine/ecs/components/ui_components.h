#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <graphic/renderer/ui_model.h>
#include <graphic/renderer/font.h>
#include <graphic/core/shader.h>

struct UITransformComponent
{
    glm::vec2 position = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(100.0f, 100.0f);
    float rotation = 0.0f;
    int zIndex = 0;

    bool usePercentage = false;
    glm::vec2 anchor = glm::vec2(0.0f); // (0,0) top-left, (0.5,0.5) center, (1,1) bottom-right
};

struct UIRendererComponent
{
    std::shared_ptr<UIModel> model = nullptr;
    std::shared_ptr<Shader> shader = nullptr;
    glm::vec4 color = glm::vec4(1.0f);
};

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

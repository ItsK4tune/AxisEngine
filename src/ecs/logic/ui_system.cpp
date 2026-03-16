#include <ecs/unit/core_components.h>
#include <ecs/unit/ui_components.h>
#include <ecs/logic/ui_system.h>
#include <platform/interface/input_codes.h>
#include <core/logic/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <platform/logic/io_handler.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_texture_manager.h>

struct UIRect
{
    glm::vec2 pos;
    glm::vec2 size;
};

UIRect CalculateRect(entt::registry& registry, entt::entity entity, float screenWidth, float screenHeight)
{
    auto& transform = registry.get<UITransformComponent>(entity);
    
    glm::vec2 parentSize = glm::vec2(screenWidth, screenHeight);
    glm::vec2 parentPos = glm::vec2(0.0f);

    if (auto* hierarchy = registry.try_get<HierarchyComponent>(entity))
    {
        if (hierarchy->parent != entt::null && registry.all_of<UITransformComponent>(hierarchy->parent))
        {
            UIRect pRect = CalculateRect(registry, hierarchy->parent, screenWidth, screenHeight);
            parentPos = pRect.pos;
            parentSize = pRect.size;
        }
    }

    // Calculate screen space anchor points
    glm::vec2 anchorMinPos = parentPos + transform.anchorMin * parentSize;
    glm::vec2 anchorMaxPos = parentPos + transform.anchorMax * parentSize;

    // Apply offsets
    glm::vec2 finalMin = anchorMinPos + transform.offsetMin;
    glm::vec2 finalMax = anchorMaxPos + transform.offsetMax;

    // Final size and position (pivot-adjusted)
    glm::vec2 size = finalMax - finalMin;
    glm::vec2 pos = finalMin + transform.pivot * size;

    return { finalMin, size };
}

void UIRenderSystem::RenderUI(Scene &scene, float screenWidth, float screenHeight, IRenderStateManager &renderState)
{
    if (!m_Enabled) return;

    renderState.Disable(ServerCapability::DepthTest);
    renderState.Disable(ServerCapability::CullFace);
    renderState.Enable(ServerCapability::Blend);
    renderState.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

    PolygonMode previousPolygonMode = renderState.GetPolygonMode();
    renderState.SetPolygonMode(CullMode::FrontAndBack, PolygonMode::Fill);

    // Filter and sort by Z-Index
    std::vector<entt::entity> sortedEntities;
    auto view = scene.registry.view<UITransformComponent>();
    for (auto entity : view) sortedEntities.push_back(entity);
    
    std::sort(sortedEntities.begin(), sortedEntities.end(), [&](entt::entity a, entt::entity b) {
        return view.get<UITransformComponent>(a).zIndex < view.get<UITransformComponent>(b).zIndex;
    });

    glm::mat4 projection = glm::ortho(0.0f, screenWidth, screenHeight, 0.0f, -1.0f, 1.0f);
    Shader *currentShader = nullptr;

    for (auto entity : sortedEntities)
    {
        auto &transform = view.get<UITransformComponent>(entity);
        UIRect rect = CalculateRect(scene.registry, entity, screenWidth, screenHeight);

        if (auto *renderer = scene.registry.try_get<UIRendererComponent>(entity))
        {
            if (renderer->model && renderer->shader)
            {
                Shader *shaderPtr = renderer->shader.get();
                if (currentShader != shaderPtr)
                {
                    currentShader = shaderPtr;
                    currentShader->use();
                    currentShader->setMat4("projection", projection);
                    currentShader->setInt("image", 0);
                }

                unsigned int texID = 0;
                if (renderer->texture) texID = renderer->texture->id;

                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(rect.pos, 0.0f));
                model = glm::scale(model, glm::vec3(rect.size, 1.0f));
                currentShader->setMat4("model", model);

                renderer->model->Draw(*currentShader, renderer->color, texID);
            }
        }

        if (auto *textComp = scene.registry.try_get<UITextComponent>(entity))
        {
            if (textComp->model && textComp->shader && textComp->font)
            {
                Shader *shaderPtr = textComp->shader.get();
                if (currentShader != shaderPtr)
                {
                    currentShader = shaderPtr;
                    currentShader->use();
                    currentShader->setMat4("projection", projection);
                    currentShader->setInt("text", 0);
                }

                float scale = textComp->scale;
                std::string text = textComp->text;
                
                // --- Simple Word Wrap (Characters based for now) ---
                std::vector<std::string> lines;
                if (textComp->wordWrap && textComp->maxWidth > 0) {
                    std::string currentLine;
                    float currentWidth = 0;
                    for (char c : text) {
                        const Character &ch = textComp->font->GetCharacter(c);
                        float charWidth = (ch.Advance >> 6) * scale;
                        if (currentWidth + charWidth > textComp->maxWidth) {
                            lines.push_back(currentLine);
                            currentLine = c;
                            currentWidth = charWidth;
                        } else {
                            currentLine += c;
                            currentWidth += charWidth;
                        }
                    }
                    lines.push_back(currentLine);
                } else {
                    lines.push_back(text);
                }

                float startY = rect.pos.y;
                for (const auto& line : lines) {
                    float lineWidth = 0;
                    for (char c : line) lineWidth += (textComp->font->GetCharacter(c).Advance >> 6) * scale;

                    float startX = rect.pos.x;
                    if (textComp->alignment == TextAlignment::Center) startX += (rect.size.x - lineWidth) * 0.5f;
                    else if (textComp->alignment == TextAlignment::Right) startX += (rect.size.x - lineWidth);

                    float x = startX;
                    for (char c : line)
                    {
                        const Character &ch = textComp->font->GetCharacter(c);

                        float xpos = x + ch.Bearing.x * scale;
                        float ypos = startY + (ch.Size.y - ch.Bearing.y) * scale;
                        float w = ch.Size.x * scale;
                        float h = ch.Size.y * scale;

                        std::vector<float> vertices = {
                            xpos, ypos - h, 0.0f, 0.0f,
                            xpos, ypos, 0.0f, 1.0f,
                            xpos + w, ypos, 1.0f, 1.0f,

                            xpos, ypos - h, 0.0f, 0.0f,
                            xpos + w, ypos, 1.0f, 1.0f,
                            xpos + w, ypos - h, 1.0f, 0.0f};

                        textComp->model->DrawDynamic(*currentShader, ch.TextureID, textComp->color, vertices);
                        x += (ch.Advance >> 6) * scale;
                    }
                    startY += textComp->font->GetFontSize() * 0.8f * scale; 
                }
            }
        }
    }

    renderState.Enable(ServerCapability::DepthTest);
    renderState.Disable(ServerCapability::Blend);
    renderState.SetPolygonMode(CullMode::FrontAndBack, previousPolygonMode);
}

void UIRenderSystem::UpdateLayout(Scene &scene, float screenWidth, float screenHeight)
{
    auto view = scene.registry.view<UITransformComponent, UIFlexLayoutComponent, HierarchyComponent>();
    for (auto entity : view)
    {
        auto &flex = view.get<UIFlexLayoutComponent>(entity);
        auto &hierarchy = view.get<HierarchyComponent>(entity);
        
        if (hierarchy.children.empty()) continue;

        UIRect parentRect = CalculateRect(scene.registry, entity, screenWidth, screenHeight);
        float currentOffset = (flex.direction == FlexDirection::Row) ? flex.padding.x : flex.padding.y;

        for (auto child : hierarchy.children)
        {
            if (!scene.registry.all_of<UITransformComponent>(child)) continue;
            auto &childTransform = scene.registry.get<UITransformComponent>(child);

            if (flex.direction == FlexDirection::Row)
            {
                childTransform.anchorMin = glm::vec2(0, 0.5f);
                childTransform.anchorMax = glm::vec2(0, 0.5f);
                childTransform.offsetMin = glm::vec2(currentOffset, -childTransform.size.y * 0.5f);
                childTransform.offsetMax = glm::vec2(currentOffset + childTransform.size.x, childTransform.size.y * 0.5f);
                currentOffset += childTransform.size.x + flex.spacing;
            }
            else
            {
                childTransform.anchorMin = glm::vec2(0.5f, 0);
                childTransform.anchorMax = glm::vec2(0.5f, 0);
                childTransform.offsetMin = glm::vec2(-childTransform.size.x * 0.5f, currentOffset);
                childTransform.offsetMax = glm::vec2(childTransform.size.x * 0.5f, currentOffset + childTransform.size.y);
                currentOffset += childTransform.size.y + flex.spacing;
            }
        }
    }
}

void UIRenderSystem::Update(Scene &scene, float dt)
{
}

void UIRenderSystem::Render(Scene &scene)
{
}

std::vector<entt::id_type> UIRenderSystem::GetReadComponents() const
{
    return {
        entt::type_id<UIRendererComponent>().hash(),
        entt::type_id<UITransformComponent>().hash()
    };
}

std::vector<entt::id_type> UIRenderSystem::GetWriteComponents() const
{
    return {};
}

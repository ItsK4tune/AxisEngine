#include <ecs/logic/ui_render_system.h>
#include <core/logic/config_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/interface/i_ui_service.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/ui_components.h>
#include <platform/interface/input_codes.h>
#include <platform/logic/io_handler.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_texture_manager.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <sstream>

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

    auto evalVec = [](const glm::vec2& v, const glm::bvec2& p, const glm::vec2& ref) {
        return glm::vec2(p.x ? (v.x * 0.01f * ref.x) : v.x, p.y ? (v.y * 0.01f * ref.y) : v.y);
    };

    glm::vec2 evalAnchorMin = evalVec(transform.anchorMin, transform.anchorMinIsPercent, glm::vec2(1.0f));
    glm::vec2 evalAnchorMax = evalVec(transform.anchorMax, transform.anchorMaxIsPercent, glm::vec2(1.0f));

    glm::vec2 anchorMinPos = parentPos + evalAnchorMin * parentSize;
    glm::vec2 anchorMaxPos = parentPos + evalAnchorMax * parentSize;

    glm::vec2 evalOffsetMin = evalVec(transform.offsetMin, transform.offsetMinIsPercent, parentSize);
    glm::vec2 evalOffsetMax = evalVec(transform.offsetMax, transform.offsetMaxIsPercent, parentSize);

    glm::vec2 finalMin = anchorMinPos + evalOffsetMin;
    glm::vec2 finalMax = anchorMaxPos + evalOffsetMax;

    // Apply explicit position offset independently of the layout system
    glm::vec2 evalPos = evalVec(transform.position, transform.positionIsPercent, parentSize);
    finalMin += evalPos;
    finalMax += evalPos;

    // If anchors are equal, use evaluate size to strictly define bounds!
    glm::vec2 evalSize = evalVec(transform.size, transform.sizeIsPercent, parentSize);
    if (evalAnchorMin.x == evalAnchorMax.x)
    {
        finalMax.x = finalMin.x + evalSize.x;
    }
    if (evalAnchorMin.y == evalAnchorMax.y)
    {
        finalMax.y = finalMin.y + evalSize.y;
    }

    glm::vec2 size = finalMax - finalMin;
    return {finalMin, size};
}

static UIRect ApplyVisualScale(const UIRect& rect, const UITransformComponent& transform, float scale)
{
    if (scale <= 0.0f || scale == 1.0f)
        return rect;

    const glm::vec2 pivot = rect.pos + transform.pivot * rect.size;
    const glm::vec2 scaledSize = rect.size * scale;
    return {pivot - transform.pivot * scaledSize, scaledSize};
}

REGISTER_SYSTEM(UIRenderSystem)

void UIRenderSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<IUIService>(this);
    sl.Register<UIRenderSystem>(this);
}

void UIRenderSystem::RenderUIPass(Scene& scene, float screenWidth, float screenHeight, IRenderStateManager& renderState)
{
    if (!m_Enabled)
        return;

    renderState.SetViewport(0, 0, (int)screenWidth, (int)screenHeight);
    renderState.Disable(ServerCapability::DepthTest);
    renderState.Disable(ServerCapability::CullFace);
    renderState.Enable(ServerCapability::Blend);
    renderState.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

    PolygonMode previousPolygonMode = renderState.GetPolygonMode();
    renderState.SetPolygonMode(CullMode::FrontAndBack, PolygonMode::Fill);

    float referenceWidth = 1920.0f;
    float referenceHeight = 1080.0f;
    if (auto* config = ServiceLocator::Instance().Resolve<ConfigManager>())
    {
        referenceWidth = (std::max)(1.0f, config->GetConfig().uiReferenceWidth);
        referenceHeight = (std::max)(1.0f, config->GetConfig().uiReferenceHeight);
    }

    float scaleFactor = std::min(screenWidth / referenceWidth, screenHeight / referenceHeight);
    if (scaleFactor <= 0.0f)
        scaleFactor = 1.0f;
    float virtualWidth = screenWidth / scaleFactor;
    float virtualHeight = screenHeight / scaleFactor;

    // Execute layout rules against virtual resolution
    UpdateLayout(scene, virtualWidth, virtualHeight);

    std::vector<entt::entity> sortedEntities;
    auto view = scene.View<UITransformComponent, InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).isActive)
            sortedEntities.push_back(entity);
    }

    std::sort(sortedEntities.begin(), sortedEntities.end(), [&](entt::entity a, entt::entity b) {
        return view.get<UITransformComponent>(a).zIndex < view.get<UITransformComponent>(b).zIndex;
    });

    glm::mat4 projection = glm::ortho(0.0f, virtualWidth, virtualHeight, 0.0f, -1.0f, 1.0f);
    Shader* currentShader = nullptr;

    for (auto entity : sortedEntities)
    {
        auto& transform = view.get<UITransformComponent>(entity);
        UIRect rect = CalculateRect(scene.GetRegistry(), entity, virtualWidth, virtualHeight);
        if (auto* animation = scene.TryGetComponent<UIAnimationComponent>(entity))
            rect = ApplyVisualScale(rect, transform, animation->visualScale);

        // Apply Position natively in CalculateRect, but rotation happens here
        glm::vec2 finalPos = rect.pos;
        glm::vec2 finalSize = rect.size;

        if (auto* renderer = scene.TryGetComponent<UIRendererComponent>(entity))
        {
            if (renderer->model && renderer->shader)
            {
                Shader* shaderPtr = renderer->shader.get();
                if (currentShader != shaderPtr)
                {
                    currentShader = shaderPtr;
                    currentShader->use();
                    currentShader->setMat4("u_Projection", projection);
                    currentShader->setInt("image", 0);
                }

                unsigned int texID = 0;
                if (renderer->texture)
                    texID = renderer->texture->id;

                const glm::vec2 flipScale(transform.flipX ? -1.0f : 1.0f, transform.flipY ? -1.0f : 1.0f);
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(finalPos + transform.pivot * finalSize, 0.0f));
                if (transform.rotation != 0.0f)
                {
                    model = glm::rotate(model, glm::radians(transform.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
                }
                model = glm::scale(model, glm::vec3(flipScale, 1.0f));
                model = glm::translate(model, glm::vec3(-transform.pivot * finalSize, 0.0f));
                model = glm::scale(model, glm::vec3(finalSize, 1.0f));
                currentShader->setMat4("u_Model", model);

                renderer->model->Draw(*currentShader, renderer->color, texID);
            }
        }

        if (auto* textComp = scene.TryGetComponent<UITextComponent>(entity))
        {
            if (textComp->model && textComp->shader && textComp->font)
            {
                Shader* shaderPtr = textComp->shader.get();
                if (currentShader != shaderPtr)
                {
                    currentShader = shaderPtr;
                    currentShader->use();
                    currentShader->setMat4("u_Projection", projection);
                    currentShader->setInt("text", 0);
                }

                float scale = textComp->scale;
                std::string text = textComp->text;

                std::vector<std::string> lines;
                if (textComp->wordWrap && textComp->maxWidth > 0)
                {
                    if (textComp->wrapByWord)
                    {
                        std::string currentLine;
                        float currentLineWidth = 0.0f;
                        std::string currentWord;
                        float wordWidth = 0.0f;

                        for (size_t i = 0; i < text.length(); ++i)
                        {
                            char c = text[i];
                            if (c == '\n')
                            {
                                currentLine += currentWord;
                                lines.push_back(currentLine);
                                currentLine.clear();
                                currentWord.clear();
                                currentLineWidth = 0.0f;
                                wordWidth = 0.0f;
                                continue;
                            }

                            const Character& ch = textComp->font->GetCharacter(c);
                            float charWidth = (ch.advance >> 6) * scale;

                            if (c == ' ')
                            {
                                if (currentLineWidth + wordWidth + charWidth > textComp->maxWidth)
                                {
                                    if (!currentLine.empty())
                                    {
                                        lines.push_back(currentLine);
                                        currentLine = currentWord;
                                        currentLineWidth = wordWidth;
                                    }
                                    else
                                    {
                                        currentLine = currentWord;
                                        lines.push_back(currentLine);
                                        currentLine.clear();
                                        currentLineWidth = 0.0f;
                                    }
                                    currentWord.clear();
                                    wordWidth = 0.0f;
                                }
                                else
                                {
                                    currentLine += currentWord + " ";
                                    currentLineWidth += wordWidth + charWidth;
                                    currentWord.clear();
                                    wordWidth = 0.0f;
                                }
                            }
                            else
                            {
                                currentWord += c;
                                wordWidth += charWidth;

                                if (currentLineWidth + wordWidth > textComp->maxWidth)
                                {
                                    if (!currentLine.empty())
                                    {
                                        lines.push_back(currentLine);
                                        currentLine.clear();
                                        currentLineWidth = 0.0f;
                                    }
                                    else
                                    {
                                        currentLine = currentWord;
                                        lines.push_back(currentLine);
                                        currentLine.clear();
                                        currentWord.clear();
                                        currentLineWidth = 0.0f;
                                        wordWidth = 0.0f;
                                    }
                                }
                            }
                        }

                        if (!currentWord.empty() || !currentLine.empty())
                        {
                            currentLine += currentWord;
                            lines.push_back(currentLine);
                        }
                    }
                    else
                    {
                        std::string currentLine;
                        float currentWidth = 0;
                        for (char c : text)
                        {
                            if (c == '\n')
                            {
                                lines.push_back(currentLine);
                                currentLine.clear();
                                currentWidth = 0;
                                continue;
                            }
                            const Character& ch = textComp->font->GetCharacter(c);
                            float charWidth = (ch.advance >> 6) * scale;
                            if (currentWidth + charWidth > textComp->maxWidth)
                            {
                                lines.push_back(currentLine);
                                currentLine = c;
                                currentWidth = charWidth;
                            }
                            else
                            {
                                currentLine += c;
                                currentWidth += charWidth;
                            }
                        }
                        lines.push_back(currentLine);
                    }
                }
                else
                {
                    std::stringstream ss(text);
                    std::string line;
                    while (std::getline(ss, line, '\n'))
                    {
                        lines.push_back(line);
                    }
                    if (lines.empty())
                    {
                        lines.push_back("");
                    }
                }

                float startY = finalPos.y;
                for (const auto& line : lines)
                {
                    float lineWidth = 0;
                    for (char c : line) lineWidth += (textComp->font->GetCharacter(c).advance >> 6) * scale;

                    float startX = finalPos.x;
                    if (textComp->alignment == TextAlignment::Center)
                        startX += (finalSize.x - lineWidth) * 0.5f;
                    else if (textComp->alignment == TextAlignment::Right)
                        startX += (finalSize.x - lineWidth);

                    float x = startX;

                    glm::vec2 pivotPos = finalPos + transform.pivot * finalSize;
                    float rotRads = glm::radians(transform.rotation);
                    float cosR = cos(rotRads);
                    float sinR = sin(rotRads);

                    auto transformPt = [&](float px, float py) -> glm::vec2 {
                        if (transform.rotation == 0.0f && !transform.flipX && !transform.flipY)
                            return {px, py};

                        float dx = px - pivotPos.x;
                        float dy = py - pivotPos.y;

                        if (transform.flipX)
                            dx = -dx;
                        if (transform.flipY)
                            dy = -dy;

                        if (transform.rotation == 0.0f)
                            return {pivotPos.x + dx, pivotPos.y + dy};

                        return {pivotPos.x + dx * cosR - dy * sinR, pivotPos.y + dx * sinR + dy * cosR};
                    };

                    for (char c : line)
                    {
                        const Character& ch = textComp->font->GetCharacter(c);

                        float xpos = x + ch.bearing.x * scale;
                        float ypos = startY + (ch.size.y - ch.bearing.y) * scale;
                        float w = ch.size.x * scale;
                        float h = ch.size.y * scale;

                        glm::vec2 p1 = transformPt(xpos, ypos - h);
                        glm::vec2 p2 = transformPt(xpos, ypos);
                        glm::vec2 p3 = transformPt(xpos + w, ypos);
                        glm::vec2 p4 = transformPt(xpos + w, ypos - h);

                        std::vector<float> vertices = {
                            p1.x, p1.y, 0.0f, 0.0f, p2.x, p2.y, 0.0f, 1.0f, p3.x, p3.y, 1.0f, 1.0f,

                            p1.x, p1.y, 0.0f, 0.0f, p3.x, p3.y, 1.0f, 1.0f, p4.x, p4.y, 1.0f, 0.0f};

                        textComp->model->DrawDynamic(*currentShader, ch.textureID, textComp->color, vertices);
                        x += (ch.advance >> 6) * scale;
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

void UIRenderSystem::UpdateLayout(Scene& scene, float screenWidth, float screenHeight)
{
    auto view = scene.View<UITransformComponent, UIFlexLayoutComponent, HierarchyComponent>();
    for (auto entity : view)
    {
        auto& flex = view.get<UIFlexLayoutComponent>(entity);
        auto& hierarchy = view.get<HierarchyComponent>(entity);
        auto& parentTransform = view.get<UITransformComponent>(entity);

        if (hierarchy.children.empty())
            continue;

        UIRect parentRect = CalculateRect(scene.GetRegistry(), entity, screenWidth, screenHeight);
        float currentOffset = (flex.direction == FlexDirection::Row) ? flex.padding.x : flex.padding.y;

        for (auto child : hierarchy.children)
        {
            if (!scene.HasAllComponents<UITransformComponent>(child))
                continue;
            auto& childTransform = scene.GetComponent<UITransformComponent>(child);

            if (flex.direction == FlexDirection::Row)
            {
                childTransform.anchorMin = glm::vec2(0, 0.5f);
                childTransform.anchorMax = glm::vec2(0, 0.5f);
                childTransform.offsetMin = glm::vec2(currentOffset, -childTransform.size.y * 0.5f);
                childTransform.offsetMax =
                    glm::vec2(currentOffset + childTransform.size.x, childTransform.size.y * 0.5f);
                currentOffset += childTransform.size.x + flex.spacing;
            }
            else
            {
                childTransform.anchorMin = glm::vec2(0.5f, 0);
                childTransform.anchorMax = glm::vec2(0.5f, 0);
                childTransform.offsetMin = glm::vec2(-childTransform.size.x * 0.5f, currentOffset);
                childTransform.offsetMax =
                    glm::vec2(childTransform.size.x * 0.5f, currentOffset + childTransform.size.y);
                currentOffset += childTransform.size.y + flex.spacing;
            }
        }

        if (flex.autoSize)
        {
            if (flex.direction == FlexDirection::Row)
            {
                float totalWidth = currentOffset - flex.spacing + flex.padding.z;
                parentTransform.size.x = totalWidth;
            }
            else
            {
                float totalHeight = currentOffset - flex.spacing + flex.padding.w;
                parentTransform.size.y = totalHeight;
            }
        }
    }
}

void UIRenderSystem::Update(Scene& scene, float dt)
{
}

void UIRenderSystem::Render(Scene& scene)
{
}

std::vector<entt::id_type> UIRenderSystem::GetReadComponents() const
{
    return {entt::type_id<UIRendererComponent>().hash(), entt::type_id<UITransformComponent>().hash(),
            entt::type_id<UIAnimationComponent>().hash()};
}

std::vector<entt::id_type> UIRenderSystem::GetWriteComponents() const
{
    return {};
}

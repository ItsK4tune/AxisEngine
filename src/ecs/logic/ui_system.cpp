#include <ecs/unit/core_components.h>
#include <ecs/unit/ui_components.h>
#include <ecs/logic/ui_system.h>
#include <platform/interface/input_codes.h>
#include <core/logic/logger.h>
#include <glm/gtc/matrix_transform.hpp>

void UIRenderSystem::RenderUI(Scene &scene, float screenWidth, float screenHeight, IRenderStateManager &renderState)
{
    if (!m_Enabled)
    {
        return;
    }

    renderState.Disable(ServerCapability::DepthTest);
    renderState.Disable(ServerCapability::CullFace);
    renderState.Enable(ServerCapability::Blend);
    renderState.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

    PolygonMode previousPolygonMode = renderState.GetPolygonMode();
    renderState.SetPolygonMode(CullMode::FrontAndBack, PolygonMode::Fill);

    scene.registry.sort<UITransformComponent>([](const auto &lhs, const auto &rhs)
                                              { return lhs.zIndex < rhs.zIndex; });

    glm::mat4 projection = glm::ortho(0.0f, screenWidth, screenHeight, 0.0f, -1.0f, 1.0f);
    Shader *currentShader = nullptr;

    auto view = scene.registry.view<UITransformComponent>();

    for (auto entity : view)
    {
        auto &transform = view.get<UITransformComponent>(entity);

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

                glm::vec2 actualPos = transform.position;
                glm::vec2 actualSize = transform.size;

                if (transform.usePercentage)
                {
                    actualPos.x = (transform.position.x / 100.0f) * screenWidth;
                    actualPos.y = (transform.position.y / 100.0f) * screenHeight;
                    actualSize.x = (transform.size.x / 100.0f) * screenWidth;
                    actualSize.y = (transform.size.y / 100.0f) * screenHeight;
                }

                actualPos -= transform.anchor * actualSize;

                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(actualPos, 0.0f));
                model = glm::scale(model, glm::vec3(actualSize, 1.0f));
                currentShader->setMat4("model", model);

                renderer->model->Draw(*currentShader, renderer->color);
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

                glm::vec2 actualPos = transform.position;
                glm::vec2 actualSize = transform.size;

                if (transform.usePercentage)
                {
                    actualPos.x = (transform.position.x / 100.0f) * screenWidth;
                    actualPos.y = (transform.position.y / 100.0f) * screenHeight;
                    actualSize.x = (transform.size.x / 100.0f) * screenWidth;
                    actualSize.y = (transform.size.y / 100.0f) * screenHeight;
                }

                actualPos -= transform.anchor * actualSize;

                float x = actualPos.x;
                float y = actualPos.y;
                float scale = textComp->scale;

                for (char c : textComp->text)
                {
                    const Character &ch = textComp->font->GetCharacter(c);

                    float xpos = x + ch.Bearing.x * scale;
                    float ypos = y + (ch.Size.y - ch.Bearing.y) * scale;
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
            }
        }
    }

    renderState.Enable(ServerCapability::DepthTest);
    renderState.Disable(ServerCapability::Blend);

    renderState.SetPolygonMode(CullMode::FrontAndBack, previousPolygonMode);
}

void UIRenderSystem::Render(Scene &scene)
{
}

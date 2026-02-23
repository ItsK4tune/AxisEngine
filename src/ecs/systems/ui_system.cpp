#include <ecs/system.h>
#include <interface/window/input_codes.h>

void UIRenderSystem::Render(Scene &scene, float screenWidth, float screenHeight, IRenderStateManager& renderState)
{
    if (!m_Enabled)
        return;

    renderState.Disable(Graphics::ServerCapability::DepthTest);
    renderState.Disable(Graphics::ServerCapability::CullFace);
    renderState.Enable(Graphics::ServerCapability::Blend);
    renderState.BlendFunc(Graphics::BlendFactor::SrcAlpha, Graphics::BlendFactor::OneMinusSrcAlpha);

    Graphics::PolygonMode previousPolygonMode = renderState.GetPolygonMode();
    renderState.PolygonMode(Graphics::CullMode::FrontAndBack, Graphics::PolygonMode::Fill);

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
                Shader* shaderPtr = renderer->shader.get();
                if (currentShader != shaderPtr)
                {
                    currentShader = shaderPtr;
                    currentShader->use();
                    currentShader->setMat4("projection", projection);
                    currentShader->setInt("image", 0);
                }

                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(transform.position, 0.0f));
                model = glm::scale(model, glm::vec3(transform.size, 1.0f));
                currentShader->setMat4("model", model);

                renderer->model->Draw(*currentShader, renderer->color);
            }
        }

        if (auto *textComp = scene.registry.try_get<UITextComponent>(entity))
        {
            if (textComp->model && textComp->shader && textComp->font)
            {
                Shader* shaderPtr = textComp->shader.get();
                if (currentShader != shaderPtr)
                {
                    currentShader = shaderPtr;
                    currentShader->use();
                    currentShader->setMat4("projection", projection);
                    currentShader->setInt("text", 0);
                }

                float x = transform.position.x;
                float y = transform.position.y;
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

    renderState.Enable(Graphics::ServerCapability::DepthTest);
    renderState.Disable(Graphics::ServerCapability::Blend);

    renderState.PolygonMode(Graphics::CullMode::FrontAndBack, previousPolygonMode);
}

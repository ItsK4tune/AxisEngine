#include <ecs/system.h>

void UIRenderSystem::Render(Scene &scene, float screenWidth, float screenHeight)
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
                if (currentShader != renderer->shader)
                {
                    currentShader = renderer->shader;
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
                if (currentShader != textComp->shader)
                {
                    currentShader = textComp->shader;
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

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void UIInteractSystem::Update(Scene &scene, float dt, const MouseManager &mouse)
{
    if (mouse.GetCursorMode() == CursorMode::Hidden)
        return;

    float mx = mouse.GetLastX();
    float my = mouse.GetLastY();
    bool isMouseDown = mouse.IsLeftButtonPressed();

    auto view = scene.registry.view<UITransformComponent, UIInteractiveComponent>();

    for (auto entity : view)
    {
        auto &transform = view.get<UITransformComponent>(entity);
        auto &interact = view.get<UIInteractiveComponent>(entity);

        bool hit = (mx >= transform.position.x && mx <= transform.position.x + transform.size.x &&
                    my >= transform.position.y && my <= transform.position.y + transform.size.y);

        if (hit)
        {
            if (!interact.isHovered)
            {
                interact.isHovered = true;
                if (interact.onHoverEnter)
                    interact.onHoverEnter(entity);
            }
        }
        else
        {
            if (interact.isHovered)
            {
                interact.isHovered = false;
                if (interact.onHoverExit)
                    interact.onHoverExit(entity);
            }
        }

        if (hit && isMouseDown)
        {
            if (!interact.isPressed)
            {
                interact.isPressed = true;
                if (interact.onClick)
                    interact.onClick(entity);
            }
        }
        else if (!isMouseDown)
        {
            interact.isPressed = false;
        }

        if (scene.registry.all_of<UIAnimationComponent>(entity))
        {
            auto &anim = scene.registry.get<UIAnimationComponent>(entity);
            auto &img = scene.registry.get_or_emplace<UIRendererComponent>(entity);

            if (interact.isHovered)
            {
                img.color = glm::mix(img.color, anim.hoverColor, dt * anim.speed);
                anim.targetScale = 1.2f;
            }
            else
            {
                img.color = glm::mix(img.color, anim.normalColor, dt * anim.speed);
                anim.targetScale = 1.0f;
            }

            anim.currentScale += (anim.targetScale - anim.currentScale) * dt * anim.speed;
        }
    }
}

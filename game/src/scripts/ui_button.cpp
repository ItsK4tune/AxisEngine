#include <core/application.h>
#include <core/scriptable.h>
#include <core/script_registry.h>
#include <iostream>
#include <glm/gtx/compatibility.hpp> // For glm::lerp if needed, or just standard mix

class UIButton : public Scriptable
{
public:
    glm::vec4 normalColor;
    glm::vec4 hoverColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
    glm::vec4 clickColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    glm::vec2 originalSize;
    bool isInit = false;

    void OnUpdate(float dt) override
    {
        if (!IsEnabled()) return;

        // Ensure we have required components
        if (!HasComponent<UITransformComponent>() || !HasComponent<UIRendererComponent>()) return;

        auto& transform = GetComponent<UITransformComponent>();
        auto& renderer = GetComponent<UIRendererComponent>();

        // Init initial state
        if (!isInit) {
            normalColor = renderer.color;
            originalSize = transform.size;
            isInit = true;
        }

        // Logic
        float mx = (float)m_App->GetMouse().GetLastX();
        float my = (float)m_App->GetMouse().GetLastY();

        bool hit = (mx >= transform.position.x && mx <= transform.position.x + transform.size.x &&
                    my >= transform.position.y && my <= transform.position.y + transform.size.y);

        glm::vec4 targetColor = normalColor;
        glm::vec2 targetSize = originalSize;

        if (hit)
        {
            if (m_App->GetMouse().IsLeftButtonPressed())
            {
                targetColor = clickColor;
                targetSize = originalSize * 0.95f; // Shrink on click
            }
            else
            {
                targetColor = hoverColor;
                targetSize = originalSize * 1.05f; // Grow on hover
            }

            // Click Event (Trigger once)
            if (m_App->GetMouse().IsLeftMouseClicked())
            {
                auto view = m_Scene->registry.view<VideoPlayerComponent>();
                bool found = false;
                for (auto entity : view)
                {
                    auto& video = view.get<VideoPlayerComponent>(entity);
                    if (video.isPlaying) 
                    {
                        video.Pause();
                        std::cout << "[UIButton] Paused Video" << std::endl;
                    }
                    else 
                    {
                        video.Play();
                        std::cout << "[UIButton] Resumed Video" << std::endl;
                    }
                    found = true;
                }
                if (!found) std::cout << "[UIButton] No Video Player found!" << std::endl;
            }
        }

        // Animated Transition
        renderer.color = glm::mix(renderer.color, targetColor, dt * 15.0f);
        transform.size = glm::mix(transform.size, targetSize, dt * 15.0f);
    }
};

REGISTER_SCRIPT(UIButton);

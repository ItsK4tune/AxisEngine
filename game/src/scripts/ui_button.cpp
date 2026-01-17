#include <app/application.h>
#include <script/scriptable.h>
#include <script/script_registry.h>
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

    void OnCreate() override
    {
        // Example: Bind 'K' to pause/resume video
        BindKey(GLFW_KEY_K, InputEvent::Pressed, [this]()
                { ToggleVideo(); });
    }

    void OnLeftClick() override
    {
        ToggleVideo();

        // Visual Feedback
        if (HasComponent<UITransformComponent>())
        {
            auto &transform = GetComponent<UITransformComponent>();
            transform.size = originalSize * 0.9f; // Snap shrink
        }
    }

    void OnLeftRelease(float duration) override
    {
        // Return to normal (handled by Update animation)
    }

    void ToggleVideo()
    {
        auto view = m_Scene->registry.view<VideoPlayerComponent>();
        bool found = false;
        for (auto entity : view)
        {
            auto &video = view.get<VideoPlayerComponent>(entity);
            if (video.isPlaying)
            {
                video.Pause();
            }
            else
            {
                video.Play();
            }
            found = true;
        }

    }

    void OnUpdate(float dt) override
    {
        if (!IsEnabled())
            return;
        if (!HasComponent<UITransformComponent>() || !HasComponent<UIRendererComponent>())
            return;

        auto &transform = GetComponent<UITransformComponent>();
        auto &renderer = GetComponent<UIRendererComponent>();

        // Init initial state
        if (!isInit)
        {
            normalColor = renderer.color;
            originalSize = transform.size;
            isInit = true;
        }

        // Visual Logic based on State (set by ScriptSystem)
        glm::vec4 targetColor = normalColor;
        glm::vec2 targetSize = originalSize;

        if (IsHovered())
        {
            if (IsLeftPressed()) // Internal state from Scriptable
            {
                targetColor = clickColor;
                targetSize = originalSize * 0.95f;
            }
            else
            {
                targetColor = hoverColor;
                targetSize = originalSize * 1.05f;
            }
        }

        // Animated Transition
        renderer.color = glm::mix(renderer.color, targetColor, dt * 15.0f);
        transform.size = glm::mix(transform.size, targetSize, dt * 15.0f);
    }
};

REGISTER_SCRIPT(UIButton);

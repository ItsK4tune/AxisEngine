#pragma once
#include <axis_all.h>
#include <string>
#include <cmath>

enum class ScenarioMode
{
    Hover,
    Click,
    Hold
};

inline std::string SecondsText(float seconds)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2fs", seconds);
    return std::string(buffer);
}

class ScenarioInteractiveControl : public InputScriptable
{
public:
    ScenarioInteractiveControl(entt::entity label, entt::entity status, entt::entity meter, ScenarioMode mode,
                                 std::string title, glm::vec4 baseColor, glm::vec4 hoverColor,
                                 glm::vec4 activeColor)
        : m_Label(label),
          m_Status(status),
          m_Meter(meter),
          m_Mode(mode),
          m_Title(std::move(title)),
          m_BaseColor(baseColor),
          m_HoverColor(hoverColor),
          m_ActiveColor(activeColor)
    {
    }

    void OnCreate() override
    {
        SetColor(m_BaseColor);
        SetMeter(0.0f);
        SetLabel("Ready");
    }

    void OnUpdate(float dt) override
    {
        if (!IsHovered() || IsLeftPressed())
            return;

        m_Pulse += dt;
        const float pulse = 0.18f + 0.12f * (0.5f + 0.5f * std::sin(m_Pulse * 7.0f));
        SetColor(glm::mix(m_HoverColor, glm::vec4(1.0f), pulse));
    }

    void OnHoverEnter() override
    {
        m_HoverTicks = 0;
        m_Pulse = 0.0f;
        SetColor(m_HoverColor);
        SetLabel("OnHoverEnter");
        SetStatus("Latest event: " + m_Title + " -> OnHoverEnter");
    }

    void OnHoverStay() override
    {
        ++m_HoverTicks;
        if (m_Mode == ScenarioMode::Hover)
            SetLabel("OnHoverStay #" + std::to_string(m_HoverTicks));
    }

    void OnHoverExit() override
    {
        SetColor(m_BaseColor);
        SetLabel("OnHoverExit");
        SetStatus("Latest event: " + m_Title + " -> OnHoverExit");
    }

    void OnLeftClick() override
    {
        ++m_ClickCount;
        SetColor(m_ActiveColor);
        if (m_Mode == ScenarioMode::Click)
            SetLabel("OnLeftClick count: " + std::to_string(m_ClickCount));
        else
            SetLabel("OnLeftClick");
        SetStatus("Latest event: " + m_Title + " -> OnLeftClick");
    }

    void OnLeftHold(float duration) override
    {
        if (m_Mode != ScenarioMode::Hold)
            return;

        SetColor(m_ActiveColor);
        SetMeter(glm::clamp(duration / 2.0f, 0.0f, 1.0f));
        SetLabel("OnLeftHold " + SecondsText(duration));
        SetStatus("Latest event: " + m_Title + " -> OnLeftHold " + SecondsText(duration));
    }

    void OnLeftRelease(float duration) override
    {
        SetColor(IsHovered() ? m_HoverColor : m_BaseColor);
        if (m_Mode == ScenarioMode::Hold)
        {
            SetMeter(0.0f);
            SetLabel("OnLeftRelease " + SecondsText(duration));
        }
        else
        {
            SetLabel("OnLeftRelease");
        }
        SetStatus("Latest event: " + m_Title + " -> OnLeftRelease " + SecondsText(duration));
    }

    void OnRightClick() override
    {
        SetColor(glm::vec4(0.78f, 0.25f, 0.29f, 0.98f));
        SetLabel("OnRightClick");
        SetStatus("Latest event: " + m_Title + " -> OnRightClick");
    }

    void OnRightRelease(float duration) override
    {
        SetColor(IsHovered() ? m_HoverColor : m_BaseColor);
        SetStatus("Latest event: " + m_Title + " -> OnRightRelease " + SecondsText(duration));
    }

    void OnMiddleClick() override
    {
        SetColor(glm::vec4(0.35f, 0.32f, 0.76f, 0.98f));
        SetLabel("OnMiddleClick");
        SetStatus("Latest event: " + m_Title + " -> OnMiddleClick");
    }

    void OnMiddleRelease(float duration) override
    {
        SetColor(IsHovered() ? m_HoverColor : m_BaseColor);
        SetStatus("Latest event: " + m_Title + " -> OnMiddleRelease " + SecondsText(duration));
    }

private:
    void SetColor(const glm::vec4& color)
    {
        auto& scene = GetScene();
        if (scene.IsValid(m_Entity))
        {
            if (auto* renderer = scene.TryGetComponent<UIRendererComponent>(m_Entity))
                renderer->color = color;
        }
    }

    void SetLabel(const std::string& detail)
    {
        auto& scene = GetScene();
        if (scene.IsValid(m_Label))
        {
            if (auto* text = scene.TryGetComponent<UITextComponent>(m_Label))
                text->text = m_Title + "\n" + detail;
        }
    }

    void SetStatus(const std::string& status)
    {
        auto& scene = GetScene();
        if (scene.IsValid(m_Status))
        {
            if (auto* text = scene.TryGetComponent<UITextComponent>(m_Status))
                text->text = status;
        }
    }

    void SetMeter(float ratio)
    {
        if (m_Meter == entt::null)
            return;

        auto& scene = GetScene();
        if (!scene.IsValid(m_Meter))
            return;

        const float clamped = glm::clamp(ratio, 0.0f, 1.0f);
        if (auto* transform = scene.TryGetComponent<UITransformComponent>(m_Meter))
            transform->size.x = 250.0f * clamped;
        if (auto* renderer = scene.TryGetComponent<UIRendererComponent>(m_Meter))
            renderer->color = glm::mix(glm::vec4(0.18f, 0.72f, 0.54f, 0.96f),
                                       glm::vec4(0.92f, 0.63f, 0.22f, 0.98f), clamped);
    }

    entt::entity m_Label = entt::null;
    entt::entity m_Status = entt::null;
    entt::entity m_Meter = entt::null;
    ScenarioMode m_Mode = ScenarioMode::Hover;
    std::string m_Title;
    glm::vec4 m_BaseColor = glm::vec4(1.0f);
    glm::vec4 m_HoverColor = glm::vec4(1.0f);
    glm::vec4 m_ActiveColor = glm::vec4(1.0f);
    int m_ClickCount = 0;
    int m_HoverTicks = 0;
    float m_Pulse = 0.0f;
};

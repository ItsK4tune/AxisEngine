#include "sample_scenario_common.h"

#include <ecs/unit/script_component.h>
#include <script/logic/input_scriptable.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>

namespace
{
enum class Scenario29Mode
{
    Hover,
    Click,
    Hold
};

std::string SecondsText(float seconds)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2fs", seconds);
    return std::string(buffer);
}

class Scenario29InteractiveControl : public InputScriptable
{
public:
    Scenario29InteractiveControl(entt::entity label, entt::entity status, entt::entity meter, Scenario29Mode mode,
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
        if (m_Mode == Scenario29Mode::Hover)
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
        if (m_Mode == Scenario29Mode::Click)
            SetLabel("OnLeftClick count: " + std::to_string(m_ClickCount));
        else
            SetLabel("OnLeftClick");
        SetStatus("Latest event: " + m_Title + " -> OnLeftClick");
    }

    void OnLeftHold(float duration) override
    {
        if (m_Mode != Scenario29Mode::Hold)
            return;

        SetColor(m_ActiveColor);
        SetMeter(glm::clamp(duration / 2.0f, 0.0f, 1.0f));
        SetLabel("OnLeftHold " + SecondsText(duration));
        SetStatus("Latest event: " + m_Title + " -> OnLeftHold " + SecondsText(duration));
    }

    void OnLeftRelease(float duration) override
    {
        SetColor(IsHovered() ? m_HoverColor : m_BaseColor);
        if (m_Mode == Scenario29Mode::Hold)
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
        auto& registry = GetScene().registry;
        if (registry.valid(m_Entity))
        {
            if (auto* renderer = registry.try_get<UIRendererComponent>(m_Entity))
                renderer->color = color;
        }
    }

    void SetLabel(const std::string& detail)
    {
        auto& registry = GetScene().registry;
        if (registry.valid(m_Label))
        {
            if (auto* text = registry.try_get<UITextComponent>(m_Label))
                text->text = m_Title + "\n" + detail;
        }
    }

    void SetStatus(const std::string& status)
    {
        auto& registry = GetScene().registry;
        if (registry.valid(m_Status))
        {
            if (auto* text = registry.try_get<UITextComponent>(m_Status))
                text->text = status;
        }
    }

    void SetMeter(float ratio)
    {
        if (m_Meter == entt::null)
            return;

        auto& registry = GetScene().registry;
        if (!registry.valid(m_Meter))
            return;

        const float clamped = glm::clamp(ratio, 0.0f, 1.0f);
        if (auto* transform = registry.try_get<UITransformComponent>(m_Meter))
            transform->size.x = 250.0f * clamped;
        if (auto* renderer = registry.try_get<UIRendererComponent>(m_Meter))
            renderer->color = glm::mix(glm::vec4(0.18f, 0.72f, 0.54f, 0.96f),
                                       glm::vec4(0.92f, 0.63f, 0.22f, 0.98f), clamped);
    }

    entt::entity m_Label = entt::null;
    entt::entity m_Status = entt::null;
    entt::entity m_Meter = entt::null;
    Scenario29Mode m_Mode = Scenario29Mode::Hover;
    std::string m_Title;
    glm::vec4 m_BaseColor = glm::vec4(1.0f);
    glm::vec4 m_HoverColor = glm::vec4(1.0f);
    glm::vec4 m_ActiveColor = glm::vec4(1.0f);
    int m_ClickCount = 0;
    int m_HoverTicks = 0;
    float m_Pulse = 0.0f;
};

void AttachScenario29Control(Scene& scene, entt::entity control, entt::entity label, entt::entity status,
                             entt::entity meter, Scenario29Mode mode, const std::string& title,
                             const glm::vec4& baseColor, const glm::vec4& hoverColor,
                             const glm::vec4& activeColor)
{
    auto& script = scene.registry.emplace_or_replace<ScriptComponent>(control);
    script.className = "Scenario29InteractiveControl";
    script.InstantiateScript = [label, status, meter, mode, title, baseColor, hoverColor, activeColor]() {
        return std::make_unique<Scenario29InteractiveControl>(label, status, meter, mode, title, baseColor, hoverColor,
                                                              activeColor);
    };
    script.DestroyScript = [](ScriptComponent* sc) { sc->instance.reset(); };
}

entt::entity CreateScenario29Control(Scene& scene, ResourceManager& res, entt::entity root, entt::entity status,
                                     const std::string& name, const glm::vec2& pos, const glm::vec2& size,
                                     Scenario29Mode mode, const glm::vec4& baseColor, const glm::vec4& hoverColor,
                                     const glm::vec4& activeColor)
{
    auto control = EntityBuilder(scene, res, "scenario")
                       .WithName(name)
                       .WithUIChild(root, pos, size, 32)
                       .WithUIRenderer(name + "_background", baseColor)
                       .Build();

    entt::entity meter = entt::null;
    if (mode == Scenario29Mode::Hold)
    {
        EntityBuilder(scene, res, "scenario")
            .WithName(name + "_MeterTrack")
            .WithUIChild(control, glm::vec2(18.0f, size.y - 34.0f), glm::vec2(250.0f, 12.0f), 33)
            .WithUIRenderer(name + "_meter_track", glm::vec4(0.08f, 0.09f, 0.10f, 0.86f))
            .Build();

        meter = EntityBuilder(scene, res, "scenario")
                    .WithName(name + "_MeterFill")
                    .WithUIChild(control, glm::vec2(18.0f, size.y - 34.0f), glm::vec2(0.0f, 12.0f), 34)
                    .WithUIRenderer(name + "_meter_fill", glm::vec4(0.18f, 0.72f, 0.54f, 0.96f))
                    .Build();
    }

    auto label = EntityBuilder(scene, res, "scenario")
                     .WithName(name + "_Label")
                     .WithUIChild(control, glm::vec2(18.0f, 18.0f), glm::vec2(size.x - 36.0f, size.y - 36.0f), 35)
                     .WithUIText(name + "\nReady", "time", 0.9f, glm::vec4(0.96f, 0.98f, 1.0f, 1.0f))
                     .WithUITextAlignment(TextAlignment::Left, true, size.x - 48.0f)
                     .Build();

    AttachScenario29Control(scene, control, label, status, meter, mode, name, baseColor, hoverColor, activeColor);
    return control;
}
}  // namespace

void SampleState::LoadScene29()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Scenario29Ground")
        .WithPBRRenderable("planeModel", "deferred_lit", glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(0.0f),
                           glm::vec3(80.0f, 1.0f, 80.0f), 0.0f, 0.82f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("Scenario29DirLight")
        .WithDirectionalLightAt(glm::vec3(16.0f, 36.0f, 18.0f), glm::vec3(-45.0f, -35.0f, 0.0f),
                                glm::normalize(glm::vec3(-0.6f, -1.0f, -0.45f)),
                                glm::vec3(1.0f, 0.96f, 0.88f), 1.2f)
        .Build();

    auto root = EntityBuilder(scene, res, "scenario")
                    .WithName("Scenario29InteractRoot")
                    .WithUIStretch(glm::vec2(0.56f, 0.08f), glm::vec2(0.98f, 0.92f))
                    .WithUIZIndex(28)
                    .WithUIRenderer("scenario29_panel", glm::vec4(0.08f, 0.09f, 0.11f, 0.94f))
                    .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("Scenario29Title")
        .WithUIChild(root, glm::vec2(32.0f, 24.0f), glm::vec2(600.0f, 64.0f), 30)
        .WithUIText("Interactive UI Callbacks", "time", 1.08f, glm::vec4(0.94f, 0.97f, 1.0f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 560.0f)
        .Build();

    auto status = EntityBuilder(scene, res, "scenario")
                      .WithName("Scenario29Status")
                      .WithUIChild(root, glm::vec2(32.0f, 548.0f), glm::vec2(600.0f, 64.0f), 35)
                      .WithUIRenderer("scenario29_status_bg", glm::vec4(0.13f, 0.14f, 0.16f, 0.92f))
                      .WithUIText("Latest event: Ready", "time", 0.82f, glm::vec4(0.78f, 0.88f, 0.96f, 1.0f))
                      .WithUITextAlignment(TextAlignment::Left, true, 560.0f)
                      .Build();

    CreateScenario29Control(scene, res, root, status, "Hover Target", glm::vec2(32.0f, 112.0f),
                            glm::vec2(600.0f, 106.0f), Scenario29Mode::Hover,
                            glm::vec4(0.16f, 0.24f, 0.26f, 0.96f), glm::vec4(0.18f, 0.46f, 0.42f, 0.98f),
                            glm::vec4(0.24f, 0.58f, 0.48f, 0.98f));

    CreateScenario29Control(scene, res, root, status, "Click Counter", glm::vec2(32.0f, 242.0f),
                            glm::vec2(288.0f, 128.0f), Scenario29Mode::Click,
                            glm::vec4(0.22f, 0.20f, 0.25f, 0.96f), glm::vec4(0.44f, 0.31f, 0.45f, 0.98f),
                            glm::vec4(0.66f, 0.39f, 0.52f, 0.98f));

    CreateScenario29Control(scene, res, root, status, "Hold Meter", glm::vec2(344.0f, 242.0f),
                            glm::vec2(288.0f, 128.0f), Scenario29Mode::Hold,
                            glm::vec4(0.18f, 0.22f, 0.30f, 0.96f), glm::vec4(0.24f, 0.36f, 0.55f, 0.98f),
                            glm::vec4(0.26f, 0.50f, 0.76f, 0.98f));

    CreateScenario29Control(scene, res, root, status, "Mouse Buttons", glm::vec2(32.0f, 394.0f),
                            glm::vec2(600.0f, 126.0f), Scenario29Mode::Click,
                            glm::vec4(0.24f, 0.22f, 0.16f, 0.96f), glm::vec4(0.48f, 0.38f, 0.20f, 0.98f),
                            glm::vec4(0.72f, 0.48f, 0.22f, 0.98f));
}

#include "sample_scenario_common.h"
#include <script/logic/input_scriptable.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>

namespace
{
} // namespace

#include "scriptable/scenario_interactive_control.h"

namespace
{

void AttachScenarioControl(Scene& scene, Entity control, Entity label, Entity status,
                             Entity meter, ScenarioMode mode, const std::string& title,
                             const glm::vec4& baseColor, const glm::vec4& hoverColor,
                             const glm::vec4& activeColor)
{
    Entity(control, &scene).AddScript<ScenarioInteractiveControl>(label, status, meter, mode, title, baseColor, hoverColor, activeColor);
}

Entity CreateScenarioControl(Scene& scene, ResourceManager& res, Entity root, Entity status,
                               const std::string& name, const glm::vec2& pos, const glm::vec2& size,
                               ScenarioMode mode, const glm::vec4& baseColor, const glm::vec4& hoverColor,
                               const glm::vec4& activeColor)
{
    auto control = EntityBuilder(scene, res, "scenario")
                       .WithName(name)
                       .WithUIChild(root, pos, size, 32)
                       .WithUIRenderer(name + "_background", baseColor)
                       .Build();

    Entity meter;
    if (mode == ScenarioMode::Hold)
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
                     .WithUIText(name + "\nReady", "time", 0.45f, glm::vec4(0.96f, 0.98f, 1.0f, 1.0f))
                     .WithUITextAlignment(TextAlignment::Left, true, size.x - 48.0f)
                     .Build();

    AttachScenarioControl(scene, control, label, status, meter, mode, name, baseColor, hoverColor, activeColor);
    return control;
}
}  // namespace

void SampleState::LoadScene17()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("ScenarioGround")
        .WithPBRRenderable("planeModel", "deferred_lit", glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(0.0f),
                           glm::vec3(80.0f, 1.0f, 80.0f), 0.0f, 0.82f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("ScenarioDirLight")
        .WithDirectionalLightAt(glm::vec3(16.0f, 36.0f, 18.0f), glm::vec3(-45.0f, -35.0f, 0.0f),
                                glm::normalize(glm::vec3(-0.6f, -1.0f, -0.45f)),
                                glm::vec3(1.0f, 0.96f, 0.88f), 1.2f)
        .Build();

    auto root = EntityBuilder(scene, res, "scenario")
                    .WithName("ScenarioInteractRoot")
                    .WithUIStretch(glm::vec2(0.56f, 0.08f), glm::vec2(0.98f, 0.92f))
                    .WithUIZIndex(28)
                    .WithUIRenderer("Scenario_panel", glm::vec4(0.08f, 0.09f, 0.11f, 0.94f))
                    .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("ScenarioTitle")
        .WithUIChild(root, glm::vec2(32.0f, 24.0f), glm::vec2(600.0f, 64.0f), 30)
        .WithUIText("Interactive UI Callbacks", "time", 0.54f, glm::vec4(0.94f, 0.97f, 1.0f, 1.0f))
        .WithUITextAlignment(TextAlignment::Left, true, 560.0f)
        .Build();

    auto status = EntityBuilder(scene, res, "scenario")
                      .WithName("ScenarioStatus")
                      .WithUIChild(root, glm::vec2(32.0f, 548.0f), glm::vec2(600.0f, 64.0f), 35)
                      .WithUIRenderer("Scenario_status_bg", glm::vec4(0.13f, 0.14f, 0.16f, 0.92f))
                      .WithUIText("Latest event: Ready", "time", 0.41f, glm::vec4(0.78f, 0.88f, 0.96f, 1.0f))
                      .WithUITextAlignment(TextAlignment::Left, true, 560.0f)
                      .Build();

    CreateScenarioControl(scene, res, root, status, "Hover Target", glm::vec2(32.0f, 112.0f),
                            glm::vec2(600.0f, 106.0f), ScenarioMode::Hover,
                            glm::vec4(0.16f, 0.24f, 0.26f, 0.96f), glm::vec4(0.18f, 0.46f, 0.42f, 0.98f),
                            glm::vec4(0.24f, 0.58f, 0.48f, 0.98f));

    CreateScenarioControl(scene, res, root, status, "Click Counter", glm::vec2(32.0f, 242.0f),
                            glm::vec2(288.0f, 128.0f), ScenarioMode::Click,
                            glm::vec4(0.22f, 0.20f, 0.25f, 0.96f), glm::vec4(0.44f, 0.31f, 0.45f, 0.98f),
                            glm::vec4(0.66f, 0.39f, 0.52f, 0.98f));

    CreateScenarioControl(scene, res, root, status, "Hold Meter", glm::vec2(344.0f, 242.0f),
                            glm::vec2(288.0f, 128.0f), ScenarioMode::Hold,
                            glm::vec4(0.18f, 0.22f, 0.30f, 0.96f), glm::vec4(0.24f, 0.36f, 0.55f, 0.98f),
                            glm::vec4(0.26f, 0.50f, 0.76f, 0.98f));

    CreateScenarioControl(scene, res, root, status, "Mouse Buttons", glm::vec2(32.0f, 394.0f),
                            glm::vec2(600.0f, 126.0f), ScenarioMode::Click,
                            glm::vec4(0.24f, 0.22f, 0.16f, 0.96f), glm::vec4(0.48f, 0.38f, 0.20f, 0.98f),
                            glm::vec4(0.72f, 0.48f, 0.22f, 0.98f));
}

#include "sample_scenario_common.h"

void SampleState::LoadScene6()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    for (int i = 0; i < 100; ++i)
    {
        float angle = (static_cast<float>(i) * 3.6f) * 3.14159f / 180.0f;
        float radius = 10.0f + static_cast<float>(i % 5) * 3.0f;
        glm::vec3 pos(cos(angle) * radius, 0.5f + static_cast<float>(i % 3) * 2.0f, sin(angle) * radius);

        std::string name = "ScriptedEntity_" + std::to_string(i);
        auto entity = EntityBuilder(scene, res, "scenario")
            .WithName(name)
            .WithTransform(pos, glm::vec3(0.0f), glm::vec3(1.0f))
            .WithMesh("cubeModel", "deferred_unlit")
            .WithPBRMaterial(0.1f, 0.5f, 1.0f)
            .Build();

        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(entity))
        {
            float hue = static_cast<float>(i) * 0.0618f;
            renderer->color = glm::vec4(
                0.35f + 0.65f * (0.5f + 0.5f * sin(hue * 6.28318f)),
                0.35f + 0.65f * (0.5f + 0.5f * sin(hue * 6.28318f + 2.09439f)),
                0.35f + 0.65f * (0.5f + 0.5f * sin(hue * 6.28318f + 4.18879f)),
                1.0f);
        }

        std::string scriptName;
        int scriptType = i % 6;
        if (scriptType == 0) scriptName = "OrbitScript";
        else if (scriptType == 1) scriptName = "PulseScaleScript";
        else if (scriptType == 2) scriptName = "ColorShiftScript";
        else if (scriptType == 3) scriptName = "RandomMoveScript";
        else if (scriptType == 4) scriptName = "RotateScript";
        else scriptName = "BouncingScript";

        auto& script = scene.registry.emplace<ScriptComponent>(entity);
        script.className = scriptName;
        script.InstantiateScript = [scriptName]() {
            auto registry = ServiceLocator::Instance().Resolve<IScriptRegistry>();
            return registry ? registry->Create(scriptName) : nullptr;
        };
        script.DestroyScript = [](ScriptComponent* nsc) { nsc->instance.reset(); };
    }
}

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

    std::string modelName = "cubeModel";
    if (m_S6MeshType == 1)
        modelName = "sphereModel";
    else if (m_S6MeshType == 2)
        modelName = "capsuleModel";
    else if (m_S6MeshType == 3)
        modelName = "cylinderModel";

    std::string shaderName = "deferred_unlit";
    if (m_S6ShaderMode == 1)
        shaderName = "deferred_lit";
    else if (m_S6ShaderMode == 2)
        shaderName = "forward_pbr_lit";

    std::vector<std::string> enabledScripts;
    if (m_S6EnableOrbit)
        enabledScripts.push_back("OrbitScript");
    if (m_S6EnablePulse)
        enabledScripts.push_back("PulseScaleScript");
    if (m_S6EnableColor)
        enabledScripts.push_back("ColorShiftScript");
    if (m_S6EnableRandomMove)
        enabledScripts.push_back("RandomMoveScript");
    if (m_S6EnableRotate)
        enabledScripts.push_back("RotateScript");
    if (m_S6EnableBounce)
        enabledScripts.push_back("BouncingScript");

    int entityCount = (std::max)(1, m_S6EntityCount);
    for (int i = 0; i < entityCount; ++i)
    {
        float angle = (static_cast<float>(i) / static_cast<float>(entityCount)) * 6.28318f;
        float radius = m_S6BaseRadius + static_cast<float>(i % 5) * m_S6RadiusStep;
        glm::vec3 pos(cos(angle) * radius, 0.5f + static_cast<float>(i % 3) * m_S6VerticalStep,
                      sin(angle) * radius);

        std::string name = "ScriptedEntity_" + std::to_string(i);
        auto entity = EntityBuilder(scene, res, "scenario")
            .WithName(name)
            .WithTransform(pos, glm::vec3(0.0f), glm::vec3(m_S6EntityScale))
            .WithPBRMesh(modelName, shaderName, 0.1f, 0.5f, 1.0f)
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

        if (enabledScripts.empty())
            continue;

        std::string scriptName = enabledScripts[i % enabledScripts.size()];
        auto& script = scene.registry.emplace<ScriptComponent>(entity);
        script.className = scriptName;
        script.InstantiateScript = [scriptName]() {
            auto registry = ServiceLocator::Instance().Resolve<IScriptRegistry>();
            return registry ? registry->Create(scriptName) : nullptr;
        };
        script.DestroyScript = [](ScriptComponent* nsc) { nsc->instance.reset(); };
    }
}

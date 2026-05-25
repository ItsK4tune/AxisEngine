#include "sample_scenario_common.h"

void SampleState::LoadScene13()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    if (auto* io = Resolve<IOHandler>())
    {
        InputSerializer serializer;
        const bool ok = serializer.Deserialize("sample/resource/binding/binding.axs", io->GetInputManager());
        EnsureScenario13AuxBindings(io->GetInputManager());
        m_S13Status = ok ? "Loaded sample/resource/binding/binding.axs. Press mapped controls to light pads and move the capsule."
                          : "Failed to load binding.axs on scene entry.";
    }

    auto player = EntityBuilder(scene, res, "scenario")
        .WithName("BindingPlayer")
        .WithTransform(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f), glm::vec3(2.0f))
        .WithMesh("capsuleModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.5f, 1.0f)
        .Build();

    auto& script = scene.registry.emplace<ScriptComponent>(player);
    script.className = "PlayerControlScript";
    script.InstantiateScript = []() {
        auto playerScript = std::make_unique<PlayerControlScript>();
        playerScript->allowMouseColor = false;
        playerScript->allowKeyboardWhileUI = true;
        return playerScript;
    };
    script.DestroyScript = [](ScriptComponent* nsc) { nsc->instance.reset(); };

    struct InputPad
    {
        const char* action;
        glm::vec3 position;
        glm::vec3 scale;
    };

    const InputPad pads[] = {
        {"PlayerForward", glm::vec3(0.0f, 0.12f, -8.0f), glm::vec3(3.0f, 0.15f, 3.0f)},
        {"PlayerLeft", glm::vec3(-4.0f, 0.12f, -4.0f), glm::vec3(3.0f, 0.15f, 3.0f)},
        {"PlayerBackward", glm::vec3(0.0f, 0.12f, -4.0f), glm::vec3(3.0f, 0.15f, 3.0f)},
        {"PlayerRight", glm::vec3(4.0f, 0.12f, -4.0f), glm::vec3(3.0f, 0.15f, 3.0f)},
        {"PlayerJump", glm::vec3(0.0f, 0.12f, 4.0f), glm::vec3(9.0f, 0.15f, 2.5f)},
    };

    for (const auto& pad : pads)
    {
        auto padEntity = EntityBuilder(scene, res, "scenario")
            .WithName(std::string("InputPad_") + pad.action)
            .WithTransform(pad.position, glm::vec3(0.0f), pad.scale)
            .WithMesh("cubeModel", "deferred_unlit")
            .WithPBRMaterial(0.0f, 0.5f, 1.0f)
            .Build();

        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(padEntity))
            renderer->color = glm::vec4(0.18f, 0.2f, 0.24f, 1.0f);
    }

}

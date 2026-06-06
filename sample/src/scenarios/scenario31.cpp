#include "sample_scenario_common.h"

void SampleState::LoadScene31()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("AudioPlatform")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(60.0f, 1.0f, 60.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.2f, 0.8f, 0.2f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("AudioSource3D")
        .WithTransform(glm::vec3(0.0f, 3.0f, 60.0f), glm::vec3(0.0f), glm::vec3(2.0f))
        .WithPBRMesh("sphereModel", "deferred_lit", 1.0f, 0.5f, 0.0f)
        .WithAudioSource(SamplePath("sample/resource/audio/sample.wav"), true, true, true, m_S31Volume3D, m_S31Pitch, 1.0f, m_S31MinDistance, m_S31MaxDistance)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("Audio2DLoop")
        .WithAudioSource(SamplePath("sample/resource/audio/sample.wav"), true, true, false, m_S31Volume2D, m_S31Pitch)
        .Build();

    m_S31Play2D = false;
    m_S31Play3D = true;
    m_S31Audio2D = nullptr;
    m_S31Audio3D = nullptr;
}

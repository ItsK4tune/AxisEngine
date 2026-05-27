#include "sample_scenario_common.h"

void SampleState::LoadScene17()
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

    auto audioSource = EntityBuilder(scene, res, "scenario")
        .WithName("AudioSource3D")
        .WithTransform(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f), glm::vec3(2.0f))
        .WithPBRMesh("sphereModel", "deferred_lit", 1.0f, 0.5f, 0.0f)
        .Build();

    auto& audio3D = scene.registry.emplace<AudioSourceComponent>(audioSource);
    audio3D.filePath = SamplePath("sample/resource/audio/sample.wav");
    audio3D.playOnAwake = true;
    audio3D.loop = true;
    audio3D.is3D = true;
    audio3D.volume = m_S17Volume3D;
    audio3D.pitch = m_S17Pitch;
    audio3D.speed = 1.0f;
    audio3D.minDistance = m_S17MinDistance;
    audio3D.maxDistance = m_S17MaxDistance;

    auto audio2D = EntityBuilder(scene, res, "scenario")
        .WithName("Audio2DLoop")
        .Build();
    auto& audio2DComp = scene.registry.emplace<AudioSourceComponent>(audio2D);
    audio2DComp.filePath = SamplePath("sample/resource/audio/sample.wav");
    audio2DComp.loop = true;
    audio2DComp.volume = m_S17Volume2D;
    audio2DComp.pitch = m_S17Pitch;

    m_S17Play2D = false;
    m_S17Play3D = true;
    m_S17Audio2D = nullptr;
    m_S17Audio3D = nullptr;
}

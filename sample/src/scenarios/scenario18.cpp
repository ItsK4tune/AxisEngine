#include "sample_scenario_common.h"

void SampleState::LoadScene18()
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

    auto tvScreen = EntityBuilder(scene, res, "scenario")
        .WithName("TVScreen")
        .WithTransform(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f), glm::vec3(16.0f, 9.0f, 0.5f))
        .WithMesh("cubeModel", "videomapShader")
        .WithPBRMaterial(0.0f, 0.4f, 1.0f)
        .Build();
    if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(tvScreen))
    {
        renderer->renderMode = RenderMode::ForceForward;
        renderer->color = glm::vec4(1.0f);
    }

    auto& video = scene.registry.emplace<VideoPlayerComponent>(tvScreen);
    video.filePath = SamplePath("sample/resource/video/sample.mp4");
    video.isLooping = true;
    video.playOnAwake = true;
    video.isPlaying = true;
    video.volume = m_S18Volume;
    video.maxDecodes = 3;

    auto uiVideo = EntityBuilder(scene, res, "scenario")
        .WithName("VideoPreviewUI")
        .WithUITransform(glm::vec2(420.0f, 30.0f), glm::vec2(320.0f, 180.0f), 10)
        .WithUIRenderer("video_preview_ui", glm::vec4(1.0f))
        .Build();
    auto& uiVideoPlayer = scene.registry.emplace<VideoPlayerComponent>(uiVideo);
    uiVideoPlayer.filePath = SamplePath("sample/resource/video/sample.mp4");
    uiVideoPlayer.isLooping = true;
    uiVideoPlayer.playOnAwake = true;
    uiVideoPlayer.isPlaying = true;
    uiVideoPlayer.volume = m_S18Volume;
    uiVideoPlayer.maxDecodes = 3;
}

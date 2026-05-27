#include "sample_scenario_common.h"

void SampleState::LoadScene18()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    auto tvScreen = EntityBuilder(scene, res, "scenario")
        .WithName("TVScreen")
        .WithTransform(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(90.0f, 0.0f, 0.0f), glm::vec3(16.0f, 0.1f, 9.0f))
        .WithPBRMesh("planeModel", "videomapShader", 0.0f, 0.4f, 1.0f)
        .WithPlayingVideo(SamplePath("sample/resource/video/sample.mp4"), true, m_S18Volume, 3)
        .Build();
    if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(tvScreen))
    {
        renderer->renderMode = RenderMode::ForceForward;
        renderer->color = glm::vec4(1.0f);
    }

    EntityBuilder(scene, res, "scenario")
        .WithName("VideoPreviewUI")
        .WithUITransformPercentPosition(glm::vec2(80.0f, 5.0f), glm::vec2(320.0f, 180.0f), 10)
        .WithUIVideo(SamplePath("sample/resource/video/sample.mp4"), "video_preview_ui", glm::vec4(1.0f), true,
                     m_S18Volume, 3)
        .Build();
}

#include "sample_scenario_common.h"

void SampleState::LoadScene32()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    // 1. Spawn ground
    EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(60.0f, 1.0f, 60.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.5f, 1.0f)
        .WithRendererColor(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f))
        .Build();

    // 2. Directional Light
    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    // 3. Target sphere to outline (placed next to portal targets, at (-54, 1.5, -50))
    m_S32OutlineSphere = EntityBuilder(scene, res, "scenario")
        .WithName("OutlineTarget")
        .WithTransform(glm::vec3(-54.0f, 1.5f, -50.0f), glm::vec3(0.0f), glm::vec3(2.0f))
        .WithPBRMesh("sphereModel", "deferred_lit", 0.0f, 0.5f, 1.0f)
        .WithRendererColor(glm::vec4(0.9f, 0.2f, 0.2f, 1.0f))
        .Build();

    // 4. Main Green cube (direct comparison object, placed at (-50, 1.5, -50))
    m_S32PortalTarget = EntityBuilder(scene, res, "scenario")
        .WithName("PortalTargetGreenCube")
        .WithTransform(glm::vec3(-50.0f, 1.5f, -50.0f), glm::vec3(0.0f), glm::vec3(2.0f))
        .WithPBRMesh("cubeModel", "deferred_lit", 0.0f, 0.5f, 1.0f)
        .WithRendererColor(glm::vec4(0.1f, 0.9f, 0.1f, 1.0f))
        .Build();

    // 5. Portal plane (Entrance: at (0, 2.5, 0), scale 10 1 5)
    // Rotate 90 degrees on X to stand vertically, and scale
    m_S32PortalPlane = EntityBuilder(scene, res, "scenario")
        .WithName("PortalPlane")
        .WithTransform(glm::vec3(0.0f, 2.5f, 0.0f), glm::vec3(90.0f, 0.0f, 0.0f), glm::vec3(10.0f, 1.0f, 5.0f))
        .WithPBRMesh("planeModel", "forward_unlit", 0.0f, 0.5f, 1.0f)
        .WithRendererColor(glm::vec4(0.05f, 0.05f, 0.05f, 1.0f))
        .Build();

    // 6. Portal Camera entity - will copy main camera transform relative to portal plane
    m_S32PortalCamera = EntityBuilder(scene, res, "scenario")
        .WithName("PortalCamera")
        .WithTransform(glm::vec3(-50.0f, 2.5f, -50.0f), glm::vec3(0.0f), glm::vec3(1.0f))
        .WithCamera(45.0f, 0.1f, 1000.0f)
        .Build();
    
    // Deactivate virtual camera from main renderer so it doesn't render normally
    auto& cam = scene.GetRegistry().get<CameraComponent>(m_S32PortalCamera);
    cam.isPrimary = false;
    cam.cullingMask = 0; // Don't draw main scene
}

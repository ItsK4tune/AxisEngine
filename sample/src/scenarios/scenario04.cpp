#include "sample_scenario_common.h"

void SampleState::LoadScene4()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    auto lightDirection =
        glm::normalize(glm::vec3(std::cos(glm::radians(m_S4LightYaw)) * std::cos(glm::radians(m_S4LightPitch)),
                                 std::sin(glm::radians(m_S4LightPitch)),
                                 std::sin(glm::radians(m_S4LightYaw)) * std::cos(glm::radians(m_S4LightPitch))));

    m_S4ReceiverEntity =
        EntityBuilder(scene, res, "scenario")
            .WithName("ShadowReceiverFloor")
            .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(m_S4ReceiverSize, 1.0f, m_S4ReceiverSize))
            .WithPBRMesh("planeModel", "deferred_lit_shadow", 0.0f, 0.8f, 1.0f)
            .Build();

    m_S4DeferredCubeEntity =
        EntityBuilder(scene, res, "scenario")
            .WithName("DeferredShadowCasterCube")
            .WithTransform(glm::vec3(-16.0f * m_S4CasterSpread, m_S4CasterHeight, -1.0f),
                           glm::vec3(0.0f, 25.0f, 0.0f), glm::vec3(4.0f, 8.0f, 4.0f) * m_S4CasterScale)
            .WithPBRMesh("cubeModel", "deferred_lit_shadow", 0.1f, 0.5f, 1.0f)
            .Build();
    {
        auto& r = scene.registry.get<MeshRendererComponent>(m_S4DeferredCubeEntity);
        r.receiveShadow = false;
        r.castShadow = true;
        r.color = glm::vec4(0.18f, 0.46f, 1.0f, 1.0f);
    }

    m_S4DeferredSphereEntity = EntityBuilder(scene, res, "scenario")
                                    .WithName("DeferredShadowCasterSphere")
                                    .WithTransform(glm::vec3(-9.0f * m_S4CasterSpread, m_S4CasterHeight, 4.0f),
                                                   glm::vec3(0.0f), glm::vec3(4.0f) * m_S4CasterScale)
                                    .WithPBRMesh("sphereModel", "deferred_lit_shadow", 0.0f, 0.35f, 1.0f)
                                    .Build();
    {
        auto& r = scene.registry.get<MeshRendererComponent>(m_S4DeferredSphereEntity);
        r.receiveShadow = false;
        r.castShadow = true;
        r.color = glm::vec4(0.10f, 0.70f, 1.0f, 1.0f);
    }

    m_S4ForwardCubeEntity =
        EntityBuilder(scene, res, "scenario")
            .WithName("ForwardShadowCasterCube")
            .WithTransform(glm::vec3(10.0f * m_S4CasterSpread, m_S4CasterHeight + 1.0f, -1.0f),
                           glm::vec3(0.0f, -18.0f, 0.0f), glm::vec3(3.5f, 7.0f, 3.5f) * m_S4CasterScale)
            .WithPBRMesh("cubeModel", "forward_pbr_lit_shadow", 0.0f, 0.45f, 1.0f)
            .Build();
    {
        auto& r = scene.registry.get<MeshRendererComponent>(m_S4ForwardCubeEntity);
        r.renderMode = RenderMode::ForceForward;
        r.receiveShadow = false;
        r.castShadow = true;
        r.color = glm::vec4(1.0f, 0.18f, 0.14f, 1.0f);
    }

    m_S4ForwardCapsuleEntity =
        EntityBuilder(scene, res, "scenario")
            .WithName("ForwardShadowCasterCapsule")
            .WithTransform(glm::vec3(18.0f * m_S4CasterSpread, m_S4CasterHeight - 1.0f, 4.0f),
                           glm::vec3(0.0f, 35.0f, 0.0f), glm::vec3(2.8f, 5.2f, 2.8f) * m_S4CasterScale)
            .WithPBRMesh("capsuleSmoothModel", "forward_unlit", 0.0f, 0.5f, 1.0f)
            .Build();
    {
        auto& r = scene.registry.get<MeshRendererComponent>(m_S4ForwardCapsuleEntity);
        r.renderMode = RenderMode::ForceForward;
        r.receiveShadow = false;
        r.castShadow = true;
        r.color = glm::vec4(1.0f, 0.36f, 0.22f, 1.0f);
    }

    m_S4LightEntity =
        EntityBuilder(scene, res, "scenario")
            .WithName("DeferredShadowDirLight")
            .WithTransform(glm::vec3(25.0f, 40.0f, 20.0f), glm::vec3(m_S4LightPitch, m_S4LightYaw, 0.0f))
            .WithDirectionalLight(lightDirection, glm::vec3(1.0f), m_S4LightIntensity)
            .Build();
    if (auto* dir = scene.registry.try_get<DirectionalLightComponent>(m_S4LightEntity))
        dir->isCastShadow = true;
}

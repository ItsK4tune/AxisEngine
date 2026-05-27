#include "sample_scenario_common.h"

void SampleState::LoadScene21()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.4f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("TransparentGround")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(70.0f, 1.0f, 70.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.75f, 1.0f)
        .Build();

    auto mover = EntityBuilder(scene, res, "scenario")
        .WithName("OpaqueMover")
        .WithTransform(glm::vec3(0.0f, 2.0f, -1.5f), glm::vec3(0.0f), glm::vec3(2.5f))
        .WithPBRMesh("sphereModel", "deferred_lit", 0.0f, 0.35f, 1.0f)
        .Build();
    if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(mover))
        renderer->color = glm::vec4(1.0f, 0.72f, 0.1f, 1.0f);

    for (int i = 0; i < 5; ++i)
    {
        float x = -12.0f + i * 6.0f;
        auto glass = EntityBuilder(scene, res, "scenario")
            .WithName("Glass_" + std::to_string(i))
            .WithTransform(glm::vec3(x, 4.0f, 0.0f), glm::vec3(0.0f, i * 14.0f, 0.0f), glm::vec3(3.0f, 7.0f, 0.35f))
            .WithPBRMesh("cubeModel", "forward_transparent", 0.0f, m_S21GlassRoughness, 1.0f)
            .Build();

        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(glass))
        {
            renderer->renderMode = RenderMode::ForceForward;
            renderer->color = glm::vec4(0.2f + 0.12f * i, 0.75f, 1.0f, 1.0f);
            renderer->castShadow = false;
        }
        if (auto* mat = scene.registry.try_get<AxisMaterialComponent>(glass))
        {
            mat->desc.opacity = m_S21GlassOpacity;
            mat->desc.blendSrc = BlendFactor::SrcAlpha;
            mat->desc.blendDst = BlendFactor::OneMinusSrcAlpha;
            mat->gpu.dirty = true;
        }
    }
}

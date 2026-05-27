#include "sample_scenario_common.h"

namespace
{
struct OrderVisual
{
    const char* name;
    int normalOrder;
    int reverseOrder;
};

const OrderVisual kOrderVisuals[] = {
    {"Panel_Red", 1, 3},
    {"Panel_Green", 2, 2},
    {"Panel_Blue", 3, 1},
    {"Solid_Red", 1, 3},
    {"Solid_Green", 2, 2},
    {"Solid_Blue", 3, 1},
};
}  // namespace

void SampleState::ApplyScenario25RenderOrder()
{
    auto& scene = GetScene();
    auto view = scene.registry.view<MeshRendererComponent, InfoComponent>();
    for (auto entity : view)
    {
        auto& info = view.get<InfoComponent>(entity);
        for (const auto& visual : kOrderVisuals)
        {
            if (info.name != visual.name)
                continue;

            auto& renderer = view.get<MeshRendererComponent>(entity);
            renderer.order = m_S25ReverseOrder ? visual.reverseOrder : visual.normalOrder;
        }
    }
}

void SampleState::LoadScene25()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    m_S25ReverseOrder = false;

    EntityBuilder(scene, res, "scenario")
        .WithName("OrderLight")
        .WithTransform(glm::vec3(0.0f, 18.0f, 18.0f), glm::vec3(-45.0f, 0.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(0.0f, -1.0f, -0.4f)), glm::vec3(1.0f), 1.1f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("OrderFloor")
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(60.0f, 1.0f, 60.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.8f, 1.0f)
        .Build();

    struct PanelDef
    {
        const char* name;
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec4 color;
    };
    PanelDef panels[] = {
        {"Panel_Red", glm::vec3(-1.8f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec4(1.0f, 0.1f, 0.1f, 0.50f)},
        {"Panel_Green", glm::vec3(0.0f, 5.4f, 0.0f), glm::vec3(0.0f), glm::vec4(0.1f, 1.0f, 0.1f, 0.50f)},
        {"Panel_Blue", glm::vec3(1.8f, 5.8f, 0.0f), glm::vec3(0.0f), glm::vec4(0.1f, 0.1f, 1.0f, 0.50f)},
    };

    for (const auto& panel : panels)
    {
        auto entity = EntityBuilder(scene, res, "scenario")
            .WithName(panel.name)
            .WithTransform(panel.position, panel.rotation, glm::vec3(8.0f, 5.5f, 0.15f))
            .WithPBRMesh("cubeModel", "forward_transparent", 0.0f, 0.2f, 1.0f)
            .Build();
        auto& renderer = scene.registry.get<MeshRendererComponent>(entity);
        renderer.renderMode = RenderMode::ForceForward;
        renderer.castShadow = false;
        renderer.ignoreDepth = true;
        renderer.color = panel.color;

        auto& mat = scene.registry.get<AxisMaterialComponent>(entity);
        mat.desc.opacity = panel.color.a;
        mat.desc.blendSrc = BlendFactor::SrcAlpha;
        mat.desc.blendDst = BlendFactor::OneMinusSrcAlpha;
        mat.gpu.dirty = true;
    }

    struct SolidDef
    {
        const char* name;
        glm::vec3 position;
        glm::vec4 color;
    };
    SolidDef solids[] = {
        {"Solid_Red", glm::vec3(-1.0f, 2.0f, 22.0f), glm::vec4(1.0f, 0.1f, 0.1f, 1.0f)},
        {"Solid_Green", glm::vec3(0.0f, 2.3f, 18.0f), glm::vec4(0.1f, 1.0f, 0.1f, 1.0f)},
        {"Solid_Blue", glm::vec3(1.0f, 2.6f, 14.0f), glm::vec4(0.1f, 0.1f, 1.0f, 1.0f)},
    };

    for (const auto& solid : solids)
    {
        auto entity = EntityBuilder(scene, res, "scenario")
            .WithName(solid.name)
            .WithTransform(solid.position, glm::vec3(0.0f), glm::vec3(3.5f, 3.5f, 3.5f))
            .WithPBRMesh("cubeModel", "forward_pbr_lit", 0.0f, 0.5f, 1.0f)
            .Build();
        auto& renderer = scene.registry.get<MeshRendererComponent>(entity);
        renderer.renderMode = RenderMode::ForceForward;
        renderer.castShadow = false;
        renderer.receiveShadow = false;
        renderer.ignoreDepth = true;
        renderer.color = solid.color;

        auto& mat = scene.registry.get<AxisMaterialComponent>(entity);
        mat.desc.opacity = 1.0f;
        mat.gpu.dirty = true;
    }

    ApplyScenario25RenderOrder();
}

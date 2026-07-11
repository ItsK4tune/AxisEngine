#include "test_framework.h"
#include "test_support.h"

#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/render_components.h>
#include <mocks/fake_physics.h>
#include <scene/logic/scene_load_finalizer.h>
#include <scene/logic/scene_post_load_fixup.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_validator.h>
#include <scene/type/scene_record.h>
#include <algorithm>
#include <limits>
#include <string>

namespace
{
bool HasIssue(const SceneValidationResult& result, const std::string& code)
{
    return std::any_of(result.issues.begin(), result.issues.end(),
                       [&](const SceneValidationIssue& issue) { return issue.code == code; });
}
}  // namespace

AXIS_TEST_CASE("SceneValidator accepts valid directional point and spot lights")
{
    Scene scene;
    auto directional = scene.CreateEntity("Directional");
    auto point = scene.CreateEntity("Point");
    auto spot = scene.CreateEntity("Spot");
    scene.AddComponent<DirectionalLightComponent>(directional);
    scene.AddComponent<PointLightComponent>(point);
    scene.AddComponent<SpotLightComponent>(spot);

    const auto result = SceneHandlers::SceneValidator::Validate(scene);
    AXIS_CHECK(!result.HasErrors());
}

AXIS_TEST_CASE("SceneValidator reports invalid light values and dependencies")
{
    Scene scene;
    const auto point = scene.GetRegistry().create();
    auto& pointLight = scene.AddComponent<PointLightComponent>(point);
    pointLight.radius = 0.0f;
    pointLight.intensity = -1.0f;

    const auto spot = scene.CreateEntity("Spot");
    scene.GetRegistry().remove<RotationComponent>(spot);
    auto& spotLight = scene.AddComponent<SpotLightComponent>(spot);
    spotLight.cutOff = -0.5f;
    spotLight.outerCutOff = 0.5f;

    const auto result = SceneHandlers::SceneValidator::Validate(scene);
    AXIS_CHECK(HasIssue(result, "LIGHT_MISSING_INFO"));
    AXIS_CHECK(HasIssue(result, "LIGHT_MISSING_POSITION"));
    AXIS_CHECK(HasIssue(result, "LIGHT_INVALID_RADIUS"));
    AXIS_CHECK(HasIssue(result, "LIGHT_NEGATIVE_INTENSITY"));
    AXIS_CHECK(HasIssue(result, "LIGHT_MISSING_ROTATION"));
    AXIS_CHECK(HasIssue(result, "LIGHT_INVALID_SPOT_CUTOFF"));
}

AXIS_TEST_CASE("SceneValidator warns when directional shadow capability is exceeded")
{
    Scene scene;
    for (int i = 0; i < 2; ++i)
    {
        auto entity = scene.CreateEntity("Directional");
        auto& light = scene.AddComponent<DirectionalLightComponent>(entity);
        light.active = true;
        light.isCastShadow = true;
    }

    SceneValidationOptions options;
    options.validateRenderCapabilities = true;
    options.renderCapabilities.singleDirectionalShadow = true;
    const auto result = SceneHandlers::SceneValidator::Validate(scene, options);
    AXIS_CHECK(HasIssue(result, "LIGHT_DIRECTIONAL_SHADOW_LIMIT"));
    AXIS_CHECK(!result.HasErrors());
}

AXIS_TEST_CASE("SceneValidator never mutates light components")
{
    Scene scene;
    auto entity = scene.CreateEntity("Directional");
    auto& light = scene.AddComponent<DirectionalLightComponent>(entity);
    light.active = true;
    light.isCastShadow = false;
    light.intensity = 2.5f;

    const auto before = light;
    (void)SceneHandlers::SceneValidator::Validate(scene);
    const auto& after = scene.GetComponent<DirectionalLightComponent>(entity);
    AXIS_CHECK(after.active == before.active);
    AXIS_CHECK(after.isCastShadow == before.isCastShadow);
    AXIS_CHECK_NEAR(after.intensity, before.intensity, 0.0001f);
}

AXIS_TEST_CASE("SceneValidator detects hierarchy cycles")
{
    Scene scene;
    auto a = scene.CreateEntity("A");
    auto b = scene.CreateEntity("B");
    auto& hierarchyA = scene.GetComponent<HierarchyComponent>(a);
    auto& hierarchyB = scene.GetComponent<HierarchyComponent>(b);
    hierarchyA.parent = b;
    hierarchyA.children.push_back(b);
    hierarchyB.parent = a;
    hierarchyB.children.push_back(a);

    const auto result = SceneHandlers::SceneValidator::Validate(scene);
    AXIS_CHECK(HasIssue(result, "HIERARCHY_CYCLE"));
    AXIS_CHECK(result.HasFatalErrors());
}

AXIS_TEST_CASE("SceneValidator reports renderable scene without camera without creating one")
{
    Scene scene;
    auto entity = scene.CreateEntity("Renderable");
    scene.AddComponent<MeshRendererComponent>(entity);

    const size_t entityCount = scene.GetEntityCount();
    const auto result = SceneHandlers::SceneValidator::Validate(scene);
    AXIS_CHECK(HasIssue(result, "CAMERA_MISSING_RENDERABLE_SCENE"));
    AXIS_CHECK(scene.GetEntityCount() == entityCount);
    AXIS_CHECK(scene.GetActiveCamera() == entt::null);
}

AXIS_TEST_CASE("ScenePostLoadFixup physics binding respects loaded entity scope")
{
    Scene scene;
    axis_test_mocks::FakePhysicsWorld physics;
    auto loaded = scene.CreateEntity("LoadedBody");
    auto existing = scene.CreateEntity("ExistingBody");
    scene.AddComponent<RigidShapeComponent>(loaded);
    scene.AddComponent<RigidShapeComponent>(existing);
    auto& loadedBody = scene.AddComponent<RigidBodyComponent>(loaded);
    auto& existingBody = scene.AddComponent<RigidBodyComponent>(existing);
    loadedBody.body = std::make_shared<axis_test_mocks::FakeRigidBody>();
    existingBody.body = std::make_shared<axis_test_mocks::FakeRigidBody>();

    SceneHandlers::ScenePostLoadFixup::InitializePhysicsBindings(scene, &physics, {loaded});

    AXIS_CHECK(physics.syncedRigidBodies.size() == 1);
    AXIS_CHECK(physics.syncedRigidBodies.front() == loadedBody.body.get());
}

AXIS_TEST_CASE("SceneLoadFinalizer reports and creates fallback camera explicitly")
{
    axis_test_support::ResetServices();
    Scene scene;
    auto renderable = scene.CreateEntity("Renderable");
    scene.AddComponent<MeshRendererComponent>(renderable);
    SceneLoadResult loadResult;
    loadResult.entities.push_back(renderable);

    const bool finalized = SceneHandlers::SceneLoadFinalizer::Finalize(scene, loadResult, nullptr);

    AXIS_CHECK(finalized);
    AXIS_CHECK(loadResult.usedFallbackCamera);
    AXIS_CHECK(HasIssue(loadResult.validation, "CAMERA_MISSING_RENDERABLE_SCENE"));
    AXIS_CHECK(scene.GetActiveCamera() != entt::null);
}

AXIS_TEST_CASE("SceneLoadFinalizer rolls back entities on fatal validation")
{
    Scene scene;
    auto a = scene.CreateEntity("A");
    auto b = scene.CreateEntity("B");
    auto& hierarchyA = scene.GetComponent<HierarchyComponent>(a);
    auto& hierarchyB = scene.GetComponent<HierarchyComponent>(b);
    hierarchyA.parent = b;
    hierarchyA.children.push_back(b);
    hierarchyB.parent = a;
    hierarchyB.children.push_back(a);

    SceneLoadResult loadResult;
    loadResult.entities = {a, b};
    SceneHandlers::SceneLoadFinalizeOptions options;
    options.ensureFallbackCamera = false;

    AXIS_EXPECT_ERROR_LOGS(2);
    const bool finalized = SceneHandlers::SceneLoadFinalizer::Finalize(scene, loadResult, nullptr, {}, options);

    AXIS_CHECK(!finalized);
    AXIS_CHECK(loadResult.validation.HasFatalErrors());
    AXIS_CHECK(loadResult.entities.empty());
    AXIS_CHECK(!scene.IsValid(a));
    AXIS_CHECK(!scene.IsValid(b));
}

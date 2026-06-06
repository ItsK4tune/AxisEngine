#include "test_framework.h"
#include "test_support.h"

#include <core/logic/config_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_serializer.h>
#include <scene/logic/scene_validator.h>
#include <fstream>
#include <sstream>

namespace
{
std::string ReadText(const std::filesystem::path& path)
{
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void InitializeConfigManager(ConfigManager& configManager)
{
    AppConfig config;
    config.headlessMode = false;
    configManager.Initialize(config);
}
}  // namespace

AXIS_TEST_CASE("SceneSerializer deserializes minimal entity")
{
    axis_test_support::HeadlessResourceFixture fixture;
    Scene scene;
    auto path = axis_test_support::WriteTempFile(
        "ss_minimal.axs",
        "axis_scene:\n"
        "  Entities:\n"
        "    Player:\n"
        "      Tag: player\n"
        "      Component: Transform\n"
        "        Position: 1 2 3\n"
        "        Rotation: 0 90 0\n"
        "        Scale: 2 2 2\n");

    auto result = SceneSerializer::Deserialize(path.string(), scene, fixture.resources, nullptr, nullptr);
    auto player = scene.FindByName("Player");

    AXIS_CHECK(result.entities.size() == 1);
    AXIS_CHECK(player != entt::null);
    AXIS_CHECK(scene.HasAllComponents<InfoComponent>(player));
    AXIS_CHECK(scene.HasAllComponents<PositionComponent>(player));
    AXIS_CHECK(scene.HasAllComponents<RotationComponent>(player));
    AXIS_CHECK(scene.HasAllComponents<ScaleComponent>(player));
    AXIS_CHECK(scene.GetComponent<InfoComponent>(player).tag == "player");
    AXIS_CHECK_NEAR(scene.GetComponent<PositionComponent>(player).value.x, 1.0f, 0.0001f);
    AXIS_CHECK_NEAR(scene.GetComponent<PositionComponent>(player).value.y, 2.0f, 0.0001f);
    AXIS_CHECK_NEAR(scene.GetComponent<PositionComponent>(player).value.z, 3.0f, 0.0001f);
}

AXIS_TEST_CASE("SceneSerializer resolves deferred parent by name")
{
    axis_test_support::HeadlessResourceFixture fixture;
    Scene scene;
    auto path = axis_test_support::WriteTempFile(
        "ss_parent.axs",
        "axis_scene:\n"
        "  Entities:\n"
        "    Root:\n"
        "      Component: Transform\n"
        "    Child:\n"
        "      Parent: Root\n"
        "      Component: Transform\n");

    auto result = SceneSerializer::Deserialize(path.string(), scene, fixture.resources, nullptr, nullptr);
    auto root = scene.FindByName("Root");
    auto child = scene.FindByName("Child");

    AXIS_CHECK(result.entities.size() == 2);
    AXIS_CHECK(root != entt::null);
    AXIS_CHECK(child != entt::null);
    AXIS_CHECK(scene.GetComponent<HierarchyComponent>(child).parent == root);
    AXIS_CHECK(scene.GetComponent<HierarchyComponent>(root).children.size() == 1);
    AXIS_CHECK(scene.GetComponent<HierarchyComponent>(root).children[0] == child);
}

AXIS_TEST_CASE("SceneSerializer applies Config block")
{
    axis_test_support::HeadlessResourceFixture fixture;
    Scene scene;
    ConfigManager configManager;
    InitializeConfigManager(configManager);
    ServiceLocator::Instance().Register<ConfigManager>(&configManager);
    auto path = axis_test_support::WriteTempFile(
        "ss_config.axs",
        "axis_scene:\n"
        "  Config:\n"
        "    WINDOW_WIDTH: 1280\n"
        "    GRAVITY: 0 -12 0\n"
        "    PHYSICS_TICKRATE: 120\n"
        "  Entities:\n"
        "    ConfigEntity:\n"
        "      Component: Transform\n");

    auto result = SceneSerializer::Deserialize(path.string(), scene, fixture.resources, nullptr, nullptr);

    AXIS_CHECK(result.hasConfig);
    AXIS_CHECK(result.appliedConfig.window.width == 1280);
    AXIS_CHECK_NEAR(result.appliedConfig.physics.gravity[1], -12.0f, 0.0001f);
    AXIS_CHECK_NEAR(result.appliedConfig.physics.physicsTickRate, 120.0f, 0.0001f);
    AXIS_CHECK(configManager.GetConfig().window.width == 1280);
}

AXIS_TEST_CASE("SceneValidator creates fallback camera for renderable scene")
{
    axis_test_support::ResetServices();
    Scene scene;
    auto entity = scene.CreateEntity("Renderable");
    scene.AddComponent<MeshRendererComponent>(entity);

    SceneHandlers::SceneValidator::ValidateCamera(scene);

    auto camera = scene.GetActiveCamera();
    AXIS_CHECK(camera != entt::null);
    AXIS_CHECK(scene.GetComponent<InfoComponent>(camera).name == "Default Spectator Camera");
    AXIS_CHECK(scene.GetComponent<CameraComponent>(camera).isPrimary);
    AXIS_CHECK_NEAR(scene.GetComponent<CameraComponent>(camera).aspectRatio, 16.0f / 9.0f, 0.0001f);
}

AXIS_TEST_CASE("SceneSerializer serializes sceneName filter")
{
    axis_test_support::HeadlessResourceFixture fixture;
    Scene scene;
    ConfigManager configManager;
    InitializeConfigManager(configManager);
    ServiceLocator::Instance().Register<ConfigManager>(&configManager);

    auto entityA = scene.CreateEntity("EntityA");
    auto entityB = scene.CreateEntity("EntityB");
    scene.GetComponent<InfoComponent>(entityA).sceneName = "A";
    scene.GetComponent<InfoComponent>(entityB).sceneName = "B";

    auto output = axis_test_support::TempPath("ss_filter_output.axs");
    AXIS_CHECK(SceneSerializer::Serialize(output.string(), scene, fixture.resources, "A"));
    const std::string text = ReadText(output);

    AXIS_CHECK(text.find("EntityA:") != std::string::npos);
    AXIS_CHECK(text.find("EntityB:") == std::string::npos);
}

AXIS_TEST_CASE("SceneSerializer deduplicates resources owned by another scene")
{
    axis_test_support::SceneServiceFixture fixture;
    ConfigManager configManager;
    InitializeConfigManager(configManager);
    SceneManager sceneManager;
    ServiceLocator::Instance().Register<ConfigManager>(&configManager);
    ServiceLocator::Instance().Register<SceneManager>(&sceneManager);

    auto ownerPath = axis_test_support::WriteTempFile(
        "ss_resource_owner.axs",
        "axis_scene:\n"
        "  Resources:\n"
        "    Texture:\n"
        "      Name: shared\n"
        "      Path: shared.png\n"
        "  Entities:\n"
        "    Owner:\n"
        "      Component: Transform\n");
    sceneManager.LoadScene(ownerPath.string(), true);
    fixture.resources.AddResourceDefinition("Texture", "shared", {{"Path", "shared.png"}});

    auto target = fixture.scene.CreateEntity("Target");
    auto& info = fixture.scene.GetComponent<InfoComponent>(target);
    info.sceneName = "target_scene";
    auto& material = fixture.scene.AddComponent<MaterialComponent>(target);
    material.desc.albedoPath = "shared.png";

    auto output = axis_test_support::TempPath("ss_resource_dedup_output.axs");
    AXIS_CHECK(SceneSerializer::Serialize(output.string(), fixture.scene, fixture.resources, "target_scene"));
    const std::string text = ReadText(output);

    AXIS_CHECK(text.find("Name: shared") == std::string::npos);
    AXIS_CHECK(text.find("Target:") != std::string::npos);
}

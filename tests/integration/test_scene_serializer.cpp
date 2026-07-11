#include "test_framework.h"
#include "test_support.h"

#include <core/logic/config_manager.h>
#include <core/logic/config_serializer.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_post_load_fixup.h>
#include <scene/logic/scene_serializer.h>
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

    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    SceneLoadResult result;
    AXIS_CHECK(serializer.Deserialize(path.string(), scene, result));
    AXIS_CHECK(!result.validation.HasErrors());
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

    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    SceneLoadResult result;
    AXIS_CHECK(serializer.Deserialize(path.string(), scene, result));
    AXIS_CHECK(!result.validation.HasErrors());
    auto root = scene.FindByName("Root");
    auto child = scene.FindByName("Child");

    AXIS_CHECK(result.entities.size() == 2);
    AXIS_CHECK(root != entt::null);
    AXIS_CHECK(child != entt::null);
    AXIS_CHECK(scene.GetComponent<HierarchyComponent>(child).parent == root);
    AXIS_CHECK(scene.GetComponent<HierarchyComponent>(root).children.size() == 1);
    AXIS_CHECK(scene.GetComponent<HierarchyComponent>(root).children[0] == child);
}



AXIS_TEST_CASE("SceneValidator creates fallback camera for renderable scene")
{
    axis_test_support::ResetServices();
    Scene scene;
    auto entity = scene.CreateEntity("Renderable");
    scene.AddComponent<MeshRendererComponent>(entity);

    const bool created = SceneHandlers::ScenePostLoadFixup::EnsureFallbackCamera(scene);

    auto camera = scene.GetActiveCamera();
    AXIS_CHECK(created);
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
    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    AXIS_CHECK(serializer.Serialize(output.string(), scene, "A"));
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
    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    AXIS_CHECK(serializer.Serialize(output.string(), fixture.scene, "target_scene"));
    const std::string text = ReadText(output);

    AXIS_CHECK(text.find("Name: shared") == std::string::npos);
    AXIS_CHECK(text.find("Target:") != std::string::npos);
}

AXIS_TEST_CASE("SceneSerializer skips transient entities")
{
    axis_test_support::HeadlessResourceFixture fixture;
    Scene scene;
    ConfigManager configManager;
    InitializeConfigManager(configManager);
    ServiceLocator::Instance().Register<ConfigManager>(&configManager);

    auto entityA = scene.CreateEntity("PersistentEntity");
    auto entityB = scene.CreateEntity("TransientEntity");
    scene.GetComponent<InfoComponent>(entityA).sceneName = "A";
    scene.GetComponent<InfoComponent>(entityB).sceneName = "A";
    scene.GetComponent<InfoComponent>(entityB).isTransient = true;

    auto output = axis_test_support::TempPath("ss_transient_output.axs");
    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    AXIS_CHECK(serializer.Serialize(output.string(), scene, "A"));
    const std::string text = ReadText(output);

    AXIS_CHECK(text.find("PersistentEntity:") != std::string::npos);
    AXIS_CHECK(text.find("TransientEntity:") == std::string::npos);
}

AXIS_TEST_CASE("ConfigSerializer serializes and deserializes config")
{
    axis_test_support::ResetServices();
    AppConfig config;
    config.window.width = 1024;
    config.window.height = 768;
    config.physics.gravity[1] = -9.81f;

    auto path = axis_test_support::TempPath("test_config.axs");
    ConfigSerializer serializer;
    AXIS_CHECK(serializer.Serialize(path.string(), config));

    AppConfig loadedConfig;
    AXIS_CHECK(serializer.Deserialize(path.string(), loadedConfig));
    AXIS_CHECK(loadedConfig.window.width == 1024);
    AXIS_CHECK(loadedConfig.window.height == 768);
    AXIS_CHECK_NEAR(loadedConfig.physics.gravity[1], -9.81f, 0.0001f);
}

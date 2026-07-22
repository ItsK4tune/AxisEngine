#include "test_framework.h"
#include "test_support.h"

#include <core/logic/config_manager.h>
#include <core/logic/config_serializer.h>
#include <core/logic/loader_utils.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/network_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/script_component.h>
#include <navigation/unit/navmesh_component.h>
#include <ecs/unit/physics_components.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/component_loader.h>
#include <scene/logic/scene_post_load_fixup.h>
#include <scene/logic/scene_serializer.h>
#include <fstream>
#include <sstream>

namespace
{
struct CustomHealthComponent
{
    int value = 100;
};

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
    auto path = axis_test_support::WriteTempFile("ss_minimal.axs",
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
    auto path = axis_test_support::WriteTempFile("ss_parent.axs",
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

    auto ownerPath = axis_test_support::WriteTempFile("ss_resource_owner.axs",
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

AXIS_TEST_CASE("SceneSerializer supports module-owned component codecs in memory")
{
    axis_test_support::HeadlessResourceFixture fixture;
    ComponentLoader::RegisterLoader(
        "CustomHealth", [](Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager&, IPhysicsWorld*) {
            scene.AddComponent<CustomHealthComponent>(
                entity, CustomHealthComponent{LoaderUtils::SafeStoi(node.GetChildValue("Value", "100"))});
        });
    ComponentLoader::RegisterSerializer("CustomHealth",
                                        [](const entt::registry& registry, entt::entity entity, YAMLNode& output) {
                                            const auto* health = registry.try_get<CustomHealthComponent>(entity);
                                            if (!health)
                                                return false;
                                            output.children.push_back({"Value", std::to_string(health->value), {}});
                                            return true;
                                        });

    Scene source;
    const auto entity = source.CreateEntity("CustomEntity");
    source.AddComponent<CustomHealthComponent>(entity, CustomHealthComponent{275});
    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    const std::string content = serializer.SerializeToString(source);
    AXIS_CHECK(content.find("Component: CustomHealth") != std::string::npos);

    Scene loaded;
    SceneLoadResult result;
    AXIS_CHECK(serializer.DeserializeFromString(content, "custom_codec", loaded, result));
    const auto loadedEntity = loaded.FindByName("CustomEntity");
    AXIS_CHECK(loadedEntity != entt::null);
    AXIS_CHECK(loaded.GetComponent<CustomHealthComponent>(loadedEntity).value == 275);

    ComponentLoader::UnregisterSerializer("CustomHealth");
    ComponentLoader::UnregisterLoader("CustomHealth");
}

AXIS_TEST_CASE("SceneSerializer preserves NavMesh and NavigationGrid provider data")
{
    axis_test_support::HeadlessResourceFixture fixture;
    Scene source;

    const auto navEntity = source.CreateEntity("NavProvider");
    auto& nav = source.AddComponent<NavMeshComponent>(navEntity);
    nav.isDynamic = true;
    nav.needsRebuild = false;
    nav.terrainGridResolution = 32;
    nav.walkableNormalY = 0.55f;
    nav.carveHeightPadding = 0.75f;
    nav.carveAgentRadius = 0.4f;
    nav.vertices = {{0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 2.0f}};
    nav.triangles.push_back({{0, 1, 2}, {0.666f, 0.0f, 0.666f}, {0.0f, 1.0f, 0.0f}, "floor"});
    nav.nodes.push_back({{0.666f, 0.0f, 0.666f}, {}, 0, "floor"});

    const auto gridEntity = source.CreateEntity("GridProvider");
    auto& grid = source.AddComponent<NavigationGridComponent>(gridEntity);
    grid.origin = {-2.0f, 0.0f, -2.0f};
    grid.width = 2;
    grid.height = 2;
    grid.cellSize = 1.5f;
    grid.allowDiagonal = true;
    grid.cells = {{true, 1.0f, "road"}, {false, 5.0f, "wall"}, {true, 2.0f, "mud"},
                  {true, 1.0f, "road"}};

    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    const std::string text = serializer.SerializeToString(source);
    AXIS_CHECK(text.find("Component: NavMesh") != std::string::npos);
    AXIS_CHECK(text.find("Component: NavigationGrid") != std::string::npos);

    Scene loaded;
    SceneLoadResult result;
    AXIS_CHECK(serializer.DeserializeFromString(text, "navigation_roundtrip", loaded, result));
    const auto loadedNavEntity = loaded.FindByName("NavProvider");
    const auto loadedGridEntity = loaded.FindByName("GridProvider");
    AXIS_CHECK(loadedNavEntity != entt::null);
    AXIS_CHECK(loadedGridEntity != entt::null);
    const auto& loadedNav = loaded.GetComponent<NavMeshComponent>(loadedNavEntity);
    const auto& loadedGrid = loaded.GetComponent<NavigationGridComponent>(loadedGridEntity);
    AXIS_CHECK(loadedNav.vertices.size() == 3);
    AXIS_CHECK(loadedNav.triangles.size() == 1);
    AXIS_CHECK(loadedNav.nodes.size() == 1);
    AXIS_CHECK(loadedNav.triangles[0].tag == "floor");
    AXIS_CHECK_NEAR(loadedNav.walkableNormalY, 0.55f, 0.0001f);
    AXIS_CHECK(loadedGrid.IsValid());
    AXIS_CHECK(loadedGrid.allowDiagonal);
    AXIS_CHECK(loadedGrid.cells[1].tag == "wall");
    AXIS_CHECK(!loadedGrid.cells[1].walkable);
}

AXIS_TEST_CASE("SceneSerializer keeps entities that only contain a custom serializable component")
{
    axis_test_support::HeadlessResourceFixture fixture;
    ComponentLoader::RegisterLoader(
        "CustomHealthOnly", [](Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager&,
                                IPhysicsWorld*) {
            scene.AddComponent<CustomHealthComponent>(
                entity, CustomHealthComponent{LoaderUtils::SafeStoi(node.GetChildValue("Value", "100"))});
        });
    ComponentLoader::RegisterSerializer("CustomHealthOnly",
                                        [](const entt::registry& registry, entt::entity entity, YAMLNode& output) {
                                            const auto* health = registry.try_get<CustomHealthComponent>(entity);
                                            if (!health)
                                                return false;
                                            output.children.push_back({"Value", std::to_string(health->value), {}});
                                            return true;
                                        });

    Scene source;
    const entt::entity entity = source.GetRegistry().create();
    source.AddComponent<InfoComponent>(entity, "CodecOnly", "default");
    source.AddComponent<CustomHealthComponent>(entity, CustomHealthComponent{901});
    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    const std::string content = serializer.SerializeToString(source);
    AXIS_CHECK(content.find("CodecOnly:") != std::string::npos);
    AXIS_CHECK(content.find("Component: CustomHealthOnly") != std::string::npos);

    ComponentLoader::UnregisterSerializer("CustomHealthOnly");
    ComponentLoader::UnregisterLoader("CustomHealthOnly");
}

AXIS_TEST_CASE("ConfigSerializer serializes and deserializes config")
{
    axis_test_support::ResetServices();
    AppConfig config;
    config.window.width = 1024;
    config.window.height = 768;
    config.physics.gravity[1] = -9.81f;
    config.render.taaFeedback = 0.82f;
    config.audio.captureInputVolume = 1.75f;
    config.audio.captureNoiseGate = 0.06f;
    config.audio.captureAttackSeconds = 0.025f;
    config.audio.captureReleaseSeconds = 0.2f;
    config.audio.capturePeakDecaySeconds = 0.4f;
    config.title.clear();
    config.iconPath.clear();
    config.defaultAssetManifest.clear();
    config.audio.audioDevice.clear();
    config.audio.captureDevice.clear();
    config.culling.stencilTestEnabled = false;
    config.culling.spatialCullingMode = SpatialCullingMode::Octree;
    config.optimization.resourceUploadBudgetEnabled = false;
    config.optimization.resourceHotReloadEnabled = true;
    config.optimization.maxModelUploadsPerFrame = 7;
    config.optimization.maxTextureUploadsPerFrame = 11;
    config.optimization.discardCpuMeshDataAfterUpload = true;
    config.optimization.compressedTextureLoadingEnabled = false;
    config.optimization.streamingUpdateThrottlingEnabled = false;
    config.optimization.streamingCheckIntervalSeconds = 0.25f;
    config.optimization.reflectionCaptureBudgetEnabled = false;
    config.optimization.maxReflectionProbeFacesPerFrame = 5;
    config.optimization.maxPlanarReflectionCapturesPerFrame = 3;
    config.optimization.shadowParallelBuildEnabled = false;
    config.optimization.shadowParallelThreshold = 33;
    config.optimization.animationParallelEvaluationEnabled = false;
    config.optimization.animationParallelThreshold = 17;
    config.optimization.navigationSpatialHashEnabled = false;
    config.optimization.navigationAgentCellSize = 3.5f;
    config.optimization.navigationAsyncPathfindingEnabled = false;
    config.optimization.navigationMaxPathRequestsPerFrame = 13;
    config.optimization.navMeshRebuildBudgetEnabled = false;
    config.optimization.maxNavMeshRebuildsPerFrame = 7;
    config.optimization.navigationDirtyTilesEnabled = false;
    config.optimization.navigationNavMeshTileSize = 12.5f;
    config.optimization.navigationMaxDirtyTilesPerFrame = 9;
    config.optimization.networkBatchingEnabled = false;
    config.optimization.networkMaxEventsPerUpdate = 123;
    config.optimization.networkMaxEventProcessingMs = 4.5f;
    config.optimization.networkMaxBytesPerUpdate = 654321;
    config.optimization.networkReplicationEnabled = false;
    config.optimization.networkReplicationRateHz = 12.5f;
    config.optimization.networkInterestRadius = 88.0f;
    config.optimization.particleSpawnBudgetEnabled = false;
    config.optimization.particleMaxSpawnPerFrame = 321;
    config.optimization.particleBatchingEnabled = false;
    config.optimization.renderStateCacheEnabled = false;
    config.optimization.persistentMappedBuffersEnabled = false;
    config.optimization.tiledLightCullingEnabled = false;
    config.optimization.tiledLightTileSize = 24;
    config.optimization.gbufferEntityIdEnabled = true;
    config.optimization.physicsMeshShapeCacheEnabled = false;
    config.optimization.uiLayoutCacheEnabled = false;
    config.optimization.videoAsyncDecodeEnabled = false;
    config.optimization.videoDecodeQueueSize = 7;
    config.optimization.videoAVSyncThresholdSeconds = 0.35f;
    config.optimization.videoLoadRetrySeconds = 2.5f;
    config.input.gamepadDeadZone = 0.22f;

    auto path = axis_test_support::TempPath("test_config.axs");
    ConfigSerializer serializer;
    AXIS_CHECK(serializer.Serialize(path.string(), config));

    AppConfig loadedConfig;
    loadedConfig.iconPath = "stale-icon";
    loadedConfig.title = "stale-title";
    loadedConfig.defaultAssetManifest = "stale-manifest";
    loadedConfig.audio.audioDevice = "stale-output";
    loadedConfig.audio.captureDevice = "stale-input";
    AXIS_CHECK(serializer.Deserialize(path.string(), loadedConfig));
    AXIS_CHECK(loadedConfig.window.width == 1024);
    AXIS_CHECK(loadedConfig.window.height == 768);
    AXIS_CHECK_NEAR(loadedConfig.physics.gravity[1], -9.81f, 0.0001f);
    AXIS_CHECK_NEAR(loadedConfig.render.taaFeedback, 0.82f, 0.0001f);
    AXIS_CHECK_NEAR(loadedConfig.audio.captureInputVolume, 1.75f, 0.0001f);
    AXIS_CHECK_NEAR(loadedConfig.audio.captureNoiseGate, 0.06f, 0.0001f);
    AXIS_CHECK_NEAR(loadedConfig.audio.captureAttackSeconds, 0.025f, 0.0001f);
    AXIS_CHECK_NEAR(loadedConfig.audio.captureReleaseSeconds, 0.2f, 0.0001f);
    AXIS_CHECK_NEAR(loadedConfig.audio.capturePeakDecaySeconds, 0.4f, 0.0001f);
    AXIS_CHECK(loadedConfig.title.empty());
    AXIS_CHECK(loadedConfig.iconPath.empty());
    AXIS_CHECK(loadedConfig.defaultAssetManifest.empty());
    AXIS_CHECK(loadedConfig.audio.audioDevice.empty());
    AXIS_CHECK(loadedConfig.audio.captureDevice.empty());
    AXIS_CHECK(!loadedConfig.culling.stencilTestEnabled);
    AXIS_CHECK(loadedConfig.culling.spatialCullingMode == SpatialCullingMode::Octree);
    AXIS_CHECK(!loadedConfig.optimization.resourceUploadBudgetEnabled);
    AXIS_CHECK(loadedConfig.optimization.resourceHotReloadEnabled);
    AXIS_CHECK(loadedConfig.optimization.maxModelUploadsPerFrame == 7);
    AXIS_CHECK(loadedConfig.optimization.maxTextureUploadsPerFrame == 11);
    AXIS_CHECK(loadedConfig.optimization.discardCpuMeshDataAfterUpload);
    AXIS_CHECK(!loadedConfig.optimization.compressedTextureLoadingEnabled);
    AXIS_CHECK(!loadedConfig.optimization.streamingUpdateThrottlingEnabled);
    AXIS_CHECK_NEAR(loadedConfig.optimization.streamingCheckIntervalSeconds, 0.25f, 0.0001f);
    AXIS_CHECK(!loadedConfig.optimization.reflectionCaptureBudgetEnabled);
    AXIS_CHECK(loadedConfig.optimization.maxReflectionProbeFacesPerFrame == 5);
    AXIS_CHECK(loadedConfig.optimization.maxPlanarReflectionCapturesPerFrame == 3);
    AXIS_CHECK(!loadedConfig.optimization.shadowParallelBuildEnabled);
    AXIS_CHECK(loadedConfig.optimization.shadowParallelThreshold == 33);
    AXIS_CHECK(!loadedConfig.optimization.animationParallelEvaluationEnabled);
    AXIS_CHECK(loadedConfig.optimization.animationParallelThreshold == 17);
    AXIS_CHECK(!loadedConfig.optimization.navigationSpatialHashEnabled);
    AXIS_CHECK_NEAR(loadedConfig.optimization.navigationAgentCellSize, 3.5f, 0.0001f);
    AXIS_CHECK(!loadedConfig.optimization.navigationAsyncPathfindingEnabled);
    AXIS_CHECK(loadedConfig.optimization.navigationMaxPathRequestsPerFrame == 13);
    AXIS_CHECK(!loadedConfig.optimization.navMeshRebuildBudgetEnabled);
    AXIS_CHECK(loadedConfig.optimization.maxNavMeshRebuildsPerFrame == 7);
    AXIS_CHECK(!loadedConfig.optimization.navigationDirtyTilesEnabled);
    AXIS_CHECK_NEAR(loadedConfig.optimization.navigationNavMeshTileSize, 12.5f, 0.0001f);
    AXIS_CHECK(loadedConfig.optimization.navigationMaxDirtyTilesPerFrame == 9);
    AXIS_CHECK(!loadedConfig.optimization.networkBatchingEnabled);
    AXIS_CHECK(loadedConfig.optimization.networkMaxEventsPerUpdate == 123);
    AXIS_CHECK_NEAR(loadedConfig.optimization.networkMaxEventProcessingMs, 4.5f, 0.0001f);
    AXIS_CHECK(loadedConfig.optimization.networkMaxBytesPerUpdate == 654321);
    AXIS_CHECK(!loadedConfig.optimization.networkReplicationEnabled);
    AXIS_CHECK_NEAR(loadedConfig.optimization.networkReplicationRateHz, 12.5f, 0.0001f);
    AXIS_CHECK_NEAR(loadedConfig.optimization.networkInterestRadius, 88.0f, 0.0001f);
    AXIS_CHECK(!loadedConfig.optimization.particleSpawnBudgetEnabled);
    AXIS_CHECK(loadedConfig.optimization.particleMaxSpawnPerFrame == 321);
    AXIS_CHECK(!loadedConfig.optimization.particleBatchingEnabled);
    AXIS_CHECK(!loadedConfig.optimization.renderStateCacheEnabled);
    AXIS_CHECK(!loadedConfig.optimization.persistentMappedBuffersEnabled);
    AXIS_CHECK(!loadedConfig.optimization.tiledLightCullingEnabled);
    AXIS_CHECK(loadedConfig.optimization.tiledLightTileSize == 24);
    AXIS_CHECK(loadedConfig.optimization.gbufferEntityIdEnabled);
    AXIS_CHECK(!loadedConfig.optimization.physicsMeshShapeCacheEnabled);
    AXIS_CHECK(!loadedConfig.optimization.uiLayoutCacheEnabled);
    AXIS_CHECK(!loadedConfig.optimization.videoAsyncDecodeEnabled);
    AXIS_CHECK(loadedConfig.optimization.videoDecodeQueueSize == 7);
    AXIS_CHECK_NEAR(loadedConfig.optimization.videoAVSyncThresholdSeconds, 0.35f, 0.0001f);
    AXIS_CHECK_NEAR(loadedConfig.optimization.videoLoadRetrySeconds, 2.5f, 0.0001f);
    AXIS_CHECK_NEAR(loadedConfig.input.gamepadDeadZone, 0.22f, 0.0001f);
}

AXIS_TEST_CASE("SceneSerializer round trips script and particle emission shape")
{
    axis_test_support::HeadlessResourceFixture fixture;
    Scene source;
    const entt::entity entity = source.GetRegistry().create();
    source.AddComponent<InfoComponent>(entity, "SerializedBehavior", "default");
    auto& script = source.AddComponent<ScriptComponent>(entity);
    script.className = "RoundTripScript";
    auto& particles = source.AddComponent<ParticleEmitterComponent>(entity);
    particles.emitter.Initialize(32);
    particles.maxParticles = 32;
    particles.emitter.Shape = ParticleEmitter::EmissionShape::CONE;

    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    const std::string text = serializer.SerializeToString(source);
    AXIS_CHECK(text.find("Component: Script") != std::string::npos);
    AXIS_CHECK(text.find("Shape: CONE") != std::string::npos);

    Scene loaded;
    SceneLoadResult result;
    AXIS_CHECK(serializer.DeserializeFromString(text, "component_roundtrip", loaded, result));
    const entt::entity loadedEntity = loaded.FindByName("SerializedBehavior");
    AXIS_CHECK(loadedEntity != entt::null);
    AXIS_CHECK(loaded.GetComponent<ScriptComponent>(loadedEntity).className == "RoundTripScript");
    AXIS_CHECK(loaded.GetComponent<ParticleEmitterComponent>(loadedEntity).emitter.Shape ==
               ParticleEmitter::EmissionShape::CONE);
}

AXIS_TEST_CASE("SceneSerializer round trips heightfield collision data")
{
    axis_test_support::HeadlessResourceFixture fixture;
    Scene source;
    ConfigManager configManager;
    InitializeConfigManager(configManager);
    ServiceLocator::Instance().Register<ConfigManager>(&configManager);

    auto entity = source.CreateEntity("HeightfieldBody", "terrain");
    source.GetComponent<InfoComponent>(entity).sceneName = "physics";
    auto& shape = source.AddComponent<RigidShapeComponent>(entity);
    shape.type = ShapeType::Heightfield;
    shape.heightfieldWidth = 2;
    shape.heightfieldLength = 2;
    shape.minHeight = -1.0f;
    shape.maxHeight = 2.0f;
    shape.heightfieldScale = {2.0f, 0.5f, 3.0f};
    shape.heightSamples = {-1.0f, 0.0f, 1.0f, 2.0f};

    const auto path = axis_test_support::TempPath("heightfield_roundtrip.axs");
    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    AXIS_CHECK(serializer.Serialize(path.string(), source, "physics"));

    Scene loaded;
    AXIS_CHECK(serializer.Deserialize(path.string(), loaded));
    const auto view = loaded.View<RigidShapeComponent>();
    AXIS_CHECK(view.size() == 1);
    const auto& restored = view.get<RigidShapeComponent>(*view.begin());
    AXIS_CHECK(restored.type == ShapeType::Heightfield);
    AXIS_CHECK(restored.heightfieldWidth == 2);
    AXIS_CHECK(restored.heightfieldLength == 2);
    AXIS_CHECK(restored.heightSamples.size() == 4);
    AXIS_CHECK_NEAR(restored.heightSamples[3], 2.0f, 0.0001f);
    AXIS_CHECK_NEAR(restored.heightfieldScale.z, 3.0f, 0.0001f);
}

AXIS_TEST_CASE("SceneSerializer round trips animation and VFX graphs")
{
    axis_test_support::HeadlessResourceFixture fixture;
    Scene source;
    const entt::entity entity = source.GetRegistry().create();
    source.AddComponent<InfoComponent>(entity, "GraphEntity", "default");

    auto& animation = source.AddComponent<AnimationComponent>(entity);
    animation.graph.enabled = true;
    animation.graph.entryState = 10;
    animation.graph.parameters.push_back({"move speed", AnimationParameterType::Float, 1.25f});
    animation.graph.states.push_back({10, "Idle State", "idle", 1.0f, glm::vec2(12, 34)});
    animation.graph.states.push_back({11, "Run State", "run", 1.2f, glm::vec2(220, 34)});
    animation.graph.transitions.push_back(
        {12, 10, 11, 0.15f, false, 0.9f, {{"move speed", AnimationConditionOp::Greater, 0.5f}}});
    animation.graph.transitions.back().conditionLogic = GraphConditionLogic::Nor;
    animation.graph.transitions.back().conditions.front().negated = true;

    auto& particles = source.AddComponent<ParticleEmitterComponent>(entity);
    particles.emitter.Initialize(16);
    particles.maxParticles = 16;
    particles.emitter.Gravity = glm::vec3(0, -4, 0);
    particles.emitter.Drag = 0.25f;
    particles.graph.enabled = true;
    particles.graph.parameters.push_back({"burst", AnimationParameterType::Trigger, 0.0f, false, true});
    particles.graph.nodes.push_back({20, VFXNodeType::Gravity, "World Gravity", glm::vec4(0, -8, 0, 0),
                                     {}, 0, 0, true, glm::vec2(40, 50)});
    particles.graph.nodes.push_back({21, VFXNodeType::Output, "Output", {}, {}, 0, 0, true,
                                     glm::vec2(260, 50)});
    particles.graph.links.push_back({22, 20, 21});
    particles.graph.links.back().conditionLogic = GraphConditionLogic::Xor;
    particles.graph.links.back().conditions.push_back(
        {"burst", AnimationConditionOp::Triggered, 0.0f, true});

    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    const std::string text = serializer.SerializeToString(source);
    AXIS_CHECK(text.find("GraphTransitionV2:") != std::string::npos);
    AXIS_CHECK(text.find("GraphNode:") != std::string::npos);
    AXIS_CHECK(text.find("GraphLinkV2:") != std::string::npos);

    Scene loaded;
    SceneLoadResult result;
    AXIS_CHECK(serializer.DeserializeFromString(text, "graph_roundtrip", loaded, result));
    const entt::entity loadedEntity = loaded.FindByName("GraphEntity");
    AXIS_CHECK(loadedEntity != entt::null);
    if (loadedEntity == entt::null) return;
    const auto& loadedAnimation = loaded.GetComponent<AnimationComponent>(loadedEntity);
    AXIS_CHECK(loadedAnimation.graph.enabled);
    AXIS_CHECK(loadedAnimation.graph.states.size() == 2);
    AXIS_CHECK(loadedAnimation.graph.transitions.size() == 1);
    AXIS_CHECK(loadedAnimation.graph.parameters.front().name == "move speed");
    AXIS_CHECK(loadedAnimation.graph.transitions.front().conditionLogic == GraphConditionLogic::Nor);
    AXIS_CHECK(loadedAnimation.graph.transitions.front().conditions.front().negated);
    const auto& loadedParticles = loaded.GetComponent<ParticleEmitterComponent>(loadedEntity);
    AXIS_CHECK(loadedParticles.graph.enabled);
    AXIS_CHECK(loadedParticles.graph.nodes.size() == 2);
    AXIS_CHECK(loadedParticles.graph.links.size() == 1);
    AXIS_CHECK(loadedParticles.graph.parameters.size() == 1);
    AXIS_CHECK(loadedParticles.graph.links.front().conditionLogic == GraphConditionLogic::Xor);
    AXIS_CHECK(loadedParticles.graph.links.front().conditions.front().negated);
    AXIS_CHECK_NEAR(loadedParticles.emitter.Drag, 0.25f, 0.0001f);
}

AXIS_TEST_CASE("SceneSerializer round trips network replication policy")
{
    axis_test_support::HeadlessResourceFixture fixture;
    Scene source;
    const entt::entity entity = source.GetRegistry().create();
    source.AddComponent<InfoComponent>(entity, "ReplicatedEntity", "network");
    auto& network = source.AddComponent<NetworkComponent>(entity);
    network.networkId = 0x10203040u;
    network.ownerId = 17u;
    network.isLocal = true;
    network.replicateTransform = false;
    network.interestRadius = 42.5f;

    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    const std::string text = serializer.SerializeToString(source);
    AXIS_CHECK(text.find("ReplicateTransform: false") != std::string::npos);
    AXIS_CHECK(text.find("InterestRadius: 42.5") != std::string::npos);

    Scene loaded;
    SceneLoadResult result;
    AXIS_CHECK(serializer.DeserializeFromString(text, "network_roundtrip", loaded, result));
    const entt::entity loadedEntity = loaded.FindByName("ReplicatedEntity");
    AXIS_CHECK(loadedEntity != entt::null);
    const auto& restored = loaded.GetComponent<NetworkComponent>(loadedEntity);
    AXIS_CHECK(restored.networkId == 0x10203040u);
    AXIS_CHECK(restored.ownerId == 17u);
    AXIS_CHECK(restored.isLocal);
    AXIS_CHECK(!restored.replicateTransform);
    AXIS_CHECK_NEAR(restored.interestRadius, 42.5f, 0.0001f);
}

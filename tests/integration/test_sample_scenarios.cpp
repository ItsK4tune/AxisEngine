#include "mocks/fake_physics.h"
#include "test_framework.h"
#include "test_support.h"

#include <audio/interface/i_audio_capture_service.h>
#include <audio/logic/audio_capture_processor.h>
#include <core/logic/config_manager.h>
#include <core/logic/localization_system.h>
#include <ecs/logic/physics_system.h>
#include <ecs/logic/system_manager.h>
#include <ecs/logic/transform_system.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/network_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/script_component.h>
#include <ecs/unit/ui_components.h>
#include <navigation/logic/navigation_system.h>
#include <navigation/unit/pathfollower_component.h>
#include <physics/logic/collision_matrix.h>
#include <physics/unit/ray.h>
#include <sample_state.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_serializer.h>
#include <scene/logic/component_codec_registry.h>
#include <fstream>
#include <memory>
#include <sstream>

using axis_test_mocks::FakePhysicsWorld;

void SampleState::ResetDefaultPlayerBindings()
{
}

void SampleState::OnEnter()
{
}

void SampleState::OnUpdate(float)
{
}

void SampleState::OnRender()
{
}

void SampleState::OnRenderDebug()
{
}

void SampleState::OnExit()
{
}

namespace
{
class FakeAudioCaptureService final : public IAudioCaptureService
{
public:
    bool Initialize(const AudioCaptureSettings& value) override
    {
        settings = value;
        return true;
    }
    void Shutdown() override
    {
        capturing = false;
    }
    bool RefreshDevices() override
    {
        ++refreshCount;
        return refreshResult;
    }
    std::vector<AudioCaptureDevice> GetDevices() const override
    {
        return devices;
    }
    AudioCaptureResult Start(const std::string& deviceId) override
    {
        ++startCount;
        lastStartedDevice = deviceId;
        if (startResult == AudioCaptureResult::Success)
            capturing = true;
        return startResult;
    }
    void Stop() override
    {
        ++stopCount;
        capturing = false;
    }
    bool IsCapturing() const override
    {
        return capturing;
    }
    void Update(float) override
    {
    }
    void BeginCalibration(float seconds) override
    {
        ++calibrationCount;
        lastCalibrationSeconds = seconds;
    }
    void SetSettings(const AudioCaptureSettings& value) override
    {
        settings = AudioCaptureProcessor::SanitizeSettings(value);
    }
    AudioCaptureSettings GetSettings() const override
    {
        return settings;
    }
    AudioCaptureSnapshot GetSnapshot() const override
    {
        return snapshot;
    }

    AudioCaptureSettings settings;
    AudioCaptureSnapshot snapshot;
    std::vector<AudioCaptureDevice> devices;
    AudioCaptureResult startResult = AudioCaptureResult::Success;
    std::string lastStartedDevice;
    int refreshCount = 0;
    int startCount = 0;
    int stopCount = 0;
    int calibrationCount = 0;
    float lastCalibrationSeconds = 0.0f;
    bool refreshResult = true;
    bool capturing = false;
};

class TestSampleState : public SampleState
{
public:
    void OnEnter() override
    {
    }

    void OnUpdate(float) override
    {
    }

    void OnRender() override
    {
    }

    void OnRenderDebug() override
    {
    }

    void OnExit() override
    {
    }
};

struct SampleScenarioFixture
{
    SampleScenarioFixture()
    {
        axis_test_support::ResetServices();
        resources.InitializeHeadless();
        AppConfig config;
        config.headlessMode = false;
        configManager.Initialize(config);

        ServiceLocator::Instance().Register<Scene>(&scene);
        ServiceLocator::Instance().Register<ResourceManager>(&resources);
        ServiceLocator::Instance().Register<IShaderLibrary>(&resources);
        ServiceLocator::Instance().Register<ITextureLibrary>(&resources);
        ServiceLocator::Instance().Register<IModelLibrary>(&resources);
        ServiceLocator::Instance().Register<ISoundLibrary>(&resources);
        ServiceLocator::Instance().Register<IFontLibrary>(&resources);
        ServiceLocator::Instance().Register<ISkyboxLibrary>(&resources);
        ServiceLocator::Instance().Register<IPhysicsWorld>(&physics);
        ServiceLocator::Instance().Register<CollisionMatrix>(&collisionMatrix);
        ServiceLocator::Instance().Register<ConfigManager>(&configManager);
        ServiceLocator::Instance().Register<SystemManager>(&systems);
        ServiceLocator::Instance().Register<ISystemRegistry>(&systems);
        ServiceLocator::Instance().Register<IAudioCaptureService>(&audioCapture);
        ServiceLocator::Instance().Register<IComponentCodecRegistry>(&componentCodecs);

        systems.RegisterSystem(std::make_unique<TransformSystem>());
        systems.RegisterSystem(std::make_unique<PhysicsSystem>());
        systems.RegisterSystem(std::make_unique<NavigationSystem>());
        auto localization = std::make_unique<LocalizationSystem>();
        ServiceLocator::Instance().Register<ILocalizationService>(localization.get());
        systems.RegisterSystem(std::move(localization));

        state.SetActiveScene(&scene);
    }

    ~SampleScenarioFixture()
    {
        if (auto* physicsSystem = systems.GetSystem<PhysicsSystem>())
            physicsSystem->Reset();
        resources.Shutdown();
        axis_test_support::ResetServices();
    }

    Scene scene;
    ResourceManager resources;
    FakePhysicsWorld physics;
    CollisionMatrix collisionMatrix;
    ConfigManager configManager;
    SystemManager systems;
    FakeAudioCaptureService audioCapture;
    ComponentCodecRegistry componentCodecs;
    TestSampleState state;
};

int CountByPrefix(Scene& scene, const std::string& prefix)
{
    int count = 0;
    auto view = scene.View<InfoComponent>();
    for (auto entity : view)
    {
        const auto& name = view.get<InfoComponent>(entity).name;
        if (name.rfind(prefix, 0) == 0)
            ++count;
    }
    return count;
}

entt::entity FindByName(Scene& scene, const std::string& name)
{
    auto view = scene.View<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            return entity;
    }
    return entt::null;
}

std::string ReadText(const std::filesystem::path& path)
{
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int RendererOrder(Scene& scene, const std::string& name)
{
    auto entity = FindByName(scene, name);
    AXIS_CHECK(entity != entt::null);
    AXIS_CHECK(scene.HasAllComponents<MeshRendererComponent>(entity));
    return scene.GetComponent<MeshRendererComponent>(entity).order;
}
}  // namespace

AXIS_TEST_CASE("Sample Scenario 01 creates requested render entity stress set")
{
    SampleScenarioFixture fixture;
    fixture.state.m_S1EntityCount = 216;
    fixture.state.m_S1MeshType = 1;
    fixture.state.m_S1UniqueTint = true;

    fixture.state.LoadScene1();

    AXIS_CHECK(CountByPrefix(fixture.scene, "Entity_") == 216);
    auto first = FindByName(fixture.scene, "Entity_0");
    AXIS_CHECK(first != entt::null);
    AXIS_CHECK(fixture.scene.HasAllComponents<MeshRendererComponent>(first));
    AXIS_CHECK(fixture.scene.HasAllComponents<MaterialComponent>(first));
}

AXIS_TEST_CASE("Sample Scenario 11 loads successfully")
{
    SampleScenarioFixture fixture;
    fixture.state.LoadScene11();
    fixture.systems.Update(fixture.scene, 0.016f);
}

AXIS_TEST_CASE("Sample Scenario 03 creates shadow casting light rig")
{
    SampleScenarioFixture fixture;

    fixture.state.LoadScene3();

    AXIS_CHECK(CountByPrefix(fixture.scene, "Cube_") == 1000);
    auto point = FindByName(fixture.scene, "PointLight");
    auto spot = FindByName(fixture.scene, "SpotLight");
    auto dir = FindByName(fixture.scene, "DirLight");
    AXIS_CHECK(point != entt::null);
    AXIS_CHECK(spot != entt::null);
    AXIS_CHECK(dir != entt::null);
    AXIS_CHECK(fixture.scene.GetComponent<PointLightComponent>(point).isCastShadow);
    AXIS_CHECK(fixture.scene.GetComponent<SpotLightComponent>(spot).isCastShadow);
    AXIS_CHECK(fixture.scene.HasAllComponents<DirectionalLightComponent>(dir));
}

AXIS_TEST_CASE("Sample Scenario 07 render order switches normal and reverse")
{
    SampleScenarioFixture fixture;

    fixture.state.LoadScene7();
    AXIS_CHECK(RendererOrder(fixture.scene, "Panel_Red") == 1);
    AXIS_CHECK(RendererOrder(fixture.scene, "Panel_Blue") == 3);

    fixture.state.m_S7ReverseOrder = true;
    fixture.state.ApplyScenario7RenderOrder();

    AXIS_CHECK(RendererOrder(fixture.scene, "Panel_Red") == 3);
    AXIS_CHECK(RendererOrder(fixture.scene, "Panel_Blue") == 1);
}

AXIS_TEST_CASE("Sample Scenario 21 creates primitive collider chain payload")
{
    SampleScenarioFixture fixture;
    fixture.state.m_S21ChainLength = 2;
    fixture.state.m_S21PayloadShape = 1;

    fixture.state.LoadScene21();

    auto payload = FindByName(fixture.scene, "Payload");
    AXIS_CHECK(payload != entt::null);
    AXIS_CHECK(fixture.scene.GetComponent<RigidShapeComponent>(payload).type == ShapeType::Sphere);
    AXIS_CHECK(fixture.scene.HasAllComponents<RigidBodyComponent>(payload));
    AXIS_CHECK(fixture.state.m_S21ChainEntities.size() == 4);
}

AXIS_TEST_CASE("Sample Scenario 26 creates controller trigger zones and fixed constraint")
{
    SampleScenarioFixture fixture;

    fixture.state.LoadScene26();

    auto controller = FindByName(fixture.scene, "S26_CharacterController");
    auto trigger = FindByName(fixture.scene, "S26_TriggerZone");
    AXIS_CHECK(controller != entt::null);
    AXIS_CHECK(trigger != entt::null);
    AXIS_CHECK(fixture.scene.HasAllComponents<CharacterControllerComponent>(controller));
    AXIS_CHECK(fixture.scene.GetComponent<RigidBodyComponent>(trigger).isTrigger);
    AXIS_CHECK(fixture.physics.fixedConstraintCreateCount >= 1);
}

AXIS_TEST_CASE("Sample Scenario 28 creates input binding player and action pads")
{
    SampleScenarioFixture fixture;

    fixture.state.LoadScene28();

    auto player = FindByName(fixture.scene, "BindingPlayer");
    AXIS_CHECK(player != entt::null);
    AXIS_CHECK(fixture.scene.HasAllComponents<ScriptComponent>(player));
    AXIS_CHECK(fixture.scene.HasAllComponents<RigidBodyComponent>(player));
    AXIS_CHECK(CountByPrefix(fixture.scene, "InputPad_") == 5);
}

AXIS_TEST_CASE("Sample Scenario 23 configures path criteria follower")
{
    SampleScenarioFixture fixture;
    fixture.state.m_S23ObstacleCount = 0;
    fixture.state.m_S23CrowdCount = 8;
    fixture.state.m_S23PathfindingCriteria = static_cast<int>(PathfindingCriteria::StayOnRoad);

    fixture.state.LoadScene23();

    auto follower = fixture.state.m_NavFollower;
    AXIS_CHECK(follower != entt::null);
    AXIS_CHECK(fixture.scene.IsValid(follower));
    const auto& pathFollower = fixture.scene.GetComponent<PathFollowerComponent>(follower);
    AXIS_CHECK(pathFollower.pathfindingOptions.criteria == PathfindingCriteria::StayOnRoad);
    AXIS_CHECK(pathFollower.pathfindingOptions.preferredTags.size() == 1);
    AXIS_CHECK(pathFollower.pathfindingOptions.preferredTags[0] == "road");
    AXIS_CHECK(!fixture.state.m_NavWaypoints.empty());
    AXIS_CHECK(fixture.state.m_S23CrowdFollowers.size() == 8);
    AXIS_CHECK(CountByPrefix(fixture.scene, "CrowdFollower_") == 8);
}

AXIS_TEST_CASE("Sample Scenario 25 scene save and reload roundtrip keeps base entities")
{
    SampleScenarioFixture fixture;

    fixture.state.LoadScene25();
    auto output = axis_test_support::TempPath("mr_scenario25.axs");
    SceneSerializer serializer(fixture.resources, nullptr, nullptr);
    AXIS_CHECK(serializer.Serialize(output.string(), fixture.scene, "scenario_base"));
    const std::string serialized = ReadText(output);

    Scene reloaded;
    SceneLoadResult result;
    serializer.Deserialize(output.string(), reloaded, result);
    AXIS_CHECK(serialized.find("Ground:") != std::string::npos);
    AXIS_CHECK(result.entities.size() >= 2);
    AXIS_CHECK(FindByName(reloaded, "Ground") != entt::null);
}

AXIS_TEST_CASE("Sample Scenario 27 creates data-driven entity grid")
{
    SampleScenarioFixture fixture;
    fixture.state.m_S27EntityCount = 12;
    fixture.state.m_S27EntitySize = 1.5f;

    fixture.state.LoadScene27();

    AXIS_CHECK(CountByPrefix(fixture.scene, "DataEntity_") == 12);
    auto first = FindByName(fixture.scene, "DataEntity_0");
    AXIS_CHECK(first != entt::null);
    AXIS_CHECK(fixture.scene.HasAllComponents<MeshRendererComponent>(first));
}

AXIS_TEST_CASE("Sample Scenario 29 loads localization and creates localized UI")
{
    SampleScenarioFixture fixture;

    fixture.state.LoadScene29();

    auto* localization = fixture.systems.GetSystem<LocalizationSystem>();
    AXIS_CHECK(localization != nullptr);
    AXIS_CHECK(localization->GetLanguage() == "en");
    AXIS_CHECK(FindByName(fixture.scene, "L10nPreviewPanel") != entt::null);
    AXIS_CHECK(FindByName(fixture.scene, "L10nCurrentLanguageText") != entt::null);
}

AXIS_TEST_CASE("Sample Scenario 30 initializes network orb and message state")
{
    SampleScenarioFixture fixture;

    fixture.state.LoadScene30();

    auto orb = FindByName(fixture.scene, "NetworkOrb");
    AXIS_CHECK(orb != entt::null);
    AXIS_CHECK(fixture.scene.HasAllComponents<MeshRendererComponent>(orb));
    AXIS_CHECK(fixture.state.m_S30SpawnCounter == 0);
    AXIS_CHECK(fixture.state.m_S30Messages.empty());
}

AXIS_TEST_CASE("Sample Scenario 31 creates 2D and 3D audio sources")
{
    SampleScenarioFixture fixture;

    fixture.state.LoadScene31();

    auto source3D = FindByName(fixture.scene, "AudioSource3D");
    auto source2D = FindByName(fixture.scene, "Audio2DLoop");
    AXIS_CHECK(source3D != entt::null);
    AXIS_CHECK(source2D != entt::null);
    const auto& audio3D = fixture.scene.GetComponent<AudioSourceComponent>(source3D);
    const auto& audio2D = fixture.scene.GetComponent<AudioSourceComponent>(source2D);
    AXIS_CHECK(audio3D.is3D);
    AXIS_CHECK(!audio2D.is3D);
    AXIS_CHECK(audio3D.loop);
    AXIS_CHECK(audio2D.loop);
    AXIS_CHECK(audio3D.playOnAwake);
    AXIS_CHECK(!audio2D.playOnAwake);
    AXIS_CHECK_NEAR(audio3D.volume, fixture.state.m_S31Volume3D, 0.0001f);
    AXIS_CHECK_NEAR(audio2D.volume, fixture.state.m_S31Volume2D, 0.0001f);
    AXIS_CHECK(fixture.state.m_S31Volume3D <= TestSampleState::kScenario31MaxVolume);
    AXIS_CHECK(fixture.state.m_S31Volume2D <= TestSampleState::kScenario31MaxVolume);
}

AXIS_TEST_CASE("Sample Scenario 31 applies 2D and 3D volume independently")
{
    SampleScenarioFixture fixture;

    fixture.state.m_S31Volume2D = 20.0f;
    fixture.state.m_S31Volume3D = 90.0f;
    fixture.state.LoadScene31();

    auto source3D = FindByName(fixture.scene, "AudioSource3D");
    auto source2D = FindByName(fixture.scene, "Audio2DLoop");
    auto& audio3D = fixture.scene.GetComponent<AudioSourceComponent>(source3D);
    auto& audio2D = fixture.scene.GetComponent<AudioSourceComponent>(source2D);

    AXIS_CHECK_NEAR(audio2D.volume, 20.0f, 0.0001f);
    AXIS_CHECK_NEAR(audio3D.volume, 90.0f, 0.0001f);
    AXIS_CHECK_NEAR(audio2D.volume, fixture.state.m_S31Volume2D, 0.0001f);
    AXIS_CHECK_NEAR(audio3D.volume, fixture.state.m_S31Volume3D, 0.0001f);
}

AXIS_TEST_CASE("Sample Scenario 33 reports an unavailable microphone without starting capture")
{
    SampleScenarioFixture fixture;

    fixture.state.LoadScene33();

    AXIS_CHECK(fixture.audioCapture.refreshCount == 1);
    AXIS_CHECK(fixture.audioCapture.startCount == 0);
    AXIS_CHECK(!fixture.state.m_S33DeviceDetected);
    AXIS_CHECK(fixture.state.m_S33LastResult == AudioCaptureResult::DeviceNotFound);
    AXIS_CHECK(fixture.state.m_S33VisualizerBars.size() == 32);
}

AXIS_TEST_CASE("Sample Scenario 33 detects starts and configures microphone capture")
{
    SampleScenarioFixture fixture;
    fixture.audioCapture.devices = {{"mic-1", "Test Microphone", true}};
    fixture.audioCapture.settings.inputVolume = 1.5f;
    fixture.audioCapture.settings.noiseGate = 0.04f;

    fixture.state.LoadScene33();

    AXIS_CHECK(fixture.state.m_S33DeviceDetected);
    AXIS_CHECK(fixture.audioCapture.startCount == 1);
    AXIS_CHECK(fixture.audioCapture.capturing);
    AXIS_CHECK(fixture.state.m_S33StartedCapture);
    AXIS_CHECK_NEAR(fixture.state.m_S33InputVolume, 1.5f, 0.0001f);
    AXIS_CHECK_NEAR(fixture.state.m_S33NoiseGate, 0.04f, 0.0001f);

    fixture.state.m_S33InputVolume = 2.0f;
    fixture.state.m_S33AttackSeconds = 0.02f;
    fixture.state.ApplyScenario33CaptureSettings();
    AXIS_CHECK_NEAR(fixture.audioCapture.settings.inputVolume, 2.0f, 0.0001f);
    AXIS_CHECK_NEAR(fixture.audioCapture.settings.attackSeconds, 0.02f, 0.0001f);

    fixture.state.StopScenario33Capture();
    AXIS_CHECK(!fixture.audioCapture.capturing);
    AXIS_CHECK(fixture.audioCapture.stopCount == 1);
}

AXIS_TEST_CASE("StreamingSystem requests and releases shared async residency by distance")
{
    SampleScenarioFixture fixture;
    auto camera = EntityBuilder(fixture.scene, fixture.resources, "test")
                      .WithName("StreamingCamera")
                      .WithTransform(glm::vec3(0.0f))
                      .WithCamera(60.0f, 0.1f, 100.0f, true)
                      .Build();
    auto streamed = EntityBuilder(fixture.scene, fixture.resources, "test")
                        .WithName("StreamingTarget")
                        .WithTransform(glm::vec3(5.0f, 0.0f, 0.0f))
                        .WithMesh("", "")
                        .WithStreaming("asset://objects/cube/cube.fbx", 10.0f, 20.0f, true)
                        .Build();

    StreamingSystem streaming;
    streaming.Initialize();
    streaming.Update(fixture.scene, 1.1f);
    auto& state = fixture.scene.GetComponent<StreamingComponent>(streamed);
    AXIS_CHECK(state.isRequested);

    fixture.scene.GetComponent<PositionComponent>(camera).value = glm::vec3(100.0f, 0.0f, 0.0f);
    streaming.Update(fixture.scene, 1.1f);
    AXIS_CHECK(!state.isRequested);
    AXIS_CHECK(!state.isResident);
    streaming.Shutdown();
}

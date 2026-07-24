#include "test_framework.h"
#include "test_support.h"

#include <audio/logic/audio_capture_processor.h>
#include <audio/logic/audio_service.h>
#include <audio/strategy/null/null_audio_engine.h>
#include <core/app/app_builder.h>
#include <core/app/application.h>
#include <core/app/state_machine.h>
#include <core/interface/i_optimization_configurable.h>
#include <core/logic/config_loader.h>
#include <core/logic/config_manager.h>
#include <core/logic/config_validation.h>
#include <core/logic/data_manager.h>
#include <core/logic/data_node_serializer.h>
#include <core/logic/filesystem.h>
#include <core/logic/job_system.h>
#include <core/logic/logger.h>
#include <core/logic/runtime_profiler.h>
#include <ecs/logic/system_manager.h>
#include <ecs/logic/audio_system.h>
#include <ecs/logic/system_factory.h>
#include <ecs/logic/effect_graph_runtime.h>
#include <ecs/logic/particle_system.h>
#include <ecs/logic/post_process_system.h>
#include <ecs/unit/media_components.h>
#include <editor/editor_shortcut.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/input_serializer.h>
#include <platform/logic/monitor_manager.h>
#include <render/type/post_process_input.h>
#include <render/type/shader_abi.h>
#include <render/logic/spatial_culling_policy.h>
#include <render/logic/particle_emitter.h>
#include <resource/unit/model.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_serializer.h>
#include <script/logic/script_registry.h>
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <future>
#include <sstream>
#include <set>
#include <stdexcept>
#include <limits>
#include <type_traits>
#include "mocks/fake_physics.h"

using axis_test_mocks::FakePhysicsWorld;

namespace
{
class RecordingAudioEngine final : public IAudioEngine
{
public:
    bool Initialize() override { return true; }
    void Update() override {}
    void Shutdown() override {}
    void SetListenerPosition(const glm::vec3&, const glm::vec3&) override {}
    void SetGlobalVolume(float) override {}
    std::shared_ptr<ISound> Play2D(const std::string& filename, bool, bool) override
    {
        path2D = filename;
        return std::make_shared<NullSound>();
    }
    std::shared_ptr<ISound> Play2D(IAudioSource*, bool, bool) override
    {
        return std::make_shared<NullSound>();
    }
    std::shared_ptr<ISound> Play3D(const std::string& filename, const glm::vec3&, bool, bool) override
    {
        path3D = filename;
        return std::make_shared<NullSound>();
    }
    std::shared_ptr<ISound> Play3D(IAudioSource*, const glm::vec3&, bool, bool) override
    {
        return std::make_shared<NullSound>();
    }
    std::shared_ptr<IAudioSource> AddSoundSourceFromFile(const std::string& filename) override
    {
        return std::make_shared<NullAudioSource>(filename);
    }
    void StopAllSounds() override {}

    std::string path2D;
    std::string path3D;
};

class FakeWindow final : public IWindow
{
public:
    bool Initialize(int, int, const std::string&, int) override
    {
        return true;
    }
    void SetTitle(const std::string&) override
    {
    }
    void SetIcon(int, int, unsigned char*) override
    {
    }
    void SetVsync(bool) override
    {
    }
    void Update() override
    {
    }
    void Shutdown() override
    {
    }
    bool ShouldClose() const override
    {
        return false;
    }
    void SetShouldClose(bool) override
    {
    }
    void SwapBuffers() override
    {
    }
    void PollEvents() override
    {
    }
    int GetWidth() const override
    {
        return 800;
    }
    int GetHeight() const override
    {
        return 600;
    }
    void SetWindowConfiguration(int, int, WindowMode, int monitorIndex, int) override
    {
        configuredMonitor = monitorIndex;
    }
    std::vector<MonitorInfo> GetMonitors() const override
    {
        return monitors;
    }
    void SetCursorMode(CursorMode mode) override
    {
        cursorMode = mode;
    }
    void SetAspectRatio(int, int) override
    {
    }
    void* GetNativeWindow() const override
    {
        return nullptr;
    }
    bool GetKey(Key key) const override
    {
        return pressedKeys.contains(key) || (key == Key::Space && spacePressed);
    }
    bool GetMouseButton(Mouse button) const override
    {
        return button == Mouse::Left && leftPressed;
    }
    bool GetGamepadButton(int deviceIndex, Gamepad button) const override
    {
        return deviceIndex == 0 && button == Gamepad::ButtonA && gamepadAPressed;
    }
    float GetGamepadAxis(int deviceIndex, GamepadAxis axis) const override
    {
        return deviceIndex == 0 && axis == GamepadAxis::LeftX ? gamepadLeftX : 0.0f;
    }
    void GetCursorPos(double& x, double& y) const override
    {
        x = y = 0.0;
    }
    void SetCursorPos(double, double) override
    {
    }
    std::vector<DeviceInfo> GetConnectedDevices() const override
    {
        return {{"keyboard_0", "Keyboard", DeviceType::Keyboard, true},
                {"mouse_0", "Mouse", DeviceType::Mouse, true},
                {"gamepad_0", "Gamepad", DeviceType::Gamepad, false}};
    }
    void SetResizeCallback(const ResizeCallback&) override
    {
    }
    void SetKeyCallback(const KeyCallback&) override
    {
    }
    void SetMouseButtonCallback(const MouseButtonCallback&) override
    {
    }
    void SetCursorPosCallback(const MousePositionCallback&) override
    {
    }
    void SetScrollCallback(const ScrollCallback&) override
    {
    }

    bool spacePressed = true;
    std::set<Key> pressedKeys;
    bool leftPressed = true;
    bool gamepadAPressed = false;
    float gamepadLeftX = 0.0f;
    int configuredMonitor = -1;
    CursorMode cursorMode = CursorMode::Normal;
    std::vector<MonitorInfo> monitors;
};

struct StateCounters
{
    int render = 0;
    int debug = 0;
    int pause = 0;
    int resume = 0;
    int exit = 0;
};

class RecordingState final : public State
{
public:
    explicit RecordingState(StateCounters& counters) : m_Counters(counters)
    {
    }
    void OnEnter() override
    {
    }
    void OnUpdate(float) override
    {
    }
    void OnRender() override
    {
        ++m_Counters.render;
    }
    void OnRenderDebug() override
    {
        ++m_Counters.debug;
    }
    void OnExit() override
    {
        ++m_Counters.exit;
    }
    void OnPause() override
    {
        ++m_Counters.pause;
    }
    void OnResume() override
    {
        ++m_Counters.resume;
    }

private:
    StateCounters& m_Counters;
};

class ThrowingEnterState final : public State
{
public:
    void OnEnter() override
    {
        throw std::runtime_error("enter failed");
    }
    void OnUpdate(float) override
    {
    }
    void OnRender() override
    {
    }
    void OnExit() override
    {
    }
};

class CustomGeometrySystem final : public IBaseSystem
{
public:
    bool IsEnabled() const override
    {
        return true;
    }
    void SetEnabled(bool) override
    {
    }
    std::string GetName() const override
    {
        return "GeometrySystem";
    }
};

class DefaultReplaceableScript final : public Scriptable
{
};

class UserReplaceableScript final : public Scriptable
{
};

class TrackingConfigLoader final : public ILoaderStrategy
{
public:
    explicit TrackingConfigLoader(bool& called) : m_Called(called)
    {
    }

    bool Load(const std::string&) override
    {
        m_Called = true;
        return true;
    }

    const char* GetName() const override
    {
        return "CONFIG";
    }

private:
    bool& m_Called;
};

class ShutdownOrderSystem final : public IBaseSystem
{
public:
    ShutdownOrderSystem(std::string name, std::vector<std::string>& shutdownOrder)
        : m_Name(std::move(name)), m_ShutdownOrder(shutdownOrder)
    {
    }

    bool IsEnabled() const override
    {
        return true;
    }
    void SetEnabled(bool) override
    {
    }
    std::string GetName() const override
    {
        return m_Name;
    }
    void Shutdown() override
    {
        m_ShutdownOrder.push_back(m_Name);
    }

private:
    std::string m_Name;
    std::vector<std::string>& m_ShutdownOrder;
};

class GraphicsRequirementSystem final : public IBaseSystem
{
public:
    bool IsEnabled() const override
    {
        return enabled;
    }
    void SetEnabled(bool value) override
    {
        enabled = value;
    }
    std::string GetName() const override
    {
        return "GraphicsRequirementSystem";
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }
    void Initialize() override
    {
        initialized = true;
    }
    void Shutdown() override
    {
        shutdown = true;
    }

    bool enabled = true;
    bool initialized = false;
    bool shutdown = false;
};

class OptimizationAwareSystem final : public IBaseSystem, public IOptimizationConfigurable
{
public:
    bool IsEnabled() const override
    {
        return true;
    }
    void SetEnabled(bool) override
    {
    }
    std::string GetName() const override
    {
        return "OptimizationAwareSystem";
    }
    void ApplyOptimizationConfig(const OptimizationConfig& config) override
    {
        observedParticleBudget = config.particleMaxSpawnPerFrame;
    }

    int observedParticleBudget = 0;
};
}  // namespace

AXIS_TEST_CASE("Editor shortcuts require exact modifiers and latch the base-key press")
{
    FakeWindow window;
    window.spacePressed = false;
    KeyboardManager keyboard(&window);
    bool pressed = false;

    window.pressedKeys = {Key::F1, Key::LeftShift};
    AXIS_CHECK(!IsEditorShortcutPressed(keyboard, Key::F1, EditorModifier::None, pressed));
    AXIS_CHECK(!IsEditorShortcutPressed(keyboard, Key::F1, EditorModifier::Shift, pressed));

    window.pressedKeys.clear();
    AXIS_CHECK(!IsEditorShortcutPressed(keyboard, Key::F1, EditorModifier::None, pressed));

    window.pressedKeys = {Key::F1};
    AXIS_CHECK(IsEditorShortcutPressed(keyboard, Key::F1, EditorModifier::None, pressed));
    AXIS_CHECK(!IsEditorShortcutPressed(keyboard, Key::F1, EditorModifier::None, pressed));
}

AXIS_TEST_CASE("Blocked editor shortcuts do not fire after text focus changes while held")
{
    FakeWindow window;
    window.spacePressed = false;
    window.pressedKeys = {Key::Delete};
    KeyboardManager keyboard(&window);
    bool pressed = false;

    AXIS_CHECK(!IsEditorShortcutPressed(keyboard, Key::Delete, EditorModifier::None, pressed, true));
    AXIS_CHECK(!IsEditorShortcutPressed(keyboard, Key::Delete, EditorModifier::None, pressed, false));

    window.pressedKeys.clear();
    AXIS_CHECK(!IsEditorShortcutPressed(keyboard, Key::Delete, EditorModifier::None, pressed));
    window.pressedKeys = {Key::Delete};
    AXIS_CHECK(IsEditorShortcutPressed(keyboard, Key::Delete, EditorModifier::None, pressed));
}

AXIS_TEST_CASE("Config validation reports and sanitizes unsafe runtime values")
{
    AppConfig input;
    input.window.width = 0;
    input.window.height = -50;
    input.physics.physicsTickRate = 0.0f;
    input.physics.maxSubSteps = 0;
    input.timeScale = std::numeric_limits<float>::quiet_NaN();
    input.audio.captureInputVolume = 50.0f;
    input.audio.captureAttackSeconds = -1.0f;
    input.culling.spatialCullingMode = static_cast<SpatialCullingMode>(99);

    const ConfigValidationResult result = ValidateAndSanitizeConfig(input);
    AXIS_CHECK(result.WasSanitized());
    AXIS_CHECK(result.config.window.width == 1);
    AXIS_CHECK(result.config.window.height == 1);
    AXIS_CHECK_NEAR(result.config.physics.physicsTickRate, 1.0f, 0.0001f);
    AXIS_CHECK(result.config.physics.maxSubSteps == 1);
    AXIS_CHECK_NEAR(result.config.timeScale, 1.0f, 0.0001f);
    AXIS_CHECK_NEAR(result.config.audio.captureInputVolume, 4.0f, 0.0001f);
    AXIS_CHECK_NEAR(result.config.audio.captureAttackSeconds, 0.001f, 0.0001f);
    AXIS_CHECK(result.config.culling.spatialCullingMode == SpatialCullingMode::Auto);
}

AXIS_TEST_CASE("Default asset bootstrap is explicit and sanitizes an empty manifest")
{
    AppConfig config;
    config.loadDefaultAssets = true;
    config.defaultAssetManifest.clear();
    const auto result = ValidateAndSanitizeConfig(config);
    AXIS_CHECK(result.WasSanitized());
    AXIS_CHECK(result.config.defaultAssetManifest == "asset://load.axs");

    config.loadDefaultAssets = false;
    const auto disabled = ValidateAndSanitizeConfig(config);
    AXIS_CHECK(disabled.config.defaultAssetManifest.empty());
}

AXIS_TEST_CASE("Config validation replaces backends that are not compiled into the build")
{
    AppConfig config;
    config.graphics.graphicsBackend = GraphicsBackend::Vulkan;
    config.physics.physicsBackend = PhysicsBackend::PhysX;
    config.audio.audioBackend = AudioBackend::OpenAL;

    const auto result = ValidateAndSanitizeConfig(config);
    AXIS_CHECK(result.WasSanitized());
#if AXIS_HAS_OPENGL_BACKEND
    AXIS_CHECK(result.config.graphics.graphicsBackend == GraphicsBackend::OpenGL);
#endif
#if AXIS_HAS_BULLET_BACKEND
    AXIS_CHECK(result.config.physics.physicsBackend == PhysicsBackend::Bullet);
#endif
#if AXIS_HAS_IRRKLANG_BACKEND
    AXIS_CHECK(result.config.audio.audioBackend == AudioBackend::IrrKlang);
#elif AXIS_HAS_FMOD_BACKEND
    AXIS_CHECK(result.config.audio.audioBackend == AudioBackend::FMOD);
#else
    AXIS_CHECK(result.config.audio.audioBackend == AudioBackend::Null);
#endif
}

AXIS_TEST_CASE("Config validation preserves backend selections owned by custom providers")
{
    AppConfig config;
    config.graphics.graphicsBackend = GraphicsBackend::Vulkan;
    config.physics.physicsBackend = PhysicsBackend::PhysX;
    config.audio.audioBackend = AudioBackend::OpenAL;

    const ConfigValidationPolicy policy{true, true, true};
    const auto result = ValidateAndSanitizeConfig(config, policy);
    AXIS_CHECK(result.config.graphics.graphicsBackend == GraphicsBackend::Vulkan);
    AXIS_CHECK(result.config.physics.physicsBackend == PhysicsBackend::PhysX);
    AXIS_CHECK(result.config.audio.audioBackend == AudioBackend::OpenAL);
}

AXIS_TEST_CASE("ConfigManager only publishes sanitized configuration snapshots")
{
    AppConfig initial;
    ConfigManager manager;
    manager.Initialize(initial);
    AppConfig observed;
    const int subscription = EventManager::Instance().Subscribe<ConfigChangedEvent>(
        [&](const ConfigChangedEvent& event) { observed = event.config; });

    AppConfig invalid = initial;
    invalid.physics.physicsTickRate = -100.0f;
    invalid.render.gamma = std::numeric_limits<float>::infinity();
    manager.UpdateConfig(invalid, ConfigChangedEvent::Physics | ConfigChangedEvent::Graphics);

    EventManager::Instance().Unsubscribe<ConfigChangedEvent>(subscription);
    AXIS_CHECK_NEAR(observed.physics.physicsTickRate, 1.0f, 0.0001f);
    AXIS_CHECK_NEAR(observed.render.gamma, 2.2f, 0.0001f);
}

AXIS_TEST_CASE("ConfigManager ignores zero-sized minimized-window resolutions")
{
    AppConfig initial;
    initial.window.width = 1280;
    initial.window.height = 720;
    ConfigManager manager;
    manager.Initialize(initial);
    manager.SetResolution(0, 0);
    const auto unchanged = manager.GetConfig();
    AXIS_CHECK(unchanged.window.width == 1280);
    AXIS_CHECK(unchanged.window.height == 720);
}

AXIS_TEST_CASE("Config change masks notify only the intended subsystem")
{
    const ConfigChangedEvent graphicsOnly(AppConfig{}, ConfigChangedEvent::Graphics);
    AXIS_CHECK(HasConfigChanged(graphicsOnly, ConfigChangedEvent::Graphics));
    AXIS_CHECK(!HasConfigChanged(graphicsOnly, ConfigChangedEvent::Audio));
    AXIS_CHECK(!HasConfigChanged(graphicsOnly, ConfigChangedEvent::Optimization));

    const ConfigChangedEvent all(AppConfig{}, ConfigChangedEvent::All);
    AXIS_CHECK(HasConfigChanged(all, ConfigChangedEvent::Graphics));
    AXIS_CHECK(HasConfigChanged(all, ConfigChangedEvent::Audio));
    AXIS_CHECK(HasConfigChanged(all, ConfigChangedEvent::Optimization));
}

AXIS_TEST_CASE("Config validation sanitizes runtime optimization budgets")
{
    AppConfig config;
    config.optimization.maxModelUploadsPerFrame = 0;
    config.optimization.maxTextureUploadsPerFrame = -4;
    config.optimization.streamingCheckIntervalSeconds = -1.0f;
    config.optimization.navigationAgentCellSize = 0.0f;
    config.optimization.navigationMaxPathRequestsPerFrame = 0;
    config.optimization.maxNavMeshRebuildsPerFrame = 0;
    config.optimization.navigationNavMeshTileSize = 0.0f;
    config.optimization.navigationMaxDirtyTilesPerFrame = 0;
    config.optimization.networkMaxEventsPerUpdate = 0;
    config.optimization.networkMaxEventProcessingMs = std::numeric_limits<float>::infinity();
    config.optimization.networkMaxBytesPerUpdate = 0;
    config.optimization.networkReplicationRateHz = 0.0f;
    config.optimization.networkInterestRadius = -1.0f;
    config.optimization.particleMaxSpawnPerFrame = 0;
    const auto result = ValidateAndSanitizeConfig(config, {true, true, true});
    AXIS_CHECK(result.config.optimization.maxModelUploadsPerFrame == 1);
    AXIS_CHECK(result.config.optimization.maxTextureUploadsPerFrame == 1);
    AXIS_CHECK_NEAR(result.config.optimization.streamingCheckIntervalSeconds, 0.0f, 0.0001f);
    AXIS_CHECK_NEAR(result.config.optimization.navigationAgentCellSize, 0.01f, 0.0001f);
    AXIS_CHECK(result.config.optimization.navigationMaxPathRequestsPerFrame == 1);
    AXIS_CHECK(result.config.optimization.maxNavMeshRebuildsPerFrame == 1);
    AXIS_CHECK_NEAR(result.config.optimization.navigationNavMeshTileSize, 0.25f, 0.0001f);
    AXIS_CHECK(result.config.optimization.navigationMaxDirtyTilesPerFrame == 1);
    AXIS_CHECK(result.config.optimization.networkMaxEventsPerUpdate == 1);
    AXIS_CHECK_NEAR(result.config.optimization.networkMaxEventProcessingMs, 2.0f, 0.0001f);
    AXIS_CHECK(result.config.optimization.networkMaxBytesPerUpdate == 1);
    AXIS_CHECK_NEAR(result.config.optimization.networkReplicationRateHz, 0.1f, 0.0001f);
    AXIS_CHECK_NEAR(result.config.optimization.networkInterestRadius, 0.0f, 0.0001f);
    AXIS_CHECK(result.config.optimization.particleMaxSpawnPerFrame == 1);
}

AXIS_TEST_CASE("Audio capture sanitizer replaces non-finite settings")
{
    AudioCaptureSettings invalid;
    invalid.inputVolume = std::numeric_limits<float>::quiet_NaN();
    invalid.noiseGate = std::numeric_limits<float>::infinity();
    invalid.attackSeconds = -std::numeric_limits<float>::infinity();

    const AudioCaptureSettings sanitized = AudioCaptureProcessor::SanitizeSettings(invalid);
    AXIS_CHECK_NEAR(sanitized.inputVolume, 1.0f, 0.0001f);
    AXIS_CHECK_NEAR(sanitized.noiseGate, 0.02f, 0.0001f);
    AXIS_CHECK_NEAR(sanitized.attackSeconds, 0.05f, 0.0001f);
}

AXIS_TEST_CASE("Application provider builders do not share instance factories")
{
    AppBuilder defaults;
    AppBuilder customized;
    customized.WithWindowFactory([] { return std::unique_ptr<IWindow>{}; });

    AXIS_CHECK(!defaults.GetCapabilities().customWindow);
    AXIS_CHECK(customized.GetCapabilities().customWindow);
}

AXIS_TEST_CASE("JobSystem executes pre-initialization work inline instead of orphaning futures")
{
    JobSystem::Instance().Shutdown();
    auto future = JobSystem::Instance().ExecuteAsync([] { return 42; });
    AXIS_CHECK(future.get() == 42);
    AXIS_CHECK(!JobSystem::Instance().IsBusy());
}

AXIS_TEST_CASE("JobSystem worker-assisted waits complete nested work with one worker")
{
    auto& jobs = JobSystem::Instance();
    jobs.Shutdown();
    jobs.Initialize(1);

    JobSystem::JobCounter outer{0};
    std::atomic<int> completed{0};
    jobs.Execute(
        [&jobs, &completed]() {
            JobSystem::JobCounter inner{0};
            for (int i = 0; i < 64; ++i)
                jobs.Execute([&completed]() { completed.fetch_add(1, std::memory_order_relaxed); }, &inner);
            jobs.Wait(&inner);
        },
        &outer);
    jobs.Wait(&outer);
    jobs.Shutdown();

    AXIS_CHECK(completed.load(std::memory_order_relaxed) == 64);
    AXIS_CHECK(!jobs.IsBusy());
}

AXIS_TEST_CASE("ConfigManager publishes immutable copy-on-write snapshots")
{
    AppConfig initial;
    initial.window.width = 1280;
    ConfigManager manager;
    manager.Initialize(initial);

    const auto before = manager.GetConfigSnapshot();
    AppConfig updated = *before;
    updated.window.width = 1920;
    manager.UpdateConfig(updated, ConfigChangedEvent::Window);
    const auto after = manager.GetConfigSnapshot();

    AXIS_CHECK(before != after);
    AXIS_CHECK(before->window.width == 1280);
    AXIS_CHECK(after->window.width == 1920);
    AXIS_CHECK(after == manager.GetConfigSnapshot());
}

AXIS_TEST_CASE("ServiceLocator copy-on-write lookups observe complete registry states")
{
    ServiceLocator services;
    int first = 1;
    int second = 2;
    services.Register<int>(&first);
    AXIS_CHECK(services.Resolve<int>() == &first);

    services.Register<int>(&second);
    AXIS_CHECK(services.Resolve<int>() == &second);
    services.Unregister<int>();
    AXIS_CHECK(services.Resolve<int>() == nullptr);
}

AXIS_TEST_CASE("ParticleEmitter maintains dense active slots without a graphics backend")
{
    ParticleEmitter::ClearManagers();
    ParticleEmitter emitter;
    emitter.Initialize(8);
    emitter.SpawnRate = 1000.0f;
    emitter.LifeTime = 0.1f;

    emitter.Update(1.0f, glm::vec3(0.0f), true);
    AXIS_CHECK(emitter.GetActiveParticleCount() == 8);
    emitter.Update(0.2f, glm::vec3(0.0f), false);
    AXIS_CHECK(emitter.GetActiveParticleCount() == 0);
}

AXIS_TEST_CASE("ParticleEmitter applies gravity and drag without a graphics backend")
{
    ParticleEmitter::ClearManagers();
    ParticleEmitter emitter;
    emitter.Initialize(1);
    emitter.SpawnRate = 1000.0f;
    emitter.LifeTime = 10.0f;
    emitter.MinVelocity = glm::vec3(0.0f);
    emitter.MaxVelocity = glm::vec3(0.0f);
    emitter.Gravity = glm::vec3(0.0f, -10.0f, 0.0f);
    emitter.Drag = 0.0f;
    emitter.Update(0.01f, glm::vec3(0.0f), true);
    const float initialY = emitter.GetInstanceData().front().offset.y;
    emitter.Update(0.5f, glm::vec3(0.0f), false);
    AXIS_CHECK(emitter.GetInstanceData().front().offset.y < initialY - 2.0f);
}

AXIS_TEST_CASE("ParticleSystem honors the emitter active flag")
{
    ParticleEmitter::ClearManagers();
    Scene scene;
    const auto entity = scene.GetRegistry().create();
    scene.AddComponent<InfoComponent>(entity, "Emitter", "test");
    scene.AddComponent<PositionComponent>(entity);
    auto& component = scene.AddComponent<ParticleEmitterComponent>(entity);
    component.emitter.Initialize(4);
    component.emitter.SpawnRate = 1000.0f;
    component.emitter.LifeTime = 10.0f;
    component.isActive = false;

    ParticleSystem system;
    system.Update(scene, 1.0f);
    AXIS_CHECK(component.emitter.GetActiveParticleCount() == 0);
    component.isActive = true;
    system.Update(scene, 1.0f);
    AXIS_CHECK(component.emitter.GetActiveParticleCount() == 4);
}

AXIS_TEST_CASE("Animation graph selects conditions and consumes only used triggers")
{
    AnimationGraph graph;
    graph.enabled = true;
    graph.entryState = 1;
    graph.activeState = 1;
    graph.parameters.push_back({"speed", AnimationParameterType::Float, 2.0f, false, false});
    graph.parameters.push_back({"jump", AnimationParameterType::Trigger, 0.0f, false, true});
    graph.states.push_back({1, "Idle", "idle"});
    graph.states.push_back({2, "Run", "run"});
    graph.states.push_back({3, "Jump", "jump"});
    graph.transitions.push_back({4, 1, 2, 0.2f, false, 0.0f,
                                 {{"speed", AnimationConditionOp::Greater, 0.5f}}});
    graph.transitions.push_back({5, 1, 3, 0.1f, false, 0.0f,
                                 {{"jump", AnimationConditionOp::Triggered, 0.0f}}});

    const auto* selected = AnimationGraphRuntime::SelectTransition(graph, 0.0f);
    AXIS_CHECK(selected != nullptr);
    if (!selected) return;
    AXIS_CHECK(selected->id == 4);
    AnimationGraphRuntime::ConsumeTriggers(graph, *selected);
    AXIS_CHECK(graph.parameters[1].triggerValue);

    graph.parameters[0].floatValue = 0.0f;
    selected = AnimationGraphRuntime::SelectTransition(graph, 0.0f);
    AXIS_CHECK(selected != nullptr);
    if (!selected) return;
    AXIS_CHECK(selected->id == 5);
    AnimationGraphRuntime::ConsumeTriggers(graph, *selected);
    AXIS_CHECK(!graph.parameters[1].triggerValue);
}

AXIS_TEST_CASE("Animation graph combines AND OR XOR NAND NOR XNOR and per-condition NOT")
{
    AnimationGraph graph;
    graph.parameters.push_back({"a", AnimationParameterType::Bool, 0.0f, true, false});
    graph.parameters.push_back({"b", AnimationParameterType::Bool, 0.0f, false, false});
    AnimationGraphTransition transition;
    transition.conditions = {{"a", AnimationConditionOp::IsTrue}, {"b", AnimationConditionOp::IsTrue}};

    transition.conditionLogic = GraphConditionLogic::And;
    AXIS_CHECK(!AnimationGraphRuntime::ConditionsPass(graph, transition));
    transition.conditionLogic = GraphConditionLogic::Or;
    AXIS_CHECK(AnimationGraphRuntime::ConditionsPass(graph, transition));
    transition.conditionLogic = GraphConditionLogic::Xor;
    AXIS_CHECK(AnimationGraphRuntime::ConditionsPass(graph, transition));
    transition.conditionLogic = GraphConditionLogic::Nand;
    AXIS_CHECK(AnimationGraphRuntime::ConditionsPass(graph, transition));
    transition.conditionLogic = GraphConditionLogic::Nor;
    AXIS_CHECK(!AnimationGraphRuntime::ConditionsPass(graph, transition));
    transition.conditionLogic = GraphConditionLogic::Xnor;
    AXIS_CHECK(!AnimationGraphRuntime::ConditionsPass(graph, transition));

    transition.conditions[1].negated = true;
    transition.conditionLogic = GraphConditionLogic::And;
    AXIS_CHECK(AnimationGraphRuntime::ConditionsPass(graph, transition));
    transition.conditionLogic = GraphConditionLogic::Xnor;
    AXIS_CHECK(AnimationGraphRuntime::ConditionsPass(graph, transition));
}

AXIS_TEST_CASE("VFX graph applies only modules connected to output")
{
    ParticleEmitterComponent component;
    component.graph.enabled = true;
    component.graph.nodes.push_back({1, VFXNodeType::Spawn, "Spawn", {}, {}, 42.0f});
    component.graph.nodes.push_back({2, VFXNodeType::Gravity, "Gravity", glm::vec4(0, -3, 0, 0)});
    component.graph.nodes.push_back({3, VFXNodeType::Drag, "Disconnected Drag", {}, {}, 8.0f});
    component.graph.nodes.push_back({4, VFXNodeType::Output, "Output"});
    component.graph.links.push_back({5, 1, 4});
    component.graph.links.push_back({6, 2, 4});

    VFXGraphRuntime::Apply(component);
    AXIS_CHECK_NEAR(component.emitter.SpawnRate, 42.0f, 0.0001f);
    AXIS_CHECK_NEAR(component.emitter.Gravity.y, -3.0f, 0.0001f);
    AXIS_CHECK_NEAR(component.emitter.Drag, 0.0f, 0.0001f);
    AXIS_CHECK(!VFXGraphRuntime::IsNodeActive(component.graph, 3));
}

AXIS_TEST_CASE("VFX graph gates links with logical conditions and consumes active triggers")
{
    ParticleEmitterComponent component;
    component.graph.enabled = true;
    component.graph.parameters.push_back({"quality", AnimationParameterType::Float, 0.25f});
    component.graph.parameters.push_back({"burst", AnimationParameterType::Trigger, 0.0f, false, true});
    component.graph.nodes.push_back({1, VFXNodeType::Spawn, "Spawn", {}, {}, 64.0f});
    component.graph.nodes.push_back({2, VFXNodeType::Output, "Output"});
    VFXGraphLink link{3, 1, 2};
    link.conditionLogic = GraphConditionLogic::And;
    link.conditions = {{"quality", AnimationConditionOp::Greater, 0.5f},
                       {"burst", AnimationConditionOp::Triggered}};
    component.graph.links.push_back(link);

    component.emitter.SpawnRate = 5.0f;
    VFXGraphRuntime::Apply(component);
    AXIS_CHECK_NEAR(component.emitter.SpawnRate, 5.0f, 0.0001f);
    AXIS_CHECK(component.graph.parameters[1].triggerValue);

    component.graph.links[0].conditionLogic = GraphConditionLogic::Or;
    VFXGraphRuntime::Apply(component);
    AXIS_CHECK_NEAR(component.emitter.SpawnRate, 64.0f, 0.0001f);
    AXIS_CHECK(!component.graph.parameters[1].triggerValue);

    component.graph.links[0].conditions[0].negated = true;
    component.graph.links[0].conditions.erase(component.graph.links[0].conditions.begin() + 1);
    component.graph.links[0].conditionLogic = GraphConditionLogic::And;
    AXIS_CHECK(VFXGraphRuntime::ConditionsPass(component.graph, component.graph.links[0]));
}

AXIS_TEST_CASE("Scene octree changes remain incremental after the initial rebuild")
{
    Scene scene;
    scene.InitializeManagers();

    std::vector<entt::entity> dirty;
    AXIS_CHECK(scene.ConsumeOctreeChanges(dirty));
    AXIS_CHECK(dirty.empty());
    AXIS_CHECK(!scene.IsOctreeDirty());

    const entt::entity entity = scene.GetRegistry().create();
    scene.MarkOctreeEntityDirty(entity);
    AXIS_CHECK(scene.IsOctreeDirty());
    AXIS_CHECK(scene.ConsumeOctreeDirtyEventCount() == 1);
    AXIS_CHECK(scene.ConsumeOctreeDirtyEventCount() == 0);
    AXIS_CHECK(!scene.ConsumeOctreeChanges(dirty));
    AXIS_CHECK(dirty.size() == 1);
    AXIS_CHECK(dirty.front() == entity);
    AXIS_CHECK(!scene.IsOctreeDirty());

    scene.SetOctreeDirty(true);
    AXIS_CHECK(scene.ConsumeOctreeChanges(dirty));
    scene.ShutdownManagers();
}

AXIS_TEST_CASE("Spatial culling policy learns the faster backend without an entity threshold")
{
    SpatialCullingPolicy policy;
    for (int frame = 0; frame < 8; ++frame)
    {
        AXIS_CHECK(policy.Select(true) == SpatialCullingMode::Linear);
        policy.RecordSample(SpatialCullingMode::Linear, 4.0f, 100, 100, 0);
    }

    AXIS_CHECK(policy.Select(true) == SpatialCullingMode::Octree);
    policy.RecordSample(SpatialCullingMode::Octree, 100.0f, 100, 10, 100, true);
    AXIS_CHECK(policy.GetMetrics().octreeSamples == 0);
    AXIS_CHECK(policy.Select(true) == SpatialCullingMode::Octree);
    policy.RecordSample(SpatialCullingMode::Octree, 0.5f, 100, 10, 0);
    AXIS_CHECK(policy.GetAutoMode() == SpatialCullingMode::Octree);

    policy.SetMode(SpatialCullingMode::Linear);
    AXIS_CHECK(policy.Select(true) == SpatialCullingMode::Linear);
    policy.SetMode(SpatialCullingMode::Octree);
    AXIS_CHECK(policy.Select(true) == SpatialCullingMode::Octree);
    AXIS_CHECK(policy.Select(false) == SpatialCullingMode::Linear);
}

AXIS_TEST_CASE("Spatial culling policy avoids octree probes during sustained transform churn")
{
    SpatialCullingPolicy policy;
    for (int frame = 0; frame < 16; ++frame)
        policy.RecordSample(SpatialCullingMode::Linear, 1.0f, 100, 100, 100);
    AXIS_CHECK(policy.GetMetrics().dirtyRatio > 0.9f);
    AXIS_CHECK(policy.Select(true) == SpatialCullingMode::Linear);
}

AXIS_TEST_CASE("RuntimeProfiler publishes the last completed frame")
{
    auto& profiler = RuntimeProfiler::Instance();
    profiler.Reset();
    profiler.BeginFrame();
    profiler.SetGpuFrameTime(1.5f);
    profiler.SetCpuFrameTime(2.5f);
    profiler.BeginFrame();
    AXIS_CHECK_NEAR(profiler.GetLastCompletedStats().cpuFrameMs, 2.5f, 0.0001f);
    AXIS_CHECK_NEAR(profiler.GetLastCompletedStats().gpuFrameMs, 1.5f, 0.0001f);
    profiler.Reset();
}

AXIS_TEST_CASE("Failed application initialization rolls back the active process context")
{
    const LogLevel processLogLevel = Logger::GetLogLevel();
    AppConfig config;
    config.headlessMode = true;
    config.loadDefaultAssets = false;
    config.logLevel = LogLevel::None;

    AppBuilder invalidProviders;
    invalidProviders.WithPhysicsWorldFactory([](const AppConfig&) { return std::unique_ptr<IPhysicsWorld>{}; });
    Application failing(std::move(invalidProviders));
    AXIS_CHECK(!failing.Initialize(config));
    AXIS_CHECK(failing.GetLifecycle() == ApplicationLifecycle::Failed);
    AXIS_CHECK(Logger::GetLogLevel() == processLogLevel);

    AppBuilder validProviders;
    validProviders.WithPhysicsWorldFactory(
        [](const AppConfig&) -> std::unique_ptr<IPhysicsWorld> { return std::make_unique<FakePhysicsWorld>(); });
    Application next(std::move(validProviders));
    AXIS_CHECK(next.Initialize(config));
    next.Shutdown();
    AXIS_CHECK(next.GetLifecycle() == ApplicationLifecycle::Stopped);
    AXIS_CHECK(Logger::GetLogLevel() == processLogLevel);
}

AXIS_TEST_CASE("Service registries isolate active application contexts")
{
    ServiceLocator first;
    ServiceLocator second;
    int firstValue = 11;
    int secondValue = 22;
    {
        auto activation = first.Activate();
        ServiceLocator::Instance().Register<int>(&firstValue);
        AXIS_CHECK(ServiceLocator::Instance().Require<int>() == 11);
        {
            auto nested = second.Activate();
            ServiceLocator::Instance().Register<int>(&secondValue);
            AXIS_CHECK(ServiceLocator::Instance().Require<int>() == 22);
        }
        AXIS_CHECK(ServiceLocator::Instance().Require<int>() == 11);
    }
}

AXIS_TEST_CASE("Application service context propagates to worker threads")
{
    ServiceLocator applicationServices;
    int value = 47;
    applicationServices.Register<int>(&value);

    const bool installed = ServiceLocator::SetProcessDefault(&applicationServices);
    int workerValue = -1;
    if (installed)
        workerValue = std::async(std::launch::async, [] { return ServiceLocator::Instance().Require<int>(); }).get();
    ServiceLocator::ClearProcessDefault(&applicationServices);

    AXIS_CHECK(installed);
    AXIS_CHECK(workerValue == 47);
}

AXIS_TEST_CASE("SystemManager shuts initialized systems down in reverse order")
{
    axis_test_support::HeadlessResourceFixture fixture;
    ConfigManager configManager;
    configManager.Initialize(AppConfig{});
    ServiceLocator::Instance().Register<ConfigManager>(&configManager);

    std::vector<std::string> order;
    SystemManager systems;
    systems.RegisterSystem(std::make_unique<ShutdownOrderSystem>("First", order));
    systems.RegisterSystem(std::make_unique<ShutdownOrderSystem>("Second", order));
    systems.Initialize(fixture.resources, 1, 1);
    systems.Shutdown();

    AXIS_CHECK(order.size() == 2);
    AXIS_CHECK(order[0] == "Second");
    AXIS_CHECK(order[1] == "First");
}

AXIS_TEST_CASE("SystemManager does not shut down systems that failed capability admission")
{
    axis_test_support::HeadlessResourceFixture fixture;
    SystemManager systems;
    auto requirement = std::make_unique<GraphicsRequirementSystem>();
    auto* observed = requirement.get();
    systems.RegisterSystem(std::move(requirement));
    systems.Initialize(fixture.resources, 1, 1);
    systems.Shutdown();

    AXIS_CHECK(!observed->enabled);
    AXIS_CHECK(!observed->initialized);
    AXIS_CHECK(!observed->shutdown);
}

AXIS_TEST_CASE("SystemManager dispatches optimization config through the optional module capability")
{
    SystemManager systems;
    auto configurable = std::make_unique<OptimizationAwareSystem>();
    auto* observed = configurable.get();
    systems.RegisterSystem(std::move(configurable));
    AppConfig config;
    config.optimization.particleMaxSpawnPerFrame = 777;
    systems.ApplyOptimizationConfig(config.optimization);
    AXIS_CHECK(observed->observedParticleBudget == 777);
}

AXIS_TEST_CASE("InputManager validates and applies selectable input devices")
{
    FakeWindow window;
    KeyboardManager keyboard(&window);
    MouseManager mouse(&window);
    mouse.UpdateButton(Mouse::Left, static_cast<int>(Action::Press), 0);
    InputManager input(keyboard, mouse, window);
    input.BindAction("key", InputType::Key, static_cast<int>(Key::Space));
    input.BindAction("mouse", InputType::MouseButton, static_cast<int>(Mouse::Left));
    input.BindAction("gamepad", InputType::GamepadButton, static_cast<int>(Gamepad::ButtonA));
    input.BindAction("move_x", InputType::GamepadAxis, static_cast<int>(GamepadAxis::LeftX));

    AXIS_CHECK(!input.SetActiveDevice("missing"));
    AXIS_CHECK(input.SetActiveDevice("keyboard_0"));
    AXIS_CHECK(input.GetAction("key"));
    AXIS_CHECK(!input.GetAction("mouse"));
    AXIS_CHECK(input.SetActiveDevice("mouse_0"));
    AXIS_CHECK(!input.GetAction("key"));
    AXIS_CHECK(input.GetAction("mouse"));
    AXIS_CHECK(!input.SetActiveDevice("0"));
    AXIS_CHECK(input.SetActiveDevice("gamepad_0"));
    AXIS_CHECK(!input.GetAction("key"));
    window.gamepadAPressed = true;
    AXIS_CHECK(input.GetAction("gamepad"));
    AXIS_CHECK(input.GetActionDown("gamepad"));
    input.Update();
    AXIS_CHECK(!input.GetActionDown("gamepad"));
    window.gamepadAPressed = false;
    AXIS_CHECK(input.GetActionUp("gamepad"));
    window.gamepadLeftX = 0.1f;
    AXIS_CHECK_NEAR(input.GetAxis("move_x"), 0.0f, 0.0001f);
    window.gamepadLeftX = 0.575f;
    AXIS_CHECK_NEAR(input.GetAxis("move_x"), 0.5f, 0.0001f);

    AXIS_CHECK(keyboard.IsKeyDown(Key::Space));
    window.spacePressed = false;
    keyboard.Update();
    AXIS_CHECK(keyboard.GetKeyUp(Key::Space));
}

AXIS_TEST_CASE("InputSerializer round trips extended keyboard mouse and gamepad bindings")
{
    FakeWindow window;
    KeyboardManager keyboard(&window);
    MouseManager mouse(&window);
    InputManager source(keyboard, mouse, window);
    source.BindAction("Extended", InputType::Key, static_cast<int>(Key::F25));
    source.BindAction("Extended", InputType::Key, static_cast<int>(Key::KpEqual));
    source.BindAction("Extended", InputType::Key, static_cast<int>(Key::RightSuper));
    source.BindAction("Extended", InputType::MouseButton, static_cast<int>(Mouse::Button8));
    source.BindAction("Extended", InputType::GamepadButton, static_cast<int>(Gamepad::ButtonGuide));
    source.BindAction("Extended", InputType::GamepadAxis, static_cast<int>(GamepadAxis::RightY));

    const auto path = axis_test_support::TempPath("extended_input.axs");
    InputSerializer serializer;
    AXIS_CHECK(serializer.Serialize(path.string(), source));

    InputManager loaded(keyboard, mouse, window);
    AXIS_CHECK(serializer.Deserialize(path.string(), loaded));
    const auto& bindings = loaded.GetActionMap().at("Extended").bindings;
    AXIS_CHECK(bindings.size() == 6);
    auto hasBinding = [&](InputType type, int code) {
        return std::any_of(bindings.begin(), bindings.end(),
                           [&](const InputBinding& binding) { return binding.type == type && binding.code == code; });
    };
    AXIS_CHECK(hasBinding(InputType::Key, static_cast<int>(Key::F25)));
    AXIS_CHECK(hasBinding(InputType::Key, static_cast<int>(Key::KpEqual)));
    AXIS_CHECK(hasBinding(InputType::Key, static_cast<int>(Key::RightSuper)));
    AXIS_CHECK(hasBinding(InputType::MouseButton, static_cast<int>(Mouse::Button8)));
    AXIS_CHECK(hasBinding(InputType::GamepadButton, static_cast<int>(Gamepad::ButtonGuide)));
    AXIS_CHECK(hasBinding(InputType::GamepadAxis, static_cast<int>(GamepadAxis::RightY)));
}

AXIS_TEST_CASE("DataNodeSerializer preserves empty documents and emits deterministic ordering")
{
    DataNodeSerializer serializer;
    const auto emptyPath = axis_test_support::TempPath("empty_data.axs");
    const std::unordered_map<std::string, DataNode> empty;
    AXIS_CHECK(serializer.Serialize(emptyPath.string(), empty));
    std::unordered_map<std::string, DataNode> loadedEmpty;
    AXIS_CHECK(serializer.Deserialize(emptyPath.string(), loadedEmpty));
    AXIS_CHECK(loadedEmpty.empty());

    std::unordered_map<std::string, DataNode> source;
    source["zeta"] = {"2", {{"z", "last"}, {"a", "first"}}};
    source["alpha"] = {"1", {}};
    const auto orderedPath = axis_test_support::TempPath("ordered_data.axs");
    AXIS_CHECK(serializer.Serialize(orderedPath.string(), source));
    std::ifstream file(orderedPath);
    std::stringstream text;
    text << file.rdbuf();
    const std::string output = text.str();
    AXIS_CHECK(output.find("alpha:") < output.find("zeta:"));
    AXIS_CHECK(output.find("a: first") < output.find("z: last"));
}

AXIS_TEST_CASE("MonitorManager resolves stable device ids instead of vector positions")
{
    MonitorManager monitors;
    AXIS_EXPECT_ERROR_LOGS(1);
    AXIS_CHECK(!monitors.Initialize(nullptr));

    auto window = std::make_unique<FakeWindow>();
    auto* windowPtr = window.get();
    window->monitors = {{"External", 7, 2560, 1440, 144, false}, {"Primary", 3, 1920, 1080, 60, true}};
    AXIS_CHECK(monitors.Initialize(std::move(window)));

    AXIS_CHECK(monitors.SetActiveDevice("7"));
    AXIS_CHECK(windowPtr->configuredMonitor == 7);
    AXIS_CHECK(monitors.GetCurrentDevice().id == "7");
    AXIS_CHECK(monitors.GetCurrentDevice().name == "External");
    AXIS_CHECK(!monitors.SetActiveDevice("1"));
    AXIS_CHECK(!monitors.SetActiveDevice("not-a-monitor"));

    AXIS_CHECK(monitors.SetActiveDevice("3"));
    AXIS_CHECK(monitors.GetCurrentDevice().isDefault);
}

AXIS_TEST_CASE("AudioCaptureProcessor sanitizes settings and keeps new pulses alive for their first frame")
{
    AudioCaptureSettings settings;
    settings.inputVolume = -1.0f;
    settings.noiseGate = -1.0f;
    settings.gain = -2.0f;
    settings.attackSeconds = 0.0f;
    settings.releaseSeconds = -1.0f;
    settings.peakDecaySeconds = 0.0f;
    settings.pulseThreshold = 4.0f;
    settings.pulseCooldown = -1.0f;
    settings.pulseDuration = -1.0f;
    AudioCaptureProcessor processor(settings);

    const auto sanitized = processor.GetSettings();
    AXIS_CHECK_NEAR(sanitized.inputVolume, 0.0f, 0.0001f);
    AXIS_CHECK_NEAR(sanitized.noiseGate, 0.0f, 0.0001f);
    AXIS_CHECK_NEAR(sanitized.gain, 0.0f, 0.0001f);
    AXIS_CHECK(sanitized.attackSeconds > 0.0f);
    AXIS_CHECK(sanitized.releaseSeconds > 0.0f);
    AXIS_CHECK(sanitized.peakDecaySeconds > 0.0f);
    AXIS_CHECK_NEAR(sanitized.pulseThreshold, 1.0f, 0.0001f);
    AXIS_CHECK_NEAR(sanitized.pulseCooldown, 0.0f, 0.0001f);
    AXIS_CHECK(sanitized.pulseDuration > 0.0f);

    settings.inputVolume = 1.0f;
    settings.noiseGate = 0.0f;
    settings.gain = 10.0f;
    settings.attackSeconds = 0.05f;
    settings.releaseSeconds = 0.05f;
    settings.peakDecaySeconds = 0.125f;
    settings.calibrationSeconds = 0.1f;
    settings.pulseThreshold = 0.1f;
    settings.pulseCooldown = 1.0f;
    settings.pulseDuration = 0.05f;
    processor.SetSettings(settings);
    processor.BeginCalibration(settings.calibrationSeconds);
    processor.Update(0.025f, true, 0.02f, 0.03f);
    processor.Update(0.075f, true, 0.04f, 0.05f);
    AXIS_CHECK_NEAR(processor.GetSnapshot().level.noiseFloor, 0.035f, 0.0001f);

    processor.BeginCalibration(0.0f);
    processor.SetPulseOrigin(glm::vec3(4.0f, 5.0f, 6.0f));
    processor.Update(0.1f, true, 1.0f, 1.0f);
    AXIS_CHECK(processor.GetSnapshot().pulses.size() == 1);
    if (!processor.GetSnapshot().pulses.empty())
    {
        AXIS_CHECK_NEAR(processor.GetSnapshot().pulses.front().age, 0.0f, 0.0001f);
        AXIS_CHECK_NEAR(processor.GetSnapshot().pulses.front().origin.x, 4.0f, 0.0001f);
        AXIS_CHECK_NEAR(processor.GetSnapshot().pulses.front().origin.y, 5.0f, 0.0001f);
        AXIS_CHECK_NEAR(processor.GetSnapshot().pulses.front().origin.z, 6.0f, 0.0001f);
    }
    processor.Update(0.06f, false, 0.0f, 0.0f);
    AXIS_CHECK(processor.GetSnapshot().pulses.empty());
}

AXIS_TEST_CASE("AudioService retains bounded world-space gameplay pulses")
{
    AudioService audio;
    audio.EmitPulse(glm::vec3(1.0f, 2.0f, 3.0f), 1.5f, 0.5f);
    AXIS_CHECK(audio.GetPulses().size() == 1);
    AXIS_CHECK_NEAR(audio.GetPulses().front().origin.x, 1.0f, 0.0001f);
    AXIS_CHECK_NEAR(audio.GetPulses().front().origin.y, 2.0f, 0.0001f);
    AXIS_CHECK_NEAR(audio.GetPulses().front().origin.z, 3.0f, 0.0001f);
    AXIS_CHECK_NEAR(audio.GetPulses().front().intensity, 1.0f, 0.0001f);
    AXIS_CHECK_NEAR(audio.GetPulses().front().peak, 1.0f, 0.0001f);

    audio.UpdatePulses(0.25f);
    AXIS_CHECK_NEAR(audio.GetPulses().front().age, 0.25f, 0.0001f);
    audio.UpdatePulses(0.25f);
    AXIS_CHECK(audio.GetPulses().empty());

    for (size_t i = 0; i < AudioPulseLimits::MaxPulses + 3; ++i)
        audio.EmitPulse(glm::vec3(static_cast<float>(i), 0.0f, 0.0f));
    AXIS_CHECK(audio.GetPulses().size() == AudioPulseLimits::MaxPulses);
    AXIS_CHECK_NEAR(audio.GetPulses().front().origin.x, 3.0f, 0.0001f);

    audio.EmitPulse(glm::vec3(std::numeric_limits<float>::quiet_NaN()), 1.0f, 1.0f);
    AXIS_CHECK(audio.GetPulses().size() == AudioPulseLimits::MaxPulses);

    audio.ClearPulses();
    audio.EmitTaggedPulse(glm::vec3(4.0f), 0.6f, 1.2f, 3.5f,
                          AudioPulseSource::Prey, 1.25f);
    AXIS_CHECK(audio.GetPulses().size() == 1);
    AXIS_CHECK_NEAR(audio.GetPulses().front().peak, 3.5f, 0.0001f);
    AXIS_CHECK_NEAR(audio.GetPulses().front().padding, 1.0125f, 0.0001f);
}

AXIS_TEST_CASE("AudioCaptureProcessor applies mic input volume and independent response timing")
{
    AudioCaptureSettings settings;
    settings.inputVolume = 2.0f;
    settings.noiseGate = 0.0f;
    settings.gain = 1.0f;
    settings.attackSeconds = 0.001f;
    settings.releaseSeconds = 1.0f;
    settings.peakDecaySeconds = 1.0f;
    settings.pulseThreshold = 1.0f;
    AudioCaptureProcessor processor(settings);

    processor.Update(0.1f, true, 0.25f, 0.4f);
    AXIS_CHECK_NEAR(processor.GetSnapshot().level.rms, 0.5f, 0.001f);
    AXIS_CHECK_NEAR(processor.GetSnapshot().level.peak, 0.8f, 0.001f);

    processor.Update(0.1f, false, 0.0f, 0.0f);
    AXIS_CHECK(processor.GetSnapshot().level.rms > 0.4f);
    AXIS_CHECK(processor.GetSnapshot().level.peak > 0.7f);
}

AXIS_TEST_CASE("ConfigLoader preserves known custom-provider backends and rejects invalid enum text")
{
    AppConfig config;
    {
        std::stringstream line("GRAPHICS_API DIRECTX");
        ConfigLoader::LoadConfig(line, config, false);
    }
    {
        std::stringstream line("PHYSICS_ENGINE PHYSX");
        ConfigLoader::LoadConfig(line, config, false);
    }
    {
        std::stringstream line("AUDIO_ENGINE OPENAL");
        ConfigLoader::LoadConfig(line, config, false);
    }
    {
        std::stringstream line("MIC_INPUT_VOLUME 1.8");
        ConfigLoader::LoadConfig(line, config, false);
    }
    {
        std::stringstream line("MIC_INPUT_THRESHOLD 0.07");
        ConfigLoader::LoadConfig(line, config, false);
    }
    {
        std::stringstream line("SPATIAL_CULLING OCTREE");
        ConfigLoader::LoadConfig(line, config, false);
    }
    AXIS_CHECK(config.graphics.graphicsBackend == GraphicsBackend::DirectX);
    AXIS_CHECK(config.physics.physicsBackend == PhysicsBackend::PhysX);
    AXIS_CHECK(config.audio.audioBackend == AudioBackend::OpenAL);
    AXIS_CHECK_NEAR(config.audio.captureInputVolume, 1.8f, 0.0001f);
    AXIS_CHECK_NEAR(config.audio.captureNoiseGate, 0.07f, 0.0001f);
    AXIS_CHECK(config.culling.spatialCullingMode == SpatialCullingMode::Octree);

    std::stringstream invalid("GRAPHICS_API TYPO_BACKEND");
    ConfigLoader::LoadConfig(invalid, config, false);
    AXIS_CHECK(config.graphics.graphicsBackend == GraphicsBackend::DirectX);

    std::stringstream invalidSpatial("SPATIAL_CULLING TYPO_MODE");
    ConfigLoader::LoadConfig(invalidSpatial, config, false);
    AXIS_CHECK(config.culling.spatialCullingMode == SpatialCullingMode::Octree);
}

AXIS_TEST_CASE("Engine assets resolve from a build output directory")
{
    const auto configPath = FileSystem::getPath("asset://config.axs");
    AXIS_CHECK(std::filesystem::exists(std::filesystem::u8path(configPath)));
}

AXIS_TEST_CASE("DataManager exposes snapshots instead of mutable internal storage")
{
    DataManager dataManager;
    DataNode node;
    node.value = "original";
    dataManager.SetDataNode("player", node);

    auto snapshot = dataManager.GetDataNodes();
    snapshot.at("player").value = "changed";
    AXIS_CHECK(dataManager.GetDataNode("player").value == "original");

    dataManager.ReplaceDataNodes(std::move(snapshot));
    AXIS_CHECK(dataManager.GetDataNode("player").value == "changed");
}

AXIS_TEST_CASE("Unified config loading applies the complete file through registered strategies")
{
    axis_test_support::ResetServices();
    AppConfig initial;
    ConfigManager configManager;
    configManager.Initialize(initial);
    ServiceLocator::Instance().Register<ConfigManager>(&configManager);

    ResourceManager resources;
    resources.InitializeHeadless();
    const auto path =
        axis_test_support::WriteTempFile("unified_config.axs", "axis_config:\n  WINDOW_WIDTH: 1234\n  VOLUME: 37.5\n");

    const auto loaderTypes = resources.GetRegisteredLoaderTypes();
    AXIS_CHECK(std::is_sorted(loaderTypes.begin(), loaderTypes.end()));
    AXIS_CHECK(std::find(loaderTypes.begin(), loaderTypes.end(), "CONFIG") != loaderTypes.end());
    AXIS_CHECK(std::find(loaderTypes.begin(), loaderTypes.end(), "INPUT") != loaderTypes.end());
    AXIS_CHECK(resources.LoadUnified("CONFIG", path.string()));
    const AppConfig loaded = configManager.GetConfig();
    AXIS_CHECK(loaded.window.width == 1234);
    AXIS_CHECK(loaded.audio.masterVolume == 37.5f);

    resources.Shutdown();
    axis_test_support::ResetServices();
}

AXIS_TEST_CASE("Resource initialization preserves a module loader registered before defaults")
{
    axis_test_support::ResetServices();
    bool called = false;
    ResourceManager resources;
    resources.RegisterLoader(std::make_unique<TrackingConfigLoader>(called));
    resources.InitializeHeadless();
    AXIS_CHECK(resources.LoadUnified("CONFIG", "module-owned-config"));
    AXIS_CHECK(called);
    resources.Shutdown();
    axis_test_support::ResetServices();
}

AXIS_TEST_CASE("Custom systems can replace a default factory system by name")
{
    axis_test_support::ResetServices();
    SystemManager systems;
    auto custom = std::make_unique<CustomGeometrySystem>();
    auto* expected = custom.get();
    systems.RegisterSystem(std::move(custom));
    systems.CreateSystems();
    AXIS_CHECK(systems.GetSystem("GeometrySystem") == expected);
    axis_test_support::ResetServices();
}

AXIS_TEST_CASE("System catalog contains built-ins and linked optional modules")
{
    const auto names = SystemFactory::GetRegisteredNames();
    AXIS_CHECK(std::is_sorted(names.begin(), names.end()));
    AXIS_CHECK(std::binary_search(names.begin(), names.end(), "TransformSystem"));
    AXIS_CHECK(std::binary_search(names.begin(), names.end(), "RenderSystem"));
    AXIS_CHECK(std::binary_search(names.begin(), names.end(), "PhysicsSystem"));
#ifdef ENABLE_EDITOR
    AXIS_CHECK(std::binary_search(names.begin(), names.end(), "EditorSystem"));
#else
    AXIS_CHECK(!std::binary_search(names.begin(), names.end(), "EditorSystem"));
#endif
}

AXIS_TEST_CASE("Consumed editor keys remain visible to raw tools and hidden from gameplay")
{
    FakeWindow window;
    window.pressedKeys.insert(Key::W);
    KeyboardManager keyboard(&window);
    AXIS_CHECK(keyboard.GetRawKey(Key::W));
    AXIS_CHECK(keyboard.GetKey(Key::W));
    keyboard.ConsumeKey(Key::W);
    AXIS_CHECK(keyboard.GetRawKey(Key::W));
    AXIS_CHECK(!keyboard.GetKey(Key::W));
    AXIS_CHECK(keyboard.IsKeyConsumed(Key::W));
    keyboard.ReleaseConsumedKey(Key::W);
    AXIS_CHECK(!keyboard.GetKey(Key::W));
    keyboard.EndFrame();
    AXIS_CHECK(!keyboard.IsKeyConsumed(Key::W));
    AXIS_CHECK(keyboard.GetKey(Key::W));
}

AXIS_TEST_CASE("Editor cursor mode restores the exact previous game mode")
{
    FakeWindow window;
    MouseManager mouse(&window);

    mouse.SetCursorMode(CursorMode::LockedHidden);
    AXIS_CHECK(mouse.GetCursorMode() == CursorMode::LockedHidden);
    AXIS_CHECK(window.cursorMode == CursorMode::LockedHidden);

    mouse.ToggleEditorMode();
    AXIS_CHECK(mouse.GetCursorMode() == CursorMode::Editor);
    AXIS_CHECK(mouse.GetModeBeforeEditor() == CursorMode::LockedHidden);
    AXIS_CHECK(window.cursorMode == CursorMode::Normal);

    mouse.UpdateButton(Mouse::Left, 1, 0);
    AXIS_CHECK(mouse.IsEditorButtonPressed(Mouse::Left));
    AXIS_CHECK(mouse.IsEditorMouseClicked(Mouse::Left));
    AXIS_CHECK(!mouse.IsLeftButtonPressed());
    AXIS_CHECK(!mouse.IsLeftMouseClicked());

    mouse.UpdatePosition(100.0, 100.0);
    mouse.UpdatePosition(112.0, 94.0);
    mouse.UpdateScroll(0.0, 2.0);
    AXIS_CHECK(mouse.GetEditorXOffset() == 12.0f);
    AXIS_CHECK(mouse.GetEditorYOffset() == 6.0f);
    AXIS_CHECK(mouse.GetEditorScrollY() == 2.0f);
    AXIS_CHECK(mouse.GetXOffset() == 0.0f);
    AXIS_CHECK(mouse.GetYOffset() == 0.0f);
    AXIS_CHECK(mouse.GetScrollY() == 0.0f);

    // Game code cannot steal the cursor or overwrite the saved mode while the
    // editor owns it.
    mouse.SetCursorMode(CursorMode::Hidden);
    AXIS_CHECK(mouse.GetCursorMode() == CursorMode::Editor);
    AXIS_CHECK(mouse.GetModeBeforeEditor() == CursorMode::LockedHidden);

    mouse.ToggleEditorMode();
    AXIS_CHECK(mouse.GetCursorMode() == CursorMode::LockedHidden);
    AXIS_CHECK(window.cursorMode == CursorMode::LockedHidden);

    mouse.SetCursorMode(CursorMode::Hidden);
    mouse.ToggleEditorMode();
    AXIS_CHECK(mouse.GetModeBeforeEditor() == CursorMode::Hidden);
    mouse.ToggleEditorMode();
    AXIS_CHECK(mouse.GetCursorMode() == CursorMode::Hidden);
    AXIS_CHECK(window.cursorMode == CursorMode::Hidden);
}

AXIS_TEST_CASE("SystemManager resolves systems through stable typed ids")
{
    SystemManager systems;
    auto custom = std::make_unique<CustomGeometrySystem>();
    auto* expected = custom.get();
    systems.RegisterSystem(std::move(custom));

    AXIS_CHECK(systems.GetSystem(SystemId::FromName("GeometrySystem")) == expected);
    AXIS_CHECK(systems.GetSystem(SystemId::FromName("MissingSystem")) == nullptr);
}

AXIS_TEST_CASE("Instance script registration can replace an existing factory by name")
{
    axis_test_support::ResetServices();
    ScriptRegistry scripts;
    scripts.Initialize();
    scripts.Register<DefaultReplaceableScript>("replaceable_script");
    scripts.Register<UserReplaceableScript>("replaceable_script");
    const auto instance = scripts.Create("replaceable_script");
    const auto names = scripts.GetRegisteredNames();
    AXIS_CHECK(dynamic_cast<UserReplaceableScript*>(instance.get()) != nullptr);
    AXIS_CHECK(std::is_sorted(names.begin(), names.end()));
    AXIS_CHECK(std::find(names.begin(), names.end(), "replaceable_script") != names.end());
    axis_test_support::ResetServices();
}

AXIS_TEST_CASE("Built-in scripts are registered without whole-archive static discovery")
{
    ScriptRegistry scripts;
    scripts.Initialize();
    const auto names = scripts.GetRegisteredNames();
    AXIS_CHECK(std::binary_search(names.begin(), names.end(), "DefaultCameraController"));
    AXIS_CHECK(scripts.Create("DefaultCameraController") != nullptr);
}

AXIS_TEST_CASE("Null audio backend preserves pause and validates source playback contracts")
{
    NullAudioEngine audio;
    AXIS_CHECK(audio.Initialize());

    auto paused = audio.Play2D("headless.wav", false, true);
    auto* nullSound = dynamic_cast<NullSound*>(paused.get());
    AXIS_CHECK(nullSound != nullptr);
    AXIS_CHECK(nullSound->IsPaused());
    nullSound->Resume();
    AXIS_CHECK(!nullSound->IsPaused());
    nullSound->Pause();
    AXIS_CHECK(nullSound->IsPaused());
    AXIS_CHECK(audio.Play2D(static_cast<IAudioSource*>(nullptr)) == nullptr);
    AXIS_CHECK(audio.Play3D(static_cast<IAudioSource*>(nullptr), glm::vec3(0.0f)) == nullptr);

    audio.Shutdown();
    AXIS_CHECK(nullSound->IsFinished());
}

AXIS_TEST_CASE("AudioSystem resolves relative component paths for 2D and 3D playback")
{
    axis_test_support::ResetServices();

    Scene scene;
    ConfigManager configManager;
    configManager.Initialize(AppConfig{});
    ServiceLocator::Instance().Register<Scene>(&scene);
    ServiceLocator::Instance().Register<ConfigManager>(&configManager);

    auto recordingEngine = std::make_unique<RecordingAudioEngine>();
    auto* recording = recordingEngine.get();
    AudioService audioService;
    AXIS_CHECK(audioService.Initialize(std::move(recordingEngine)));

    const std::string relativePath = "sample/resource/audio/sample.wav";
    auto entity2D = scene.CreateEntity("Relative2D");
    auto& source2D = scene.AddComponent<AudioSourceComponent>(entity2D);
    source2D.filePath = relativePath;
    source2D.shouldPlay = true;

    auto entity3D = scene.CreateEntity("Relative3D");
    auto& source3D = scene.AddComponent<AudioSourceComponent>(entity3D);
    source3D.filePath = relativePath;
    source3D.is3D = true;
    source3D.shouldPlay = true;

    AudioSystem system;
    system.Initialize();
    system.Update(scene, 0.0f);

    const std::string expected = FileSystem::getPath(relativePath);
    AXIS_CHECK(recording->path2D == expected);
    AXIS_CHECK(recording->path3D == expected);
    AXIS_CHECK(std::filesystem::exists(std::filesystem::u8path(expected)));
    AXIS_CHECK(source2D.sound != nullptr);
    AXIS_CHECK(source3D.sound != nullptr);

    system.Shutdown();
    audioService.Shutdown();
    axis_test_support::ResetServices();
}

AXIS_TEST_CASE("StateMachine exposes an explicit debug render pass")
{
    axis_test_support::ResetServices();
    Scene scene;
    ServiceLocator::Instance().Register<Scene>(&scene);
    StateCounters counters;
    StateMachine machine;
    machine.PushState(std::make_unique<RecordingState>(counters));
    machine.Render();
    machine.RenderDebug();
    AXIS_CHECK(counters.render == 1);
    AXIS_CHECK(counters.debug == 1);
    machine.Shutdown();
    axis_test_support::ResetServices();
}

AXIS_TEST_CASE("StateMachine restores the previous state when OnEnter fails")
{
    axis_test_support::ResetServices();
    Scene scene;
    ServiceLocator::Instance().Register<Scene>(&scene);
    StateCounters counters;
    StateMachine machine;
    machine.PushState(std::make_unique<RecordingState>(counters));

    bool threw = false;
    try
    {
        machine.PushState(std::make_unique<ThrowingEnterState>());
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }

    AXIS_CHECK(threw);
    AXIS_CHECK(dynamic_cast<RecordingState*>(machine.GetCurrentState()) != nullptr);
    AXIS_CHECK(counters.pause == 1);
    AXIS_CHECK(counters.resume == 1);
    machine.Shutdown();
    AXIS_CHECK(counters.exit == 1);
    axis_test_support::ResetServices();
}

AXIS_TEST_CASE("Shader ABI constants and post-process mask stay synchronized")
{
    AXIS_CHECK(ShaderABI::MaxBones == 128);
    AXIS_CHECK(ShaderABI::CameraUBOBinding == 20);
    AXIS_CHECK(ShaderABI::PulseSSBOBinding == 26);
    AXIS_CHECK(ShaderABI::LightTileGridSSBOBinding == 27);
    AXIS_CHECK(ShaderABI::LightTileIndicesSSBOBinding == 28);
    AXIS_CHECK(ShaderABI::EditorSelectionSSBOBinding == 29);
    AXIS_CHECK(ShaderABI::MaxAudioPulses == 64);
    AXIS_CHECK(sizeof(AudioPulse) == 32);
    AXIS_CHECK(HasPostProcessInput(PostProcessInput::Standard, PostProcessInput::AudioPulses));
    AXIS_CHECK(static_cast<uint32_t>(PostProcessInput::Standard) == 63u);
}

AXIS_TEST_CASE("Editor scene snapshots preserve transient entities without changing normal scene saves")
{
    axis_test_support::ResetServices();
    ResourceManager resources;
    resources.InitializeHeadless();
    Scene source;
    const entt::entity entity = source.GetRegistry().create();
    auto& info = source.GetRegistry().emplace<InfoComponent>(entity);
    info.name = "TransientSphere";
    info.sceneName = "scenario";
    info.isTransient = true;
    source.GetRegistry().emplace<PositionComponent>(entity);
    source.GetRegistry().emplace<RotationComponent>(entity);
    source.GetRegistry().emplace<ScaleComponent>(entity);
    source.GetRegistry().emplace<HierarchyComponent>(entity);
    source.GetRegistry().emplace<WorldTransformComponent>(entity);

    SceneSerializer serializer(resources, nullptr, nullptr);
    const std::string normalSave = serializer.SerializeToString(source);
    const std::string editorSnapshot = serializer.SerializeToString(source, "", true);
    AXIS_CHECK(normalSave.find("TransientSphere") == std::string::npos);
    AXIS_CHECK(editorSnapshot.find("TransientSphere") != std::string::npos);
    AXIS_CHECK(editorSnapshot.find("Transient: true") != std::string::npos);

    Scene restored;
    SceneLoadResult result;
    AXIS_CHECK(serializer.DeserializeFromString(editorSnapshot, "editor_snapshot", restored, result));
    AXIS_CHECK(result.entities.size() == 1);
    AXIS_CHECK(restored.GetComponent<InfoComponent>(result.entities.front()).isTransient);
    resources.Shutdown();
    axis_test_support::ResetServices();
}

AXIS_TEST_CASE("Prefab serialization strips instance namespaces from entity trees")
{
    axis_test_support::ResetServices();
    ResourceManager resources;
    resources.InitializeHeadless();
    Scene scene;
    const entt::entity anchor = scene.CreateEmptyEntity("House Instance");
    const entt::entity root = scene.CreateEmptyEntity("House Instance.Root");
    const entt::entity child = scene.CreateEmptyEntity("House Instance.Root.Window");
    scene.SetParent(root, anchor, false);
    scene.SetParent(child, root, false);

    SceneSerializer serializer(resources, nullptr, nullptr);
    const std::string prefab =
        serializer.SerializeEntitiesToString(scene, {root}, "House Instance.");
    AXIS_CHECK(prefab.starts_with("Entities:\n"));
    AXIS_CHECK(prefab.find("  Root:") != std::string::npos);
    AXIS_CHECK(prefab.find("House Instance.") == std::string::npos);
    AXIS_CHECK(prefab.find("Window:") != std::string::npos);
    resources.Shutdown();
    axis_test_support::ResetServices();
}

AXIS_TEST_CASE("Post-process registry owns effects by module and orders them deterministically")
{
    PostProcessSystem registry;
    const auto late =
        registry.RegisterEffect({"module-b", "late", "shader-b", 0, 0, 0, 0, 100, false, PostProcessInput::Color});
    const auto early =
        registry.RegisterEffect({"module-a", "early", "shader-a", 0, 0, 0, 0, -10, false, PostProcessInput::Standard});
    const auto effects = registry.GetRegisteredEffects();
    AXIS_CHECK(late != 0);
    AXIS_CHECK(early != 0);
    AXIS_CHECK(effects.size() == 2);
    AXIS_CHECK(effects[0].handle == early);
    AXIS_CHECK(registry.RegisterEffect({"module-a", "early", "duplicate"}) == 0);
    AXIS_CHECK(registry.RegisterEffect({"module-c", "invalid-rect", "shader-c", 0, 0, -1, 1}) == 0);
    AXIS_CHECK(registry.UnregisterOwner("module-a") == 1);
    AXIS_CHECK(registry.GetRegisteredEffects().size() == 1);
}

AXIS_TEST_CASE("Async model publication adopts decoded CPU data without replacing resource identity")
{
    static_assert(!std::is_copy_constructible_v<Mesh>);
    static_assert(std::is_nothrow_move_constructible_v<Mesh>);
    Model published;
    published.SetName("stable-resource-handle");

    Model decoded;
    decoded.SetName("worker-temporary");
    decoded.directory = "asset://models/test";
    decoded.gammaCorrection = true;
    std::vector<uint8_t> vertices(sizeof(float) * 8, 0);
    const glm::vec3 expectedPosition(2.0f, 3.0f, 4.0f);
    std::memcpy(vertices.data(), &expectedPosition, sizeof(expectedPosition));
    decoded.meshes.emplace_back(std::move(vertices), 1, sizeof(float) * 8, false,
                                std::vector<unsigned int>{0}, std::vector<Texture>{}, false);

    published.AdoptCpuData(std::move(decoded));

    AXIS_CHECK(published.GetName() == "stable-resource-handle");
    AXIS_CHECK(published.directory == "asset://models/test");
    AXIS_CHECK(published.gammaCorrection);
    AXIS_CHECK(published.meshes.size() == 1);
    AXIS_CHECK(glm::all(glm::equal(published.meshes.front().GetPosition(0), expectedPosition)));
    AXIS_CHECK(decoded.meshes.empty());
    AXIS_CHECK(!published.IsReadyToRender());
}

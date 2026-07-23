#include <editor/modules/camera_editor_module.h>
#include <ecs/unit/core_components.h>

#ifdef ENABLE_EDITOR

#include <core/logic/service_locator.h>
#include <ecs/interface/i_script_registry.h>
#include <ecs/unit/script_component.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <scene/logic/scene.h>

namespace
{
constexpr const char* kDebugCameraName = "Debug Camera";
constexpr const char* kDebugCameraTag = "Editor";
constexpr const char* kDefaultCameraController = "DefaultCameraController";

void ResetScriptInstance(ScriptComponent* script)
{
    if (!script)
        return;
    script->instance.reset();
    script->scriptableInstance = nullptr;
    script->inputScriptableInstance = nullptr;
}

void AttachDebugCameraScript(Scene& scene, entt::entity camera)
{
    auto& script = scene.GetOrAddComponent<ScriptComponent>(camera);
    script.className = kDefaultCameraController;
    script.InstantiateScript = []() {
        auto* registry = ServiceLocator::Instance().Resolve<IScriptRegistry>();
        return registry ? registry->Create(kDefaultCameraController) : nullptr;
    };
    script.DestroyScript = ResetScriptInstance;

    if (!script.instance)
    {
        script.instance = script.InstantiateScript ? script.InstantiateScript() : nullptr;
        if (script.instance)
        {
            script.instance->Initialize(camera, &scene);
            script.instance->OnCreate();
        }
    }
}

void SetDebugCameraScriptEnabled(Scene& scene, entt::entity camera, bool enabled)
{
    if (!scene.IsValid(camera))
        return;
    if (auto* script = scene.TryGetComponent<ScriptComponent>(camera); script && script->instance)
        script->instance->SetEnabled(enabled);
}

entt::entity FindFallbackGameCamera(Scene& scene, entt::entity debugCamera)
{
    auto cameras = scene.View<CameraComponent>();
    for (auto entity : cameras)
    {
        if (entity != debugCamera)
            return entity;
    }
    return entt::null;
}
}  // namespace

void CameraEditorModule::Shutdown()
{
    if (auto* scene = ServiceLocator::Instance().Resolve<Scene>())
        RestoreGameCamera(*scene);
}

void CameraEditorModule::ProcessInput(KeyboardManager& keyboard)
{
    if (!m_Enabled)
        return;

    ProcessKey(keyboard, Key::F11, m_F11Pressed, [this, &keyboard]() {
        if (keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift))
            ToggleDebugCamera();
    });
}

void CameraEditorModule::SetEnabled(bool enabled)
{
    if (m_Enabled == enabled)
        return;

    if (!enabled)
    {
        if (auto* scene = ServiceLocator::Instance().Resolve<Scene>())
            RestoreGameCamera(*scene);
    }
    m_Enabled = enabled;
}

entt::entity CameraEditorModule::EnsureDebugCamera(Scene& scene)
{
    if (m_DebugCameraScene != &scene)
    {
        m_DebugCamera = entt::null;
        m_DebugCameraScene = &scene;
    }
    if (scene.IsValid(m_DebugCamera) && scene.HasAllComponents<CameraComponent>(m_DebugCamera))
        return m_DebugCamera;

    auto& registry = scene.GetRegistry();
    m_DebugCamera = scene.CreateEntity(kDebugCameraName, kDebugCameraTag);

    auto& position = registry.get_or_emplace<PositionComponent>(m_DebugCamera);
    auto& rotation = registry.get_or_emplace<RotationComponent>(m_DebugCamera);
    (void)registry.get_or_emplace<ScaleComponent>(m_DebugCamera);
    (void)registry.get_or_emplace<WorldTransformComponent>(m_DebugCamera);

    if (scene.IsValid(m_LastActiveCamera))
    {
        if (auto* sourcePosition = registry.try_get<PositionComponent>(m_LastActiveCamera))
            position.value = sourcePosition->value;
        if (auto* sourceRotation = registry.try_get<RotationComponent>(m_LastActiveCamera))
            rotation.value = sourceRotation->value;
    }
    else
    {
        position.value = glm::vec3(0.0f, 5.0f, 10.0f);
    }
    position.prev = position.value;
    rotation.prev = rotation.value;

    auto& camera = registry.get_or_emplace<CameraComponent>(m_DebugCamera);
    camera.isPrimary = false;
    if (scene.IsValid(m_LastActiveCamera))
    {
        if (auto* sourceCamera = registry.try_get<CameraComponent>(m_LastActiveCamera))
        {
            camera.fov = sourceCamera->fov;
            camera.nearPlane = sourceCamera->nearPlane;
            camera.farPlane = sourceCamera->farPlane * 20.0f;
            camera.screenWidth = sourceCamera->screenWidth;
            camera.screenHeight = sourceCamera->screenHeight;
            camera.aspectRatio = sourceCamera->aspectRatio;
            camera.isOrthographic = sourceCamera->isOrthographic;
            camera.orthoSize = sourceCamera->orthoSize;
        }
    }
    else
    {
        camera.fov = 45.0f;
        camera.nearPlane = 0.1f;
        camera.farPlane = 10000.0f;
        auto& monitor = ServiceLocator::Instance().Require<IOHandler>().GetMonitorManager();
        camera.screenWidth = monitor.GetWidth();
        camera.screenHeight = monitor.GetHeight();
        camera.aspectRatio = camera.screenHeight > 0
                                 ? static_cast<float>(camera.screenWidth) / static_cast<float>(camera.screenHeight)
                                 : 16.0f / 9.0f;
    }

    AttachDebugCameraScript(scene, m_DebugCamera);
    return m_DebugCamera;
}

void CameraEditorModule::ActivateDebugCamera(Scene& scene)
{
    if (m_DebugCameraScene != &scene)
    {
        if (m_RestoreCursorMode)
        {
            if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>())
            {
                auto& mouse = io->GetMouse();
                if (mouse.IsEditorMode())
                    mouse.ExitEditorMode();
            }
        }
        m_IsDebugCameraActive = false;
        m_LastActiveCamera = entt::null;
        m_DebugCamera = entt::null;
        m_DebugCameraScene = &scene;
        m_RestoreCursorMode = false;
    }
    m_LastActiveCamera = scene.GetActiveCamera();

    if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>())
    {
        auto& mouse = io->GetMouse();
        m_RestoreCursorMode = !mouse.IsEditorMode();
        if (m_RestoreCursorMode)
        {
            mouse.EnterEditorMode();
        }
    }

    const entt::entity debugCamera = EnsureDebugCamera(scene);
    auto& registry = scene.GetRegistry();
    if (scene.IsValid(m_LastActiveCamera))
    {
        if (auto* sourcePosition = registry.try_get<PositionComponent>(m_LastActiveCamera))
        {
            auto& position = registry.get<PositionComponent>(debugCamera);
            position.value = sourcePosition->value;
            position.prev = position.value;
        }
        if (auto* sourceRotation = registry.try_get<RotationComponent>(m_LastActiveCamera))
        {
            auto& rotation = registry.get<RotationComponent>(debugCamera);
            rotation.value = sourceRotation->value;
            rotation.prev = rotation.value;
        }
    }

    AttachDebugCameraScript(scene, debugCamera);
    SetDebugCameraScriptEnabled(scene, debugCamera, true);
    scene.SetActiveCamera(debugCamera);
    m_IsDebugCameraActive = true;
}

void CameraEditorModule::RestoreGameCamera(Scene& scene)
{
    if (!m_IsDebugCameraActive)
        return;

    if (m_DebugCameraScene != &scene)
    {
        if (m_RestoreCursorMode)
        {
            if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>())
            {
                auto& mouse = io->GetMouse();
                if (mouse.IsEditorMode())
                    mouse.ExitEditorMode();
            }
        }
        m_IsDebugCameraActive = false;
        m_LastActiveCamera = entt::null;
        m_DebugCamera = entt::null;
        m_DebugCameraScene = &scene;
        m_RestoreCursorMode = false;
        return;
    }

    SetDebugCameraScriptEnabled(scene, m_DebugCamera, false);

    entt::entity restoreCamera = m_LastActiveCamera;
    if (!scene.IsValid(restoreCamera) || !scene.HasAllComponents<CameraComponent>(restoreCamera))
        restoreCamera = FindFallbackGameCamera(scene, m_DebugCamera);
    scene.SetActiveCamera(restoreCamera);

    if (m_RestoreCursorMode)
    {
        if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>())
        {
            auto& mouse = io->GetMouse();
            if (mouse.IsEditorMode())
                mouse.ExitEditorMode();
        }
    }

    m_IsDebugCameraActive = false;
    m_LastActiveCamera = entt::null;
    m_RestoreCursorMode = false;
}

void CameraEditorModule::ToggleDebugCamera()
{
    auto& scene = ServiceLocator::Instance().Require<Scene>();
    if (m_IsDebugCameraActive && m_DebugCameraScene != &scene)
        RestoreGameCamera(scene);

    if (m_IsDebugCameraActive)
        RestoreGameCamera(scene);
    else
        ActivateDebugCamera(scene);
}

void CameraEditorModule::ProcessKey(KeyboardManager& keyboard, Key key, bool& pressedState,
                                    std::function<void()> action)
{
    if (keyboard.GetKey(key))
    {
        if (!pressedState)
        {
            action();
            pressedState = true;
        }
    }
    else
    {
        pressedState = false;
    }
}

#endif

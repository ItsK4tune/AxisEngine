#include <editor/modules/camera_editor_module.h>
#include <ecs/unit/core_components.h>

#ifdef ENABLE_EDITOR

#include <core/app/application.h>
#include <core/app/runtime_core.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <script/logic/default_camera_controller.h>
#include <script/logic/script_registry.h>
#include <vector>

namespace
{
constexpr const char* kDebugCameraName = "Debug Camera";
constexpr const char* kDebugCameraTag = "Debug";
constexpr const char* kDefaultCameraController = "DefaultCameraController";

bool IsDebugCamera(Scene& scene, entt::entity entity)
{
    if (!scene.IsValid(entity) || !scene.HasAllComponents<CameraComponent>(entity))
        return false;

    auto* info = scene.TryGetComponent<InfoComponent>(entity);
    return info && (info->name == kDebugCameraName || info->tag == kDebugCameraTag);
}

entt::entity FindDebugCamera(Scene& scene)
{
    auto view = scene.View<CameraComponent, InfoComponent>();
    for (auto entity : view)
    {
        auto& info = view.get<InfoComponent>(entity);
        if (info.name == kDebugCameraName || info.tag == kDebugCameraTag)
            return entity;
    }
    return entt::null;
}

void RemoveDuplicateDebugCameras(Scene& scene, entt::entity keep)
{
    std::vector<entt::entity> duplicates;
    auto view = scene.View<CameraComponent, InfoComponent>();
    for (auto entity : view)
    {
        if (entity == keep)
            continue;

        auto& info = view.get<InfoComponent>(entity);
        if (info.name == kDebugCameraName || info.tag == kDebugCameraTag)
            duplicates.push_back(entity);
    }

    auto* sceneMgr = ServiceLocator::Instance().Resolve<SceneManager>();
    for (auto entity : duplicates)
    {
        if (scene.IsValid(entity))
            scene.DestroyEntity(entity, sceneMgr);
    }
}

void ResetScriptInstance(ScriptComponent* sc)
{
    if (!sc)
        return;
    sc->instance.reset();
    sc->scriptableInstance = nullptr;
    sc->inputScriptableInstance = nullptr;
}

void AttachDebugCameraScript(Scene& scene, entt::entity camera)
{
    auto& scriptComp = scene.GetOrAddComponent<ScriptComponent>(camera);
    scriptComp.className = kDefaultCameraController;
    scriptComp.InstantiateScript = []() {
        return ServiceLocator::Instance().Require<ScriptRegistry>().Create(kDefaultCameraController);
    };
    scriptComp.DestroyScript = ResetScriptInstance;

    if (!scriptComp.instance)
    {
        scriptComp.instance = scriptComp.InstantiateScript ? scriptComp.InstantiateScript() : nullptr;
        if (scriptComp.instance)
        {
            scriptComp.instance->Initialize(camera, &scene);
            scriptComp.instance->OnCreate();
        }
    }
}
}  // namespace

CameraEditorModule::CameraEditorModule()
{
}
CameraEditorModule::~CameraEditorModule()
{
}

void CameraEditorModule::Initialize()
{
}
void CameraEditorModule::OnUpdate(float dt)
{
    if (m_IsDebugCameraActive)
    {
        auto& scene = ServiceLocator::Instance().Require<Scene>();
        if (scene.IsValid(m_DebugCamera))
        {
            if (auto* sc = scene.TryGetComponent<ScriptComponent>(m_DebugCamera))
            {
                if (sc->instance)
                {
                    sc->instance->OnUpdate(dt);
                }
            }
        }
    }
}
void CameraEditorModule::Render(Scene& scene)
{
}

void CameraEditorModule::ProcessInput(KeyboardManager& keyboard)
{
    if (!m_Enabled)
        return;

    ProcessKey(keyboard, Key::F11, m_F11Pressed, [this, &keyboard]() {
        if (keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift))
        {
            ToggleDebugCamera();
        }
    });
}

void CameraEditorModule::ToggleDebugCamera()
{
    auto& scene = ServiceLocator::Instance().Require<Scene>();
    auto& registry = scene.GetRegistry();
    if (!IsDebugCamera(scene, m_DebugCamera))
        m_DebugCamera = FindDebugCamera(scene);
    if (m_DebugCamera != entt::null)
        RemoveDuplicateDebugCameras(scene, m_DebugCamera);

    if (m_IsDebugCameraActive)
    {
        if (registry.valid(m_DebugCamera) && registry.all_of<CameraComponent>(m_DebugCamera))
        {
            registry.get<CameraComponent>(m_DebugCamera).isPrimary = false;
        }

        if (registry.valid(m_LastActiveCamera) && registry.all_of<CameraComponent>(m_LastActiveCamera))
        {
            registry.get<CameraComponent>(m_LastActiveCamera).isPrimary = true;
        }
        else
        {
            auto view = registry.view<CameraComponent>();
            for (auto entity : view)
            {
                if (entity != m_DebugCamera)
                {
                    view.get<CameraComponent>(entity).isPrimary = true;
                    break;
                }
            }
        }
        m_IsDebugCameraActive = false;
    }
    else
    {
        m_LastActiveCamera = scene.GetActiveCamera();
        if (registry.valid(m_LastActiveCamera))
        {
            if (registry.all_of<CameraComponent>(m_LastActiveCamera))
                registry.get<CameraComponent>(m_LastActiveCamera).isPrimary = false;
        }

        if (!registry.valid(m_DebugCamera))
        {
            m_DebugCamera = scene.CreateEntity(kDebugCameraName, kDebugCameraTag);
            auto& info = registry.get<InfoComponent>(m_DebugCamera);
            info.name = kDebugCameraName;
            info.tag = kDebugCameraTag;

            auto& posComp = registry.get_or_emplace<PositionComponent>(m_DebugCamera);
            auto& rotComp = registry.get_or_emplace<RotationComponent>(m_DebugCamera);
            auto& scaleComp = registry.get_or_emplace<ScaleComponent>(m_DebugCamera);
            auto& worldComp = registry.get_or_emplace<WorldTransformComponent>(m_DebugCamera);
            (void)scaleComp;
            (void)worldComp;

            if (registry.valid(m_LastActiveCamera) && registry.all_of<PositionComponent>(m_LastActiveCamera))
            {
                posComp.value = registry.get<PositionComponent>(m_LastActiveCamera).value;
            }
            else
            {
                posComp.value = glm::vec3(0.0f, 5.0f, 10.0f);
            }

            posComp.prev = posComp.value;

            auto& cam = registry.get_or_emplace<CameraComponent>(m_DebugCamera);
            cam.isPrimary = true;

            if (registry.valid(m_LastActiveCamera) && registry.all_of<CameraComponent>(m_LastActiveCamera))
            {
                auto& lastCam = registry.get<CameraComponent>(m_LastActiveCamera);
                cam.fov = lastCam.fov;
                cam.nearPlane = lastCam.nearPlane;
                cam.farPlane = lastCam.farPlane * 20.0f; // Multiplied by 20 to ensure it is much further than gameplay camera
                cam.screenWidth = lastCam.screenWidth;
                cam.screenHeight = lastCam.screenHeight;
                cam.aspectRatio = lastCam.aspectRatio;
                cam.isOrthographic = lastCam.isOrthographic;
                cam.orthoSize = lastCam.orthoSize;
            }
            else
            {
                cam.fov = 45.0f;
                cam.nearPlane = 0.1f;
                cam.farPlane = 10000.0f;
                auto& mm = ServiceLocator::Instance().Require<IOHandler>().GetMonitorManager();
                cam.screenWidth = mm.GetWidth();
                cam.screenHeight = mm.GetHeight();
                cam.aspectRatio = (float)cam.screenWidth / (float)cam.screenHeight;
            }

            rotComp.prev = rotComp.value;
            AttachDebugCameraScript(scene, m_DebugCamera);
        }
        else if (registry.all_of<CameraComponent>(m_DebugCamera))
        {
            registry.get<CameraComponent>(m_DebugCamera).isPrimary = true;
            if (auto* info = registry.try_get<InfoComponent>(m_DebugCamera))
            {
                info->name = kDebugCameraName;
                info->tag = kDebugCameraTag;
            }
            if (registry.valid(m_LastActiveCamera) && registry.all_of<PositionComponent>(m_LastActiveCamera))
            {
                registry.get<PositionComponent>(m_DebugCamera).value =
                    registry.get<PositionComponent>(m_LastActiveCamera).value;
                if (registry.all_of<RotationComponent>(m_LastActiveCamera) &&
                    registry.all_of<RotationComponent>(m_DebugCamera))
                    registry.get<RotationComponent>(m_DebugCamera).value =
                        registry.get<RotationComponent>(m_LastActiveCamera).value;
            }
            AttachDebugCameraScript(scene, m_DebugCamera);
        }
        m_IsDebugCameraActive = true;
    }
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
        pressedState = false;
}

#endif

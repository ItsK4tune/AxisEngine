#include <editor/modules/camera_editor_module.h>
#include <ecs/unit/core_components.h>

#ifdef ENABLE_EDITOR

#include <core/app/application.h>
#include <core/app/runtime_core.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/entity_manager.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <scene/logic/scene.h>
#include <script/logic/default_camera_controller.h>
#include <script/logic/script_registry.h>

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
    auto& registry = scene.registry;

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
        m_LastActiveCamera = EntityManager::GetActiveCamera(scene);
        if (registry.valid(m_LastActiveCamera))
        {
            if (registry.all_of<CameraComponent>(m_LastActiveCamera))
                registry.get<CameraComponent>(m_LastActiveCamera).isPrimary = false;
        }

        if (!registry.valid(m_DebugCamera))
        {
            m_DebugCamera = EntityManager::CreateEntity(scene, "Debug_Camera");
            registry.emplace<InfoComponent>(m_DebugCamera, "Debug Camera", "Debug");

            auto& posComp = registry.emplace<PositionComponent>(m_DebugCamera);
            registry.emplace<RotationComponent>(m_DebugCamera);
            registry.emplace<ScaleComponent>(m_DebugCamera);
            registry.emplace<WorldTransformComponent>(m_DebugCamera);

            if (registry.valid(m_LastActiveCamera) && registry.all_of<PositionComponent>(m_LastActiveCamera))
            {
                posComp.value = registry.get<PositionComponent>(m_LastActiveCamera).value;
            }
            else
            {
                posComp.value = glm::vec3(0.0f, 5.0f, 10.0f);
            }

            auto& cam = registry.emplace<CameraComponent>(m_DebugCamera);
            cam.isPrimary = true;

            if (registry.valid(m_LastActiveCamera) && registry.all_of<CameraComponent>(m_LastActiveCamera))
            {
                auto& lastCam = registry.get<CameraComponent>(m_LastActiveCamera);
                cam.fov = lastCam.fov;
                cam.nearPlane = lastCam.nearPlane;
                cam.farPlane = lastCam.farPlane;
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
                cam.farPlane = 1000.0f;
                auto& mm = ServiceLocator::Instance().Require<IOHandler>().GetMonitorManager();
                cam.screenWidth = mm.GetWidth();
                cam.screenHeight = mm.GetHeight();
                cam.aspectRatio = (float)cam.screenWidth / (float)cam.screenHeight;
            }

            auto scriptInstance =
                ServiceLocator::Instance().Require<ScriptRegistry>().Create("DefaultCameraController");
            if (scriptInstance)
            {
                auto& scriptComp = registry.emplace<ScriptComponent>(m_DebugCamera);
                scriptComp.instance = std::move(scriptInstance);
                scriptComp.InstantiateScript = []() {
                    return ServiceLocator::Instance().Require<ScriptRegistry>().Create("DefaultCameraController");
                };
                scriptComp.DestroyScript = [](ScriptComponent* nsc) { nsc->instance.reset(); };
                scriptComp.instance->Initialize(m_DebugCamera, &scene);
                scriptComp.instance->OnCreate();
            }
        }
        else if (registry.all_of<CameraComponent>(m_DebugCamera))
        {
            registry.get<CameraComponent>(m_DebugCamera).isPrimary = true;
            if (registry.valid(m_LastActiveCamera) && registry.all_of<PositionComponent>(m_LastActiveCamera))
            {
                registry.get<PositionComponent>(m_DebugCamera).value =
                    registry.get<PositionComponent>(m_LastActiveCamera).value;
                if (registry.all_of<RotationComponent>(m_LastActiveCamera) &&
                    registry.all_of<RotationComponent>(m_DebugCamera))
                    registry.get<RotationComponent>(m_DebugCamera).value =
                        registry.get<RotationComponent>(m_LastActiveCamera).value;
            }
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

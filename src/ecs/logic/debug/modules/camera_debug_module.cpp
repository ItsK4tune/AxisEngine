#include <ecs/unit/core_components.h>
#include <ecs/logic/debug/modules/camera_debug_module.h>

#ifdef ENABLE_EDITOR

#include <core/app/application.h>
#include <platform/logic/input_manager.h>
#include <script/logic/script_registry.h>
#include <script/logic/default_camera_controller.h>
#include <ecs/logic/entity_manager.h>
#include <core/logic/service_locator.h>
#include <scene/logic/scene.h>
#include <core/app/runtime_core.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <core/logic/logger.h>

CameraDebugModule::CameraDebugModule() {}
CameraDebugModule::~CameraDebugModule() {}

void CameraDebugModule::Initialize() {}
void CameraDebugModule::OnUpdate(float dt) {}
void CameraDebugModule::Render(Scene &scene) {}

void CameraDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_Enabled) return;

    ProcessKey(keyboard, Key::F11, m_F11Pressed, [this, &keyboard]() {
        if (keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift)) {
            ToggleDebugCamera();
        }
    });
}

void CameraDebugModule::ToggleDebugCamera()
{
    auto& scene = ServiceLocator::Instance().Require<Scene>();
    auto &registry = scene.registry;

    if (m_IsDebugCameraActive)
    {
        if (registry.valid(m_DebugCamera) && registry.all_of<CameraComponent>(m_DebugCamera)) {
            registry.get<CameraComponent>(m_DebugCamera).isPrimary = false;
        }

        if (registry.valid(m_LastActiveCamera) && registry.all_of<CameraComponent>(m_LastActiveCamera)) {
            registry.get<CameraComponent>(m_LastActiveCamera).isPrimary = true;
            LOGGER_INFO("Debug") << "Switched to User Camera (Entity " << (uint32_t)m_LastActiveCamera << ")";
        }
        else {
            auto view = registry.view<CameraComponent>();
            for (auto entity : view) {
                if (entity != m_DebugCamera) {
                    view.get<CameraComponent>(entity).isPrimary = true;
                    LOGGER_INFO("Debug") << "Original camera missing. Switched to fallback (Entity " << (uint32_t)entity << ")";
                    break;
                }
            }
        }
        m_IsDebugCameraActive = false;
        LOGGER_INFO("Debug") << "Debug Camera: OFF";
    }
    else
    {
        m_LastActiveCamera = EntityManager::GetActiveCamera(scene);
        if (registry.valid(m_LastActiveCamera)) {
            if (registry.all_of<CameraComponent>(m_LastActiveCamera))
                registry.get<CameraComponent>(m_LastActiveCamera).isPrimary = false;
        }

        if (!registry.valid(m_DebugCamera))
        {
            m_DebugCamera = EntityManager::CreateEntity(scene, "Debug_Camera");
            registry.emplace<InfoComponent>(m_DebugCamera, "Debug Camera", "Debug");

            auto &posComp = registry.emplace<PositionComponent>(m_DebugCamera);
            registry.emplace<RotationComponent>(m_DebugCamera);
            registry.emplace<ScaleComponent>(m_DebugCamera);
            registry.emplace<WorldTransformComponent>(m_DebugCamera);

            if (registry.valid(m_LastActiveCamera) && registry.all_of<PositionComponent>(m_LastActiveCamera)) {
                posComp.value = registry.get<PositionComponent>(m_LastActiveCamera).value;
            } else {
                posComp.value = glm::vec3(0.0f, 5.0f, 10.0f);
            }

            auto &cam = registry.emplace<CameraComponent>(m_DebugCamera);
            cam.isPrimary = true;
            
            if (registry.valid(m_LastActiveCamera) && registry.all_of<CameraComponent>(m_LastActiveCamera)) {
                auto &lastCam = registry.get<CameraComponent>(m_LastActiveCamera);
                cam.fov = lastCam.fov; cam.nearPlane = lastCam.nearPlane; cam.farPlane = lastCam.farPlane;
                cam.screenWidth = lastCam.screenWidth; cam.screenHeight = lastCam.screenHeight;
                cam.aspectRatio = lastCam.aspectRatio; cam.isOrthographic = lastCam.isOrthographic;
                cam.orthoSize = lastCam.orthoSize;
                
                LOGGER_DEBUG("Debug") << "Inherited Camera Props: " << cam.screenWidth << "x" << cam.screenHeight;
            } else {
                cam.fov = 45.0f; cam.nearPlane = 0.1f; cam.farPlane = 1000.0f;
                auto& mm = ServiceLocator::Instance().Require<IOHandler>().GetMonitorManager();
                cam.screenWidth = mm.GetWidth(); cam.screenHeight = mm.GetHeight();
                cam.aspectRatio = (float)cam.screenWidth / (float)cam.screenHeight;
            }

            auto scriptInstance = ServiceLocator::Instance().Require<ScriptRegistry>().Create("DefaultCameraController");
            if (scriptInstance) {
                auto &scriptComp = registry.emplace<ScriptComponent>(m_DebugCamera);
                scriptComp.instance = std::move(scriptInstance);
                scriptComp.InstantiateScript = []() { return ServiceLocator::Instance().Require<ScriptRegistry>().Create("DefaultCameraController"); };
                scriptComp.DestroyScript = [](ScriptComponent *nsc) { nsc->instance.reset(); };
                scriptComp.instance->Initialize(m_DebugCamera, &scene);
                scriptComp.instance->OnCreate();
            }
        }
        else if (registry.all_of<CameraComponent>(m_DebugCamera))
        {
            registry.get<CameraComponent>(m_DebugCamera).isPrimary = true;
            if (registry.valid(m_LastActiveCamera) && registry.all_of<PositionComponent>(m_LastActiveCamera)) {
                registry.get<PositionComponent>(m_DebugCamera).value = registry.get<PositionComponent>(m_LastActiveCamera).value;
                if (registry.all_of<RotationComponent>(m_LastActiveCamera) && registry.all_of<RotationComponent>(m_DebugCamera))
                    registry.get<RotationComponent>(m_DebugCamera).value = registry.get<RotationComponent>(m_LastActiveCamera).value;
            }
        }
        m_IsDebugCameraActive = true;
        LOGGER_INFO("Debug") << "Debug Camera: ON (Shift+F11)";
    }
}

void CameraDebugModule::ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action)
{
    if (keyboard.GetKey(key)) {
        if (!pressedState) {
            action(); pressedState = true;
        }
    } else pressedState = false;
}

#endif

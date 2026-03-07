#include <core/debug/modules/camera_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <core/app/application.h>
#include <systems/input/keyboard_manager.h>
#include <core/scripting/script_registry.h>
#include <core/scripting/default_camera_controller.h>
#include <iostream>
#include <ecs/entity_manager.h>

CameraDebugModule::CameraDebugModule() {}
CameraDebugModule::~CameraDebugModule() {}

void CameraDebugModule::Init(EngineContext ctx)
{
    m_Ctx = ctx;
}

void CameraDebugModule::OnUpdate(float dt)
{

}

void CameraDebugModule::Render(Scene &scene)
{

}

void CameraDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_Ctx.IsValid() || !m_Enabled)
        return;

    ProcessKey(keyboard, Input::Key::F11, m_F11Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Input::Key::LeftShift) || keyboard.GetKey(Input::Key::RightShift);
        if (shift)
        {
            ToggleDebugCamera();
        } });
}

void CameraDebugModule::ToggleDebugCamera()
{
    if (!m_Ctx.IsValid())
        return;

    auto &scene = *m_Ctx.scene;
    auto &registry = scene.registry;

    if (m_IsDebugCameraActive)
    {
        if (registry.valid(m_DebugCamera) && registry.all_of<CameraComponent>(m_DebugCamera))
        {
            registry.get<CameraComponent>(m_DebugCamera).isPrimary = false;
        }

        if (registry.valid(m_LastActiveCamera) && registry.all_of<CameraComponent>(m_LastActiveCamera))
        {
            registry.get<CameraComponent>(m_LastActiveCamera).isPrimary = true;
            std::cout << "[Debug] Switched to User Camera (Entity " << (uint32_t)m_LastActiveCamera << ")" << std::endl;
        }
        else
        {
            entt::entity fallback = EntityManager::GetActiveCamera(scene);
            if (fallback == entt::null)
            {
                auto view = registry.view<CameraComponent>();
                for (auto entity : view)
                {
                    if (entity != m_DebugCamera)
                    {
                        view.get<CameraComponent>(entity).isPrimary = true;
                        std::cout << "[Debug] Original camera invalid. Switched to fallback camera (Entity " << (uint32_t)entity << ")" << std::endl;
                        break;
                    }
                }
            }
        }

        m_IsDebugCameraActive = false;
        std::cout << "========== Debug Camera: OFF ==========" << std::endl;
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
            m_DebugCamera = EntityManager::CreateEntity(scene);
            registry.emplace<InfoComponent>(m_DebugCamera, "Debug Camera", "Debug");

            auto &posComp = registry.emplace<PositionComponent>(m_DebugCamera);
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

            auto &cam = registry.emplace<CameraComponent>(m_DebugCamera);
            cam.isPrimary = true;
            cam.fov = 45.0f;
            cam.nearPlane = 0.1f;
            cam.farPlane = 1000.0f;

            std::string scriptName = "DefaultCameraController";
            auto scriptInstance = ScriptRegistry::Instance().Create(scriptName);
            if (scriptInstance)
            {
                auto &scriptComp = registry.emplace<ScriptComponent>(m_DebugCamera);
                scriptComp.instance = std::move(scriptInstance);
                scriptComp.InstantiateScript = [scriptName]()
                { return ScriptRegistry::Instance().Create(scriptName); };
                scriptComp.DestroyScript = [](ScriptComponent *nsc)
                { nsc->instance.reset(); };
                scriptComp.instance->Init(m_DebugCamera, &scene, m_Ctx);
                scriptComp.instance->OnCreate();
            }
        }
        else
        {
            if (registry.all_of<CameraComponent>(m_DebugCamera))
            {
                registry.get<CameraComponent>(m_DebugCamera).isPrimary = true;

                if (registry.valid(m_LastActiveCamera) && registry.all_of<PositionComponent>(m_LastActiveCamera))
                {
                    auto &userPos = registry.get<PositionComponent>(m_LastActiveCamera);
                    auto &debugPos = registry.get<PositionComponent>(m_DebugCamera);
                    debugPos.value = userPos.value;
                    
                    if (registry.all_of<RotationComponent>(m_LastActiveCamera) && registry.all_of<RotationComponent>(m_DebugCamera))
                    {
                        registry.get<RotationComponent>(m_DebugCamera).value = registry.get<RotationComponent>(m_LastActiveCamera).value;
                    }
                }
            }
        }

        m_IsDebugCameraActive = true;
        std::cout << "========== Debug Camera: ON ==========" << std::endl;
        std::cout << "[Debug] Switched to Free Cam (Shift+F11)" << std::endl;
    }
}

void CameraDebugModule::ProcessKey(KeyboardManager &keyboard, Input::Key key, bool &pressedState, std::function<void()> action)
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

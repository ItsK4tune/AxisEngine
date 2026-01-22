#include <debug/modules/camera_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>
#include <script/script_registry.h>
#include <script/default_camera_controller.h>
#include <iostream>
#include <GLFW/glfw3.h>

CameraDebugModule::CameraDebugModule() {}
CameraDebugModule::~CameraDebugModule() {}

void CameraDebugModule::Init(Application *app)
{
    m_App = app;
}

void CameraDebugModule::OnUpdate(float dt)
{
    // No per-frame updates needed
}

void CameraDebugModule::Render(Scene &scene)
{
    // This module doesn't render anything
}

void CameraDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_App || !m_Enabled)
        return;

    ProcessKey(keyboard, GLFW_KEY_F11, m_F11Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
        if (shift)
        {
            ToggleDebugCamera();
        } });
}

void CameraDebugModule::ToggleDebugCamera()
{
    auto &scene = m_App->GetScene();
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
            entt::entity fallback = scene.GetActiveCamera();
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
        m_LastActiveCamera = scene.GetActiveCamera();

        if (registry.valid(m_LastActiveCamera))
        {
            if (registry.all_of<CameraComponent>(m_LastActiveCamera))
                registry.get<CameraComponent>(m_LastActiveCamera).isPrimary = false;
        }

        if (!registry.valid(m_DebugCamera))
        {
            m_DebugCamera = scene.createEntity();
            registry.emplace<InfoComponent>(m_DebugCamera, "Debug Camera", "Debug");

            auto &trans = registry.emplace<TransformComponent>(m_DebugCamera);
            if (registry.valid(m_LastActiveCamera) && registry.all_of<TransformComponent>(m_LastActiveCamera))
            {
                trans.position = registry.get<TransformComponent>(m_LastActiveCamera).position;
            }
            else
            {
                trans.position = glm::vec3(0.0f, 5.0f, 10.0f);
            }

            auto &cam = registry.emplace<CameraComponent>(m_DebugCamera);
            cam.isPrimary = true;
            cam.fov = 45.0f;
            cam.nearPlane = 0.1f;
            cam.farPlane = 1000.0f;

            std::string scriptName = "DefaultCameraController";
            Scriptable *scriptInstance = ScriptRegistry::Instance().Create(scriptName);
            if (scriptInstance)
            {
                auto &scriptComp = registry.emplace<ScriptComponent>(m_DebugCamera);
                scriptComp.instance = scriptInstance;
                scriptComp.InstantiateScript = [scriptName]()
                { return ScriptRegistry::Instance().Create(scriptName); };
                scriptComp.DestroyScript = [](ScriptComponent *nsc)
                { delete nsc->instance; nsc->instance = nullptr; };
                scriptComp.instance->Init(m_DebugCamera, &scene, m_App);
                scriptComp.instance->OnCreate();
            }
        }
        else
        {
            if (registry.all_of<CameraComponent>(m_DebugCamera))
            {
                registry.get<CameraComponent>(m_DebugCamera).isPrimary = true;

                if (registry.valid(m_LastActiveCamera) && registry.all_of<TransformComponent>(m_LastActiveCamera))
                {
                    auto &userTrans = registry.get<TransformComponent>(m_LastActiveCamera);
                    auto &debugTrans = registry.get<TransformComponent>(m_DebugCamera);
                    debugTrans.position = userTrans.position;
                    debugTrans.rotation = userTrans.rotation;
                }
            }
        }

        m_IsDebugCameraActive = true;
        std::cout << "========== Debug Camera: ON ==========" << std::endl;
        std::cout << "[Debug] Switched to Free Cam (Shift+F11)" << std::endl;
    }
}

void CameraDebugModule::ProcessKey(KeyboardManager &keyboard, int key, bool &pressedState, std::function<void()> action)
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

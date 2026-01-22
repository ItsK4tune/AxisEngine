#include <debug/modules/physics_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

PhysicsDebugModule::PhysicsDebugModule() {}
PhysicsDebugModule::~PhysicsDebugModule() {}

void PhysicsDebugModule::Init(Application *app)
{
    m_App = app;
}

void PhysicsDebugModule::OnUpdate(float dt)
{
    // No per-frame updates needed
}

void PhysicsDebugModule::Render(Scene &scene)
{
    if (!m_App || !m_Enabled)
        return;

    int width = m_App->GetWidth();
    int height = m_App->GetHeight();

    if (m_ShowPhysicsDebug)
    {
        auto &res = m_App->GetResourceManager();
        Shader *debugShader = res.GetShader("debugLine");
        if (debugShader)
        {
            m_App->GetPhysicsSystem().RenderDebug(scene, m_App->GetPhysicsWorld(), *debugShader, width, height);
        }
    }

    if (m_ShowAudioDebug)
    {
        auto view = scene.registry.view<AudioSourceComponent, TransformComponent>();
        for (auto entity : view)
        {
            const auto &transform = view.get<TransformComponent>(entity);
            glm::vec3 pos = transform.GetWorldModelMatrix(scene.registry)[3];
            // TODO: Render audio source gizmo at pos
        }
    }

    if (m_ShowParticleDebug)
    {
        auto view = scene.registry.view<ParticleEmitterComponent, TransformComponent>();
        for (auto entity : view)
        {
            const auto &transform = view.get<TransformComponent>(entity);
            glm::vec3 pos = transform.GetWorldModelMatrix(scene.registry)[3];
            // TODO: Render particle emitter boundary at pos
        }
    }
}

void PhysicsDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_App || !m_Enabled)
        return;

    ProcessKey(keyboard, GLFW_KEY_F8, m_F8Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
        if (shift) {
            m_ShowAudioDebug = !m_ShowAudioDebug;
            std::cout << "\n========== Audio Debug (Shift+F8) ==========" << std::endl;
            std::cout << "[Debug] Audio Debug: " << (m_ShowAudioDebug ? "ON" : "OFF") << std::endl;
            std::cout << "[Info] Shows 3D audio source positions" << std::endl;
            std::cout << "==========================================" << std::endl;
        } else {
            TogglePhysicsDebug();
        } });

    ProcessKey(keyboard, GLFW_KEY_F9, m_F9Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
        if (shift) {
            m_ShowParticleDebug = !m_ShowParticleDebug;
            std::cout << "\n========== Particle Debug (Shift+F9) ==========" << std::endl;
            std::cout << "[Debug] Particle Debug: " << (m_ShowParticleDebug ? "ON" : "OFF") << std::endl;
            std::cout << "[Info] Shows particle emitter boundaries" << std::endl;
            std::cout << "=============================================" << std::endl;
        } });
}

void PhysicsDebugModule::TogglePhysicsDebug()
{
    m_ShowPhysicsDebug = !m_ShowPhysicsDebug;
    std::cout << "\n========== Physics Debug (F8) ==========" << std::endl;
    std::cout << "[Debug] Physics Debug: " << (m_ShowPhysicsDebug ? "ON" : "OFF") << std::endl;
    std::cout << "========================================" << std::endl;
}

void PhysicsDebugModule::ProcessKey(KeyboardManager &keyboard, int key, bool &pressedState, std::function<void()> action)
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

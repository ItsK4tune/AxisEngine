#include <debug/modules/physics_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>
#include <iostream>

#include <glm/glm.hpp>
#include <interface/graphic/i_graphics_context.h>
#include <interface/graphic/i_render_state_manager.h>
#include <debug/debug_config.h>

PhysicsDebugModule::PhysicsDebugModule() {}
PhysicsDebugModule::~PhysicsDebugModule() {}

void PhysicsDebugModule::Init(std::shared_ptr<Application> app)
{
    m_App = app;
}

void PhysicsDebugModule::OnUpdate(float dt)
{
}

void PhysicsDebugModule::Render(Scene &scene)
{
    if (!m_App || !m_Enabled)
        return;

    int width = m_App->GetWidth();
    int height = m_App->GetHeight();

    if (DebugConfig::ShowPhysics)
    {
        auto &res = m_App->GetResourceManager();
        auto debugShader = res.GetShader("debugLine");
        if (debugShader)
        {
            m_App->GetPhysicsSystem().RenderDebug(scene, m_App->GetPhysicsWorld(), *debugShader, width, height, m_App->GetGraphicsContext().GetRenderStateManager());
        }
    }

    if (m_ShowAudioDebug)
    {
        auto view = scene.registry.view<AudioSourceComponent, TransformComponent>();
        for (auto entity : view)
        {
            const auto &transform = view.get<TransformComponent>(entity);
            glm::vec3 pos = transform.GetWorldModelMatrix(scene.registry)[3];
        }
    }

    if (m_ShowParticleDebug)
    {
        auto view = scene.registry.view<ParticleEmitterComponent, TransformComponent>();
        for (auto entity : view)
        {
            const auto &transform = view.get<TransformComponent>(entity);
            glm::vec3 pos = transform.GetWorldModelMatrix(scene.registry)[3];
        }
    }
}

void PhysicsDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_App || !m_Enabled)
        return;

    ProcessKey(keyboard, Input::Key::F8, m_F8Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Input::Key::LeftShift) || keyboard.GetKey(Input::Key::RightShift);
        if (shift) {
            m_ShowAudioDebug = !m_ShowAudioDebug;
            std::cout << "\n========== Audio Debug (Shift+F8) ==========" << std::endl;
            std::cout << "[Debug] Audio Debug: " << (m_ShowAudioDebug ? "ON" : "OFF") << std::endl;
            std::cout << "[Info] Shows 3D audio source positions" << std::endl;
            std::cout << "==========================================" << std::endl;
        } else {
            TogglePhysicsDebug();
        } });

    ProcessKey(keyboard, Input::Key::F9, m_F9Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Input::Key::LeftShift) || keyboard.GetKey(Input::Key::RightShift);
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
    DebugConfig::ShowPhysics = !DebugConfig::ShowPhysics;
    std::cout << "\n========== Physics Debug (F8) ==========" << std::endl;
    std::cout << "[Debug] Physics Debug: " << (DebugConfig::ShowPhysics ? "ON" : "OFF") << std::endl;
    std::cout << "========================================" << std::endl;
}

void PhysicsDebugModule::ProcessKey(KeyboardManager &keyboard, Input::Key key, bool &pressedState, std::function<void()> action)
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

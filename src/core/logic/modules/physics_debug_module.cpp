#include <ecs/unit/core_components.h>
#include <core/logic/modules/physics_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <core/logic/app_framework.h>
#include <platform/logic/input_system.h>
#include <resource/manager/resource_manager.h>
#include <ecs/logic/physics_system.h>
#include <scene/logic/scene.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/media_components.h>
#include <core/unit/engine_context.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <core/manager/system_manager.h>
#include <platform/interface/i_window.h>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <core/logic/debug_core.h>
PhysicsDebugModule::PhysicsDebugModule() {}
PhysicsDebugModule::~PhysicsDebugModule() {}

void PhysicsDebugModule::Init(EngineContext ctx)
{
    m_Ctx = ctx;
}

void PhysicsDebugModule::OnUpdate(float dt)
{
}

void PhysicsDebugModule::Render(Scene &scene)
{
    if (!m_Ctx.IsValid() || !m_Enabled)
        return;

    int width = m_Ctx.io->GetMonitorManager().GetWidth();
    int height = m_Ctx.io->GetMonitorManager().GetHeight();

    if (DebugConfig::ShowPhysics)
    {
        auto &res = *m_Ctx.resources;
        auto debugShader = res.GetShader("debugLine");
        if (debugShader)
        {
            m_Ctx.systems->GetSystem<PhysicsSystem>()->RenderDebug(scene, *m_Ctx.physics, *debugShader, width, height, m_Ctx.io->GetGraphicsContext().GetRenderStateManager());
        }
    }

    if (m_ShowAudioDebug)
    {
        auto view = scene.registry.view<AudioSourceComponent, WorldTransformComponent>();
        for (auto entity : view)
        {
            const auto &world = view.get<WorldTransformComponent>(entity);
            glm::vec3 pos = glm::vec3(world.worldMatrix[3]);
        }
    }

    if (m_ShowParticleDebug)
    {
        auto view = scene.registry.view<ParticleEmitterComponent, WorldTransformComponent>();
        for (auto entity : view)
        {
            const auto &world = view.get<WorldTransformComponent>(entity);
            glm::vec3 pos = glm::vec3(world.worldMatrix[3]);
        }
    }
}

void PhysicsDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_Ctx.IsValid() || !m_Enabled)
        return;

    ProcessKey(keyboard, Key::F8, m_F8Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift) {
            m_ShowAudioDebug = !m_ShowAudioDebug;
            std::cout << "\n========== Audio Debug (Shift+F8) ==========" << std::endl;
            std::cout << "[Debug] Audio Debug: " << (m_ShowAudioDebug ? "ON" : "OFF") << std::endl;
            std::cout << "[Info] Shows 3D audio source positions" << std::endl;
            std::cout << "==========================================" << std::endl;
        } else {
            TogglePhysicsDebug();
        } });

    ProcessKey(keyboard, Key::F9, m_F9Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
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

void PhysicsDebugModule::ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action)
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

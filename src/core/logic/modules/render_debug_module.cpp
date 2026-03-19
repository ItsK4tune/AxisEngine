#include <core/logic/modules/render_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <core/logic/application.h>
#include <platform/logic/input_manager.h>
#include <ecs/logic/render_system.h>
#include <ecs/logic/terrain_system.h>
#include <ecs/logic/skybox_render_system.h>
#include <ecs/logic/ui_render_system.h>
#include <iostream>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <core/logic/system_manager.h>
#include <core/logic/debug_system.h>
#include <core/logic/service_locator.h>
RenderDebugModule::RenderDebugModule() {}
RenderDebugModule::~RenderDebugModule() {}

void RenderDebugModule::Initialize()
{
}

void RenderDebugModule::OnUpdate(float dt)
{

}

void RenderDebugModule::Render(Scene &scene)
{

}

void RenderDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_Enabled)
        return;

    ProcessKey(keyboard, Key::F6, m_F6Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift) {
            auto& systems = ServiceLocator::Instance().Require<SystemManager>();
            static bool skyboxEnabled = true;
            skyboxEnabled = !skyboxEnabled;
            systems.GetSystem<SkyboxRenderSystem>()->SetEnabled(skyboxEnabled);
            std::cout << "\n========== Skybox Toggle (Shift+F6) ==========" << std::endl;
            std::cout << "[Debug] Skybox: " << (skyboxEnabled ? "ON" : "OFF") << std::endl;
            std::cout << "==============================================" << std::endl;
        } else {
            DebugConfig::ShowWireframe = !DebugConfig::ShowWireframe;

            std::cout << "\n========== Wireframe Mode (F6) ==========" << std::endl;
            std::cout << "[Debug] Wireframe: " << (DebugConfig::ShowWireframe ? "ON" : "OFF") << std::endl;
            std::cout << "=========================================" << std::endl;
        } });

    ProcessKey(keyboard, Key::F7, m_F7Pressed, [this, &keyboard]()
               {
         bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
         if (shift) {
             auto& systems = ServiceLocator::Instance().Require<SystemManager>();
             bool shadow = !systems.GetSystem<RenderSystem>()->IsShadowsEnabled();
             systems.GetSystem<RenderSystem>()->SetEnableShadows(shadow);
             std::cout << "\n========== Shadow Toggle (Shift+F7) ==========" << std::endl;
             std::cout << "[Debug] Shadows: " << (shadow ? "ON" : "OFF") << std::endl;
             std::cout << "============================================" << std::endl;
         } else {
             auto& systems = ServiceLocator::Instance().Require<SystemManager>();
             m_NoTextureMode = !m_NoTextureMode;
             systems.GetSystem<RenderSystem>()->SetDebugNoTexture(m_NoTextureMode);
             systems.GetSystem<TerrainSystem>()->SetDebugNoTexture(m_NoTextureMode);
             std::cout << "\n========== No Texture Mode (F7) ==========" << std::endl;
             std::cout << "[Debug] No Texture Mode: " << (m_NoTextureMode ? "ON" : "OFF") << std::endl;
             std::cout << "==========================================" << std::endl;
         } });

    ProcessKey(keyboard, Key::F9, m_F9Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift) {

        } else {
            auto& systems = ServiceLocator::Instance().Require<SystemManager>();
            static bool uiEnabled = true;
            uiEnabled = !uiEnabled;
            systems.GetSystem<UIRenderSystem>()->SetEnabled(uiEnabled);
            std::cout << "\n========== UI System (F9) ==========" << std::endl;
            std::cout << "[Debug] UI System: " << (uiEnabled ? "ON" : "OFF") << std::endl;
            std::cout << "====================================" << std::endl;
        } });
}

void RenderDebugModule::ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action)
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
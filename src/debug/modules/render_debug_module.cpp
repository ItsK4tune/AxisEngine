#include <debug/modules/render_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>
#include <input/keyboard_manager.h>
#include <ecs/systems/render_system.h>
#include <ecs/systems/skybox_system.h>
#include <ecs/systems/ui_system.h>
#include <iostream>
#include <interface/graphic/i_graphics_context.h>
#include <interface/graphic/i_render_state_manager.h>
#include <debug/debug_config.h>

RenderDebugModule::RenderDebugModule() {}
RenderDebugModule::~RenderDebugModule() {}

void RenderDebugModule::Init(std::shared_ptr<Application> app)
{
    m_App = app;
}

void RenderDebugModule::OnUpdate(float dt)
{

}

void RenderDebugModule::Render(Scene &scene)
{

}

void RenderDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_App || !m_Enabled)
        return;

    ProcessKey(keyboard, Input::Key::F6, m_F6Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Input::Key::LeftShift) || keyboard.GetKey(Input::Key::RightShift);
        if (shift) {
            static bool skyboxEnabled = true;
            skyboxEnabled = !skyboxEnabled;
            m_App->GetSkyboxRenderSystem().SetEnabled(skyboxEnabled);
            std::cout << "\n========== Skybox Toggle (Shift+F6) ==========" << std::endl;
            std::cout << "[Debug] Skybox: " << (skyboxEnabled ? "ON" : "OFF") << std::endl;
            std::cout << "==============================================" << std::endl;
        } else {
            DebugConfig::ShowWireframe = !DebugConfig::ShowWireframe;

            std::cout << "\n========== Wireframe Mode (F6) ==========" << std::endl;
            std::cout << "[Debug] Wireframe: " << (DebugConfig::ShowWireframe ? "ON" : "OFF") << std::endl;
            std::cout << "=========================================" << std::endl;
        } });

    ProcessKey(keyboard, Input::Key::F7, m_F7Pressed, [this, &keyboard]()
               {
         bool shift = keyboard.GetKey(Input::Key::LeftShift) || keyboard.GetKey(Input::Key::RightShift);
         if (shift) {
             bool shadow = !m_App->GetRenderSystem().IsShadowsEnabled();
             m_App->GetRenderSystem().SetEnableShadows(shadow);
             std::cout << "\n========== Shadow Toggle (Shift+F7) ==========" << std::endl;
             std::cout << "[Debug] Shadows: " << (shadow ? "ON" : "OFF") << std::endl;
             std::cout << "============================================" << std::endl;
         } else {
             m_NoTextureMode = !m_NoTextureMode;
             m_App->GetRenderSystem().SetDebugNoTexture(m_NoTextureMode);
             std::cout << "\n========== No Texture Mode (F7) ==========" << std::endl;
             std::cout << "[Debug] No Texture Mode: " << (m_NoTextureMode ? "ON" : "OFF") << std::endl;
             std::cout << "==========================================" << std::endl;
         } });

    ProcessKey(keyboard, Input::Key::F9, m_F9Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Input::Key::LeftShift) || keyboard.GetKey(Input::Key::RightShift);
        if (shift) {

        } else {
            static bool uiEnabled = true;
            uiEnabled = !uiEnabled;
            m_App->GetUIRenderSystem().SetEnabled(uiEnabled);
            std::cout << "\n========== UI System (F9) ==========" << std::endl;
            std::cout << "[Debug] UI System: " << (uiEnabled ? "ON" : "OFF") << std::endl;
            std::cout << "====================================" << std::endl;
        } });
}

void RenderDebugModule::ProcessKey(KeyboardManager &keyboard, Input::Key key, bool &pressedState, std::function<void()> action)
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

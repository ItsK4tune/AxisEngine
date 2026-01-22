#include <debug/modules/render_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>
#include <iostream>
#include <GLFW/glfw3.h>

RenderDebugModule::RenderDebugModule() {}
RenderDebugModule::~RenderDebugModule() {}

void RenderDebugModule::Init(Application *app)
{
    m_App = app;
}

void RenderDebugModule::OnUpdate(float dt)
{
    // No per-frame updates needed
}

void RenderDebugModule::Render(Scene &scene)
{
    // This module doesn't render anything, only toggles render states
}

void RenderDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_App || !m_Enabled)
        return;

    ProcessKey(keyboard, GLFW_KEY_F6, m_F6Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
        if (shift) {
            static bool skyboxEnabled = true;
            skyboxEnabled = !skyboxEnabled;
            m_App->GetSkyboxRenderSystem().SetEnabled(skyboxEnabled);
            std::cout << "\n========== Skybox Toggle (Shift+F6) ==========" << std::endl;
            std::cout << "[Debug] Skybox: " << (skyboxEnabled ? "ON" : "OFF") << std::endl;
            std::cout << "==============================================" << std::endl;
        } else {
            m_WireframeMode = !m_WireframeMode;
            if (m_WireframeMode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            std::cout << "\n========== Wireframe Mode (F6) ==========" << std::endl;
            std::cout << "[Debug] Wireframe: " << (m_WireframeMode ? "ON" : "OFF") << std::endl;
            std::cout << "=========================================" << std::endl;
        } });

    ProcessKey(keyboard, GLFW_KEY_F7, m_F7Pressed, [this, &keyboard]()
               {
         bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
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

    ProcessKey(keyboard, GLFW_KEY_F9, m_F9Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
        if (shift) {
            // Particle debug - handled by PhysicsDebugModule
        } else {
            static bool uiEnabled = true;
            uiEnabled = !uiEnabled;
            m_App->GetUIRenderSystem().SetEnabled(uiEnabled);
            std::cout << "\n========== UI System (F9) ==========" << std::endl;
            std::cout << "[Debug] UI System: " << (uiEnabled ? "ON" : "OFF") << std::endl;
            std::cout << "====================================" << std::endl;
        } });
}

void RenderDebugModule::ProcessKey(KeyboardManager &keyboard, int key, bool &pressedState, std::function<void()> action)
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

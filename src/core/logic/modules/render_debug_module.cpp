#include <core/logic/modules/render_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <core/logic/app_framework.h>
#include <platform/logic/input_system.h>
#include <ecs/logic/render_system.h>
#include <ecs/logic/terrain_system.h>
#include <ecs/logic/skybox_system.h>
#include <ecs/logic/ui_system.h>
#include <iostream>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <core/unit/engine_context.h>
#include <core/manager/system_manager.h>
#include <core/logic/debug_core.h>
RenderDebugModule::RenderDebugModule() {}
RenderDebugModule::~RenderDebugModule() {}

void RenderDebugModule::Initialize(EngineContext ctx)
{
    m_Ctx = ctx;
}

void RenderDebugModule::OnUpdate(float dt)
{

}

void RenderDebugModule::Render(Scene &scene)
{

}

void RenderDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_Ctx.IsValid() || !m_Enabled)
        return;

    ProcessKey(keyboard, Key::F6, m_F6Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift) {
            static bool skyboxEnabled = true;
            skyboxEnabled = !skyboxEnabled;
            m_Ctx.systems->GetSystem<SkyboxRenderSystem>()->SetEnabled(skyboxEnabled);
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
             bool shadow = !m_Ctx.systems->GetSystem<RenderSystem>()->IsShadowsEnabled();
             m_Ctx.systems->GetSystem<RenderSystem>()->SetEnableShadows(shadow);
             std::cout << "\n========== Shadow Toggle (Shift+F7) ==========" << std::endl;
             std::cout << "[Debug] Shadows: " << (shadow ? "ON" : "OFF") << std::endl;
             std::cout << "============================================" << std::endl;
         } else {
             m_NoTextureMode = !m_NoTextureMode;
             m_Ctx.systems->GetSystem<RenderSystem>()->SetDebugNoTexture(m_NoTextureMode);
             m_Ctx.systems->GetSystem<TerrainSystem>()->SetDebugNoTexture(m_NoTextureMode);
             std::cout << "\n========== No Texture Mode (F7) ==========" << std::endl;
             std::cout << "[Debug] No Texture Mode: " << (m_NoTextureMode ? "ON" : "OFF") << std::endl;
             std::cout << "==========================================" << std::endl;
         } });

    ProcessKey(keyboard, Key::F9, m_F9Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift) {

        } else {
            static bool uiEnabled = true;
            uiEnabled = !uiEnabled;
            m_Ctx.systems->GetSystem<UIRenderSystem>()->SetEnabled(uiEnabled);
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
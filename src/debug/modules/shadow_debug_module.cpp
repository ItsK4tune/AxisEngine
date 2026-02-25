#include <debug/modules/shadow_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>
#include <input/keyboard_manager.h>
#include <scene/scene.h>

ShadowDebugModule::ShadowDebugModule() {}
ShadowDebugModule::~ShadowDebugModule() {}

void ShadowDebugModule::Init(Application* app)
{
    m_App = app;
}

void ShadowDebugModule::OnUpdate(float dt)
{

}

void ShadowDebugModule::Render(Scene &scene)
{

}

void ShadowDebugModule::ProcessInput(KeyboardManager &keyboard)
{

}

#endif

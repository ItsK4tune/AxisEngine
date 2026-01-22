#include <debug/modules/shadow_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>

ShadowDebugModule::ShadowDebugModule() {}
ShadowDebugModule::~ShadowDebugModule() {}

void ShadowDebugModule::Init(Application *app)
{
    m_App = app;
}

void ShadowDebugModule::OnUpdate(float dt)
{
    // Reserved for future implementation
}

void ShadowDebugModule::Render(Scene &scene)
{
    // Reserved for future implementation
}

void ShadowDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    // Reserved for future implementation
}

#endif

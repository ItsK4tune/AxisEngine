#include <core/debug/modules/shadow_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <core/app/application.h>
#include <systems/input/keyboard_manager.h>
#include <scene/scene.h>

ShadowDebugModule::ShadowDebugModule() {}
ShadowDebugModule::~ShadowDebugModule() {}

void ShadowDebugModule::Init(EngineContext ctx)
{
    m_Ctx = ctx;
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

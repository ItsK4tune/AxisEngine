#include <core/logic/modules/shadow_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <core/logic/app_framework.h>
#include <platform/logic/input_system.h>
#include <scene/logic/scene.h>

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

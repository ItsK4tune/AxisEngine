#include <ecs/logic/deferred_lighting_system.h>
#include <ecs/logic/render_system.h>
#include <ecs/logic/system_manager.h>
#include <render/interface/i_graphics_context.h>
#include <core/logic/service_locator.h>

void DeferredLightingSystem::Initialize() {
}

void DeferredLightingSystem::RenderAlpha(Scene &scene, int width, int height, float alpha) {
    if (!m_Enabled) return;
    
    auto rs_ptr = ServiceLocator::Instance().Require<SystemManager>().GetSystem<RenderSystem>();
    if (!rs_ptr || !rs_ptr->IsDeferredRenderingEnabled()) return;
    
    rs_ptr->RenderDeferredLighting(scene, width, height);
}

void DeferredLightingSystem::Render(Scene &scene) {
    // Logic moved to RenderAlpha to run before Skybox/Transparent
}

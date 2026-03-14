#include <ecs/logic/deferred_lighting_system.h>
#include <ecs/logic/render_system.h>
#include <core/manager/system_manager.h>
#include <platform/logic/io_handler.h>
#include <render/interface/i_graphics_context.h>

void DeferredLightingSystem::Initialize(EngineContext ctx) {
    m_Ctx = ctx;
}

void DeferredLightingSystem::Render(Scene &scene) {
    if (!m_Enabled) return;
    
    auto rs = m_Ctx.systems->GetSystem<RenderSystem>();
    if (!rs || !rs->IsDeferredRenderingEnabled()) return;
    
    int width = rs->GetGBufferWidth();
    int height = rs->GetGBufferHeight();
    
    rs->RenderDeferredLighting(scene, width, height);
}

#include <core/debug/debug_system.h>
#include <core/debug/interfaces/i_debug_module.h>
#include <core/debug/modules/general_debug_module.h>
#include <core/debug/modules/overlay_debug_module.h>
#include <core/debug/modules/render_debug_module.h>
#include <core/debug/modules/physics_debug_module.h>
#include <core/debug/modules/gizmo_debug_module.h>
#include <core/debug/modules/camera_debug_module.h>
#include <core/debug/modules/shadow_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <systems/window/io_handler.h>
#include <core/utils/logger.h>
#include <scene/scene.h>
#include <systems/input/keyboard_manager.h>
#include <resource/resource_manager.h>
#include <rendering/interfaces/i_graphics_context.h>
#include <rendering/interfaces/i_render_state_manager.h>
#include <algorithm>

DebugSystem::DebugSystem() {}
DebugSystem::~DebugSystem() {}

void DebugSystem::Init(EngineContext ctx)
{
    m_Ctx = ctx;

    auto &res = *m_Ctx.resources;

    res.LoadFont("debug_font", "includes/engine/asset/fonts/time.ttf", 24);
    res.LoadShader("debug_text", "includes/engine/asset/shaders/text.vs", "includes/engine/asset/shaders/text.fs");

    if (!res.GetUIModel("debug_sys_model"))
    {
        res.CreateUIModel("debug_sys_model", UIType::Text);
    }

    m_DebugFont = res.GetFont("debug_font");
    m_TextShader = res.GetShader("debug_text");
    m_TextQuad = res.GetUIModel("debug_sys_model");

    LOGGER_INFO("DebugSystem") << "Initializing debug modules...";

    auto generalModule = std::make_unique<GeneralDebugModule>();
    generalModule->Init(ctx);
    m_Modules.push_back(std::move(generalModule));
    LOGGER_INFO("DebugSystem") << "  - GeneralDebugModule initialized";

    auto overlayModule = std::make_unique<OverlayDebugModule>();
    overlayModule->Init(ctx);
    overlayModule->SetSharedResources(m_DebugFont, m_TextShader, m_TextQuad);
    m_Modules.push_back(std::move(overlayModule));
    LOGGER_INFO("DebugSystem") << "  - OverlayDebugModule initialized";

    auto renderModule = std::make_unique<RenderDebugModule>();
    renderModule->Init(ctx);
    m_Modules.push_back(std::move(renderModule));
    LOGGER_INFO("DebugSystem") << "  - RenderDebugModule initialized";

    auto physicsModule = std::make_unique<PhysicsDebugModule>();
    physicsModule->Init(ctx);
    m_Modules.push_back(std::move(physicsModule));
    LOGGER_INFO("DebugSystem") << "  - PhysicsDebugModule initialized";

    auto gizmoModule = std::make_unique<GizmoDebugModule>();
    gizmoModule->Init(ctx);
    gizmoModule->SetSharedResources(m_DebugFont, m_TextShader, m_TextQuad);
    m_Modules.push_back(std::move(gizmoModule));
    LOGGER_INFO("DebugSystem") << "  - GizmoDebugModule initialized";

    auto cameraModule = std::make_unique<CameraDebugModule>();
    cameraModule->Init(ctx);
    m_Modules.push_back(std::move(cameraModule));
    LOGGER_INFO("DebugSystem") << "  - CameraDebugModule initialized";

    auto shadowModule = std::make_unique<ShadowDebugModule>();
    shadowModule->Init(ctx);
    m_Modules.push_back(std::move(shadowModule));
    LOGGER_INFO("DebugSystem") << "  - ShadowDebugModule initialized";

    LOGGER_INFO("DebugSystem") << "All " << m_Modules.size() << " modules initialized successfully!";
}

void DebugSystem::OnUpdate(float dt)
{
    if (!m_Ctx.IsValid())
        return;

    m_FpsTimer += dt;
    m_FrameCount++;
    if (m_FpsTimer >= 1.0f)
    {
        m_CurrentFps = (float)m_FrameCount / m_FpsTimer;
        m_CurrentFrameTime = (m_FpsTimer / m_FrameCount) * 1000.0f;
        m_FpsTimer = 0.0f;
        m_FrameCount = 0;
    }

    for (auto &module : m_Modules)
    {
        if (auto overlayModule = dynamic_cast<OverlayDebugModule *>(module.get()))
        {
            overlayModule->SetStats(m_CurrentFps, m_CurrentFrameTime);
        }
    }

    for (auto &module : m_Modules)
    {
        if (module->IsEnabled())
        {
            module->OnUpdate(dt);
        }
    }

    auto &keyboard = m_Ctx.io->GetKeyboard();
    for (auto &module : m_Modules)
    {
        if (module->IsEnabled())
        {
            module->ProcessInput(keyboard);
        }
    }
}

void DebugSystem::Render(Scene &scene)
{
    if (!m_Ctx.IsValid())
        return;

    auto& rsm = m_Ctx.io->GetGraphicsContext().GetRenderStateManager();
    auto oldMode = rsm.GetPolygonMode();

    rsm.PolygonMode(Graphics::CullMode::FrontAndBack, Graphics::PolygonMode::Fill);

    std::vector<IDebugModule*> sortedModules;
    for (auto& module : m_Modules) {
        if (module->IsEnabled()) {
            sortedModules.push_back(module.get());
        }
    }

    std::sort(sortedModules.begin(), sortedModules.end(), [](IDebugModule* a, IDebugModule* b) {
        return a->GetRenderOrder() < b->GetRenderOrder();
    });

    for (auto module : sortedModules)
    {
        module->Render(scene);
    }

    rsm.PolygonMode(Graphics::CullMode::FrontAndBack, oldMode);
}

#endif

#include <ecs/logic/debug/debug_system.h>
#include <ecs/logic/debug/i_debug_module.h>
#include <ecs/logic/debug/modules/general_debug_module.h>
#include <ecs/logic/debug/modules/overlay_debug_module.h>
#include <ecs/logic/debug/modules/render_debug_module.h>
#include <ecs/logic/debug/modules/physics_debug_module.h>
#include <ecs/logic/debug/modules/gizmo_debug_module.h>
#include <ecs/logic/debug/modules/camera_debug_module.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/debug/modules/shadow_debug_module.h>
#include <ecs/logic/system_factory.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <platform/logic/io_handler.h>
#include <core/logic/logger.h>
#include <scene/logic/scene.h>
#include <platform/logic/input_manager.h>
#include <resource/logic/resource_manager.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <algorithm>

DebugSystem::DebugSystem() {}
DebugSystem::~DebugSystem() {}

REGISTER_SYSTEM(DebugSystem)

void DebugSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<DebugSystem>(this);
    auto* res = sl.Resolve<ResourceManager>();
    if (!res) {
        LOGGER_WARN("DebugSystem") << "Skipping full initialization (missing ResourceManager)";
        return;
    }

    m_TextShader = res->GetShader("debug_text");

    if (!res->GetUIModel("debug_sys_model"))
    {
        res->CreateUIModel("debug_sys_model", ::UIType::Text);
    }

    m_DebugFont = res->GetFont("debug_font");
    m_TextQuad = res->GetUIModel("debug_sys_model");

    LOGGER_INFO("DebugSystem") << "Initializing debug modules...";

    auto generalModule = std::make_unique<GeneralDebugModule>();
    generalModule->Initialize();
    m_Modules.push_back(std::move(generalModule));
    LOGGER_INFO("DebugSystem") << "  - GeneralDebugModule initialized";

    auto overlayModule = std::make_unique<OverlayDebugModule>();
    overlayModule->Initialize();
    overlayModule->SetSharedResources(m_DebugFont, m_TextShader, m_TextQuad);
    m_Modules.push_back(std::move(overlayModule));
    LOGGER_INFO("DebugSystem") << "  - OverlayDebugModule initialized";

    auto renderModule = std::make_unique<RenderDebugModule>();
    renderModule->Initialize();
    m_Modules.push_back(std::move(renderModule));
    LOGGER_INFO("DebugSystem") << "  - RenderDebugModule initialized";

    auto physicsModule = std::make_unique<PhysicsDebugModule>();
    physicsModule->Initialize();
    m_Modules.push_back(std::move(physicsModule));
    LOGGER_INFO("DebugSystem") << "  - PhysicsDebugModule initialized";

    auto gizmoModule = std::make_unique<GizmoDebugModule>();
    gizmoModule->Initialize();
    gizmoModule->SetSharedResources(m_DebugFont, m_TextShader, m_TextQuad);
    m_Modules.push_back(std::move(gizmoModule));
    LOGGER_INFO("DebugSystem") << "  - GizmoDebugModule initialized";

    auto cameraModule = std::make_unique<CameraDebugModule>();
    cameraModule->Initialize();
    m_Modules.push_back(std::move(cameraModule));
    LOGGER_INFO("DebugSystem") << "  - CameraDebugModule initialized";

    auto shadowModule = std::make_unique<ShadowDebugModule>();
    shadowModule->Initialize();
    m_Modules.push_back(std::move(shadowModule));
    LOGGER_INFO("DebugSystem") << "  - ShadowDebugModule initialized";

    LOGGER_INFO("DebugSystem") << "All " << m_Modules.size() << " modules initialized successfully!";
}

void DebugSystem::OnUpdate(float dt)
{
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

    auto& sl = ServiceLocator::Instance();
    auto* io = sl.Resolve<IOHandler>();
    if (!io) return;
    auto& keyboard = io->GetKeyboard();
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
    auto* context = ServiceLocator::Instance().Resolve<IGraphicsContext>();
    if (!context) return;
    auto& rsm = context->GetRenderStateManager();
    auto oldMode = rsm.GetPolygonMode();

    rsm.SetPolygonMode(CullMode::FrontAndBack, PolygonMode::Fill);

    std::vector<IDebugModule*> gizmoModules;
    for (auto &module : m_Modules)
    {
        if (module->IsEnabled() && !dynamic_cast<OverlayDebugModule*>(module.get()))
        {
            gizmoModules.push_back(module.get());
        }
    }

    std::sort(gizmoModules.begin(), gizmoModules.end(), [](IDebugModule* a, IDebugModule* b) {
        return a->GetRenderOrder() < b->GetRenderOrder();
    });

    for (auto module : gizmoModules)
    {
        module->Render(scene);
    }

    rsm.SetPolygonMode(CullMode::FrontAndBack, oldMode);
}

void DebugSystem::RenderUIPass(Scene &scene, float width, float height, IRenderStateManager &renderState)
{
    if (!m_Enabled) return;

    std::vector<IDebugModule*> overlayModules;
    for (auto &module : m_Modules)
    {
        if (module->IsEnabled() && dynamic_cast<OverlayDebugModule*>(module.get()))
        {
            overlayModules.push_back(module.get());
        }
    }

    std::sort(overlayModules.begin(), overlayModules.end(), [](IDebugModule* a, IDebugModule* b) {
        return a->GetRenderOrder() < b->GetRenderOrder();
    });

    for (auto module : overlayModules)
    {
        module->Render(scene);
    }
}

#endif

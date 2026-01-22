#include <debug/debug_system.h>
#include <debug/modules/i_debug_module.h>
#include <debug/modules/general_debug_module.h>
#include <debug/modules/overlay_debug_module.h>
#include <debug/modules/render_debug_module.h>
#include <debug/modules/physics_debug_module.h>
#include <debug/modules/gizmo_debug_module.h>
#include <debug/modules/camera_debug_module.h>
#include <debug/modules/shadow_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>
#include <iostream>
#include <GLFW/glfw3.h>

DebugSystem::DebugSystem() {}
DebugSystem::~DebugSystem() {}

void DebugSystem::Init(Application *app)
{
    m_App = app;

    // Load shared resources
    auto &res = m_App->GetResourceManager();

    res.LoadFont("debug_font", "src/asset/fonts/time.ttf", 24);
    res.LoadShader("debug_text", "src/asset/shaders/text.vs", "src/asset/shaders/text.fs");

    if (!res.GetUIModel("debug_sys_model"))
    {
        res.CreateUIModel("debug_sys_model", UIType::Text);
    }

    m_DebugFont = res.GetFont("debug_font");
    m_TextShader = res.GetShader("debug_text");
    m_TextQuad = res.GetUIModel("debug_sys_model");

    // Create and initialize all debug modules
    std::cout << "[DebugSystem] Initializing debug modules..." << std::endl;

    // General debug module (logging, stats, controls)
    auto generalModule = std::make_unique<GeneralDebugModule>();
    generalModule->Init(app);
    m_Modules.push_back(std::move(generalModule));
    std::cout << "  - GeneralDebugModule initialized" << std::endl;

    // Overlay debug module (HUD stats display)
    auto overlayModule = std::make_unique<OverlayDebugModule>();
    overlayModule->Init(app);
    overlayModule->SetSharedResources(m_DebugFont, m_TextShader, m_TextQuad);
    m_Modules.push_back(std::move(overlayModule));
    std::cout << "  - OverlayDebugModule initialized" << std::endl;

    // Render debug module (wireframe, textures, shadows, etc.)
    auto renderModule = std::make_unique<RenderDebugModule>();
    renderModule->Init(app);
    m_Modules.push_back(std::move(renderModule));
    std::cout << "  - RenderDebugModule initialized" << std::endl;

    // Physics debug module (physics, audio, particle visualization)
    auto physicsModule = std::make_unique<PhysicsDebugModule>();
    physicsModule->Init(app);
    m_Modules.push_back(std::move(physicsModule));
    std::cout << "  - PhysicsDebugModule initialized" << std::endl;

    // Gizmo debug module (entity names, transform gizmos, light gizmos)
    auto gizmoModule = std::make_unique<GizmoDebugModule>();
    gizmoModule->Init(app);
    gizmoModule->SetSharedResources(m_DebugFont, m_TextShader, m_TextQuad);
    m_Modules.push_back(std::move(gizmoModule));
    std::cout << "  - GizmoDebugModule initialized" << std::endl;

    // Camera debug module (free camera)
    auto cameraModule = std::make_unique<CameraDebugModule>();
    cameraModule->Init(app);
    m_Modules.push_back(std::move(cameraModule));
    std::cout << "  - CameraDebugModule initialized" << std::endl;

    // Shadow debug module (reserved for future use)
    auto shadowModule = std::make_unique<ShadowDebugModule>();
    shadowModule->Init(app);
    m_Modules.push_back(std::move(shadowModule));
    std::cout << "  - ShadowDebugModule initialized (stub)" << std::endl;

    std::cout << "[DebugSystem] All " << m_Modules.size() << " modules initialized successfully!" << std::endl;
}

void DebugSystem::OnUpdate(float dt)
{
    if (!m_App)
        return;

    // Update FPS stats
    m_FpsTimer += dt;
    m_FrameCount++;
    if (m_FpsTimer >= 1.0f)
    {
        m_CurrentFps = (float)m_FrameCount / m_FpsTimer;
        m_CurrentFrameTime = (m_FpsTimer / m_FrameCount) * 1000.0f;
        m_FpsTimer = 0.0f;
        m_FrameCount = 0;
    }

    // Update overlay module with current stats
    for (auto &module : m_Modules)
    {
        if (auto overlayModule = dynamic_cast<OverlayDebugModule *>(module.get()))
        {
            overlayModule->SetStats(m_CurrentFps, m_CurrentFrameTime);
        }
    }

    // Update all modules
    for (auto &module : m_Modules)
    {
        if (module->IsEnabled())
        {
            module->OnUpdate(dt);
        }
    }

    // Process input for all modules
    auto &keyboard = m_App->GetKeyboard();
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
    if (!m_App)
        return;

    // Save polygon mode to restore later
    GLint polygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Render all modules
    for (auto &module : m_Modules)
    {
        if (module->IsEnabled())
        {
            module->Render(scene);
        }
    }

    // Restore polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
}

#endif

#include <debug/debug_system.h>
#include <interface/debug_module.h>
#include <debug/modules/general_debug_module.h>
#include <debug/modules/overlay_debug_module.h>
#include <debug/modules/render_debug_module.h>
#include <debug/modules/physics_debug_module.h>
#include <debug/modules/gizmo_debug_module.h>
#include <debug/modules/camera_debug_module.h>
#include <debug/modules/shadow_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>
#include <utils/logger.h>
#include <GLFW/glfw3.h>

DebugSystem::DebugSystem() {}
DebugSystem::~DebugSystem() {}

void DebugSystem::Init(Application *app)
{
    m_App = app;

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
    
    LOGGER_INFO("DebugSystem") << "Initializing debug modules...";

    auto generalModule = std::make_unique<GeneralDebugModule>();
    generalModule->Init(app);
    m_Modules.push_back(std::move(generalModule));
    LOGGER_INFO("DebugSystem") << "  - GeneralDebugModule initialized";

    auto overlayModule = std::make_unique<OverlayDebugModule>();
    overlayModule->Init(app);
    overlayModule->SetSharedResources(m_DebugFont, m_TextShader, m_TextQuad);
    m_Modules.push_back(std::move(overlayModule));
    LOGGER_INFO("DebugSystem") << "  - OverlayDebugModule initialized";

    auto renderModule = std::make_unique<RenderDebugModule>();
    renderModule->Init(app);
    m_Modules.push_back(std::move(renderModule));
    LOGGER_INFO("DebugSystem") << "  - RenderDebugModule initialized";

    auto physicsModule = std::make_unique<PhysicsDebugModule>();
    physicsModule->Init(app);
    m_Modules.push_back(std::move(physicsModule));
    LOGGER_INFO("DebugSystem") << "  - PhysicsDebugModule initialized";

    auto gizmoModule = std::make_unique<GizmoDebugModule>();
    gizmoModule->Init(app);
    gizmoModule->SetSharedResources(m_DebugFont, m_TextShader, m_TextQuad);
    m_Modules.push_back(std::move(gizmoModule));
    LOGGER_INFO("DebugSystem") << "  - GizmoDebugModule initialized";

    auto cameraModule = std::make_unique<CameraDebugModule>();
    cameraModule->Init(app);
    m_Modules.push_back(std::move(cameraModule));
    LOGGER_INFO("DebugSystem") << "  - CameraDebugModule initialized";

    auto shadowModule = std::make_unique<ShadowDebugModule>();
    shadowModule->Init(app);
    m_Modules.push_back(std::move(shadowModule));
    LOGGER_INFO("DebugSystem") << "  - ShadowDebugModule initialized";

    LOGGER_INFO("DebugSystem") << "All " << m_Modules.size() << " modules initialized successfully!";
}

void DebugSystem::OnUpdate(float dt)
{
    if (!m_App)
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

    GLint polygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    for (auto &module : m_Modules)
    {
        if (module->IsEnabled())
        {
            module->Render(scene);
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
}

#endif

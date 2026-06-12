#include <editor/imgui_layer.h>

#ifdef ENABLE_EDITOR

#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <render/interface/i_graphics_context.h>
#include <render/rhi/i_render_backend.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#if AXIS_HAS_OPENGL_BACKEND
#include <imgui_impl_opengl3.h>
#endif

void ImGuiLayer::Initialize(void* nativeWindow)
{
    if (m_Initialized)
        return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.IndentSpacing = 14.0f;
    style.ItemSpacing = ImVec2(8.0f, 5.0f);
    style.WindowPadding = ImVec2(10.0f, 8.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.15f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.24f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.40f, 0.68f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.38f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.40f, 0.65f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.50f, 0.80f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.14f, 0.32f, 0.55f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.40f, 0.65f, 0.70f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.50f, 0.80f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.14f, 0.32f, 0.55f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.50f, 0.80f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.14f, 0.40f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.12f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.28f, 0.50f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.14f, 0.40f, 0.68f, 0.70f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.22f, 0.50f, 0.80f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.14f, 0.40f, 0.68f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.18f, 0.40f, 0.65f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.22f, 0.50f, 0.80f, 0.70f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.14f, 0.40f, 0.68f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.26f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.18f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);

#if AXIS_HAS_OPENGL_BACKEND
    ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(nativeWindow), true);
    ImGui_ImplOpenGL3_Init("#version 460");
#else
    ImGui_ImplGlfw_InitForOther(static_cast<GLFWwindow*>(nativeWindow), true);
    auto& sl = ServiceLocator::Instance();
    if (auto* graphicsCtx = sl.Resolve<IGraphicsContext>())
    {
        if (auto* backend = graphicsCtx->GetRenderBackend())
        {
            backend->ImGuiInit(nativeWindow);
        }
    }
#endif

    m_Initialized = true;
    LOGGER_INFO("ImGuiLayer") << "ImGui initialized (docking branch).";
}

void ImGuiLayer::BeginFrame()
{
    if (!m_Initialized)
        return;
#if AXIS_HAS_OPENGL_BACKEND
    ImGui_ImplOpenGL3_NewFrame();
#else
    auto& sl = ServiceLocator::Instance();
    if (auto* graphicsCtx = sl.Resolve<IGraphicsContext>())
    {
        if (auto* backend = graphicsCtx->GetRenderBackend())
        {
            backend->ImGuiNewFrame();
        }
    }
#endif
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::EndFrame()
{
    if (!m_Initialized)
        return;
    ImGui::Render();
#if AXIS_HAS_OPENGL_BACKEND
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#else
    auto& sl = ServiceLocator::Instance();
    if (auto* graphicsCtx = sl.Resolve<IGraphicsContext>())
    {
        if (auto* backend = graphicsCtx->GetRenderBackend())
        {
            backend->ImGuiRender();
        }
    }
#endif
}

void ImGuiLayer::Shutdown()
{
    if (!m_Initialized)
        return;
#if AXIS_HAS_OPENGL_BACKEND
    ImGui_ImplOpenGL3_Shutdown();
#else
    auto& sl = ServiceLocator::Instance();
    if (auto* graphicsCtx = sl.Resolve<IGraphicsContext>())
    {
        if (auto* backend = graphicsCtx->GetRenderBackend())
        {
            backend->ImGuiShutdown();
        }
    }
#endif
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_Initialized = false;
    LOGGER_INFO("ImGuiLayer") << "ImGui context destroyed.";
}

#endif

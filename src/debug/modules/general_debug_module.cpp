#include <debug/modules/general_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include <iomanip>
#include <sstream>
#include <intrin.h>

GeneralDebugModule::GeneralDebugModule() {}
GeneralDebugModule::~GeneralDebugModule() {}

void GeneralDebugModule::Init(Application *app)
{
    m_App = app;

    const GLubyte *renderer = glGetString(GL_RENDERER);
    if (renderer)
        m_GpuName = std::string((const char *)renderer);
    else
        m_GpuName = "Unknown GPU";

    m_CpuName = "Unknown CPU";

#ifdef _WIN32
    int cpuInfo[4] = {0};
    char cpuBrandString[0x40] = {0};

    __cpuid(cpuInfo, 0x80000000);
    unsigned int nExIds = cpuInfo[0];

    if (nExIds >= 0x80000004)
    {
        __cpuid((int *)(cpuBrandString + 0), 0x80000002);
        __cpuid((int *)(cpuBrandString + 16), 0x80000003);
        __cpuid((int *)(cpuBrandString + 32), 0x80000004);

        m_CpuName = std::string(cpuBrandString);

        size_t start = m_CpuName.find_first_not_of(" \t");
        size_t end = m_CpuName.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos)
            m_CpuName = m_CpuName.substr(start, end - start + 1);
    }
#endif
}

void GeneralDebugModule::OnUpdate(float dt)
{
    if (!m_App || !m_Enabled)
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
}

void GeneralDebugModule::Render(Scene &scene)
{
    // This module doesn't render anything, only logs to console
}

void GeneralDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_App || !m_Enabled)
        return;

    ProcessKey(keyboard, GLFW_KEY_F1, m_F1Pressed, [this]()
               { LogControls(); });

    ProcessKey(keyboard, GLFW_KEY_F2, m_F2Pressed, [this]()
               { LogDevices(); });

    ProcessKey(keyboard, GLFW_KEY_F3, m_F3Pressed, [this]()
               { LogStats(); });

    ProcessKey(keyboard, GLFW_KEY_F4, m_F4Pressed, [this]()
               { LogEntityStats(); });

    ProcessKey(keyboard, GLFW_KEY_F5, m_F5Pressed, [this]()
               { LogSceneGraph(); });

    ProcessKey(keyboard, GLFW_KEY_F11, m_F11Pressed, [this]()
               {
        bool paused = !m_App->IsPaused();
        m_App->SetPaused(paused);
        std::cout << "\n========== Game Pause (F11) ==========" << std::endl;
        std::cout << "[Debug] Game Paused: " << (paused ? "YES" : "NO") << std::endl;
        std::cout << "======================================" << std::endl; });

    ProcessKey(keyboard, GLFW_KEY_F12, m_F12Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
        
        if (shift)
        {
            auto& mouse = m_App->GetMouse();
            CursorMode current = mouse.GetCursorMode();
            CursorMode next = CursorMode::Normal;
            std::string modeName = "Normal";

            switch (current)
            {
            case CursorMode::Normal: next = CursorMode::Hidden; modeName = "Hidden"; break;
            case CursorMode::Hidden: next = CursorMode::LockedCenter; modeName = "LockedCenter"; break;
            case CursorMode::LockedCenter: next = CursorMode::LockedHiddenCenter; modeName = "LockedHiddenCenter"; break;
            case CursorMode::LockedHiddenCenter: next = CursorMode::Normal; modeName = "Normal"; break;
            }

            mouse.SetCursorMode(next);
            std::cout << "\n========== Cursor Mode (Shift+F12) ==========" << std::endl;
            std::cout << "[Debug] Cursor Mode: " << modeName << std::endl;
            std::cout << "============================================" << std::endl;
        }
        else
        {
            float current = m_App->GetTimeScale();
            float next = 1.0f;
            if (abs(current - 0.25f) < 0.01f) next = 0.5f;
            else if (abs(current - 0.5f) < 0.01f) next = 1.0f;
            else if (abs(current - 1.0f) < 0.01f) next = 1.5f;
            else if (abs(current - 1.5f) < 0.01f) next = 2.0f;
            else if (abs(current - 2.0f) < 0.01f) next = 0.25f;
            else next = 1.0f;
            
            m_App->SetTimeScale(next);
            std::cout << "\n========== Time Scale (F12) ==========" << std::endl;
            std::cout << "[Debug] Time Scale: " << next << "x" << std::endl;
            std::cout << "======================================" << std::endl;
        } });
}

void GeneralDebugModule::LogDevices()
{
    std::cout << "\n========== DEVICE DEBUG INFO (F2) ==========" << std::endl;
    std::cout << "[Hardware]" << std::endl;
    std::cout << "  GPU: " << m_GpuName << std::endl;
    std::cout << "  CPU: " << m_CpuName << std::endl;
    std::cout << std::endl;

    auto logHelper = [&](const std::string &category, const std::vector<DeviceInfo> &devices, const std::string &activeId)
    {
        std::cout << category << ":" << std::endl;
        for (const auto &dev : devices)
        {
            bool isActive = (dev.id == activeId);
            std::cout << "  [" << (isActive ? "*" : " ") << "] " << dev.name << (dev.isDefault ? " (Default)" : "") << std::endl;
        }
    };

    auto &mons = m_App->GetMonitorManager();
    std::string activeMonId = mons.GetCurrentDevice().id;
    logHelper("Monitors", mons.GetAllDevices(), activeMonId);

    auto &inputs = m_App->GetInputManager();
    auto allInputs = inputs.GetAllDevices();
    std::cout << "Inputs:" << std::endl;
    for (const auto &dev : allInputs)
    {
        bool isActive = dev.isDefault;
        std::cout << "  [" << (isActive ? "*" : " ") << "] " << dev.name << (dev.isDefault ? " (Default)" : "") << std::endl;
    }

    auto &audio = m_App->GetSoundManager();
    std::string activeAudio = audio.GetCurrentDevice().id;
    logHelper("Audio", audio.GetAllDevices(), activeAudio);

    std::cout << "============================================" << std::endl;
}

void GeneralDebugModule::LogSceneGraph()
{
    std::cout << "\n========== SCENE GRAPH DUMP (F5) ==========" << std::endl;
    auto view = m_App->GetScene().registry.view<InfoComponent>();
    int count = 0;
    for (auto entity : view)
    {
        const auto &info = view.get<InfoComponent>(entity);
        std::cout << "[" << count++ << "] ID: " << (uint32_t)entity
                  << " | Name: " << info.name
                  << " | Tag: " << info.tag << std::endl;
    }
    std::cout << "===========================================" << std::endl;
}

void GeneralDebugModule::LogControls()
{
    std::cout << "\n========== DEBUG CONTROLS ==========" << std::endl;
    std::cout << std::endl;
    std::cout << "INFORMATION & LOGGING:" << std::endl;
    std::cout << "  F1        : Show Controls (This List)" << std::endl;
    std::cout << "  F2        : Log Devices (Hardware Info)" << std::endl;
    std::cout << "  F3        : Log Performance Stats" << std::endl;
    std::cout << "  Shift+F3  : Toggle Entity Names" << std::endl;
    std::cout << "  F4        : Log Entity Stats" << std::endl;
    std::cout << "  Shift+F4  : Toggle Transform Gizmos" << std::endl;
    std::cout << "  F5        : Dump Scene Graph" << std::endl;
    std::cout << "  Shift+F5  : Toggle Light Gizmos" << std::endl;
    std::cout << std::endl;
    std::cout << "VISUAL TOGGLES:" << std::endl;
    std::cout << "  F6        : Toggle Wireframe" << std::endl;
    std::cout << "  Shift+F6  : Toggle Skybox" << std::endl;
    std::cout << "  F7        : Toggle No-Texture Mode" << std::endl;
    std::cout << "  Shift+F7  : Toggle Shadows" << std::endl;
    std::cout << std::endl;
    std::cout << "SYSTEM TOGGLES:" << std::endl;
    std::cout << "  F8        : Toggle Physics Debug" << std::endl;
    std::cout << "  Shift+F8  : Toggle Audio Debug" << std::endl;
    std::cout << "  F9        : Toggle UI System" << std::endl;
    std::cout << "  Shift+F9  : Toggle Particle Debug" << std::endl;
    std::cout << "  F10       : Toggle Stats Overlay (HUD)" << std::endl;
    std::cout << "  Shift+F10 : Cycle Overlay Modes (3 modes)" << std::endl;
    std::cout << std::endl;
    std::cout << "GAME CONTROL:" << std::endl;
    std::cout << "  F11       : Pause/Resume Game" << std::endl;
    std::cout << "  Shift+F11 : Toggle Debug Camera (Free Cam)" << std::endl;
    std::cout << "  F12       : Cycle Time Scale (0.25x -> 2x)" << std::endl;
    std::cout << "  Shift+F12 : Cycle Cursor Mode" << std::endl;
    std::cout << "====================================" << std::endl;
}

void GeneralDebugModule::LogStats()
{
    std::cout << "\n========== PERFORMANCE STATS (F3) ==========" << std::endl;
    std::cout << "FPS        : " << m_CurrentFps << std::endl;
    std::cout << "Frame Time : " << m_CurrentFrameTime << " ms" << std::endl;
    std::cout << "============================================" << std::endl;
}

void GeneralDebugModule::LogEntityStats()
{
    auto &reg = m_App->GetScene().registry;
    size_t total = reg.storage<entt::entity>().size();

    size_t uiEntities = reg.view<UITransformComponent>().size();
    size_t renderEntities = reg.view<MeshRendererComponent>().size();
    size_t physEntities = reg.view<RigidBodyComponent>().size();

    std::cout << "\n========== ENTITY DETAILED STATS (F4) ==========" << std::endl;
    std::cout << "Total Entities      : " << total << std::endl;
    std::cout << "Renderable Objects  : " << renderEntities << std::endl;
    std::cout << "UI Elements         : " << uiEntities << std::endl;
    std::cout << "Physics Objects     : " << physEntities << std::endl;
    std::cout << "================================================" << std::endl;
}

void GeneralDebugModule::ProcessKey(KeyboardManager &keyboard, int key, bool &pressedState, std::function<void()> action)
{
    if (keyboard.GetKey(key))
    {
        if (!pressedState)
        {
            action();
            pressedState = true;
        }
    }
    else
    {
        pressedState = false;
    }
}

#endif

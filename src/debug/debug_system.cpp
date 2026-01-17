#include <debug/debug_system.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include <iomanip>
#include <sstream>

DebugSystem::DebugSystem() {}
DebugSystem::~DebugSystem() {}

void DebugSystem::Init(Application* app)
{
    m_App = app;

    // Load Resources
    auto& res = m_App->GetResourceManager();
    
    // Load Debug Font (Using existing time.ttf or default if not found)
    res.LoadFont("debug_font", "resources/fonts/time.ttf", 24); 
    res.LoadShader("debug_text", "resources/shaders/text.vs", "resources/shaders/text.fs");
    
    // Ensure UI Quad exists for Debug
    if (!res.GetUIModel("debug_sys_model"))
    {
        res.CreateUIModel("debug_sys_model", UIType::Text); // Optimized for dynamic text
    }

    m_DebugFont = res.GetFont("debug_font");
    m_TextShader = res.GetShader("debug_text");
    m_TextQuad = res.GetUIModel("debug_sys_model");

    // Get GPU Info
    const GLubyte* renderer = glGetString(GL_RENDERER);
    if (renderer) m_GpuName = std::string((const char*)renderer);
    else m_GpuName = "Unknown GPU";
    
    
    m_CpuName = "CPU (Details Unavailable)"; 
}

void DebugSystem::OnUpdate(float dt)
{
    if (!m_App) return;

    auto& keyboard = m_App->GetKeyboard();

    // F1: Physics Debug
    if (keyboard.GetKey(GLFW_KEY_F1))
    {
        if (!m_F1Pressed)
        {
            TogglePhysicsDebug();
            m_F1Pressed = true;
        }
    }
    else { m_F1Pressed = false; }

    // F2: Log Devices
    if (keyboard.GetKey(GLFW_KEY_F2))
    {
        if (!m_F2Pressed)
        {
            LogDevices();
            m_F2Pressed = true;
        }
    }
    else { m_F2Pressed = false; }

    // F3: Log Stats
    if (keyboard.GetKey(GLFW_KEY_F3))
    {
        if (!m_F3Pressed)
        {
            LogStats();
            m_F3Pressed = true;
        }
    }
    else { m_F3Pressed = false; }

    // F4: Log Entity Stats
    if (keyboard.GetKey(GLFW_KEY_F4))
    {
        if (!m_F4Pressed)
        {
            LogEntityStats();
            m_F4Pressed = true;
        }
    }
    else { m_F4Pressed = false; }

    // F12: Stats Overlay
    if (keyboard.GetKey(GLFW_KEY_F12))
    {
        if (!m_F12Pressed)
        {
            ToggleStatsOverlay();
            m_F12Pressed = true;
        }
    }
    else { m_F12Pressed = false; }

    // FPS Counter Update
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

void DebugSystem::Render(Scene& scene)
{
    if (!m_App) return;

    int width = m_App->GetWidth();
    int height = m_App->GetHeight();

    // Physics Debug
    if (m_ShowPhysicsDebug)
    {
        auto& res = m_App->GetResourceManager();
        Shader* debugShader = res.GetShader("debugLine");
        if (debugShader)
        {
            m_App->GetPhysicsSystem().RenderDebug(scene, m_App->GetPhysicsWorld(), *debugShader, width, height);
        }
    }

    // Stats Overlay (F12)
    if (m_ShowStatsOverlay && m_DebugFont && m_TextShader && m_TextQuad)
    {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Prep Text
        size_t totalEntities = scene.registry.storage<entt::entity>().size();
        int renderedEntities = m_App->GetRenderSystem().GetRenderedCount();

        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "FPS: " << m_CurrentFps << " (" << m_CurrentFrameTime << " ms)\n";
        ss << "Entities: " << totalEntities << " | Rendered: " << renderedEntities;
        
        std::string fullText = ss.str();
        
        // Split lines manually for simple rendering
        std::istringstream textStream(fullText);
        std::string line;
        float yStart = 30.0f;
        float scale = 0.5f;
        
        while (std::getline(textStream, line)) {
            // Right align logic
            float textWidth = 0.0f;
            for (char c : line) {
                const Character& ch = m_DebugFont->GetCharacter(c);
                textWidth += (ch.Advance >> 6) * scale;
            }
            float x = (float)width - textWidth - 10.0f;
            
            RenderText(line, x, yStart, scale, glm::vec3(0.0f, 1.0f, 0.0f));
            yStart += 25.0f; // Line spacing
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
}

void DebugSystem::RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color)
{
    int width = m_App->GetWidth();
    int height = m_App->GetHeight();

    m_TextShader->use();
    glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    m_TextShader->setMat4("projection", projection);
    m_TextShader->setInt("text", 0);

    for (char c : text)
    {
        const Character& ch = m_DebugFont->GetCharacter(c);

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y + (ch.Size.y - ch.Bearing.y) * scale;
        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        std::vector<float> vertices = {
             xpos,     ypos - h,   0.0f, 0.0f,            
             xpos,     ypos,       0.0f, 1.0f,
             xpos + w, ypos,       1.0f, 1.0f,

             xpos,     ypos - h,   0.0f, 0.0f,
             xpos + w, ypos,       1.0f, 1.0f,
             xpos + w, ypos - h,   1.0f, 0.0f
        };

        m_TextQuad->DrawDynamic(*m_TextShader, ch.TextureID, glm::vec4(color, 1.0f), vertices);

        x += (ch.Advance >> 6) * scale;
    }
}

void DebugSystem::TogglePhysicsDebug()
{
    m_ShowPhysicsDebug = !m_ShowPhysicsDebug;
    std::cout << "[Debug] Physics Debug: " << (m_ShowPhysicsDebug ? "ON" : "OFF") << std::endl;
}

void DebugSystem::ToggleStatsOverlay()
{
    m_ShowStatsOverlay = !m_ShowStatsOverlay;
    std::cout << "[DebugSystem] Stats Overlay: " << (m_ShowStatsOverlay ? "ON" : "OFF") << std::endl;
}

void DebugSystem::LogDevices()
{
    std::cout << "\n========== DEVICE DEBUG INFO (F2) ==========" << std::endl;
    // Hardware Info
    std::cout << "[Hardware]" << std::endl;
    std::cout << "  GPU: " << m_GpuName << std::endl;
    std::cout << "  CPU: " << m_CpuName << std::endl;
    std::cout << std::endl;

    // Devices
    auto logHelper = [&](const std::string& category, const std::vector<DeviceInfo>& devices, const std::string& activeId) {
        std::cout << category << ":" << std::endl;
        for (const auto& dev : devices) {
            bool isActive = (dev.id == activeId);
            std::cout << "  [" << (isActive ? "*" : " ") << "] " << dev.name << (dev.isDefault ? " (Default)" : "") << std::endl;
        }
    };
    
    // Monitors
    auto& mons = m_App->GetMonitorManager();
    logHelper("Monitors", mons.GetAllDevices(), ""); 

    // Inputs
    auto& inputs = m_App->GetInputManager();
    logHelper("Inputs", inputs.GetAllDevices(), "");

    // Audio
    auto& audio = m_App->GetSoundManager();
    std::string activeAudio = audio.GetCurrentDevice().id;
    logHelper("Audio", audio.GetAllDevices(), activeAudio);
        
    std::cout << "============================================" << std::endl;
}

void DebugSystem::LogStats()
{
    std::cout << "\n========== PERFORMANCE STATS (F3) ==========" << std::endl;
    std::cout << "FPS        : " << m_CurrentFps << std::endl;
    std::cout << "Frame Time : " << m_CurrentFrameTime << " ms" << std::endl;
    std::cout << "============================================" << std::endl;
}

void DebugSystem::LogEntityStats()
{
    size_t total = m_App->GetScene().registry.storage<entt::entity>().size();
    int rendered = m_App->GetRenderSystem().GetRenderedCount();

    std::cout << "\n========== ENTITY STATS (F4) ==========" << std::endl;
    std::cout << "Total Entities    : " << total << std::endl;
    std::cout << "Rendered Objects  : " << rendered << std::endl;
    std::cout << "=======================================" << std::endl;
}

#endif

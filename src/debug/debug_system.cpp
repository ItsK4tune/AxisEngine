#include <debug/debug_system.h>
#include <script/script_registry.h>
#include <script/default_camera_controller.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include <iomanip>
#include <sstream>
#include <intrin.h> // For __cpuid on Windows

DebugSystem::DebugSystem() {}
DebugSystem::~DebugSystem() {}

void DebugSystem::Init(Application* app)
{
    m_App = app;

    // Load Resources
    auto& res = m_App->GetResourceManager();
    
    // Load Debug Font (Using existing time.ttf or default if not found)
    res.LoadFont("debug_font", "src/asset/fonts/time.ttf", 24); 
    res.LoadShader("debug_text", "src/asset/shaders/text.vs", "src/asset/shaders/text.fs");
    
    // Ensure UI Quad exists for Debug
    if (!res.GetUIModel("debug_sys_model"))
    {
        res.CreateUIModel("debug_sys_model", UIType::Text);
    }

    m_DebugFont = res.GetFont("debug_font");
    m_TextShader = res.GetShader("debug_text");
    m_TextQuad = res.GetUIModel("debug_sys_model");

    // Get GPU Info
    const GLubyte* renderer = glGetString(GL_RENDERER);
    if (renderer) m_GpuName = std::string((const char*)renderer);
    else m_GpuName = "Unknown GPU";
    
    // Get CPU Info using CPUID
    m_CpuName = "Unknown CPU";
    
#ifdef _WIN32
    // Use CPUID to get processor brand string
    int cpuInfo[4] = {0};
    char cpuBrandString[0x40] = {0};
    
    // Check if brand string is supported (EAX=0x80000000)
    __cpuid(cpuInfo, 0x80000000);
    unsigned int nExIds = cpuInfo[0];
    
    if (nExIds >= 0x80000004)
    {
        // Get the processor brand string (requires 3 calls)
        __cpuid((int*)(cpuBrandString +  0), 0x80000002);
        __cpuid((int*)(cpuBrandString + 16), 0x80000003);
        __cpuid((int*)(cpuBrandString + 32), 0x80000004);
        
        m_CpuName = std::string(cpuBrandString);
        
        // Trim leading/trailing spaces
        size_t start = m_CpuName.find_first_not_of(" \t");
        size_t end = m_CpuName.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos)
            m_CpuName = m_CpuName.substr(start, end - start + 1);
    }
#endif 
}

void DebugSystem::OnUpdate(float dt)
{
    if (!m_App) return;

    auto& keyboard = m_App->GetKeyboard();

    ProcessKey(keyboard, GLFW_KEY_F1, m_F1Pressed, [this](){ LogControls(); });
    ProcessKey(keyboard, GLFW_KEY_F2, m_F2Pressed, [this](){ LogDevices(); });
    ProcessKey(keyboard, GLFW_KEY_F3, m_F3Pressed, [this](){ LogStats(); });
    ProcessKey(keyboard, GLFW_KEY_F4, m_F4Pressed, [this](){ LogEntityStats(); });
    ProcessKey(keyboard, GLFW_KEY_F5, m_F5Pressed, [this](){ LogSceneGraph(); });

    ProcessKey(keyboard, GLFW_KEY_F6, m_F6Pressed, [this, &keyboard](){
        bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
        if (shift) {
            static bool skyboxEnabled = true;
            skyboxEnabled = !skyboxEnabled;
            m_App->GetSkyboxRenderSystem().SetEnabled(skyboxEnabled);
            std::cout << "\n========== Skybox Toggle (Shift+F6) ==========" << std::endl;
            std::cout << "[Debug] Skybox: " << (skyboxEnabled ? "ON" : "OFF") << std::endl;
            std::cout << "=============================================" << std::endl;
        } else {
            m_WireframeMode = !m_WireframeMode;
            if (m_WireframeMode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            std::cout << "\n========== Wireframe Mode (F6) ==========" << std::endl;
            std::cout << "[Debug] Wireframe: " << (m_WireframeMode ? "ON" : "OFF") << std::endl;
            std::cout << "=========================================" << std::endl;
        }
    });

    // F7 No Texture / Shadows
    ProcessKey(keyboard, GLFW_KEY_F7, m_F7Pressed, [this, &keyboard](){
         bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
         if (shift) {
             bool shadow = !m_App->GetRenderSystem().IsShadowsEnabled();
             m_App->GetRenderSystem().SetEnableShadows(shadow);
             std::cout << "\n========== Shadow Toggle (Shift+F7) ==========" << std::endl;
             std::cout << "[Debug] Shadows: " << (shadow ? "ON" : "OFF") << std::endl;
             std::cout << "============================================" << std::endl;
         } else {
             m_NoTextureMode = !m_NoTextureMode;
             m_App->GetRenderSystem().SetDebugNoTexture(m_NoTextureMode);
             std::cout << "\n========== No Texture Mode (F7) ==========" << std::endl;
             std::cout << "[Debug] No Texture Mode: " << (m_NoTextureMode ? "ON" : "OFF") << std::endl;
             std::cout << "==========================================" << std::endl;
         }
    });

    ProcessKey(keyboard, GLFW_KEY_F8, m_F8Pressed, [this, &keyboard](){
        bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
        if (shift) {
            m_ShowAudioDebug = !m_ShowAudioDebug;
            std::cout << "\n========== Audio Debug (Shift+F8) ==========" << std::endl;
            std::cout << "[Debug] Audio Debug: " << (m_ShowAudioDebug ? "ON" : "OFF") << std::endl;
            std::cout << "[Info] Shows 3D audio source positions" << std::endl;
            std::cout << "==========================================" << std::endl;
        } else {
            TogglePhysicsDebug();
        }
    });
    
    ProcessKey(keyboard, GLFW_KEY_F9, m_F9Pressed, [this, &keyboard](){
        bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
        if (shift) {
            m_ShowParticleDebug = !m_ShowParticleDebug;
            std::cout << "\n========== Particle Debug (Shift+F9) ==========" << std::endl;
            std::cout << "[Debug] Particle Debug: " << (m_ShowParticleDebug ? "ON" : "OFF") << std::endl;
            std::cout << "[Info] Shows particle emitter boundaries" << std::endl;
            std::cout << "=============================================" << std::endl;
        } else {
            static bool uiEnabled = true;
            uiEnabled = !uiEnabled;
            m_App->GetUIRenderSystem().SetEnabled(uiEnabled);
            std::cout << "\n========== UI System (F9) ==========" << std::endl;
            std::cout << "[Debug] UI System: " << (uiEnabled ? "ON" : "OFF") << std::endl;
            std::cout << "====================================" << std::endl;
        }
    });

    ProcessKey(keyboard, GLFW_KEY_F10, m_F10Pressed, [this, &keyboard](){
        bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
        if (shift) {
            m_OverlayMode = (m_OverlayMode % 3) + 1;
            std::cout << "\n========== Overlay Mode (Shift+F10) ==========" << std::endl;
            std::cout << "[Debug] Mode " << m_OverlayMode << "/3: ";
            if (m_OverlayMode == 1) std::cout << "FPS Only";
            else if (m_OverlayMode == 2) std::cout << "FPS + Entities";
            else if (m_OverlayMode == 3) std::cout << "Advanced Stats";
            std::cout << std::endl;
            std::cout << "============================================" << std::endl;
        } else {
            ToggleStatsOverlay();
        }
    });

    // F11 Pause / Debug Camera
    ProcessKey(keyboard, GLFW_KEY_F11, m_F11Pressed, [this, &keyboard](){
        bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
        if (shift)
        {
            ToggleDebugCamera();
        }
        else
        {
            bool paused = !m_App->IsPaused();
            m_App->SetPaused(paused);
            std::cout << "\n========== Game Pause (F11) ==========" << std::endl;
            std::cout << "[Debug] Game Paused: " << (paused ? "YES" : "NO") << std::endl;
            std::cout << "======================================" << std::endl;
        }
    });

    // F12 Slow Mo / Cursor Mode
    ProcessKey(keyboard, GLFW_KEY_F12, m_F12Pressed, [this, &keyboard](){
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
            // Cycle: 0.25 -> 0.5 -> 1.0 -> 1.5 -> 2.0 -> 0.25
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
        }
    });
    
    // FPS Calculation (Realtime, unaware of pause)
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
void DebugSystem::LogDevices()
{
    std::cout << "\n========== DEVICE DEBUG INFO (F2) ==========" << std::endl;
    std::cout << "[Hardware]" << std::endl;
    std::cout << "  GPU: " << m_GpuName << std::endl;
    std::cout << "  CPU: " << m_CpuName << std::endl;
    std::cout << std::endl;

    auto logHelper = [&](const std::string& category, const std::vector<DeviceInfo>& devices, const std::string& activeId) {
        std::cout << category << ":" << std::endl;
        for (const auto& dev : devices) {
            // Check active based on ID
            bool isActive = (dev.id == activeId);
            std::cout << "  [" << (isActive ? "*" : " ") << "] " << dev.name << (dev.isDefault ? " (Default)" : "") << std::endl;
        }
    };
    
    // Monitors
    auto& mons = m_App->GetMonitorManager();
    std::string activeMonId = mons.GetCurrentDevice().id;
    logHelper("Monitors", mons.GetAllDevices(), activeMonId); 

    // Inputs
    auto& inputs = m_App->GetInputManager();
    auto allInputs = inputs.GetAllDevices();
    std::cout << "Inputs:" << std::endl;
    for (const auto& dev : allInputs) {
        // Assume default devices are always "Active"
        bool isActive = dev.isDefault; 
        std::cout << "  [" << (isActive ? "*" : " ") << "] " << dev.name << (dev.isDefault ? " (Default)" : "") << std::endl;
    }

    // Audio
    auto& audio = m_App->GetSoundManager();
    std::string activeAudio = audio.GetCurrentDevice().id;
    logHelper("Audio", audio.GetAllDevices(), activeAudio);
        
    std::cout << "============================================" << std::endl;
}


void DebugSystem::LogSceneGraph()
{
    std::cout << "\n========== SCENE GRAPH DUMP (F5) ==========" << std::endl;
    auto view = m_App->GetScene().registry.view<InfoComponent>();
    int count = 0;
    for(auto entity : view) {
            const auto& info = view.get<InfoComponent>(entity);
            std::cout << "[" << count++ << "] ID: " << (uint32_t)entity 
                      << " | Name: " << info.name 
                      << " | Tag: " << info.tag << std::endl;
    }
    std::cout << "===========================================" << std::endl;
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

        size_t totalEntities = scene.registry.storage<entt::entity>().size();
        int renderedEntities = m_App->GetRenderSystem().GetRenderedCount();

        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        
        if (m_OverlayMode == 1) {
            ss << "FPS: " << m_CurrentFps << " (" << m_CurrentFrameTime << " ms)";
        } else if (m_OverlayMode == 2) {
            ss << "FPS: " << m_CurrentFps << " (" << m_CurrentFrameTime << " ms)\n";
            ss << "Entities: " << totalEntities << " | Rendered: " << renderedEntities;
        } else if (m_OverlayMode == 3) {
            ss << "FPS: " << m_CurrentFps << " (" << m_CurrentFrameTime << " ms)\n";
            ss << "Entities: " << totalEntities << " | Rendered: " << renderedEntities << "\n";
            ss << "TimeScale: " << m_App->GetTimeScale() << "x | Paused: " << (m_App->IsPaused() ? "YES" : "NO");
        }
        
        std::string fullText = ss.str();
        
        std::istringstream textStream(fullText);
        std::string line;
        float yStart = 30.0f;
        float scale = 0.5f;
        
        while (std::getline(textStream, line)) {
            float textWidth = 0.0f;
            for (char c : line) {
                const Character& ch = m_DebugFont->GetCharacter(c);
                textWidth += (ch.Advance >> 6) * scale;
            }
            float x = (float)width - textWidth - 10.0f;
            
            RenderText(line, x, yStart, scale, glm::vec3(0.0f, 1.0f, 0.0f));
            yStart += 25.0f;
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
    
    if (m_ShowAudioDebug)
    {
        auto view = scene.registry.view<AudioSourceComponent, TransformComponent>();
        for (auto entity : view) {
            const auto& transform = view.get<TransformComponent>(entity);
            glm::vec3 pos = transform.GetWorldModelMatrix(scene.registry)[3];
        }
    }
    
    if (m_ShowParticleDebug)
    {
        auto view = scene.registry.view<ParticleEmitterComponent, TransformComponent>();
        for (auto entity : view) {
            const auto& transform = view.get<TransformComponent>(entity);
            glm::vec3 pos = transform.GetWorldModelMatrix(scene.registry)[3];
        }
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

        m_TextQuad->DrawDynamic(*m_TextShader, ch.TextureID, color, vertices);

        x += (ch.Advance >> 6) * scale;
    }
}

void DebugSystem::TogglePhysicsDebug()
{
    m_ShowPhysicsDebug = !m_ShowPhysicsDebug;
    std::cout << "\n========== Physics Debug (F8) ==========" << std::endl;
    std::cout << "[Debug] Physics Debug: " << (m_ShowPhysicsDebug ? "ON" : "OFF") << std::endl;
    std::cout << "========================================" << std::endl;
}

void DebugSystem::ToggleStatsOverlay()
{
    m_ShowStatsOverlay = !m_ShowStatsOverlay;
    std::cout << "\n========== Stats Overlay (F10) ==========" << std::endl;
    std::cout << "[DebugSystem] Stats Overlay: " << (m_ShowStatsOverlay ? "ON" : "OFF") << std::endl;
    
    // Resource Check
    if (m_ShowStatsOverlay)
    {
        bool fontOK = (m_DebugFont != nullptr);
        bool shaderOK = (m_TextShader != nullptr);
        bool quadOK = (m_TextQuad != nullptr);
        
        std::cout << "[DebugSystem] Resources Status:" << std::endl;
        std::cout << "  Font:   " << (fontOK ? "OK" : "MISSING (Check src/asset/fonts/time.ttf)") << std::endl;
        std::cout << "  Shader: " << (shaderOK ? "OK" : "MISSING (Check src/asset/shaders/text.vs/fs)") << std::endl;
        std::cout << "  Quad:   " << (quadOK ? "OK" : "MISSING") << std::endl;
        
        if (!fontOK || !shaderOK || !quadOK) {
             std::cout << "[WARNING] Overlay will NOT render due to missing resources!" << std::endl;
        }
    }
    std::cout << "=========================================" << std::endl;
}

void DebugSystem::LogControls()
{
    std::cout << "\n========== DEBUG CONTROLS ==========" << std::endl;
    std::cout << std::endl;
    std::cout << "INFORMATION & LOGGING:" << std::endl;
    std::cout << "  F1        : Show Controls (This List)" << std::endl;
    std::cout << "  F2        : Log Devices (Hardware Info)" << std::endl;
    std::cout << "  F3        : Log Performance Stats" << std::endl;
    std::cout << "  F4        : Log Entity Stats" << std::endl;
    std::cout << "  F5        : Dump Scene Graph" << std::endl;
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

void DebugSystem::ProcessKey(KeyboardManager& keyboard, int key, bool& pressedState, std::function<void()> action)
{
    if (keyboard.GetKey(key))
    {
        if (!pressedState)
        {
            action();
            pressedState = true;
        }
    } else { pressedState = false; }
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
    auto& reg = m_App->GetScene().registry;
    size_t total = reg.storage<entt::entity>().size();
    
    // Count specific types
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


void DebugSystem::ToggleDebugCamera()
{
    auto& scene = m_App->GetScene();
    auto& registry = scene.registry;

    if (m_IsDebugCameraActive)
    {
        // Switch BACK to user camera
        // Disable debug camera
        if (registry.valid(m_DebugCamera) && registry.all_of<CameraComponent>(m_DebugCamera))
        {
            registry.get<CameraComponent>(m_DebugCamera).isPrimary = false;
        }

        // Re-enable user camera
        if (registry.valid(m_LastActiveCamera) && registry.all_of<CameraComponent>(m_LastActiveCamera))
        {
            registry.get<CameraComponent>(m_LastActiveCamera).isPrimary = true;
            std::cout << "[Debug] Switched to User Camera (Entity " << (uint32_t)m_LastActiveCamera << ")" << std::endl;
        }
        else
        {
            // If user camera is gone, try to find ANY camera
            entt::entity fallback = scene.GetActiveCamera();
            if (fallback == entt::null)
            {
                // Iterate to find one
                 auto view = registry.view<CameraComponent>();
                 for(auto entity : view) {
                     if (entity != m_DebugCamera) {
                         view.get<CameraComponent>(entity).isPrimary = true;
                         std::cout << "[Debug] Original camera invalid. Switched to fallback camera (Entity " << (uint32_t)entity << ")" << std::endl;
                         break;
                     }
                 }
            }
        }
        
        m_IsDebugCameraActive = false;
        std::cout << "========== Debug Camera: OFF ==========" << std::endl;
    }
    else
    {
        // Switch TO Debug Camera
        m_LastActiveCamera = scene.GetActiveCamera(); // Save current valid camera

        // Disable current camera
        if (registry.valid(m_LastActiveCamera))
        {
            if(registry.all_of<CameraComponent>(m_LastActiveCamera))
                registry.get<CameraComponent>(m_LastActiveCamera).isPrimary = false;
        }
        
        // Ensure Debug Camera exists
        if (!registry.valid(m_DebugCamera))
        {
            m_DebugCamera = scene.createEntity();
            registry.emplace<InfoComponent>(m_DebugCamera, "Debug Camera", "Debug");
            
            auto& trans = registry.emplace<TransformComponent>(m_DebugCamera);
            // Copy position from last camera if possible, or use default
            if (registry.valid(m_LastActiveCamera) && registry.all_of<TransformComponent>(m_LastActiveCamera)) {
                trans.position = registry.get<TransformComponent>(m_LastActiveCamera).position;
                // Offset slightly to avoid Z-fighting or clipping if exact same pos? Not really needed for camera.
            } else {
                trans.position = glm::vec3(0.0f, 5.0f, 10.0f);
            }

            auto& cam = registry.emplace<CameraComponent>(m_DebugCamera);
            cam.isPrimary = true;
            cam.fov = 45.0f;
            cam.nearPlane = 0.1f;
            cam.farPlane = 1000.0f;
            
            // Script
            std::string scriptName = "DefaultCameraController";
            Scriptable* scriptInstance = ScriptRegistry::Instance().Create(scriptName);
            if (scriptInstance)
            {
                 auto& scriptComp = registry.emplace<ScriptComponent>(m_DebugCamera);
                 scriptComp.instance = scriptInstance;
                 scriptComp.InstantiateScript = [scriptName](){ return ScriptRegistry::Instance().Create(scriptName); };
                 scriptComp.DestroyScript = [](ScriptComponent* nsc){ delete nsc->instance; nsc->instance = nullptr; };
                 scriptComp.instance->Init(m_DebugCamera, &scene, m_App);
                 scriptComp.instance->OnCreate();
            }
        }
        else
        {
             // Enable existing debug camera
             if (registry.all_of<CameraComponent>(m_DebugCamera)) {
                 registry.get<CameraComponent>(m_DebugCamera).isPrimary = true;
                 
                 // Smooth transition: Copy position from User Camera
                 if (registry.valid(m_LastActiveCamera) && registry.all_of<TransformComponent>(m_LastActiveCamera)) {
                    auto& userTrans = registry.get<TransformComponent>(m_LastActiveCamera);
                    auto& debugTrans = registry.get<TransformComponent>(m_DebugCamera);
                    debugTrans.position = userTrans.position;
                    debugTrans.rotation = userTrans.rotation;
                 }
             }
        }

        m_IsDebugCameraActive = true;
        std::cout << "========== Debug Camera: ON ==========" << std::endl;
        std::cout << "[Debug] Switched to Free Cam (Shift+F11)" << std::endl;
    }
}

#endif


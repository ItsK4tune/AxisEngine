#pragma once

#ifdef ENABLE_DEBUG_SYSTEM

#include <graphic/font.h>
#include <graphic/ui_model.h>
#include <graphic/shader.h>

class Application;
class Scene;

class DebugSystem
{
public:
    DebugSystem();
    ~DebugSystem();

    void Init(Application* app);
    void OnUpdate(float dt);
    void Render(Scene& scene);

private:
    void TogglePhysicsDebug();
    void ToggleStatsOverlay();
    
    void LogDevices();
    void LogStats(); // F3
    void LogEntityStats(); // F4

    // Text Rendering
    void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color);

    Application* m_App = nullptr;

    bool m_F1Pressed = false; // Physics Debug Toggle
    bool m_F2Pressed = false; // Log Devices
    bool m_F3Pressed = false; // Log Stats
    bool m_F4Pressed = false; // Log Entity Stats
    bool m_F12Pressed = false; // Toggle Overlay

    bool m_ShowPhysicsDebug = false;
    bool m_ShowStatsOverlay = false;
    
    // Stats
    float m_FpsTimer = 0.0f;
    int m_FrameCount = 0;
    float m_CurrentFps = 0.0f;
    float m_CurrentFrameTime = 0.0f;

    // Resources for Text
    Font* m_DebugFont = nullptr;
    Shader* m_TextShader = nullptr;
    UIModel* m_TextQuad = nullptr;
    
    std::string m_GpuName;
    std::string m_CpuName;
};


#endif

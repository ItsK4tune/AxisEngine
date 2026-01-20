#pragma once

#ifdef ENABLE_DEBUG_SYSTEM

#include <graphic/font.h>
#include <graphic/ui_model.h>
#include <graphic/shader.h>
#include <functional>
#include <input/keyboard_manager.h>
#include <entt/entity/entity.hpp>

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
    void LogControls(); // F1
    void LogSceneGraph(); // F5

    // Text Rendering
    void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color);
    
    // Helper
    void ProcessKey(KeyboardManager& keyboard, int key, bool& pressedState, std::function<void()> action);

    void ToggleDebugCamera();

    Application* m_App = nullptr;

    bool m_F1Pressed = false; // Controls
    bool m_F2Pressed = false; // Devices
    bool m_F3Pressed = false; // Perf Stats
    bool m_F4Pressed = false; // Entity Stats
    bool m_F5Pressed = false; // Scene Graph
    bool m_F6Pressed = false; // Wireframe
    bool m_F7Pressed = false; // No Texture Mode
    bool m_F8Pressed = false; // Physics Debug
    bool m_F9Pressed = false; // UI Toggle
    bool m_F10Pressed = false; // Stats Overlay Toggle
    bool m_F11Pressed = false; // Pause
    bool m_F12Pressed = false; // Slow Mo

    bool m_ShowPhysicsDebug = false;
    bool m_ShowStatsOverlay = true; 
    bool m_WireframeMode = false;
    bool m_NoTextureMode = false;
    bool m_ShowAudioDebug = false;
    bool m_ShowParticleDebug = false;
    int m_OverlayMode = 1;
    
    // Extended Visualization
    bool m_ShowEntityNames = false;
    bool m_ShowTransformGizmos = false;
    bool m_ShowLightGizmos = false;

    void ToggleEntityNames() { m_ShowEntityNames = !m_ShowEntityNames; }
    void ToggleTransformGizmos() { m_ShowTransformGizmos = !m_ShowTransformGizmos; }
    void ToggleLightGizmos() { m_ShowLightGizmos = !m_ShowLightGizmos; }
    
    // Debug Camera State
    bool m_IsDebugCameraActive = false;
    entt::entity m_LastActiveCamera = entt::null;
    entt::entity m_DebugCamera = entt::null;

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

    // Entity Labels Management
    std::unordered_map<entt::entity, entt::entity> m_EntityLabelMap;
    void UpdateDebugLabels(Scene& scene);
    void ClearDebugLabels(Scene& scene);

    // Light Labels Management
    std::unordered_map<entt::entity, entt::entity> m_LightLabelMap;
    void UpdateLightLabels(Scene& scene);
    void ClearLightLabels(Scene& scene);
};


#endif

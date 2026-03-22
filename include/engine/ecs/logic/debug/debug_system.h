#pragma once

#include <memory>
#include <vector>
#include <core/interface/i_debug_system.h>

struct Scene;

// --- Debug Interface ---

// IDebugSystem moved to <core/interface/i_debug_system.h>

// --- Debug Config ---

#ifdef ENABLE_DEBUG_SYSTEM

struct DebugConfig
{
    static bool ShowWireframe;
    static bool ShowPhysics;
    static bool ShowGizmos;
    static bool ShowEntityNames;
    static bool ShowLightGizmos;
};

// --- Debug Implementation ---

#include <resource/unit/font.h>
#include <resource/unit/shader.h>
#include <resource/unit/ui_model.h>

class IDebugModule;

class DebugSystem : public IDebugSystem
{
public:
    DebugSystem();
    ~DebugSystem();

    void Initialize() override;
    void OnUpdate(float dt) override;
    void Render(Scene& scene) override;
 
private:

    std::shared_ptr<Font> m_DebugFont = nullptr;
    std::shared_ptr<Shader> m_TextShader = nullptr;
    std::shared_ptr<UIModel> m_TextQuad = nullptr;

    float m_FpsTimer = 0.0f;
    int m_FrameCount = 0;
    float m_CurrentFps = 0.0f;
    float m_CurrentFrameTime = 0.0f;

    std::vector<std::unique_ptr<IDebugModule>> m_Modules;
};

#else

// --- Null Implementation ---

class NullDebugSystem : public IDebugSystem
{
public:
    void Initialize() override {}
    void OnUpdate(float) override {}
    void Render(Scene&) override {}
};

#endif

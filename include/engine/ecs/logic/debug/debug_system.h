#pragma once

#include <ecs/interface/i_update_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_base_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <memory>
#include <string>
#include <vector>

struct Scene;

// --- Debug Interface ---

// IDebugSystem removed in favor of IUpdateSystem and IRenderSystem

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

class DebugSystem : public IUpdateSystem, public IRenderSystem, public IECSSystem
{
public:
    DebugSystem();
    ~DebugSystem();

    void Initialize() override;
    void Shutdown() override {}
    
    void Update(Scene& scene, float dt) override { OnUpdate(dt); }
    void OnUpdate(float dt);
    
    void Render(Scene& scene) override;
    
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetName() const override { return "DebugSystem"; }
    int GetPriority() const override { return 1000; } // Rendered last
 
    std::vector<entt::id_type> GetReadComponents() const override { return {}; }
    std::vector<entt::id_type> GetWriteComponents() const override { return {}; }

private:

    bool m_Enabled = true;
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

class NullDebugSystem : public IUpdateSystem, public IRenderSystem, public IECSSystem
{
public:
    void Initialize() override {}
    void Shutdown() override {}
    void Update(Scene&, float) override {}
    void OnUpdate(float) {}
    void Render(Scene&) override {}
    bool IsEnabled() const override { return false; }
    void SetEnabled(bool) override {}
    std::string GetName() const override { return "NullDebugSystem"; }
    int GetPriority() const override { return 1000; }
    
    std::vector<entt::id_type> GetReadComponents() const override { return {}; }
    std::vector<entt::id_type> GetWriteComponents() const override { return {}; }
};

#endif

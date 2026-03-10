#pragma once

#include <memory>
#include <vector>
#include <core/unit/engine_context.h>

class Scene;

// --- Debug Interface ---

class IDebugSystem
{
public:
    virtual ~IDebugSystem() = default;
    virtual void Initialize(EngineContext ctx) = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void Render(Scene& scene) = 0;
};

// --- Debug Config ---

#ifdef ENABLE_DEBUG_SYSTEM

struct DebugConfig
{
    static bool ShowWireframe;
    static bool ShowPhysics;
    static bool ShowGizmos;
};

// --- Debug Implementation ---

#include <render/logic/font.h>
#include <render/logic/shader.h>
#include <render/logic/ui_model.h>

class IDebugModule;

class DebugSystem : public IDebugSystem
{
public:
    DebugSystem();
    ~DebugSystem();

    void Initialize(EngineContext ctx) override;
    void OnUpdate(float dt) override;
    void Render(Scene& scene) override;

private:
    EngineContext m_Ctx;

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
    void Initialize(EngineContext) override {}
    void OnUpdate(float) override {}
    void Render(Scene&) override {}
};

#endif

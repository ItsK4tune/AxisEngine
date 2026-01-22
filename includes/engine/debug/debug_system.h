#pragma once

#ifdef ENABLE_DEBUG_SYSTEM

#include <graphic/renderer/font.h>
#include <graphic/renderer/ui_model.h>
#include <graphic/core/shader.h>
#include <memory>
#include <vector>

class Application;
class Scene;
class IDebugModule;

class DebugSystem
{
public:
    DebugSystem();
    ~DebugSystem();

    void Init(Application *app);
    void OnUpdate(float dt);
    void Render(Scene &scene);

private:
    Application *m_App = nullptr;

    Font *m_DebugFont = nullptr;
    Shader *m_TextShader = nullptr;
    UIModel *m_TextQuad = nullptr;

    float m_FpsTimer = 0.0f;
    int m_FrameCount = 0;
    float m_CurrentFps = 0.0f;
    float m_CurrentFrameTime = 0.0f;

    std::vector<std::unique_ptr<IDebugModule>> m_Modules;
};

#endif

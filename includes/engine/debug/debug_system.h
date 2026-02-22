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

    void Init(std::shared_ptr<Application> app);
    void OnUpdate(float dt);
    void Render(Scene &scene);

private:
    std::shared_ptr<Application> m_App = nullptr;

    std::shared_ptr<Font> m_DebugFont = nullptr;
    std::shared_ptr<Shader> m_TextShader = nullptr;
    std::shared_ptr<UIModel> m_TextQuad = nullptr;

    float m_FpsTimer = 0.0f;
    int m_FrameCount = 0;
    float m_CurrentFps = 0.0f;
    float m_CurrentFrameTime = 0.0f;

    std::vector<std::unique_ptr<IDebugModule>> m_Modules;
};

#endif

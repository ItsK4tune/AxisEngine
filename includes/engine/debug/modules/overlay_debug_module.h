#pragma once

#ifdef ENABLE_DEBUG_SYSTEM

#include <debug/modules/i_debug_module.h>
#include <graphic/renderer/font.h>
#include <graphic/renderer/ui_model.h>
#include <graphic/core/shader.h>
#include <functional>
#include <string>

class Application;

class OverlayDebugModule : public IDebugModule
{
public:
    OverlayDebugModule();
    ~OverlayDebugModule() override;

    void Init(Application *app) override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "OverlayDebugModule"; }

    void SetSharedResources(Font *font, Shader *shader, UIModel *quad);

    void SetStats(float fps, float frameTime);

private:
    void ToggleStatsOverlay();
    void RenderText(const std::string &text, float x, float y, float scale, glm::vec3 color);
    void ProcessKey(KeyboardManager &keyboard, int key, bool &pressedState, std::function<void()> action);

    Application *m_App = nullptr;
    bool m_Enabled = true;

    bool m_F10Pressed = false;
    bool m_ShowStatsOverlay = true;
    int m_OverlayMode = 1; // 1=Stats, 2=Tools, 3=All

    float m_CurrentFps = 0.0f;
    float m_CurrentFrameTime = 0.0f;

    Font *m_DebugFont = nullptr;
    Shader *m_TextShader = nullptr;
    UIModel *m_TextQuad = nullptr;
};

#endif

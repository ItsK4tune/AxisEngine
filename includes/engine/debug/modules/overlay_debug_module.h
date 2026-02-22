#pragma once

#ifdef ENABLE_DEBUG_SYSTEM

#include <interface/debug/i_debug_module.h>
#include <vector>
#include <string>
#include <interface/window/input_codes.h>
#include <graphic/renderer/ui_model.h>
#include <graphic/core/shader.h>
#include <memory>
#include <functional>
#include <string>

class Font;
class Application;

class OverlayDebugModule : public IDebugModule
{
public:
    OverlayDebugModule();
    ~OverlayDebugModule() override;

    void Init(std::shared_ptr<Application> app) override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "OverlayDebugModule"; }

    void SetSharedResources(std::shared_ptr<Font> font, std::shared_ptr<Shader> shader, std::shared_ptr<UIModel> quad);

    void SetStats(float fps, float frameTime);

private:
    void ToggleStatsOverlay();
    void RenderText(const std::string &text, float x, float y, float scale, glm::vec3 color);
    void ProcessKey(KeyboardManager &keyboard, Input::Key key, bool &pressedState, std::function<void()> action);

    std::shared_ptr<Application> m_App = nullptr;
    bool m_Enabled = true;

    bool m_F10Pressed = false;
    bool m_ShowStatsOverlay = true;
    int m_OverlayMode = 1;

    float m_CurrentFps = 0.0f;
    float m_CurrentFrameTime = 0.0f;

    std::shared_ptr<Font> m_DebugFont = nullptr;
    std::shared_ptr<Shader> m_TextShader = nullptr;
    std::shared_ptr<UIModel> m_TextQuad = nullptr;
};

#endif

#pragma once

#include <core/interface/i_debug_module.h>
#include <core/unit/engine_context.h>
#include <functional>
#include <memory>
#include <platform/interface/input_codes.h>
#include <render/logic/shader.h>
#include <render/logic/ui_model.h>
#include <string>
#include <vector>

class Application;
class Font;

#ifdef ENABLE_DEBUG_SYSTEM



class OverlayDebugModule : public IDebugModule
{
public:
    OverlayDebugModule();
    ~OverlayDebugModule() override;

    void Initialize(EngineContext ctx) override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "OverlayDebugModule"; }
    int GetRenderOrder() const override { return 100; }

    void SetSharedResources(std::shared_ptr<Font> font, std::shared_ptr<Shader> shader, std::shared_ptr<UIModel> quad);

    void SetStats(float fps, float frameTime);

private:
    void ToggleStatsOverlay();
    void RenderText(const std::string &text, float x, float y, float scale, glm::vec3 color);
    void ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action);

    EngineContext m_Ctx;
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
#pragma once

#include <core/interface/i_debug_module.h>
#include <core/unit/engine_context.h>
#include <entt/entity/entity.hpp>
#include <functional>
#include <platform/interface/input_codes.h>
#include <string>

class Application;

#ifdef ENABLE_DEBUG_SYSTEM



class CameraDebugModule : public IDebugModule
{
public:
    CameraDebugModule();
    ~CameraDebugModule() override;

    virtual void Init(EngineContext ctx) override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "CameraDebugModule"; }

    bool IsDebugCameraActive() const { return m_IsDebugCameraActive; }
    entt::entity GetDebugCamera() const { return m_DebugCamera; }

private:
    void ToggleDebugCamera();
    void ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action);

    EngineContext m_Ctx;
    bool m_Enabled = true;

    bool m_F11Pressed = false;

    bool m_IsDebugCameraActive = false;
    entt::entity m_LastActiveCamera = entt::null;
    entt::entity m_DebugCamera = entt::null;
};

#endif
#pragma once

#include <editor/i_editor_module.h>
#include <platform/interface/input_codes.h>
#include <entt/entity/entity.hpp>
#include <functional>
#include <string>

class Application;

#ifdef ENABLE_EDITOR

class CameraEditorModule : public IEditorModule
{
public:
    CameraEditorModule();
    ~CameraEditorModule() override;

    virtual void Initialize() override;
    void OnUpdate(float dt) override;
    void Render(Scene& scene) override;
    void ProcessInput(KeyboardManager& keyboard) override;

    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enabled) override
    {
        m_Enabled = enabled;
    }
    std::string GetModuleName() const override
    {
        return "CameraEditorModule";
    }

    bool IsDebugCameraActive() const
    {
        return m_IsDebugCameraActive;
    }
    entt::entity GetDebugCamera() const
    {
        return m_DebugCamera;
    }

private:
    void ToggleDebugCamera();
    void ProcessKey(KeyboardManager& keyboard, Key key, bool& pressedState, std::function<void()> action);

    bool m_Enabled = true;

    bool m_F11Pressed = false;

    bool m_IsDebugCameraActive = false;
    entt::entity m_LastActiveCamera = entt::null;
    entt::entity m_DebugCamera = entt::null;
};

#endif

#pragma once

#include <editor/i_editor_module.h>
#include <platform/interface/input_codes.h>
#include <entt/entity/entity.hpp>
#include <functional>

class Application;

#ifdef ENABLE_EDITOR

class CameraEditorModule : public IEditorModule
{
public:
    CameraEditorModule() = default;
    ~CameraEditorModule() override = default;

    void OnUpdate(float dt) override;
    void ProcessInput(KeyboardManager& keyboard) override;

private:
    void ToggleDebugCamera();
    void ProcessKey(KeyboardManager& keyboard, Key key, bool& pressedState, std::function<void()> action);

    bool m_F11Pressed = false;

    bool m_IsDebugCameraActive = false;
    entt::entity m_LastActiveCamera = entt::null;
    entt::entity m_DebugCamera = entt::null;
};

#endif

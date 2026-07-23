#pragma once

#include <editor/i_editor_module.h>
#include <entt/entity/entity.hpp>

#ifdef ENABLE_EDITOR

class CameraEditorModule : public IEditorModule
{
public:
    CameraEditorModule() = default;
    ~CameraEditorModule() override = default;

    void Shutdown() override;
    void ProcessInput(KeyboardManager& keyboard) override;
    void OnUpdate(float dt) override;
    void SetEnabled(bool enabled) override;

private:
    void ToggleDebugCamera();
    entt::entity EnsureDebugCamera(Scene& scene);
    void ActivateDebugCamera(Scene& scene);
    void RestoreGameCamera(Scene& scene);
    bool m_F10Pressed = false;

    bool m_IsDebugCameraActive = false;
    entt::entity m_LastActiveCamera = entt::null;
    entt::entity m_DebugCamera = entt::null;
    Scene* m_DebugCameraScene = nullptr;
    bool m_RestoreCursorMode = false;
};

#endif

#pragma once

#include <platform/interface/i_device_manager.h>
#include <platform/interface/i_window.h>
#include <platform/interface/input_codes.h>
#include <platform/type/input_binding.h>
#include <core/logic/event_system.h>
#include <scene/logic/scene.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

// --- Keyboard Manager ---

class KeyboardManager
{
public:
    KeyboardManager(IWindow *window);

    void Update();

    bool GetKey(Key key) const;
    bool GetKeyUp(Key key) const;
    bool IsKeyDown(Key key) const;

private:
    friend class IOHandler;
    void SetWindow(IWindow* window);

    IWindow *m_Window = nullptr;
    std::unordered_map<Key, bool> m_CurrentState;
    std::unordered_map<Key, bool> m_PreviousState;
};

// --- Mouse Manager ---

class MouseManager
{
public:
    MouseManager(IWindow *window);

    void UpdatePosition(double xpos, double ypos);
    void UpdateScroll(double xoffset, double yoffset);
    void UpdateButton(Mouse button, int action, int mods);
    void Update();
    void EndFrame();

    void SetCursorMode(CursorMode mode);
    CursorMode GetCursorMode() const;

    float GetXOffset() const;
    float GetYOffset() const;
    float GetScrollY() const;

    float GetLastX() const;
    float GetLastY() const;

    bool IsLeftButtonPressed() const;
    bool IsRightButtonPressed() const;
    bool IsMiddleButtonPressed() const;

    bool IsLeftMouseClicked() const;
    bool IsRightMouseClicked() const;
    bool IsMiddleMouseClicked() const;

    bool IsLeftMouseReleased() const;
    bool IsRightMouseReleased() const;
    bool IsMiddleMouseReleased() const;

    bool IsMouse4ButtonPressed() const;
    bool IsMouse4MouseClicked() const;
    bool IsMouse4MouseReleased() const;

    bool IsMouse5ButtonPressed() const;
    bool IsMouse5MouseClicked() const;
    bool IsMouse5MouseReleased() const;

    bool IsWheelUp() const;
    bool IsWheelDown() const;

    void SetWindowSize(int width, int height);
    void SetLastPosition(double x, double y);
    void SetLockPosition(double x, double y) { m_LockX = x; m_LockY = y; }

private:
    friend class IOHandler;
    void SetWindow(IWindow* window);

    IWindow *m_Window = nullptr;
    int m_WindowWidth = 800;
    int m_WindowHeight = 600;

    double m_LastX;
    double m_LastY;

    double m_LockX;
    double m_LockY;

    float m_XOffset;
    float m_YOffset;
    float m_ScrollY;

    bool m_FirstMouse;

    bool m_LeftButtonPressed;
    bool m_RightButtonPressed;
    bool m_MiddleButtonPressed;

    bool m_LeftMouseClicked;
    bool m_RightMouseClicked;
    bool m_MiddleMouseClicked;

    bool m_LeftMouseReleased;
    bool m_RightMouseReleased;
    bool m_MiddleMouseReleased;

    bool m_Mouse4ButtonPressed;
    bool m_Mouse5ButtonPressed;

    bool m_Mouse4MouseClicked;
    bool m_Mouse5MouseClicked;

    bool m_Mouse4MouseReleased;
    bool m_Mouse5MouseReleased;

    CursorMode m_Mode;
};

// --- Input Manager ---

class InputManager : public IDeviceManager
{
public:
    InputManager(const KeyboardManager &keyboard, const MouseManager &mouse, const IWindow& window);

    void BindAction(const std::string &actionName, InputType type, int code);
    void UnbindAction(const std::string &actionName);
    void FlushBindings();

    void Update();

    bool GetAction(const std::string &actionName) const;
    bool GetActionDown(const std::string &actionName) const;
    bool GetActionUp(const std::string &actionName) const;

    std::vector<DeviceInfo> GetAllDevices() const override;
    DeviceInfo GetCurrentDevice() const override;
    bool SetActiveDevice(const std::string &deviceId) override;

private:
    const KeyboardManager &m_Keyboard;
    const MouseManager &m_Mouse;
    const IWindow &m_Window;
    std::unordered_map<std::string, InputActionBinding> m_ActionMap;
    std::unordered_map<std::string, bool> m_PreviousState;
};


// --- Script Input Handler ---

class ScriptInputHandler
{
public:
    static void HandleInput(ScriptComponent& script, Scene& scene, float dt, entt::entity entity);
};

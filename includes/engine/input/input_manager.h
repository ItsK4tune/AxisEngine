#pragma once

#include <input/input_types.h>
#include <input/keyboard_manager.h>
#include <input/mouse_manager.h>
#include <window/interfaces/i_device_manager.h>
#include <string>
#include <unordered_map>

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

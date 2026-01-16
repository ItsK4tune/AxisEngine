#pragma once

#include <string>
#include <unordered_map>
#include <core/keyboard_manager.h>
#include <core/mouse_manager.h>

enum class InputType
{
    Key,
    MouseButton
};

struct InputBinding
{
    InputType type;
    int code;
};

#include <core/device_manager.h>

class InputManager : public IDeviceManager
{
public:
    InputManager(const KeyboardManager& keyboard, const MouseManager& mouse);

    void BindAction(const std::string& actionName, InputType type, int code);
    void UnbindAction(const std::string& actionName);

    bool GetAction(const std::string& actionName) const;
    bool GetActionDown(const std::string& actionName) const;
    bool GetActionUp(const std::string& actionName) const;

    // IDeviceManager Implementation
    std::vector<DeviceInfo> GetAllDevices() const override;
    DeviceInfo GetCurrentDevice() const override;
    bool SetActiveDevice(const std::string& deviceId) override;

private:
    const KeyboardManager& m_Keyboard;
    const MouseManager& m_Mouse;
    std::unordered_map<std::string, InputBinding> m_ActionMap;
};

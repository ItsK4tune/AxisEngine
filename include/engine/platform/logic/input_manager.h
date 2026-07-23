#pragma once

#include <platform/interface/i_device_manager.h>
#include <platform/interface/i_window.h>
#include <platform/interface/input_codes.h>
#include <platform/logic/keyboard_manager.h>
#include <platform/logic/mouse_manager.h>
#include <platform/type/input_binding.h>
#include <memory>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

class InputManager : public IDeviceManager
{
public:
    InputManager(const KeyboardManager& keyboard, const MouseManager& mouse, const IWindow& window);

    void BindAction(const std::string& actionName, InputType type, int code);
    bool RemoveBinding(const std::string& actionName, size_t bindingIndex);
    void UnbindAction(const std::string& actionName);
    void FlushBindings();

    void Update();

    bool GetAction(const std::string& actionName) const;
    bool GetActionDown(const std::string& actionName) const;
    bool GetActionUp(const std::string& actionName) const;
    float GetAxis(const std::string& actionName) const;
    void SetGamepadDeadZone(float deadZone) { m_GamepadDeadZone = (std::clamp)(deadZone, 0.0f, 0.99f); }
    float GetGamepadDeadZone() const { return m_GamepadDeadZone; }

    std::vector<DeviceInfo> GetAllDevices() const override;
    DeviceInfo GetCurrentDevice() const override;
    bool SetActiveDevice(const std::string& deviceId) override;

    const std::unordered_map<std::string, InputActionBinding>& GetActionMap() const
    {
        return m_ActionMap;
    }

    bool HasAction(const std::string& actionName) const
    {
        return m_ActionMap.find(actionName) != m_ActionMap.end();
    }

private:
    bool IsBindingTypeActive(InputType type) const;
    bool IsGamepadButtonPressed(int code) const;
    float ReadGamepadAxis(int code) const;
    static bool TryParseGamepadDeviceIndex(const std::string& deviceId, int& index);

    const KeyboardManager& m_Keyboard;
    const MouseManager& m_Mouse;
    const IWindow& m_Window;
    std::unordered_map<std::string, InputActionBinding> m_ActionMap;
    std::unordered_map<std::string, bool> m_PreviousState;
    std::string m_ActiveDeviceId = "merged_input";
    float m_GamepadDeadZone = 0.15f;
};

#include <platform/logic/input_manager.h>
#include <core/logic/event_manager.h>
#include <core/type/event_types.h>
#include <platform/interface/i_window.h>
#include <platform/interface/input_codes.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <string>

InputManager::InputManager(const KeyboardManager& keyboard, const MouseManager& mouse, const IWindow& window)
    : m_Keyboard(keyboard), m_Mouse(mouse), m_Window(window)
{
}

void InputManager::BindAction(const std::string& actionName, InputType type, int code)
{
    m_ActionMap[actionName].bindings.push_back({type, code});
}

bool InputManager::RemoveBinding(const std::string& actionName, size_t bindingIndex)
{
    const auto action = m_ActionMap.find(actionName);
    if (action == m_ActionMap.end() || bindingIndex >= action->second.bindings.size())
        return false;
    action->second.bindings.erase(action->second.bindings.begin() + static_cast<std::ptrdiff_t>(bindingIndex));
    if (action->second.bindings.empty())
        UnbindAction(actionName);
    return true;
}

void InputManager::UnbindAction(const std::string& actionName)
{
    m_ActionMap.erase(actionName);
    m_PreviousState.erase(actionName);
}

void InputManager::FlushBindings()
{
    m_ActionMap.clear();
    m_PreviousState.clear();
}

void InputManager::Update()
{
    if (m_ActiveDeviceId != "merged_input")
    {
        const auto devices = GetAllDevices();
        if (std::none_of(devices.begin(), devices.end(),
                         [&](const DeviceInfo& device) { return device.id == m_ActiveDeviceId; }))
            m_ActiveDeviceId = "merged_input";
    }

    float xOffset = m_Mouse.GetXOffset();
    float yOffset = m_Mouse.GetYOffset();
    if ((xOffset != 0.0f || yOffset != 0.0f) && EventManager::Instance().HasListeners<MouseMovedEvent>())
    {
        EventManager::Instance().Publish(
            MouseMovedEvent{static_cast<double>(m_Mouse.GetLastX()), static_cast<double>(m_Mouse.GetLastY())});
    }

    bool hasPressedListeners = EventManager::Instance().HasListeners<InputActionPressedEvent>();
    bool hasReleasedListeners = EventManager::Instance().HasListeners<InputActionReleasedEvent>();
    bool hasHeldListeners = EventManager::Instance().HasListeners<InputActionHeldEvent>();

    if (hasPressedListeners || hasReleasedListeners || hasHeldListeners)
    {
        for (const auto& [actionName, binding] : m_ActionMap)
        {
            if (hasPressedListeners && GetActionDown(actionName))
                EventManager::Instance().Publish(InputActionPressedEvent{actionName});
            else if (hasReleasedListeners && GetActionUp(actionName))
                EventManager::Instance().Publish(InputActionReleasedEvent{actionName});
            else if (hasHeldListeners && GetAction(actionName))
                EventManager::Instance().Publish(InputActionHeldEvent{actionName});
        }
    }

    for (const auto& [actionName, binding] : m_ActionMap)
    {
        m_PreviousState[actionName] = GetAction(actionName);
    }
}

bool InputManager::GetAction(const std::string& actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it == m_ActionMap.end())
        return false;

    for (const auto& binding : it->second.bindings)
    {
        if (!IsBindingTypeActive(binding.type))
            continue;
        if (binding.type == InputType::Key)
        {
            if (m_Keyboard.GetKey(static_cast<Key>(binding.code)))
                return true;
        }
        else if (binding.type == InputType::MouseButton)
        {
            Mouse btn = static_cast<Mouse>(binding.code);
            if (btn == Mouse::WheelUp)
            {
                if (m_Mouse.IsWheelUp())
                    return true;
            }
            else if (btn == Mouse::WheelDown)
            {
                if (m_Mouse.IsWheelDown())
                    return true;
            }
            else if (m_Mouse.IsButtonPressed(btn))
                return true;
        }
        else if (binding.type == InputType::GamepadButton && IsGamepadButtonPressed(binding.code))
        {
            return true;
        }
        else if (binding.type == InputType::GamepadAxis && std::abs(ReadGamepadAxis(binding.code)) > 0.0f)
            return true;
    }
    return false;
}

bool InputManager::GetActionDown(const std::string& actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it == m_ActionMap.end())
        return false;

    bool hasActiveGamepadBinding = false;
    for (const auto& binding : it->second.bindings)
    {
        if (!IsBindingTypeActive(binding.type))
            continue;
        if (binding.type == InputType::Key)
        {
            if (m_Keyboard.IsKeyDown(static_cast<Key>(binding.code)))
                return true;
        }
        else if (binding.type == InputType::MouseButton)
        {
            Mouse btn = static_cast<Mouse>(binding.code);
            if (btn == Mouse::WheelUp)
            {
                if (m_Mouse.IsWheelUp())
                    return true;
            }
            else if (btn == Mouse::WheelDown)
            {
                if (m_Mouse.IsWheelDown())
                    return true;
            }
            else if (m_Mouse.IsMouseClicked(btn))
                return true;
        }
        else if (binding.type == InputType::GamepadButton || binding.type == InputType::GamepadAxis)
        {
            hasActiveGamepadBinding = true;
        }
    }
    const auto previous = m_PreviousState.find(actionName);
    return hasActiveGamepadBinding && GetAction(actionName) &&
           (previous == m_PreviousState.end() || !previous->second);
}

bool InputManager::GetActionUp(const std::string& actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it == m_ActionMap.end())
        return false;

    bool hasActiveGamepadBinding = false;
    for (const auto& binding : it->second.bindings)
    {
        if (!IsBindingTypeActive(binding.type))
            continue;
        if (binding.type == InputType::Key)
        {
            if (m_Keyboard.GetKeyUp(static_cast<Key>(binding.code)))
                return true;
        }
        else if (binding.type == InputType::MouseButton)
        {
            Mouse btn = static_cast<Mouse>(binding.code);
            if (m_Mouse.IsMouseReleased(btn))
                return true;
        }
        else if (binding.type == InputType::GamepadButton || binding.type == InputType::GamepadAxis)
        {
            hasActiveGamepadBinding = true;
        }
    }
    const auto previous = m_PreviousState.find(actionName);
    return hasActiveGamepadBinding && !GetAction(actionName) && previous != m_PreviousState.end() && previous->second;
}

std::vector<DeviceInfo> InputManager::GetAllDevices() const
{
    std::vector<DeviceInfo> devices;
    devices.push_back({"merged_input", "Keyboard & Mouse", DeviceType::Keyboard, true});
    for (const auto& device : m_Window.GetConnectedDevices())
    {
        if (device.type == DeviceType::Keyboard || device.type == DeviceType::Mouse ||
            device.type == DeviceType::Gamepad || device.type == DeviceType::Joystick)
            devices.push_back(device);
    }
    return devices;
}

DeviceInfo InputManager::GetCurrentDevice() const
{
    for (const auto& device : GetAllDevices())
    {
        if (device.id == m_ActiveDeviceId)
            return device;
    }
    return {"merged_input", "Keyboard & Mouse", DeviceType::Keyboard, true};
}

bool InputManager::SetActiveDevice(const std::string& deviceId)
{
    const auto devices = GetAllDevices();
    const auto found =
        std::find_if(devices.begin(), devices.end(), [&](const DeviceInfo& device) { return device.id == deviceId; });
    if (found == devices.end())
        return false;
    m_ActiveDeviceId = deviceId;
    return true;
}

bool InputManager::IsBindingTypeActive(InputType type) const
{
    if (m_ActiveDeviceId == "merged_input")
        return true;
    if (m_ActiveDeviceId == "keyboard_0")
        return type == InputType::Key;
    if (m_ActiveDeviceId == "mouse_0")
        return type == InputType::MouseButton;
    int gamepadIndex = -1;
    if (TryParseGamepadDeviceIndex(m_ActiveDeviceId, gamepadIndex))
        return type == InputType::GamepadButton || type == InputType::GamepadAxis;
    return false;
}

float InputManager::GetAxis(const std::string& actionName) const
{
    const auto found = m_ActionMap.find(actionName);
    if (found == m_ActionMap.end())
        return 0.0f;
    float strongest = 0.0f;
    for (const auto& binding : found->second.bindings)
    {
        if (binding.type != InputType::GamepadAxis || !IsBindingTypeActive(binding.type))
            continue;
        const float value = ReadGamepadAxis(binding.code);
        if (std::abs(value) > std::abs(strongest))
            strongest = value;
    }
    return strongest;
}

float InputManager::ReadGamepadAxis(int code) const
{
    const auto axis = static_cast<GamepadAxis>(code);
    auto applyDeadZone = [&](float value) {
        const bool trigger = axis == GamepadAxis::LeftTrigger || axis == GamepadAxis::RightTrigger;
        const float magnitude = trigger ? value : std::abs(value);
        if (magnitude <= m_GamepadDeadZone)
            return 0.0f;
        const float scaled = (magnitude - m_GamepadDeadZone) / (1.0f - m_GamepadDeadZone);
        return trigger || value >= 0.0f ? scaled : -scaled;
    };
    int activeIndex = -1;
    if (TryParseGamepadDeviceIndex(m_ActiveDeviceId, activeIndex))
        return applyDeadZone(m_Window.GetGamepadAxis(activeIndex, axis));
    if (m_ActiveDeviceId != "merged_input")
        return 0.0f;
    float strongest = 0.0f;
    for (const auto& device : m_Window.GetConnectedDevices())
    {
        int index = -1;
        if ((device.type == DeviceType::Gamepad || device.type == DeviceType::Joystick) &&
            TryParseGamepadDeviceIndex(device.id, index))
        {
            const float value = applyDeadZone(m_Window.GetGamepadAxis(index, axis));
            if (std::abs(value) > std::abs(strongest))
                strongest = value;
        }
    }
    return strongest;
}

bool InputManager::IsGamepadButtonPressed(int code) const
{
    const auto button = static_cast<Gamepad>(code);
    int activeIndex = -1;
    if (TryParseGamepadDeviceIndex(m_ActiveDeviceId, activeIndex))
        return m_Window.GetGamepadButton(activeIndex, button);

    if (m_ActiveDeviceId != "merged_input")
        return false;

    for (const auto& device : m_Window.GetConnectedDevices())
    {
        if (device.type != DeviceType::Gamepad && device.type != DeviceType::Joystick)
            continue;
        int index = -1;
        if (TryParseGamepadDeviceIndex(device.id, index) && m_Window.GetGamepadButton(index, button))
            return true;
    }
    return false;
}

bool InputManager::TryParseGamepadDeviceIndex(const std::string& deviceId, int& index)
{
    constexpr const char* prefixes[] = {"gamepad_", "joystick_"};
    for (const char* prefix : prefixes)
    {
        const std::string prefixValue(prefix);
        if (deviceId.rfind(prefixValue, 0) != 0)
            continue;
        try
        {
            size_t consumed = 0;
            index = std::stoi(deviceId.substr(prefixValue.size()), &consumed);
            return consumed == deviceId.size() - prefixValue.size() && index >= 0;
        }
        catch (...)
        {
            return false;
        }
    }
    return false;
}

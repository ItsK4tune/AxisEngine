#include <platform/logic/input_manager.h>
#include <core/logic/event_manager.h>
#include <core/type/event_types.h>
#include <platform/interface/i_window.h>
#include <platform/interface/input_codes.h>
#include <iostream>
#include <algorithm>
#include <string>

InputManager::InputManager(const KeyboardManager& keyboard, const MouseManager& mouse, const IWindow& window)
    : m_Keyboard(keyboard), m_Mouse(mouse), m_Window(window)
{
}

void InputManager::BindAction(const std::string& actionName, InputType type, int code)
{
    m_ActionMap[actionName].bindings.push_back({type, code});
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
    }
    return false;
}

bool InputManager::GetActionDown(const std::string& actionName) const
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
    }
    return false;
}

bool InputManager::GetActionUp(const std::string& actionName) const
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
            if (m_Keyboard.GetKeyUp(static_cast<Key>(binding.code)))
                return true;
        }
        else if (binding.type == InputType::MouseButton)
        {
            Mouse btn = static_cast<Mouse>(binding.code);
            if (m_Mouse.IsMouseReleased(btn))
                return true;
        }
    }
    return false;
}

std::vector<DeviceInfo> InputManager::GetAllDevices() const
{
    std::vector<DeviceInfo> devices;
    devices.push_back({"merged_input", "Keyboard & Mouse", DeviceType::Keyboard, true});
    for (const auto& device : m_Window.GetConnectedDevices())
    {
        if (device.type == DeviceType::Keyboard || device.type == DeviceType::Mouse)
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
    return false;
}

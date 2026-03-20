#include <platform/logic/input_manager.h>
#include <platform/interface/i_window.h>
#include <platform/interface/input_codes.h>
#include <iostream>
#include <string>

InputManager::InputManager(const KeyboardManager &keyboard, const MouseManager &mouse, const IWindow &window)
    : m_Keyboard(keyboard), m_Mouse(mouse), m_Window(window)
{
}

void InputManager::BindAction(const std::string &actionName, InputType type, int code)
{
    m_ActionMap[actionName].bindings.push_back({type, code});
}

void InputManager::UnbindAction(const std::string &actionName)
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
    // State tracking is handled by GetAction, GetActionDown, etc.
    // using m_Keyboard and m_Mouse. 
    // m_PreviousState is updated here for GetActionDown/Up logic.
    for (const auto &[actionName, binding] : m_ActionMap)
    {
        m_PreviousState[actionName] = GetAction(actionName);
    }
}

bool InputManager::GetAction(const std::string &actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it != m_ActionMap.end())
    {
        for (const auto &binding : it->second.bindings)
        {
            if (binding.type == InputType::Key)
            {
                if (m_Keyboard.GetKey(static_cast<Key>(binding.code)))
                    return true;
            }
            else if (binding.type == InputType::MouseButton)
            {
                if (binding.code == (int)Mouse::Left && m_Mouse.IsLeftButtonPressed())
                    return true;
                if (binding.code == (int)Mouse::Right && m_Mouse.IsRightButtonPressed())
                    return true;
                if (binding.code == (int)Mouse::Middle && m_Mouse.IsMiddleButtonPressed())
                    return true;
                if (binding.code == (int)Mouse::Button4 && m_Mouse.IsMouse4ButtonPressed())
                    return true;
                if (binding.code == (int)Mouse::Button5 && m_Mouse.IsMouse5ButtonPressed())
                    return true;
                if (binding.code == (int)Mouse::WheelUp && m_Mouse.IsWheelUp())
                    return true;
                if (binding.code == (int)Mouse::WheelDown && m_Mouse.IsWheelDown())
                    return true;
            }
            else if (binding.type == InputType::GamepadButton)
            {
            }
        }
    }
    return false;
}

bool InputManager::GetActionDown(const std::string &actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it != m_ActionMap.end())
    {
        for (const auto &binding : it->second.bindings)
        {
            if (binding.type == InputType::Key)
            {
                if (const_cast<KeyboardManager &>(m_Keyboard).IsKeyDown(static_cast<Key>(binding.code)))
                    return true;
            }
            else if (binding.type == InputType::MouseButton)
            {
                if (binding.code == (int)Mouse::Left && m_Mouse.IsLeftMouseClicked())
                    return true;
                if (binding.code == (int)Mouse::Right && m_Mouse.IsRightMouseClicked())
                    return true;
                if (binding.code == (int)Mouse::Middle && m_Mouse.IsMiddleMouseClicked())
                    return true;
                if (binding.code == (int)Mouse::Button4 && m_Mouse.IsMouse4MouseClicked())
                    return true;
                if (binding.code == (int)Mouse::Button5 && m_Mouse.IsMouse5MouseClicked())
                    return true;
                if (binding.code == (int)Mouse::WheelUp && m_Mouse.IsWheelUp())
                    return true;
                if (binding.code == (int)Mouse::WheelDown && m_Mouse.IsWheelDown())
                    return true;
            }
            else if (binding.type == InputType::GamepadButton)
            {
            }
        }
    }
    return false;
}

bool InputManager::GetActionUp(const std::string &actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it != m_ActionMap.end())
    {
        for (const auto &binding : it->second.bindings)
        {
            if (binding.type == InputType::Key)
            {
                if (m_Keyboard.GetKeyUp(static_cast<Key>(binding.code)))
                    return true;
            }
            else if (binding.type == InputType::MouseButton)
            {
                if (binding.code == (int)Mouse::Left && m_Mouse.IsLeftMouseReleased())
                    return true;
                if (binding.code == (int)Mouse::Right && m_Mouse.IsRightMouseReleased())
                    return true;
                if (binding.code == (int)Mouse::Middle && m_Mouse.IsMiddleMouseReleased())
                    return true;
                if (binding.code == (int)Mouse::Button4 && m_Mouse.IsMouse4MouseReleased())
                    return true;
                if (binding.code == (int)Mouse::Button5 && m_Mouse.IsMouse5MouseReleased())
                    return true;
            }
            else if (binding.type == InputType::GamepadButton)
            {
            }
        }
    }
    return false;
}

std::vector<DeviceInfo> InputManager::GetAllDevices() const
{
    return m_Window.GetConnectedDevices();
}

DeviceInfo InputManager::GetCurrentDevice() const
{
    DeviceInfo info;
    info.id = "merged_input";
    info.name = "Keyboard & Mouse";
    info.type = DeviceType::Keyboard;
    info.isDefault = true;
    return info;
}

bool InputManager::SetActiveDevice(const std::string &deviceId)
{
    return true;
}

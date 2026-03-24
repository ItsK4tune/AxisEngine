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



    for (const auto &[actionName, binding] : m_ActionMap)
    {
        m_PreviousState[actionName] = GetAction(actionName);
    }
}

bool InputManager::GetAction(const std::string &actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it == m_ActionMap.end()) return false;

    for (const auto &binding : it->second.bindings)
    {
        if (binding.type == InputType::Key)
        {
            if (m_Keyboard.GetKey(static_cast<Key>(binding.code))) return true;
        }
        else if (binding.type == InputType::MouseButton)
        {
            Mouse btn = static_cast<Mouse>(binding.code);
            if (btn == Mouse::WheelUp)   { if (m_Mouse.IsWheelUp()) return true; }
            else if (btn == Mouse::WheelDown) { if (m_Mouse.IsWheelDown()) return true; }
            else if (m_Mouse.IsButtonPressed(btn)) return true;
        }
    }
    return false;
}

bool InputManager::GetActionDown(const std::string &actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it == m_ActionMap.end()) return false;

    for (const auto &binding : it->second.bindings)
    {
        if (binding.type == InputType::Key)
        {
            if (const_cast<KeyboardManager &>(m_Keyboard).IsKeyDown(static_cast<Key>(binding.code))) return true;
        }
        else if (binding.type == InputType::MouseButton)
        {
            Mouse btn = static_cast<Mouse>(binding.code);
            if (btn == Mouse::WheelUp)   { if (m_Mouse.IsWheelUp()) return true; }
            else if (btn == Mouse::WheelDown) { if (m_Mouse.IsWheelDown()) return true; }
            else if (m_Mouse.IsMouseClicked(btn)) return true;
        }
    }
    return false;
}

bool InputManager::GetActionUp(const std::string &actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it == m_ActionMap.end()) return false;

    for (const auto &binding : it->second.bindings)
    {
        if (binding.type == InputType::Key)
        {
            if (m_Keyboard.GetKeyUp(static_cast<Key>(binding.code))) return true;
        }
        else if (binding.type == InputType::MouseButton)
        {
            Mouse btn = static_cast<Mouse>(binding.code);
            if (m_Mouse.IsMouseReleased(btn)) return true;
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

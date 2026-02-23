#include <input/input_manager.h>
#include <interface/window/i_window.h>
#include <interface/window/input_codes.h>
#include <string>
#include <iostream>
#include <event/input_events.h>
#include <event/event_system.h>

InputManager::InputManager(const KeyboardManager &keyboard, const MouseManager &mouse, const IWindow &window)
    : m_Keyboard(keyboard), m_Mouse(mouse), m_Window(window)
{
}

void InputManager::BindAction(const std::string &actionName, InputType type, int code)
{
    m_ActionMap[actionName] = {type, code};
}

void InputManager::UnbindAction(const std::string &actionName)
{
    m_ActionMap.erase(actionName);
    m_PreviousState.erase(actionName);
}

void InputManager::Update()
{
    for (const auto& [actionName, binding] : m_ActionMap)
    {
        bool currentState = GetAction(actionName);
        bool previousState = m_PreviousState[actionName];

        if (currentState && !previousState) {
            EventSystem::Instance().Publish(InputActionPressedEvent{actionName});
        }
        else if (!currentState && previousState) {
            EventSystem::Instance().Publish(InputActionReleasedEvent{actionName});
        }
        else if (currentState && previousState) {
            EventSystem::Instance().Publish(InputActionHeldEvent{actionName});
        }

        m_PreviousState[actionName] = currentState;
    }
}

bool InputManager::GetAction(const std::string &actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it != m_ActionMap.end())
    {
        if (it->second.type == InputType::Key)
        {
            return m_Keyboard.GetKey(static_cast<Input::Key>(it->second.code));
        }
        else if (it->second.type == InputType::MouseButton)
        {
            if (it->second.code == (int)Input::Mouse::Left)
                return m_Mouse.IsLeftButtonPressed();
            if (it->second.code == (int)Input::Mouse::Right)
                return m_Mouse.IsRightButtonPressed();
        }
    }
    return false;
}

bool InputManager::GetActionDown(const std::string &actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it != m_ActionMap.end())
    {
        if (it->second.type == InputType::Key)
        {
            return const_cast<KeyboardManager &>(m_Keyboard).IsKeyDown(static_cast<Input::Key>(it->second.code));
        }
        else if (it->second.type == InputType::MouseButton)
        {
            if (it->second.code == (int)Input::Mouse::Left)
                return m_Mouse.IsLeftMouseClicked();
            if (it->second.code == (int)Input::Mouse::Right)
                return m_Mouse.IsRightMouseClicked();
        }
    }
    return false;
}

bool InputManager::GetActionUp(const std::string &actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it != m_ActionMap.end())
    {
        if (it->second.type == InputType::Key)
        {
            return m_Keyboard.GetKeyUp(static_cast<Input::Key>(it->second.code));
        }
        else if (it->second.type == InputType::MouseButton)
        {
            return false;
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
    info.type = AxisDeviceType::Keyboard;
    info.isDefault = true;
    return info;
}

bool InputManager::SetActiveDevice(const std::string &deviceId)
{
    return true;
}

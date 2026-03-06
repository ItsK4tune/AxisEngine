#include <event/event_system.h>
#include <event/input_events.h>
#include <input/input_manager.h>
#include <window/interfaces/i_window.h>
#include <window/interfaces/input_codes.h>
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
        bool currentState = GetAction(actionName);
        bool previousState = m_PreviousState[actionName];

        if (currentState && !previousState)
        {
            EventSystem::Instance().Publish(InputActionPressedEvent{actionName});
        }
        else if (!currentState && previousState)
        {
            EventSystem::Instance().Publish(InputActionReleasedEvent{actionName});
        }
        else if (currentState && previousState)
        {
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
        for (const auto &binding : it->second.bindings)
        {
            if (binding.type == InputType::Key)
            {
                if (m_Keyboard.GetKey(static_cast<Input::Key>(binding.code)))
                    return true;
            }
            else if (binding.type == InputType::MouseButton)
            {
                if (binding.code == (int)Input::Mouse::Left && m_Mouse.IsLeftButtonPressed())
                    return true;
                if (binding.code == (int)Input::Mouse::Right && m_Mouse.IsRightButtonPressed())
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
                if (const_cast<KeyboardManager &>(m_Keyboard).IsKeyDown(static_cast<Input::Key>(binding.code)))
                    return true;
            }
            else if (binding.type == InputType::MouseButton)
            {
                if (binding.code == (int)Input::Mouse::Left && m_Mouse.IsLeftMouseClicked())
                    return true;
                if (binding.code == (int)Input::Mouse::Right && m_Mouse.IsRightMouseClicked())
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
                if (m_Keyboard.GetKeyUp(static_cast<Input::Key>(binding.code)))
                    return true;
            }
            else if (binding.type == InputType::MouseButton)
            {
                if (binding.code == (int)Input::Mouse::Left && m_Mouse.IsLeftMouseReleased())
                    return true;
                if (binding.code == (int)Input::Mouse::Right && m_Mouse.IsRightMouseReleased())
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
    info.type = AxisDeviceType::Keyboard;
    info.isDefault = true;
    return info;
}

bool InputManager::SetActiveDevice(const std::string &deviceId)
{
    return true;
}

#include <engine/core/input_manager.h>

InputManager::InputManager(const KeyboardManager& keyboard, const MouseManager& mouse)
    : m_Keyboard(keyboard), m_Mouse(mouse)
{
}

void InputManager::BindAction(const std::string& actionName, InputType type, int code)
{
    m_ActionMap[actionName] = { type, code };
}

void InputManager::UnbindAction(const std::string& actionName)
{
    m_ActionMap.erase(actionName);
}

bool InputManager::GetAction(const std::string& actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it != m_ActionMap.end())
    {
        if (it->second.type == InputType::Key)
        {
            return m_Keyboard.GetKey(it->second.code);
        }
        else if (it->second.type == InputType::MouseButton)
        {
            if (it->second.code == GLFW_MOUSE_BUTTON_LEFT) return m_Mouse.IsLeftButtonPressed();
            if (it->second.code == GLFW_MOUSE_BUTTON_RIGHT) return m_Mouse.IsRightButtonPressed();
            // Extend for Middle/Other buttons if MouseManager supports them
        }
    }
    return false;
}

bool InputManager::GetActionDown(const std::string& actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it != m_ActionMap.end())
    {
        if (it->second.type == InputType::Key)
        {
            return const_cast<KeyboardManager&>(m_Keyboard).IsKeyDown(it->second.code); 
        }
        else if (it->second.type == InputType::MouseButton)
        {
             if (it->second.code == GLFW_MOUSE_BUTTON_LEFT) return m_Mouse.IsLeftMouseClicked();
             if (it->second.code == GLFW_MOUSE_BUTTON_RIGHT) return m_Mouse.IsRightMouseClicked();
        }
    }
    return false;
}

bool InputManager::GetActionUp(const std::string& actionName) const
{
    auto it = m_ActionMap.find(actionName);
    if (it != m_ActionMap.end())
    {
        if (it->second.type == InputType::Key)
        {
            return m_Keyboard.GetKeyUp(it->second.code);
        }
        else if (it->second.type == InputType::MouseButton)
        {
            return false; 
        }
    }
    return false;
}

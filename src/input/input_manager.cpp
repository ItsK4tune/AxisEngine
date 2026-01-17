#include <input/input_manager.h>

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

// IDeviceManager Implementation
std::vector<DeviceInfo> InputManager::GetAllDevices() const
{
    std::vector<DeviceInfo> devices;
    
    // Keyboard
    DeviceInfo kbdInfo;
    kbdInfo.id = "keyboard_0";
    kbdInfo.name = "Primary Keyboard";
    kbdInfo.type = DeviceType::Keyboard;
    kbdInfo.isDefault = true;
    devices.push_back(kbdInfo);

    // Mouse
    DeviceInfo mouseInfo;
    mouseInfo.id = "mouse_0";
    mouseInfo.name = "Primary Mouse";
    mouseInfo.type = DeviceType::Mouse;
    mouseInfo.isDefault = true;
    devices.push_back(mouseInfo);

    // Joysticks
    for (int i = 0; i <= GLFW_JOYSTICK_LAST; i++)
    {
        if (glfwJoystickPresent(i))
        {
            DeviceInfo joyInfo;
            joyInfo.id = std::to_string(i);
            const char* name = glfwGetJoystickName(i);
            joyInfo.name = name ? name : "Unknown Joystick";
            joyInfo.type = DeviceType::Joystick;
            joyInfo.isDefault = false;
            devices.push_back(joyInfo);
        }
    }

    return devices;
}

DeviceInfo InputManager::GetCurrentDevice() const
{
    // Return Primary Keyboard/Mouse combo as "current"
    DeviceInfo info;
    info.id = "merged_input";
    info.name = "Keyboard & Mouse";
    info.type = DeviceType::Keyboard; // Placeholder type
    info.isDefault = true;
    return info;
}

bool InputManager::SetActiveDevice(const std::string& deviceId)
{
    // Input switching logic (e.g. focusing on a specific gamepad) is not yet implemented.
    return true;
}

#pragma once

#include <string>
#include <unordered_map>
#include <engine/core/keyboard_manager.h>
#include <engine/core/mouse_manager.h>

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

class InputManager
{
public:
    InputManager(const KeyboardManager& keyboard, const MouseManager& mouse);

    void BindAction(const std::string& actionName, InputType type, int code);
    void UnbindAction(const std::string& actionName);

    bool GetAction(const std::string& actionName) const;
    bool GetActionDown(const std::string& actionName) const;
    bool GetActionUp(const std::string& actionName) const;

private:
    const KeyboardManager& m_Keyboard;
    const MouseManager& m_Mouse;
    std::unordered_map<std::string, InputBinding> m_ActionMap;
};

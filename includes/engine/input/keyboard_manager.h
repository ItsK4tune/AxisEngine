#pragma once

#include <interface/window/i_window.h>
#include <interface/window/input_codes.h>

#include <unordered_map>

class KeyboardManager
{
public:
    KeyboardManager(IWindow *window);

    void Update();

    bool GetKey(Input::Key key) const;
    bool GetKeyUp(Input::Key key) const;
    bool IsKeyDown(Input::Key key) const;

private:
    friend class IOHandler;
    void SetWindow(IWindow* window);

    IWindow *m_Window = nullptr;
    std::unordered_map<Input::Key, bool> m_CurrentState;
    std::unordered_map<Input::Key, bool> m_PreviousState;
};

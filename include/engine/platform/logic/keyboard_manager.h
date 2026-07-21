#pragma once

#include <platform/interface/i_window.h>
#include <platform/interface/key.h>
#include <unordered_map>

class KeyboardManager
{
public:
    KeyboardManager(IWindow* window);

    void Update();

    bool GetKey(Key key) const;
    bool GetKeyUp(Key key) const;
    bool IsKeyDown(Key key) const;

private:
    friend class IOHandler;
    void SetWindow(IWindow* window);

    IWindow* m_Window = nullptr;
    mutable std::unordered_map<Key, bool> m_CurrentState;
    mutable std::unordered_map<Key, bool> m_PreviousState;
};

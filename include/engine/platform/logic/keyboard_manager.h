#pragma once

#include <platform/interface/i_window.h>
#include <platform/interface/key.h>
#include <unordered_map>
#include <unordered_set>

class KeyboardManager
{
public:
    KeyboardManager(IWindow* window);

    void Update();
    void EndFrame();

    bool GetKey(Key key) const;
    bool GetRawKey(Key key) const;
    bool GetKeyUp(Key key) const;
    bool IsKeyDown(Key key) const;
    void ConsumeKey(Key key);
    void ReleaseConsumedKey(Key key);
    bool IsKeyConsumed(Key key) const;

private:
    friend class IOHandler;
    void SetWindow(IWindow* window);

    IWindow* m_Window = nullptr;
    mutable std::unordered_map<Key, bool> m_CurrentState;
    mutable std::unordered_map<Key, bool> m_PreviousState;
    std::unordered_set<Key> m_ConsumedKeys;
    std::unordered_set<Key> m_ReleasedConsumedKeys;
};

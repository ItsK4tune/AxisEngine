#pragma once
#include <script/scriptable.h>
#include <functional>
#include <vector>
#include <script/script_types.h>

class InputScriptable : public virtual Scriptable {
public:
    virtual void OnLeftClick() {}
    virtual void OnLeftHold(float duration) {}
    virtual void OnLeftRelease(float duration) {}

    virtual void OnRightClick() {}
    virtual void OnRightHold(float duration) {}
    virtual void OnRightRelease(float duration) {}

    virtual void OnMiddleClick() {}
    virtual void OnMiddleHold(float duration) {}
    virtual void OnMiddleRelease(float duration) {}

    virtual void OnHoverEnter() {}
    virtual void OnHoverStay() {}
    virtual void OnHoverExit() {}

    void BindKey(int key, InputEvent event, std::function<void()> callback) {
        m_KeyBindings.push_back({key, event, callback});
    }

    bool IsHovered() const { return m_IsHovered; }
    bool IsLeftPressed() const { return m_LeftPressed; }
    bool IsRightPressed() const { return m_RightPressed; }
    bool IsMiddlePressed() const { return m_MiddlePressed; }

    void SetHovered(bool hovered) { m_IsHovered = hovered; }

    bool &GetLeftPressedRef() { return m_LeftPressed; }
    float &GetLeftHoldTimeRef() { return m_LeftHoldTime; }

    bool &GetRightPressedRef() { return m_RightPressed; }
    float &GetRightHoldTimeRef() { return m_RightHoldTime; }

    bool &GetMiddlePressedRef() { return m_MiddlePressed; }
    float &GetMiddleHoldTimeRef() { return m_MiddleHoldTime; }

    const std::vector<KeyBinding> &GetKeyBindings() const { return m_KeyBindings; }

private:
    bool m_IsHovered = false;

    bool m_LeftPressed = false;
    float m_LeftHoldTime = 0.0f;

    bool m_RightPressed = false;
    float m_RightHoldTime = 0.0f;

    bool m_MiddlePressed = false;
    float m_MiddleHoldTime = 0.0f;

    std::vector<KeyBinding> m_KeyBindings;
};

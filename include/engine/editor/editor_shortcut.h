#pragma once

#include <platform/interface/key.h>
#include <platform/logic/keyboard_manager.h>
#include <cstdint>

enum class EditorModifier : uint8_t
{
    None = 0,
    Control = 1 << 0,
    Shift = 1 << 1,
    Alt = 1 << 2
};

constexpr EditorModifier operator|(EditorModifier left, EditorModifier right)
{
    return static_cast<EditorModifier>(static_cast<uint8_t>(left) | static_cast<uint8_t>(right));
}

inline EditorModifier GetEditorModifiers(const KeyboardManager& keyboard)
{
    uint8_t modifiers = 0;
    if (keyboard.GetRawKey(Key::LeftControl) || keyboard.GetRawKey(Key::RightControl))
        modifiers |= static_cast<uint8_t>(EditorModifier::Control);
    if (keyboard.GetRawKey(Key::LeftShift) || keyboard.GetRawKey(Key::RightShift))
        modifiers |= static_cast<uint8_t>(EditorModifier::Shift);
    if (keyboard.GetRawKey(Key::LeftAlt) || keyboard.GetRawKey(Key::RightAlt))
        modifiers |= static_cast<uint8_t>(EditorModifier::Alt);
    return static_cast<EditorModifier>(modifiers);
}

// A chord is sampled when its non-modifier key first goes down. Recording the
// latch even when modifiers do not match prevents a held base key from becoming
// a different shortcut after the user presses or releases a modifier.
inline bool IsEditorShortcutPressed(const KeyboardManager& keyboard, Key key, EditorModifier required,
                                    bool& keyWasPressed, bool inputBlocked = false)
{
    const bool keyDown = keyboard.GetRawKey(key);
    if (!keyDown)
    {
        keyWasPressed = false;
        return false;
    }
    if (keyWasPressed)
        return false;

    keyWasPressed = true;
    return !inputBlocked && GetEditorModifiers(keyboard) == required;
}

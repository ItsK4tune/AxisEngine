#pragma once

enum class CursorMode
{
    Normal,
    Hidden,
    Locked,
    LockedHidden,
    Disabled,
    // Visible, freely moving cursor owned by the editor overlay. Game-facing
    // mouse queries are suppressed until the previous cursor mode is restored.
    Editor
};

#pragma once

#include <cstdint>

struct EditorViewportRect
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    bool visible = false;
    bool hovered = false;
    bool focused = false;

    bool Contains(float px, float py) const
    {
        return visible && px >= x && py >= y && px < x + width && py < y + height;
    }
};

// Main game viewport interaction state. The game is rendered directly to the
// platform backbuffer; ImGui panels are only an overlay.
class EditorViewportState
{
public:
    EditorViewportRect rect;
};

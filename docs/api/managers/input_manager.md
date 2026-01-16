# InputManager API

**Include:** `<engine/core/app_handler.h>`

The Input system is managed by `AppHandler`, which aggregates `KeyboardManager` and `MouseManager`.

## Access
Accessible via `m_App->GetAppHandler()`.

## Keyboard Manager
Access: `m_App->GetAppHandler().GetKeyboard()`

*   `bool GetKey(int key)`
    *   Returns `true` if the key is currently held down.
    *   Key codes use GLFW constants (e.g., `GLFW_KEY_W`, `GLFW_KEY_SPACE`).
*   `bool GetKeyDown(int key)`
    *   Returns `true` only on the frame the key was pressed.
*   `bool GetKeyUp(int key)`
    *   Returns `true` only on the frame the key was released.

## Mouse Manager
Access: `m_App->GetAppHandler().GetMouse()`

*   `bool IsButtonDown(int button)`
    *   Returns `true` if the mouse button is held.
    *   Buttons: `GLFW_MOUSE_BUTTON_LEFT`, `GLFW_MOUSE_BUTTON_RIGHT`, `GLFW_MOUSE_BUTTON_MIDDLE`.
*   `glm::vec2 GetPosition()`
    *   Returns the current X,Y cursor position relative to the window.
*   `glm::vec2 GetScrollOffset()`
    *   Returns the scroll wheel delta for this frame.

## Action System (Scriptable Helper)
Scripts inherit helper methods that map string actions to inputs (defined in `settings.json`):

*   `bool GetAction(const std::string& name)`
*   `bool GetActionDown(const std::string& name)`
*   `bool GetActionUp(const std::string& name)`

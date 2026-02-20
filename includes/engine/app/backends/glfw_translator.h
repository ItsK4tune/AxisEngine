#pragma once

#include <GLFW/glfw3.h>
#include <interface/window/input_codes.h>
#include <map>

class GLFWTranslator {
public:
    static Input::Key ToInputKey(int glfwKey) {
        switch (glfwKey) {
            case GLFW_KEY_SPACE: return Input::Key::Space;
            case GLFW_KEY_APOSTROPHE: return Input::Key::Apostrophe;
            case GLFW_KEY_COMMA: return Input::Key::Comma;
            case GLFW_KEY_MINUS: return Input::Key::Minus;
            case GLFW_KEY_PERIOD: return Input::Key::Period;
            case GLFW_KEY_SLASH: return Input::Key::Slash;
            case GLFW_KEY_0: return Input::Key::_0;
            case GLFW_KEY_1: return Input::Key::_1;
            case GLFW_KEY_2: return Input::Key::_2;
            case GLFW_KEY_3: return Input::Key::_3;
            case GLFW_KEY_4: return Input::Key::_4;
            case GLFW_KEY_5: return Input::Key::_5;
            case GLFW_KEY_6: return Input::Key::_6;
            case GLFW_KEY_7: return Input::Key::_7;
            case GLFW_KEY_8: return Input::Key::_8;
            case GLFW_KEY_9: return Input::Key::_9;
            case GLFW_KEY_SEMICOLON: return Input::Key::Semicolon;
            case GLFW_KEY_EQUAL: return Input::Key::Equal;
            case GLFW_KEY_A: return Input::Key::A;
            case GLFW_KEY_B: return Input::Key::B;
            case GLFW_KEY_C: return Input::Key::C;
            case GLFW_KEY_D: return Input::Key::D;
            case GLFW_KEY_E: return Input::Key::E;
            case GLFW_KEY_F: return Input::Key::F;
            case GLFW_KEY_G: return Input::Key::G;
            case GLFW_KEY_H: return Input::Key::H;
            case GLFW_KEY_I: return Input::Key::I;
            case GLFW_KEY_J: return Input::Key::J;
            case GLFW_KEY_K: return Input::Key::K;
            case GLFW_KEY_L: return Input::Key::L;
            case GLFW_KEY_M: return Input::Key::M;
            case GLFW_KEY_N: return Input::Key::N;
            case GLFW_KEY_O: return Input::Key::O;
            case GLFW_KEY_P: return Input::Key::P;
            case GLFW_KEY_Q: return Input::Key::Q;
            case GLFW_KEY_R: return Input::Key::R;
            case GLFW_KEY_S: return Input::Key::S;
            case GLFW_KEY_T: return Input::Key::T;
            case GLFW_KEY_U: return Input::Key::U;
            case GLFW_KEY_V: return Input::Key::V;
            case GLFW_KEY_W: return Input::Key::W;
            case GLFW_KEY_X: return Input::Key::X;
            case GLFW_KEY_Y: return Input::Key::Y;
            case GLFW_KEY_Z: return Input::Key::Z;
            case GLFW_KEY_LEFT_BRACKET: return Input::Key::LeftBracket;
            case GLFW_KEY_BACKSLASH: return Input::Key::Backslash;
            case GLFW_KEY_RIGHT_BRACKET: return Input::Key::RightBracket;
            case GLFW_KEY_GRAVE_ACCENT: return Input::Key::GraveAccent;
            case GLFW_KEY_WORLD_1: return Input::Key::World1;
            case GLFW_KEY_WORLD_2: return Input::Key::World2;
            case GLFW_KEY_ESCAPE: return Input::Key::Escape;
            case GLFW_KEY_ENTER: return Input::Key::Enter;
            case GLFW_KEY_TAB: return Input::Key::Tab;
            case GLFW_KEY_BACKSPACE: return Input::Key::Backspace;
            case GLFW_KEY_INSERT: return Input::Key::Insert;
            case GLFW_KEY_DELETE: return Input::Key::Delete;
            case GLFW_KEY_RIGHT: return Input::Key::Right;
            case GLFW_KEY_LEFT: return Input::Key::Left;
            case GLFW_KEY_DOWN: return Input::Key::Down;
            case GLFW_KEY_UP: return Input::Key::Up;
            case GLFW_KEY_PAGE_UP: return Input::Key::PageUp;
            case GLFW_KEY_PAGE_DOWN: return Input::Key::PageDown;
            case GLFW_KEY_HOME: return Input::Key::Home;
            case GLFW_KEY_END: return Input::Key::End;
            case GLFW_KEY_CAPS_LOCK: return Input::Key::CapsLock;
            case GLFW_KEY_SCROLL_LOCK: return Input::Key::ScrollLock;
            case GLFW_KEY_NUM_LOCK: return Input::Key::NumLock;
            case GLFW_KEY_PRINT_SCREEN: return Input::Key::PrintScreen;
            case GLFW_KEY_PAUSE: return Input::Key::Pause;
            case GLFW_KEY_F1: return Input::Key::F1;
            case GLFW_KEY_F2: return Input::Key::F2;
            case GLFW_KEY_F3: return Input::Key::F3;
            case GLFW_KEY_F4: return Input::Key::F4;
            case GLFW_KEY_F5: return Input::Key::F5;
            case GLFW_KEY_F6: return Input::Key::F6;
            case GLFW_KEY_F7: return Input::Key::F7;
            case GLFW_KEY_F8: return Input::Key::F8;
            case GLFW_KEY_F9: return Input::Key::F9;
            case GLFW_KEY_F10: return Input::Key::F10;
            case GLFW_KEY_F11: return Input::Key::F11;
            case GLFW_KEY_F12: return Input::Key::F12;
            case GLFW_KEY_F13: return Input::Key::F13;
            case GLFW_KEY_F14: return Input::Key::F14;
            case GLFW_KEY_F15: return Input::Key::F15;
            case GLFW_KEY_F16: return Input::Key::F16;
            case GLFW_KEY_F17: return Input::Key::F17;
            case GLFW_KEY_F18: return Input::Key::F18;
            case GLFW_KEY_F19: return Input::Key::F19;
            case GLFW_KEY_F20: return Input::Key::F20;
            case GLFW_KEY_F21: return Input::Key::F21;
            case GLFW_KEY_F22: return Input::Key::F22;
            case GLFW_KEY_F23: return Input::Key::F23;
            case GLFW_KEY_F24: return Input::Key::F24;
            case GLFW_KEY_F25: return Input::Key::F25;
            case GLFW_KEY_KP_0: return Input::Key::Kp0;
            case GLFW_KEY_KP_1: return Input::Key::Kp1;
            case GLFW_KEY_KP_2: return Input::Key::Kp2;
            case GLFW_KEY_KP_3: return Input::Key::Kp3;
            case GLFW_KEY_KP_4: return Input::Key::Kp4;
            case GLFW_KEY_KP_5: return Input::Key::Kp5;
            case GLFW_KEY_KP_6: return Input::Key::Kp6;
            case GLFW_KEY_KP_7: return Input::Key::Kp7;
            case GLFW_KEY_KP_8: return Input::Key::Kp8;
            case GLFW_KEY_KP_9: return Input::Key::Kp9;
            case GLFW_KEY_KP_DECIMAL: return Input::Key::KpDecimal;
            case GLFW_KEY_KP_DIVIDE: return Input::Key::KpDivide;
            case GLFW_KEY_KP_MULTIPLY: return Input::Key::KpMultiply;
            case GLFW_KEY_KP_SUBTRACT: return Input::Key::KpSubtract;
            case GLFW_KEY_KP_ADD: return Input::Key::KpAdd;
            case GLFW_KEY_KP_ENTER: return Input::Key::KpEnter;
            case GLFW_KEY_KP_EQUAL: return Input::Key::KpEqual;
            case GLFW_KEY_LEFT_SHIFT: return Input::Key::LeftShift;
            case GLFW_KEY_LEFT_CONTROL: return Input::Key::LeftControl;
            case GLFW_KEY_LEFT_ALT: return Input::Key::LeftAlt;
            case GLFW_KEY_LEFT_SUPER: return Input::Key::LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT: return Input::Key::RightShift;
            case GLFW_KEY_RIGHT_CONTROL: return Input::Key::RightControl;
            case GLFW_KEY_RIGHT_ALT: return Input::Key::RightAlt;
            case GLFW_KEY_RIGHT_SUPER: return Input::Key::RightSuper;
            case GLFW_KEY_MENU: return Input::Key::Menu;
            default: return Input::Key::Unknown;
        }
    }

    static int ToGLFWKey(Input::Key key) {
        switch (key) {
            case Input::Key::Space: return GLFW_KEY_SPACE;
            case Input::Key::Apostrophe: return GLFW_KEY_APOSTROPHE;
            case Input::Key::Comma: return GLFW_KEY_COMMA;
            case Input::Key::Minus: return GLFW_KEY_MINUS;
            case Input::Key::Period: return GLFW_KEY_PERIOD;
            case Input::Key::Slash: return GLFW_KEY_SLASH;
            case Input::Key::_0: return GLFW_KEY_0;
            case Input::Key::_1: return GLFW_KEY_1;
            case Input::Key::_2: return GLFW_KEY_2;
            case Input::Key::_3: return GLFW_KEY_3;
            case Input::Key::_4: return GLFW_KEY_4;
            case Input::Key::_5: return GLFW_KEY_5;
            case Input::Key::_6: return GLFW_KEY_6;
            case Input::Key::_7: return GLFW_KEY_7;
            case Input::Key::_8: return GLFW_KEY_8;
            case Input::Key::_9: return GLFW_KEY_9;
            case Input::Key::Semicolon: return GLFW_KEY_SEMICOLON;
            case Input::Key::Equal: return GLFW_KEY_EQUAL;
            case Input::Key::A: return GLFW_KEY_A;
            case Input::Key::B: return GLFW_KEY_B;
            case Input::Key::C: return GLFW_KEY_C;
            case Input::Key::D: return GLFW_KEY_D;
            case Input::Key::E: return GLFW_KEY_E;
            case Input::Key::F: return GLFW_KEY_F;
            case Input::Key::G: return GLFW_KEY_G;
            case Input::Key::H: return GLFW_KEY_H;
            case Input::Key::I: return GLFW_KEY_I;
            case Input::Key::J: return GLFW_KEY_J;
            case Input::Key::K: return GLFW_KEY_K;
            case Input::Key::L: return GLFW_KEY_L;
            case Input::Key::M: return GLFW_KEY_M;
            case Input::Key::N: return GLFW_KEY_N;
            case Input::Key::O: return GLFW_KEY_O;
            case Input::Key::P: return GLFW_KEY_P;
            case Input::Key::Q: return GLFW_KEY_Q;
            case Input::Key::R: return GLFW_KEY_R;
            case Input::Key::S: return GLFW_KEY_S;
            case Input::Key::T: return GLFW_KEY_T;
            case Input::Key::U: return GLFW_KEY_U;
            case Input::Key::V: return GLFW_KEY_V;
            case Input::Key::W: return GLFW_KEY_W;
            case Input::Key::X: return GLFW_KEY_X;
            case Input::Key::Y: return GLFW_KEY_Y;
            case Input::Key::Z: return GLFW_KEY_Z;
            case Input::Key::LeftBracket: return GLFW_KEY_LEFT_BRACKET;
            case Input::Key::Backslash: return GLFW_KEY_BACKSLASH;
            case Input::Key::RightBracket: return GLFW_KEY_RIGHT_BRACKET;
            case Input::Key::GraveAccent: return GLFW_KEY_GRAVE_ACCENT;
            case Input::Key::World1: return GLFW_KEY_WORLD_1;
            case Input::Key::World2: return GLFW_KEY_WORLD_2;
            case Input::Key::Escape: return GLFW_KEY_ESCAPE;
            case Input::Key::Enter: return GLFW_KEY_ENTER;
            case Input::Key::Tab: return GLFW_KEY_TAB;
            case Input::Key::Backspace: return GLFW_KEY_BACKSPACE;
            case Input::Key::Insert: return GLFW_KEY_INSERT;
            case Input::Key::Delete: return GLFW_KEY_DELETE;
            case Input::Key::Right: return GLFW_KEY_RIGHT;
            case Input::Key::Left: return GLFW_KEY_LEFT;
            case Input::Key::Down: return GLFW_KEY_DOWN;
            case Input::Key::Up: return GLFW_KEY_UP;
            case Input::Key::PageUp: return GLFW_KEY_PAGE_UP;
            case Input::Key::PageDown: return GLFW_KEY_PAGE_DOWN;
            case Input::Key::Home: return GLFW_KEY_HOME;
            case Input::Key::End: return GLFW_KEY_END;
            case Input::Key::CapsLock: return GLFW_KEY_CAPS_LOCK;
            case Input::Key::ScrollLock: return GLFW_KEY_SCROLL_LOCK;
            case Input::Key::NumLock: return GLFW_KEY_NUM_LOCK;
            case Input::Key::PrintScreen: return GLFW_KEY_PRINT_SCREEN;
            case Input::Key::Pause: return GLFW_KEY_PAUSE;
            case Input::Key::F1: return GLFW_KEY_F1;
            case Input::Key::F2: return GLFW_KEY_F2;
            case Input::Key::F3: return GLFW_KEY_F3;
            case Input::Key::F4: return GLFW_KEY_F4;
            case Input::Key::F5: return GLFW_KEY_F5;
            case Input::Key::F6: return GLFW_KEY_F6;
            case Input::Key::F7: return GLFW_KEY_F7;
            case Input::Key::F8: return GLFW_KEY_F8;
            case Input::Key::F9: return GLFW_KEY_F9;
            case Input::Key::F10: return GLFW_KEY_F10;
            case Input::Key::F11: return GLFW_KEY_F11;
            case Input::Key::F12: return GLFW_KEY_F12;
            case Input::Key::F13: return GLFW_KEY_F13;
            case Input::Key::F14: return GLFW_KEY_F14;
            case Input::Key::F15: return GLFW_KEY_F15;
            case Input::Key::F16: return GLFW_KEY_F16;
            case Input::Key::F17: return GLFW_KEY_F17;
            case Input::Key::F18: return GLFW_KEY_F18;
            case Input::Key::F19: return GLFW_KEY_F19;
            case Input::Key::F20: return GLFW_KEY_F20;
            case Input::Key::F21: return GLFW_KEY_F21;
            case Input::Key::F22: return GLFW_KEY_F22;
            case Input::Key::F23: return GLFW_KEY_F23;
            case Input::Key::F24: return GLFW_KEY_F24;
            case Input::Key::F25: return GLFW_KEY_F25;
            case Input::Key::Kp0: return GLFW_KEY_KP_0;
            case Input::Key::Kp1: return GLFW_KEY_KP_1;
            case Input::Key::Kp2: return GLFW_KEY_KP_2;
            case Input::Key::Kp3: return GLFW_KEY_KP_3;
            case Input::Key::Kp4: return GLFW_KEY_KP_4;
            case Input::Key::Kp5: return GLFW_KEY_KP_5;
            case Input::Key::Kp6: return GLFW_KEY_KP_6;
            case Input::Key::Kp7: return GLFW_KEY_KP_7;
            case Input::Key::Kp8: return GLFW_KEY_KP_8;
            case Input::Key::Kp9: return GLFW_KEY_KP_9;
            case Input::Key::KpDecimal: return GLFW_KEY_KP_DECIMAL;
            case Input::Key::KpDivide: return GLFW_KEY_KP_DIVIDE;
            case Input::Key::KpMultiply: return GLFW_KEY_KP_MULTIPLY;
            case Input::Key::KpSubtract: return GLFW_KEY_KP_SUBTRACT;
            case Input::Key::KpAdd: return GLFW_KEY_KP_ADD;
            case Input::Key::KpEnter: return GLFW_KEY_KP_ENTER;
            case Input::Key::KpEqual: return GLFW_KEY_KP_EQUAL;
            case Input::Key::LeftShift: return GLFW_KEY_LEFT_SHIFT;
            case Input::Key::LeftControl: return GLFW_KEY_LEFT_CONTROL;
            case Input::Key::LeftAlt: return GLFW_KEY_LEFT_ALT;
            case Input::Key::LeftSuper: return GLFW_KEY_LEFT_SUPER;
            case Input::Key::RightShift: return GLFW_KEY_RIGHT_SHIFT;
            case Input::Key::RightControl: return GLFW_KEY_RIGHT_CONTROL;
            case Input::Key::RightAlt: return GLFW_KEY_RIGHT_ALT;
            case Input::Key::RightSuper: return GLFW_KEY_RIGHT_SUPER;
            case Input::Key::Menu: return GLFW_KEY_MENU;
            default: return GLFW_KEY_UNKNOWN;
        }
    }

    static Input::Mouse ToInputMouse(int glfwButton) {
        switch(glfwButton) {
            case GLFW_MOUSE_BUTTON_LEFT: return Input::Mouse::Left;
            case GLFW_MOUSE_BUTTON_RIGHT: return Input::Mouse::Right;
            case GLFW_MOUSE_BUTTON_MIDDLE: return Input::Mouse::Middle;
            case GLFW_MOUSE_BUTTON_4: return Input::Mouse::Button4;
            case GLFW_MOUSE_BUTTON_5: return Input::Mouse::Button5;
            case GLFW_MOUSE_BUTTON_6: return Input::Mouse::Button6;
            case GLFW_MOUSE_BUTTON_7: return Input::Mouse::Button7;
            case GLFW_MOUSE_BUTTON_8: return Input::Mouse::Button8;
            default: return Input::Mouse::Left;
        }
    }

    static int ToGLFWMouse(Input::Mouse button) {
        switch(button) {
            case Input::Mouse::Left: return GLFW_MOUSE_BUTTON_LEFT;
            case Input::Mouse::Right: return GLFW_MOUSE_BUTTON_RIGHT;
            case Input::Mouse::Middle: return GLFW_MOUSE_BUTTON_MIDDLE;
            case Input::Mouse::Button4: return GLFW_MOUSE_BUTTON_4;
            case Input::Mouse::Button5: return GLFW_MOUSE_BUTTON_5;
            case Input::Mouse::Button6: return GLFW_MOUSE_BUTTON_6;
            case Input::Mouse::Button7: return GLFW_MOUSE_BUTTON_7;
            case Input::Mouse::Button8: return GLFW_MOUSE_BUTTON_8;
            default: return GLFW_MOUSE_BUTTON_LEFT;
        }
    }

    static int ToGLFWCursorMode(Input::CursorMode mode) {
        switch(mode) {
            case Input::CursorMode::Normal: return GLFW_CURSOR_NORMAL;
            case Input::CursorMode::Hidden: return GLFW_CURSOR_HIDDEN;
            case Input::CursorMode::Disabled: return GLFW_CURSOR_DISABLED;
            case Input::CursorMode::LockedHidden: return GLFW_CURSOR_DISABLED;
            case Input::CursorMode::Locked: return GLFW_CURSOR_NORMAL;
            default: return GLFW_CURSOR_NORMAL;
        }
    }
};

#pragma once

class IGraphicsContext;
class InputManager;
class IOHandler;
class IWindow;
class KeyboardManager;
class MonitorManager;
class MouseManager;

struct IOContext
{
    IWindow& window;
    KeyboardManager& keyboard;
    MouseManager& mouse;
    InputManager& input;
    MonitorManager& monitor;
    IGraphicsContext& graphics;
    IOHandler& ioHandler;
};

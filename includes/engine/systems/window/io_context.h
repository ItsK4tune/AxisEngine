#pragma once

class IWindow;
class KeyboardManager;
class MouseManager;
class InputManager;
class MonitorManager;
class IGraphicsContext;
class IOHandler;

struct IOContext
{
    IWindow&          window;
    KeyboardManager&  keyboard;
    MouseManager&     mouse;
    InputManager&     input;
    MonitorManager&   monitor;
    IGraphicsContext& graphics;
    IOHandler&        ioHandler;
};

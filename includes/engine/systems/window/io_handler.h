#pragma once

#include <memory>
#include <functional>

#include <systems/window/monitor_manager.h>
#include <systems/audio/interfaces/i_audio_engine.h>
#include <systems/window/interfaces/i_window.h>
#include <systems/input/input_manager.h>

class IGraphicsContext;
class AudioManager;
class KeyboardManager;
class MouseManager;
class MouseManager;

class IOHandler
{
public:
    IOHandler(std::unique_ptr<IGraphicsContext> graphics, std::unique_ptr<IAudioEngine> audioEngine);
    ~IOHandler();

    bool Init(std::unique_ptr<IWindow> window, const std::string& title, int width, int height, int windowMode, int monitorIndex, int refreshRate, bool vsync, int frameRateLimit);
    void SetWindow(IWindow* window);
    void ProcessInput();

    void OnResize(int width, int height);
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);

    MonitorManager& GetMonitorManager() { return *m_MonitorManager; }
    AudioManager& GetAudioManager() { return *m_AudioManager; }
    KeyboardManager& GetKeyboard() const { return *m_KeyboardManager; }
    MouseManager& GetMouse() const { return *m_MouseManager; }
    InputManager& GetInputManager() const { return *m_InputManager; }
    IGraphicsContext& GetGraphicsContext() const { return *m_Graphics; }

private:
    std::unique_ptr<IGraphicsContext> m_Graphics;
    std::unique_ptr<MonitorManager> m_MonitorManager;
    std::unique_ptr<AudioManager> m_AudioManager;
    std::unique_ptr<KeyboardManager> m_KeyboardManager;
    std::unique_ptr<MouseManager> m_MouseManager;
    std::unique_ptr<InputManager> m_InputManager;
};

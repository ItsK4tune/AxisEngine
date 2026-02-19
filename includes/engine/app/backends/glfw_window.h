#ifndef GLFW_WINDOW_H
#define GLFW_WINDOW_H

#include <interface/window/i_window.h>
#include <GLFW/glfw3.h>

class GLFWWindow : public IWindow {
public:
    GLFWWindow();
    ~GLFWWindow() override;

    bool Init(int width, int height, const std::string& title) override;
    void SetTitle(const std::string& title) override;
    void SetIcon(int width, int height, unsigned char* pixels) override;
    void SetVsync(bool enabled) override;

    void Update() override;
    void Shutdown() override;

    bool ShouldClose() const override;
    void SetShouldClose(bool value) override;

    void SwapBuffers() override;
    void PollEvents() override;

    int GetWidth() const override;
    int GetHeight() const override;

    void SetWindowConfiguration(int width, int height, WindowMode mode, int monitorIndex, int refreshRate) override;
    std::vector<MonitorInfo> GetMonitors() const override;
    
    void SetCursorMode(Input::CursorMode mode) override;

    void* GetNativeWindow() const override;

    bool GetKey(Input::Key key) const override;
    bool GetMouseButton(Input::Mouse button) const override;
    void GetCursorPos(double& x, double& y) const override;
    void SetCursorPos(double x, double y) override;

    void SetResizeCallback(const ResizeCallback& callback) override { m_ResizeCallback = callback; }
    void SetKeyCallback(const KeyCallback& callback) override { m_KeyCallback = callback; }
    void SetMouseButtonCallback(const MouseButtonCallback& callback) override { m_MouseButtonCallback = callback; }
    void SetCursorPosCallback(const MousePositionCallback& callback) override { m_CursorPosCallback = callback; }
    void SetScrollCallback(const ScrollCallback& callback) override { m_ScrollCallback = callback; }

private:
    GLFWwindow* m_Window = nullptr;
    int m_Width = 0;
    int m_Height = 0;

    ResizeCallback m_ResizeCallback;
    KeyCallback m_KeyCallback;
    MouseButtonCallback m_MouseButtonCallback;
    MousePositionCallback m_CursorPosCallback;
    ScrollCallback m_ScrollCallback;

    // Static GLFW callbacks
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

    // Helpers
    int MapInputKeyToGLFW(int key); // Might define mapping elsewhere or here? 
                                   // Actually we need GLFW -> InputKey for the callback.
};

#endif // GLFW_WINDOW_H

#pragma once

#ifdef ENABLE_EDITOR

struct GLFWwindow;

class ImGuiLayer
{
public:
    void Initialize(GLFWwindow* window);
    void BeginFrame();
    void EndFrame();
    void Shutdown();
    bool IsInitialized() const { return m_Initialized; }

private:
    bool m_Initialized = false;
};

#endif

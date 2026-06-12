#pragma once

#ifdef ENABLE_EDITOR

class ImGuiLayer
{
public:
    void Initialize(void* nativeWindow);
    void BeginFrame();
    void EndFrame();
    void Shutdown();
    bool IsInitialized() const
    {
        return m_Initialized;
    }

private:
    bool m_Initialized = false;
};

#endif

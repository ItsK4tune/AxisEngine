#pragma once

#ifdef ENABLE_EDITOR

#include <platform/interface/i_ui_input_capture.h>

class IGraphicsContext;
class IWindow;

class ImGuiLayer : public IUIInputCapture
{
public:
    bool Initialize(IWindow& window, IGraphicsContext& graphicsContext);
    void BeginFrame();
    void EndFrame();
    void Shutdown();
    void SetPointerInputEnabled(bool enabled)
    {
        m_PointerInputEnabled = enabled;
    }
    bool IsInitialized() const
    {
        return m_Initialized;
    }
    bool WantsPointerInput() const override;
    bool WantsTextInput() const override;

private:
    bool m_Initialized = false;
    bool m_PointerInputEnabled = false;
    IGraphicsContext* m_GraphicsContext = nullptr;
};

#endif

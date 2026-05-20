#include <platform/logic/io_handler.h>
#include <core/logic/logger.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/keyboard_manager.h>
#include <platform/logic/monitor_manager.h>
#include <platform/logic/mouse_manager.h>
#include <render/interface/i_graphics_context.h>

IOHandler::IOHandler()
{
    m_MonitorManager = std::make_unique<MonitorManager>();
}

IOHandler::~IOHandler()
{
}

bool IOHandler::Initialize(std::unique_ptr<IWindow> window, const std::string& title, int width, int height,
                           int windowMode, int monitorIndex, int refreshRate, bool vsync, int frameRateLimit)
{
    m_MonitorManager->SetWindowTitle(title);
    m_MonitorManager->SetWindowConfiguration(width, height, (WindowMode)windowMode, monitorIndex, refreshRate);
    m_MonitorManager->SetFrameRateLimit(frameRateLimit);

    if (!m_MonitorManager->Initialize(std::move(window)))
    {
        LOGGER_ERROR("IOHandler") << "Failed to initialize MonitorManager";
        return false;
    }

    m_MonitorManager->SetVsync(vsync);
    LOGGER_INFO("IOHandler") << "VSync initialized to: " << (vsync ? "ON" : "OFF");
    LOGGER_INFO("IOHandler") << "Frame Rate Limit initialized to: " << frameRateLimit;

    m_KeyboardManager = std::make_unique<KeyboardManager>(m_MonitorManager->GetWindow());
    m_MouseManager = std::make_unique<MouseManager>(m_MonitorManager->GetWindow());
    m_InputManager =
        std::make_unique<InputManager>(*m_KeyboardManager, *m_MouseManager, *m_MonitorManager->GetWindow());

    m_MouseManager->SetLastPosition(width / 2.0, height / 2.0);
    m_MouseManager->SetWindowSize(width, height);

    return true;
}

void IOHandler::SetWindow(IWindow* window)
{
    if (m_KeyboardManager)
        m_KeyboardManager->SetWindow(window);
    if (m_MouseManager)
        m_MouseManager->SetWindow(window);
}

void IOHandler::ProcessInput()
{
    if (m_KeyboardManager)
        m_KeyboardManager->Update();
}

void IOHandler::OnResize(int width, int height)
{
    m_MonitorManager->OnResize(width, height);
    if (m_MouseManager)
        m_MouseManager->SetWindowSize(width, height);
}

void IOHandler::OnMouseMove(double xpos, double ypos)
{
    if (m_MouseManager)
        m_MouseManager->UpdatePosition(xpos, ypos);
}

void IOHandler::OnMouseButton(int button, int action, int mods)
{
    if (m_MouseManager)
        m_MouseManager->UpdateButton(static_cast<Mouse>(button), action, mods);
}

void IOHandler::OnScroll(double xoffset, double yoffset)
{
    if (m_MouseManager)
        m_MouseManager->UpdateScroll(xoffset, yoffset);
}

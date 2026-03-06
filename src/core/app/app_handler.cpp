#include <core/app/app_handler.h>
#include <systems/input/input_manager.h>
#include <systems/input/keyboard_manager.h>
#include <systems/input/mouse_manager.h>
#include <systems/window/interfaces/i_window.h>
#include <iostream>

AppHandler::AppHandler(IWindow* window)
{
    m_KeyboardManager = std::make_unique<KeyboardManager>(window);
    m_MouseManager = std::make_unique<MouseManager>(window);
    m_InputManager = std::make_unique<InputManager>(*m_KeyboardManager, *m_MouseManager, *window);
}

AppHandler::~AppHandler()
{
}

void AppHandler::ProcessInput(IWindow* window)
{
    if (m_KeyboardManager->GetKey(Input::Key::Escape))
        window->SetShouldClose(true);
}

void AppHandler::OnMouseMove(double xpos, double ypos)
{
    m_MouseManager->UpdatePosition(xpos, ypos);
}

void AppHandler::OnMouseButton(int button, int action, int mods)
{
    m_MouseManager->UpdateButton(static_cast<Input::Mouse>(button), action, mods);
}

void AppHandler::OnResize(int width, int height)
{
    m_MouseManager->SetWindowSize(width, height);
}

void AppHandler::OnScroll(double xoffset, double yoffset)
{
    m_MouseManager->UpdateScroll(xoffset, yoffset);
}

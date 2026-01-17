#include <app/app_handler.h>
#include <iostream>

AppHandler::AppHandler(GLFWwindow* window)
{
    m_KeyboardManager = std::make_unique<KeyboardManager>(window);
    m_MouseManager = std::make_unique<MouseManager>(window);
    m_InputManager = std::make_unique<InputManager>(*m_KeyboardManager, *m_MouseManager);
}

AppHandler::~AppHandler()
{
}

void AppHandler::ProcessInput(GLFWwindow* window)
{
    if (m_KeyboardManager->GetKey(GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(window, true);
}

void AppHandler::OnMouseMove(double xpos, double ypos)
{
    m_MouseManager->UpdatePosition(xpos, ypos);
}

void AppHandler::OnMouseButton(int button, int action, int mods)
{
    m_MouseManager->UpdateButton(button, action, mods);
}

void AppHandler::OnScroll(double xoffset, double yoffset)
{
    m_MouseManager->UpdateScroll(xoffset, yoffset);
}

#include <core/app_handler.h>
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
    
    // F2 Debug Logging
    if (m_KeyboardManager->GetKey(GLFW_KEY_F2))
    {
        if (!m_F2Pressed) 
        {
            if (m_OnDebugLog) m_OnDebugLog();
            m_F2Pressed = true;
        }
    }
    else
    {
        m_F2Pressed = false;
    }

    // F1 Physics Toggle
    if (m_KeyboardManager->GetKey(GLFW_KEY_F1))
    {
        if (!m_F1Pressed)
        {
            if (m_OnPhysicsDebugToggle) m_OnPhysicsDebugToggle();
            m_F1Pressed = true;
        }
    }
    else
    {
        m_F1Pressed = false;
    }
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

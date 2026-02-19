#include <graphic/backends/opengl_context.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <utils/logger.h>

bool OpenGLContext::Init()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LOGGER_ERROR("OpenGLContext") << "Failed to initialize GLAD";
        return false;
    }

    LOGGER_INFO("OpenGLContext") << "OpenGL initialized: " << glGetString(GL_VERSION);
    return true;
}

void OpenGLContext::Shutdown()
{
}

void OpenGLContext::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void OpenGLContext::SetDepthTest(bool enable)
{
    if (enable)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void OpenGLContext::SetCullFace(bool enable)
{
    if (enable)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}



#include <graphic/backends/opengl_translator.h>

void OpenGLContext::Clear(Graphics::BufferBit flags)
{
    glClear(GLTranslator::ToGL(flags));
}

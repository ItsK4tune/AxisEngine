#include <glad/glad.h>
#include <render/strategy/opengl/opengl_context.h>
#include <GLFW/glfw3.h>
#include <core/logic/logger.h>

bool OpenGLContext::Init()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LOGGER_ERROR("OpenGLContext") << "Failed to initialize GLAD";
        return false;
    }

    LOGGER_INFO("OpenGLContext") << "OpenGL initialized: " << glGetString(GL_VERSION);

    m_DrawContext.SetRenderStateManager(&m_RenderStateManager);

    return true;
}

void OpenGLContext::Shutdown()
{
}

void OpenGLContext::SetViewport(int x, int y, int width, int height)
{
    m_RenderStateManager.SetViewport(x, y, width, height);
}

void OpenGLContext::SetDepthTest(bool enable)
{
    if (enable)
        m_RenderStateManager.Enable(ServerCapability::DepthTest);
    else
        m_RenderStateManager.Disable(ServerCapability::DepthTest);
}

void OpenGLContext::SetCullFace(bool enable)
{
    if (enable)
        m_RenderStateManager.Enable(ServerCapability::CullFace);
    else
        m_RenderStateManager.Disable(ServerCapability::CullFace);
}

void OpenGLContext::SetBlending(bool enable)
{
    if (enable)
        m_RenderStateManager.Enable(ServerCapability::Blend);
    else
        m_RenderStateManager.Disable(ServerCapability::Blend);
}

void OpenGLContext::SetBlendFunc(BlendFactor src, BlendFactor dst)
{
    m_RenderStateManager.SetBlendFunc(src, dst);
}

#include <render/strategy/opengl/opengl_translator.h>

void OpenGLContext::Clear(BufferBit flags)
{
    glClear(GLTranslator::ToGL(flags));
}

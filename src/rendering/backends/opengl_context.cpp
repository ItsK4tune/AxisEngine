#include <glad/glad.h>
#include <rendering/backends/opengl_context.h>
#include <GLFW/glfw3.h>
#include <core/utils/logger.h>

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
    m_RenderStateManager.Viewport(x, y, width, height);
}

void OpenGLContext::SetDepthTest(bool enable)
{
    if (enable)
        m_RenderStateManager.Enable(Graphics::ServerCapability::DepthTest);
    else
        m_RenderStateManager.Disable(Graphics::ServerCapability::DepthTest);
}

void OpenGLContext::SetCullFace(bool enable)
{
    if (enable)
        m_RenderStateManager.Enable(Graphics::ServerCapability::CullFace);
    else
        m_RenderStateManager.Disable(Graphics::ServerCapability::CullFace);
}

void OpenGLContext::SetBlending(bool enable)
{
    if (enable)
        m_RenderStateManager.Enable(Graphics::ServerCapability::Blend);
    else
        m_RenderStateManager.Disable(Graphics::ServerCapability::Blend);
}

void OpenGLContext::SetBlendFunc(Graphics::BlendFactor src, Graphics::BlendFactor dst)
{
    m_RenderStateManager.BlendFunc(src, dst);
}

#include <rendering/backends/opengl_translator.h>

void OpenGLContext::Clear(Graphics::BufferBit flags)
{
    glClear(GLTranslator::ToGL(flags));
}

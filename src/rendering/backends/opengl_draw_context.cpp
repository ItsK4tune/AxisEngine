#include <rendering/backends/opengl_draw_context.h>
#include <rendering/interfaces/i_render_state_manager.h>
#include <rendering/backends/opengl_translator.h>
#include <glad/glad.h>

void OpenGLDrawContext::Clear(Graphics::BufferBit mask)
{
    glClear(GLTranslator::ToGL(mask));
}

void OpenGLDrawContext::ClearColor(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
}

void OpenGLDrawContext::SetViewport(int x, int y, int width, int height)
{
    if (m_RenderStateManager)
        m_RenderStateManager->Viewport(x, y, width, height);
    else
        glViewport(x, y, width, height);
}

void OpenGLDrawContext::Scissor(int x, int y, int width, int height)
{
    if (m_RenderStateManager)
        m_RenderStateManager->Scissor(x, y, width, height);
    else
        glScissor(x, y, width, height);
}

void OpenGLDrawContext::DrawArrays(Graphics::Primitive mode, int first, int count)
{
    glDrawArrays(GLTranslator::ToGL(mode), first, count);
}

void OpenGLDrawContext::DrawElements(Graphics::Primitive mode, int count, Graphics::DataType type, const void *indices)
{
    glDrawElements(GLTranslator::ToGL(mode), count, GLTranslator::ToGL(type), indices);
}

void OpenGLDrawContext::DrawArraysInstanced(Graphics::Primitive mode, int first, int count, int instancecount)
{
    glDrawArraysInstanced(GLTranslator::ToGL(mode), first, count, instancecount);
}

void OpenGLDrawContext::DrawElementsInstanced(Graphics::Primitive mode, int count, Graphics::DataType type, const void *indices, int instancecount)
{
    glDrawElementsInstanced(GLTranslator::ToGL(mode), count, GLTranslator::ToGL(type), indices, instancecount);
}

const char *OpenGLDrawContext::GetBackendName() const
{
    return "OpenGL";
}

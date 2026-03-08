#include <render/strategy/opengl/opengl_draw_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/strategy/opengl/opengl_translator.h>
#include <glad/glad.h>

void OpenGLDrawContext::Clear(BufferBit mask)
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
        m_RenderStateManager->SetViewport(x, y, width, height);
    else
        glViewport(x, y, width, height);
}

void OpenGLDrawContext::Scissor(int x, int y, int width, int height)
{
    if (m_RenderStateManager)
        m_RenderStateManager->SetScissor(x, y, width, height);
    else
        glScissor(x, y, width, height);
}

void OpenGLDrawContext::DrawArrays(Primitive mode, int first, int count)
{
    glDrawArrays(GLTranslator::ToGL(mode), first, count);
}

void OpenGLDrawContext::DrawElements(Primitive mode, int count, DataType type, const void *indices)
{
    glDrawElements(GLTranslator::ToGL(mode), count, GLTranslator::ToGL(type), indices);
}

void OpenGLDrawContext::DrawArraysInstanced(Primitive mode, int first, int count, int instancecount)
{
    glDrawArraysInstanced(GLTranslator::ToGL(mode), first, count, instancecount);
}

void OpenGLDrawContext::DrawElementsInstanced(Primitive mode, int count, DataType type, const void *indices, int instancecount)
{
    glDrawElementsInstanced(GLTranslator::ToGL(mode), count, GLTranslator::ToGL(type), indices, instancecount);
}

const char *OpenGLDrawContext::GetBackendName() const
{
    return "OpenGL";
}

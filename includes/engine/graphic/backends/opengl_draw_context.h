#pragma once

#include <interface/graphic/i_draw_context.h>
#include <graphic/backends/opengl_translator.h>
#include <glad/glad.h>

class OpenGLDrawContext : public IDrawContext
{
public:
    void Clear(Graphics::BufferBit mask) override { glClear(GLTranslator::ToGL(mask)); }
    void ClearColor(float r, float g, float b, float a) override { glClearColor(r, g, b, a); }

    void SetViewport(int x, int y, int width, int height) override { glViewport(x, y, width, height); }
    void Scissor(int x, int y, int width, int height) override { glScissor(x, y, width, height); }

    void DrawArrays(Graphics::Primitive mode, int first, int count) override { glDrawArrays(GLTranslator::ToGL(mode), first, count); }

    void DrawElements(Graphics::Primitive mode, int count, Graphics::DataType type, const void *indices) override
    {
        glDrawElements(GLTranslator::ToGL(mode), count, GLTranslator::ToGL(type), indices);
    }

    void DrawArraysInstanced(Graphics::Primitive mode, int first, int count, int instancecount) override
    {
        glDrawArraysInstanced(GLTranslator::ToGL(mode), first, count, instancecount);
    }

    void DrawElementsInstanced(Graphics::Primitive mode, int count, Graphics::DataType type, const void *indices, int instancecount) override
    {
        glDrawElementsInstanced(GLTranslator::ToGL(mode), count, GLTranslator::ToGL(type), indices, instancecount);
    }

    const char *GetBackendName() const override { return "OpenGL"; }
};

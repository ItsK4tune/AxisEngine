#pragma once

#include <graphics/interfaces/i_draw_context.h>

class IRenderStateManager;

class OpenGLDrawContext : public IDrawContext
{
public:
    void SetRenderStateManager(IRenderStateManager *rsm) { m_RenderStateManager = rsm; }

    void Clear(Graphics::BufferBit mask) override;
    void ClearColor(float r, float g, float b, float a) override;

    void SetViewport(int x, int y, int width, int height) override;
    void Scissor(int x, int y, int width, int height) override;

    void DrawArrays(Graphics::Primitive mode, int first, int count) override;
    void DrawElements(Graphics::Primitive mode, int count, Graphics::DataType type, const void *indices) override;
    void DrawArraysInstanced(Graphics::Primitive mode, int first, int count, int instancecount) override;
    void DrawElementsInstanced(Graphics::Primitive mode, int count, Graphics::DataType type, const void *indices, int instancecount) override;

    const char *GetBackendName() const override;

private:
    IRenderStateManager *m_RenderStateManager = nullptr;
};

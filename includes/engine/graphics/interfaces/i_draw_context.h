#pragma once

#include <cstdint>
#include <graphics/interfaces/graphics_types.h>

class IDrawContext
{
public:
    virtual ~IDrawContext() = default;

    virtual void Clear(Graphics::BufferBit mask) = 0;
    virtual void ClearColor(float r, float g, float b, float a) = 0;

    virtual void SetViewport(int x, int y, int width, int height) = 0;
    virtual void Scissor(int x, int y, int width, int height) = 0;

    virtual void DrawArrays(Graphics::Primitive mode, int first, int count) = 0;
    virtual void DrawElements(Graphics::Primitive mode, int count, Graphics::DataType type, const void *indices) = 0;
    virtual void DrawArraysInstanced(Graphics::Primitive mode, int first, int count, int instancecount) = 0;
    virtual void DrawElementsInstanced(Graphics::Primitive mode, int count, Graphics::DataType type, const void *indices, int instancecount) = 0;

    virtual const char *GetBackendName() const = 0;
};

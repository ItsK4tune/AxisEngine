#pragma once

#include <cstddef>
#include <rendering/types/graphics_enums.h>
#include <rendering/types/buffer_types.h>
#include <rendering/types/texture_types.h>
#include <rendering/types/render_state_types.h>
#include <rendering/types/framebuffer_types.h>
#include <rendering/types/graphics_query_types.h>
#include <rendering/types/gpu_handle.h>

class IBufferManager
{
public:
    virtual ~IBufferManager() = default;

    virtual unsigned int CreateVertexArray() = 0;
    virtual unsigned int GenVertexArray() = 0;
    virtual void BindVertexArray(unsigned int vao) = 0;
    virtual void DeleteVertexArray(unsigned int vao) = 0;
    virtual void DeleteVertexArrays(int n, const unsigned int* arrays) = 0;

    virtual unsigned int CreateBuffer() = 0;
    virtual unsigned int GenBuffer() = 0;
    virtual void BindBuffer(Graphics::BufferType target, unsigned int buffer) = 0;
    virtual void BufferData(Graphics::BufferType target, size_t size, const void *data, Graphics::BufferUsage usage) = 0;
    virtual void BufferSubData(Graphics::BufferType target, size_t offset, size_t size, const void *data) = 0;
    virtual void DeleteBuffer(unsigned int buffer) = 0;
    virtual void DeleteBuffers(int n, const unsigned int* buffers) = 0;

    virtual void BindBufferBase(Graphics::BufferType target, unsigned int index, unsigned int buffer) = 0;
    virtual void BindBufferRange(Graphics::BufferType target, unsigned int index, unsigned int buffer, size_t offset, size_t size) = 0;

    virtual void EnableVertexAttribArray(unsigned int index) = 0;
    virtual void VertexAttribPointer(unsigned int index, int size, Graphics::DataType type, bool normalized, int stride, const void *pointer) = 0;
    virtual void VertexAttribIPointer(unsigned int index, int size, Graphics::DataType type, int stride, const void *pointer) = 0;
    virtual void VertexAttribDivisor(unsigned int index, unsigned int divisor) = 0;

    virtual const char *GetBackendName() const = 0;
};

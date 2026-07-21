#pragma once

#include <render/type/graphics_types.h>
#include <cstddef>
#include <cstdint>

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
    virtual void BindBuffer(BufferType target, unsigned int buffer) = 0;
    virtual void BufferData(BufferType target, size_t size, const void* data, BufferUsage usage) = 0;
    virtual void BufferSubData(BufferType target, size_t offset, size_t size, const void* data) = 0;

    // Optional high-throughput transient upload path. Default implementations
    // keep third-party backends source-compatible and select the BufferSubData
    // fallback in TransientBufferRing.
    virtual bool SupportsPersistentMapping() const { return false; }
    virtual size_t GetBufferOffsetAlignment(BufferType) const { return 1; }
    virtual void* AllocatePersistentStorage(BufferType, size_t) { return nullptr; }
    virtual void FlushPersistentWrites() {}
    virtual void UnmapPersistentStorage(BufferType) {}
    virtual uintptr_t InsertGpuFence() { return 0; }
    virtual void WaitAndDeleteGpuFence(uintptr_t) {}
    virtual void DeleteBuffer(unsigned int buffer) = 0;
    virtual void DeleteBuffers(int n, const unsigned int* buffers) = 0;

    virtual void BindBufferBase(BufferType target, unsigned int index, unsigned int buffer) = 0;
    virtual void BindBufferRange(BufferType target, unsigned int index, unsigned int buffer, size_t offset,
                                 size_t size) = 0;

    virtual void EnableVertexAttribArray(unsigned int index) = 0;
    virtual void VertexAttribPointer(unsigned int index, int size, DataType type, bool normalized, int stride,
                                     const void* pointer) = 0;
    virtual void VertexAttribIPointer(unsigned int index, int size, DataType type, int stride, const void* pointer) = 0;
    virtual void VertexAttribDivisor(unsigned int index, unsigned int divisor) = 0;

    virtual const char* GetBackendName() const = 0;
};

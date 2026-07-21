#include <render/interface/i_buffer_manager.h>
#include <render/strategy/opengl/opengl_translator.h>
#include <glad/glad.h>
#include <core/logic/runtime_profiler.h>
#include <array>
#include <algorithm>
#include <limits>

class OpenGLBufferManager : public IBufferManager
{
public:
    OpenGLBufferManager()
    {
        InvalidateCache();
    }

    void InvalidateCache()
    {
        m_BoundVertexArray = InvalidBinding;
        m_BoundBuffers.fill(InvalidBinding);
    }

    void SetCacheEnabled(bool enabled)
    {
        m_CacheEnabled = enabled;
        InvalidateCache();
    }

    unsigned int CreateVertexArray() override
    {
        unsigned int vao;
        glGenVertexArrays(1, &vao);
        return vao;
    }

    unsigned int GenVertexArray() override
    {
        return CreateVertexArray();
    }

    void BindVertexArray(unsigned int vao) override
    {
        if (!m_CacheEnabled || m_BoundVertexArray != vao)
        {
            glBindVertexArray(vao);
            m_BoundVertexArray = vao;
            m_BoundBuffers[static_cast<size_t>(BufferType::ElementArrayBuffer)] = InvalidBinding;
            RuntimeProfiler::Instance().AddStateChanges();
        }
    }

    void DeleteVertexArray(unsigned int vao) override
    {
        glDeleteVertexArrays(1, &vao);
        if (m_BoundVertexArray == vao)
            m_BoundVertexArray = InvalidBinding;
    }
    void DeleteVertexArrays(int n, const unsigned int* arrays) override
    {
        glDeleteVertexArrays(n, arrays);
    }

    unsigned int CreateBuffer() override
    {
        unsigned int buffer;
        glGenBuffers(1, &buffer);
        return buffer;
    }

    unsigned int GenBuffer() override
    {
        return CreateBuffer();
    }

    void BindBuffer(BufferType target, unsigned int buffer) override
    {
        auto& current = m_BoundBuffers[static_cast<size_t>(target)];
        if (!m_CacheEnabled || current != buffer)
        {
            glBindBuffer(GLTranslator::ToGL(target), buffer);
            current = buffer;
            RuntimeProfiler::Instance().AddStateChanges();
        }
    }

    void BufferData(BufferType target, size_t size, const void* data, BufferUsage usage) override
    {
        glBufferData(GLTranslator::ToGL(target), static_cast<GLsizeiptr>(size), data, GLTranslator::ToGL(usage));
        RuntimeProfiler::Instance().AddUploadBytes(size);
        RuntimeProfiler::Instance().AddTransientAllocations();
    }

    void BufferSubData(BufferType target, size_t offset, size_t size, const void* data) override
    {
        glBufferSubData(GLTranslator::ToGL(target), static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
        RuntimeProfiler::Instance().AddUploadBytes(size);
    }

    bool SupportsPersistentMapping() const override
    {
        return glBufferStorage != nullptr && glMapBufferRange != nullptr && glFenceSync != nullptr &&
               glClientWaitSync != nullptr;
    }

    size_t GetBufferOffsetAlignment(BufferType target) const override
    {
        GLint alignment = 1;
        if (target == BufferType::UniformBuffer)
            glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
        else if (target == BufferType::ShaderStorageBuffer)
            glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &alignment);
        return static_cast<size_t>((std::max)(1, alignment));
    }

    void* AllocatePersistentStorage(BufferType target, size_t size) override
    {
        const GLenum glTarget = GLTranslator::ToGL(target);
        constexpr GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage(glTarget, static_cast<GLsizeiptr>(size), nullptr, flags);
        return glMapBufferRange(glTarget, 0, static_cast<GLsizeiptr>(size), flags);
    }

    void UnmapPersistentStorage(BufferType target) override
    {
        glUnmapBuffer(GLTranslator::ToGL(target));
    }

    void FlushPersistentWrites() override
    {
        glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
    }

    uintptr_t InsertGpuFence() override
    {
        return reinterpret_cast<uintptr_t>(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
    }

    void WaitAndDeleteGpuFence(uintptr_t handle) override
    {
        if (!handle)
            return;
        GLsync fence = reinterpret_cast<GLsync>(handle);
        GLenum result = GL_TIMEOUT_EXPIRED;
        while (result == GL_TIMEOUT_EXPIRED)
            result = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000ULL);
        glDeleteSync(fence);
    }

    void DeleteBuffer(unsigned int buffer) override
    {
        glDeleteBuffers(1, &buffer);
        for (auto& bound : m_BoundBuffers)
            if (bound == buffer)
                bound = InvalidBinding;
    }
    void DeleteBuffers(int n, const unsigned int* buffers) override
    {
        glDeleteBuffers(n, buffers);
        InvalidateCache();
    }

    void BindBufferBase(BufferType target, unsigned int index, unsigned int buffer) override
    {
        glBindBufferBase(GLTranslator::ToGL(target), index, buffer);
        m_BoundBuffers[static_cast<size_t>(target)] = buffer;
        RuntimeProfiler::Instance().AddStateChanges();
    }

    void BindBufferRange(BufferType target, unsigned int index, unsigned int buffer, size_t offset,
                         size_t size) override
    {
        glBindBufferRange(GLTranslator::ToGL(target), index, buffer, static_cast<GLintptr>(offset),
                          static_cast<GLsizeiptr>(size));
        m_BoundBuffers[static_cast<size_t>(target)] = buffer;
        RuntimeProfiler::Instance().AddStateChanges();
    }

    void EnableVertexAttribArray(unsigned int index) override
    {
        glEnableVertexAttribArray(index);
    }

    void VertexAttribPointer(unsigned int index, int size, DataType type, bool normalized, int stride,
                             const void* pointer) override
    {
        glVertexAttribPointer(index, size, GLTranslator::ToGL(type), normalized ? GL_TRUE : GL_FALSE, stride, pointer);
    }

    void VertexAttribIPointer(unsigned int index, int size, DataType type, int stride, const void* pointer) override
    {
        glVertexAttribIPointer(index, size, GLTranslator::ToGL(type), stride, pointer);
    }

    void VertexAttribDivisor(unsigned int index, unsigned int divisor) override
    {
        glVertexAttribDivisor(index, divisor);
    }

    const char* GetBackendName() const override
    {
        return "OpenGL";
    }

private:
    static constexpr unsigned int InvalidBinding = (std::numeric_limits<unsigned int>::max)();
    unsigned int m_BoundVertexArray = InvalidBinding;
    std::array<unsigned int, 12> m_BoundBuffers = {};
    bool m_CacheEnabled = true;
};

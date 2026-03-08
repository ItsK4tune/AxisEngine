#include <glad/glad.h>
#include <render/interface/i_buffer_manager.h>
#include <render/strategy/opengl/opengl_translator.h>

class OpenGLBufferManager : public IBufferManager
{
public:
    unsigned int CreateVertexArray() override
    {
        unsigned int vao;
        glGenVertexArrays(1, &vao);
        return vao;
    }

    unsigned int GenVertexArray() override { return CreateVertexArray(); }

    void BindVertexArray(unsigned int vao) override { glBindVertexArray(vao); }

    void DeleteVertexArray(unsigned int vao) override { glDeleteVertexArrays(1, &vao); }
    void DeleteVertexArrays(int n, const unsigned int* arrays) override { glDeleteVertexArrays(n, arrays); }

    unsigned int CreateBuffer() override
    {
        unsigned int buffer;
        glGenBuffers(1, &buffer);
        return buffer;
    }

    unsigned int GenBuffer() override { return CreateBuffer(); }

    void BindBuffer(BufferType target, unsigned int buffer) override { glBindBuffer(GLTranslator::ToGL(target), buffer); }

    void BufferData(BufferType target, size_t size, const void *data, BufferUsage usage) override
    {
        glBufferData(GLTranslator::ToGL(target), static_cast<GLsizeiptr>(size), data, GLTranslator::ToGL(usage));
    }

    void BufferSubData(BufferType target, size_t offset, size_t size, const void *data) override
    {
        glBufferSubData(GLTranslator::ToGL(target), static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
    }

    void DeleteBuffer(unsigned int buffer) override { glDeleteBuffers(1, &buffer); }
    void DeleteBuffers(int n, const unsigned int* buffers) override { glDeleteBuffers(n, buffers); }

    void BindBufferBase(BufferType target, unsigned int index, unsigned int buffer) override
    {
        glBindBufferBase(GLTranslator::ToGL(target), index, buffer);
    }

    void BindBufferRange(BufferType target, unsigned int index, unsigned int buffer, size_t offset, size_t size) override
    {
        glBindBufferRange(GLTranslator::ToGL(target), index, buffer, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size));
    }

    void EnableVertexAttribArray(unsigned int index) override { glEnableVertexAttribArray(index); }

    void VertexAttribPointer(unsigned int index, int size, DataType type, bool normalized, int stride, const void *pointer) override
    {
        glVertexAttribPointer(index, size, GLTranslator::ToGL(type), normalized ? GL_TRUE : GL_FALSE, stride, pointer);
    }

    void VertexAttribIPointer(unsigned int index, int size, DataType type, int stride, const void *pointer) override
    {
        glVertexAttribIPointer(index, size, GLTranslator::ToGL(type), stride, pointer);
    }

    void VertexAttribDivisor(unsigned int index, unsigned int divisor) override { glVertexAttribDivisor(index, divisor); }

    const char *GetBackendName() const override { return "OpenGL"; }
};
#pragma once

#include <render/interface/i_shader_manager.h>
#include <render/strategy/opengl/opengl_translator.h>
#include <glad/glad.h>
#include <core/logic/runtime_profiler.h>
#include <limits>

class OpenGLShaderManager : public IShaderManager
{
public:
    void InvalidateCache()
    {
        m_CurrentProgram = InvalidProgram;
    }

    void SetCacheEnabled(bool enabled)
    {
        m_CacheEnabled = enabled;
        InvalidateCache();
    }

    unsigned int CreateShader(ShaderType type) override
    {
        return glCreateShader(GLTranslator::ToGL(type));
    }

    void ShaderSource(unsigned int shader, const char* source) override
    {
        glShaderSource(shader, 1, &source, NULL);
    }

    void CompileShader(unsigned int shader) override
    {
        glCompileShader(shader);
    }
    void DeleteShader(unsigned int shader) override
    {
        glDeleteShader(shader);
    }

    bool GetShaderCompileStatus(unsigned int shader) override
    {
        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        return success != 0;
    }

    std::string GetShaderInfoLog(unsigned int shader) override
    {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        return std::string(infoLog);
    }

    unsigned int CreateProgram() override
    {
        return glCreateProgram();
    }
    void AttachShader(unsigned int program, unsigned int shader) override
    {
        glAttachShader(program, shader);
    }
    void LinkProgram(unsigned int program) override
    {
        glLinkProgram(program);
    }
    void UseProgram(unsigned int program) override
    {
        if (!m_CacheEnabled || m_CurrentProgram != program)
        {
            glUseProgram(program);
            m_CurrentProgram = program;
            RuntimeProfiler::Instance().AddStateChanges();
        }
    }
    void DeleteProgram(unsigned int program) override
    {
        glDeleteProgram(program);
        if (m_CurrentProgram == program)
            m_CurrentProgram = InvalidProgram;
    }

    bool GetProgramLinkStatus(unsigned int program) override
    {
        int success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        return success != 0;
    }

    std::string GetProgramInfoLog(unsigned int program) override
    {
        char infoLog[1024];
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        return std::string(infoLog);
    }

    int GetUniformLocation(unsigned int program, const char* name) override
    {
        return glGetUniformLocation(program, name);
    }
    void SetUniform1i(int location, int value) override
    {
        glUniform1i(location, value);
    }
    void SetUniform1ui(int location, unsigned int value) override
    {
        glUniform1ui(location, value);
    }
    void SetUniform1uiv(int location, int count, const unsigned int* value) override
    {
        glUniform1uiv(location, count, value);
    }
    void SetUniform1f(int location, float value) override
    {
        glUniform1f(location, value);
    }
    void SetUniform1fv(int location, int count, const float* value) override
    {
        glUniform1fv(location, count, value);
    }
    void SetUniform2f(int location, float v0, float v1) override
    {
        glUniform2f(location, v0, v1);
    }
    void SetUniform2fv(int location, const float* value) override
    {
        glUniform2fv(location, 1, value);
    }
    void SetUniform3f(int location, float v0, float v1, float v2) override
    {
        glUniform3f(location, v0, v1, v2);
    }
    void SetUniform3fv(int location, const float* value) override
    {
        glUniform3fv(location, 1, value);
    }
    void SetUniform4f(int location, float v0, float v1, float v2, float v3) override
    {
        glUniform4f(location, v0, v1, v2, v3);
    }
    void SetUniform4fv(int location, const float* value) override
    {
        glUniform4fv(location, 1, value);
    }
    void SetUniformMatrix2fv(int location, const float* value) override
    {
        glUniformMatrix2fv(location, 1, GL_FALSE, value);
    }
    void SetUniformMatrix3fv(int location, const float* value) override
    {
        glUniformMatrix3fv(location, 1, GL_FALSE, value);
    }
    void SetUniformMatrix4fv(int location, const float* value) override
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, value);
    }
    void SetUniform3fvArray(int location, int count, const float* value) override
    {
        glUniform3fv(location, count, value);
    }
    void SetUniformMatrix4fvArray(int location, int count, const float* value) override
    {
        glUniformMatrix4fv(location, count, GL_FALSE, value);
    }

    void DispatchCompute(unsigned int numGroupsX, unsigned int numGroupsY, unsigned int numGroupsZ) override
    {
        glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
    }

    void MemoryBarrier(MemoryBarrierBit barriers) override
    {
        glMemoryBarrier(GLTranslator::ToGL(barriers));
    }

    const char* GetBackendName() const override
    {
        return "OpenGL";
    }

private:
    static constexpr unsigned int InvalidProgram = (std::numeric_limits<unsigned int>::max)();
    unsigned int m_CurrentProgram = InvalidProgram;
    bool m_CacheEnabled = true;
};

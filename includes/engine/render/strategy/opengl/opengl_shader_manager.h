#pragma once

#include <glad/glad.h>
#include <render/interface/i_shader_manager.h>
#include <render/strategy/opengl/opengl_translator.h>

class OpenGLShaderManager : public IShaderManager
{
public:
    unsigned int CreateShader(ShaderType type) override { return glCreateShader(GLTranslator::ToGL(type)); }

    void ShaderSource(unsigned int shader, const char *source) override
    {
        glShaderSource(shader, 1, &source, NULL);
    }

    void CompileShader(unsigned int shader) override { glCompileShader(shader); }
    void DeleteShader(unsigned int shader) override { glDeleteShader(shader); }

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

    unsigned int CreateProgram() override { return glCreateProgram(); }
    void AttachShader(unsigned int program, unsigned int shader) override { glAttachShader(program, shader); }
    void LinkProgram(unsigned int program) override { glLinkProgram(program); }
    void UseProgram(unsigned int program) override { glUseProgram(program); }
    void DeleteProgram(unsigned int program) override { glDeleteProgram(program); }

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

    int GetUniformLocation(unsigned int program, const char *name) override { return glGetUniformLocation(program, name); }
    void SetUniform1i(int location, int value) override { glUniform1i(location, value); }
    void SetUniform1f(int location, float value) override { glUniform1f(location, value); }
    void SetUniform1fv(int location, int count, const float *value) override { glUniform1fv(location, count, value); }
    void SetUniform2f(int location, float v0, float v1) override { glUniform2f(location, v0, v1); }
    void SetUniform2fv(int location, const float *value) override { glUniform2fv(location, 1, value); }
    void SetUniform3f(int location, float v0, float v1, float v2) override { glUniform3f(location, v0, v1, v2); }
    void SetUniform3fv(int location, const float *value) override { glUniform3fv(location, 1, value); }
    void SetUniform4f(int location, float v0, float v1, float v2, float v3) override { glUniform4f(location, v0, v1, v2, v3); }
    void SetUniform4fv(int location, const float *value) override { glUniform4fv(location, 1, value); }
    void SetUniformMatrix2fv(int location, const float *value) override { glUniformMatrix2fv(location, 1, GL_FALSE, value); }
    void SetUniformMatrix3fv(int location, const float *value) override { glUniformMatrix3fv(location, 1, GL_FALSE, value); }
    void SetUniformMatrix4fv(int location, const float *value) override { glUniformMatrix4fv(location, 1, GL_FALSE, value); }
    void SetUniformMatrix4fvArray(int location, int count, const float *value) override { glUniformMatrix4fv(location, count, GL_FALSE, value); }

    void DispatchCompute(unsigned int numGroupsX, unsigned int numGroupsY, unsigned int numGroupsZ) override
    {
        glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
    }

    void MemoryBarrier(MemoryBarrierBit barriers) override { glMemoryBarrier(GLTranslator::ToGL(barriers)); }

    const char *GetBackendName() const override { return "OpenGL"; }
};
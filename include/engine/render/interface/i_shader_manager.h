#pragma once

#include <render/type/graphics_types.h>
#include <string>

class IShaderManager
{
public:
    virtual ~IShaderManager() = default;

    virtual unsigned int CreateShader(ShaderType type) = 0;
    virtual void ShaderSource(unsigned int shader, const char* source) = 0;
    virtual void CompileShader(unsigned int shader) = 0;
    virtual void DeleteShader(unsigned int shader) = 0;

    virtual bool GetShaderCompileStatus(unsigned int shader) = 0;
    virtual std::string GetShaderInfoLog(unsigned int shader) = 0;

    virtual unsigned int CreateProgram() = 0;
    virtual void AttachShader(unsigned int program, unsigned int shader) = 0;
    virtual void LinkProgram(unsigned int program) = 0;
    virtual void UseProgram(unsigned int program) = 0;
    virtual void DeleteProgram(unsigned int program) = 0;

    virtual bool GetProgramLinkStatus(unsigned int program) = 0;
    virtual std::string GetProgramInfoLog(unsigned int program) = 0;

    virtual int GetUniformLocation(unsigned int program, const char* name) = 0;
    virtual void SetUniform1i(int location, int value) = 0;
    virtual void SetUniform1ui(int location, unsigned int value) = 0;
    virtual void SetUniform1f(int location, float value) = 0;
    virtual void SetUniform1fv(int location, int count, const float* value) = 0;
    virtual void SetUniform2f(int location, float v0, float v1) = 0;
    virtual void SetUniform2fv(int location, const float* value) = 0;
    virtual void SetUniform3f(int location, float v0, float v1, float v2) = 0;
    virtual void SetUniform3fv(int location, const float* value) = 0;
    virtual void SetUniform4f(int location, float v0, float v1, float v2, float v3) = 0;
    virtual void SetUniform4fv(int location, const float* value) = 0;
    virtual void SetUniformMatrix2fv(int location, const float* value) = 0;
    virtual void SetUniformMatrix3fv(int location, const float* value) = 0;
    virtual void SetUniformMatrix4fv(int location, const float* value) = 0;
    virtual void SetUniform3fvArray(int location, int count, const float* value) = 0;
    virtual void SetUniformMatrix4fvArray(int location, int count, const float* value) = 0;

    virtual void DispatchCompute(unsigned int numGroupsX, unsigned int numGroupsY, unsigned int numGroupsZ) = 0;
    virtual void MemoryBarrier(MemoryBarrierBit barriers) = 0;

    virtual const char* GetBackendName() const = 0;
};

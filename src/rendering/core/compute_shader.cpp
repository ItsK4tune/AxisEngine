#include <fstream>
#include <rendering/core/compute_shader.h>
#include <rendering/interfaces/i_shader_manager.h>
#include <sstream>
#include <core/utils/logger.h>

namespace {
    constexpr unsigned int COMPUTE_SHADER = 0x91B9;
    constexpr unsigned int FALSE = 0;
}

ComputeShader::ComputeShader(IShaderManager& manager, const char *computePath)
    : ID(0), m_ShaderManager(manager)
{
    

    std::string computeCode;
    std::ifstream cShaderFile;

    cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        cShaderFile.open(computePath);

        std::stringstream cShaderStream;
        cShaderStream << cShaderFile.rdbuf();
        cShaderFile.close();
        computeCode = cShaderStream.str();
    }
    catch (std::ifstream::failure &e)
    {
        LOGGER_ERROR("ComputeShader") << "FILE_NOT_SUCCESSFULLY_READ: " << e.what();
    }
    const char *cShaderCode = computeCode.c_str();

    unsigned int compute;
    auto& sm = m_ShaderManager;

    compute = sm.CreateShader(Graphics::ShaderType::Compute);
    sm.ShaderSource(compute, cShaderCode);
    sm.CompileShader(compute);
    checkCompileErrors(compute, "COMPUTE");

    ID = sm.CreateProgram();
    sm.AttachShader(ID, compute);
    sm.LinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    sm.DeleteShader(compute);
}

ComputeShader::~ComputeShader()
{
    if (ID != 0)
    {
        m_ShaderManager.DeleteProgram(ID);
    }
}

void ComputeShader::use()
{
    m_ShaderManager.UseProgram(ID);
}

void ComputeShader::setBool(const std::string &name, bool value) const
{
    
    m_ShaderManager.SetUniform1i(m_ShaderManager.GetUniformLocation(ID, name.c_str()), (int)value);
}

void ComputeShader::setInt(const std::string &name, int value) const
{
    
    m_ShaderManager.SetUniform1i(m_ShaderManager.GetUniformLocation(ID, name.c_str()), value);
}

void ComputeShader::setFloat(const std::string &name, float value) const
{
    
    m_ShaderManager.SetUniform1f(m_ShaderManager.GetUniformLocation(ID, name.c_str()), value);
}

void ComputeShader::setVec2(const std::string &name, const glm::vec2 &value) const
{
    
    m_ShaderManager.SetUniform2f(m_ShaderManager.GetUniformLocation(ID, name.c_str()), value.x, value.y);
}

void ComputeShader::setVec2(const std::string &name, float x, float y) const
{
    
    m_ShaderManager.SetUniform2f(m_ShaderManager.GetUniformLocation(ID, name.c_str()), x, y);
}

void ComputeShader::setVec3(const std::string &name, const glm::vec3 &value) const
{
    
    m_ShaderManager.SetUniform3f(m_ShaderManager.GetUniformLocation(ID, name.c_str()), value.x, value.y, value.z);
}

void ComputeShader::setVec3(const std::string &name, float x, float y, float z) const
{
    
    m_ShaderManager.SetUniform3f(m_ShaderManager.GetUniformLocation(ID, name.c_str()), x, y, z);
}

void ComputeShader::setVec4(const std::string &name, const glm::vec4 &value) const
{
    
    m_ShaderManager.SetUniform4f(m_ShaderManager.GetUniformLocation(ID, name.c_str()), value.x, value.y, value.z, value.w);
}

void ComputeShader::setVec4(const std::string &name, float x, float y, float z, float w)
{
    
    m_ShaderManager.SetUniform4f(m_ShaderManager.GetUniformLocation(ID, name.c_str()), x, y, z, w);
}

void ComputeShader::setMat2(const std::string &name, const glm::mat2 &mat) const
{
    
    m_ShaderManager.SetUniformMatrix2fv(m_ShaderManager.GetUniformLocation(ID, name.c_str()), &mat[0][0]);
}

void ComputeShader::setMat3(const std::string &name, const glm::mat3 &mat) const
{
    
    m_ShaderManager.SetUniformMatrix3fv(m_ShaderManager.GetUniformLocation(ID, name.c_str()), &mat[0][0]);
}

void ComputeShader::setMat4(const std::string &name, const glm::mat4 &mat) const
{
    
    m_ShaderManager.SetUniformMatrix4fv(m_ShaderManager.GetUniformLocation(ID, name.c_str()), &mat[0][0]);
}

void ComputeShader::checkCompileErrors(unsigned int shader, std::string type)
{
    
    auto& sm = m_ShaderManager;

    bool success;
    if (type != "PROGRAM")
    {
        success = sm.GetShaderCompileStatus(shader);
        if (!success)
        {
            std::string infoLog = sm.GetShaderInfoLog(shader);
            LOGGER_ERROR("ComputeShader") << "COMPILATION_ERROR of type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- ";
        }
    }
    else
    {
        success = sm.GetProgramLinkStatus(shader);
        if (!success)
        {
            std::string infoLog = sm.GetProgramInfoLog(shader);
            LOGGER_ERROR("ComputeShader") << "LINKING_ERROR of type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- ";
        }
    }
}
